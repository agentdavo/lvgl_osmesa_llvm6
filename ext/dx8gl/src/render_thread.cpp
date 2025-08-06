#include "render_thread.h"
#include "state_manager.h"
#include "vertex_shader_manager.h"
#include "pixel_shader_manager.h"
#include "shader_program_manager.h"
#include "render_backend.h"
#include "osmesa_gl_loader.h"

namespace dx8gl {

RenderThread::RenderThread()
    : running_(false)
    , stop_requested_(false)
    , commands_processed_(0)
    , commands_pending_(0)
    , state_manager_(nullptr)
    , vertex_shader_manager_(nullptr)
    , pixel_shader_manager_(nullptr)
    , shader_program_manager_(nullptr)
    , render_backend_(nullptr)
    , owns_context_(false) {
    DX8GL_DEBUG("RenderThread created");
}

RenderThread::~RenderThread() {
    stop();
    DX8GL_DEBUG("RenderThread destroyed - processed %zu commands total", 
                commands_processed_.load());
}

bool RenderThread::initialize(StateManager* state_manager,
                             VertexShaderManager* vertex_shader_mgr,
                             PixelShaderManager* pixel_shader_mgr,
                             ShaderProgramManager* shader_program_mgr,
                             DX8RenderBackend* render_backend) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (running_) {
        DX8GL_WARNING("RenderThread already initialized");
        return false;
    }
    
    // Store manager pointers
    state_manager_ = state_manager;
    vertex_shader_manager_ = vertex_shader_mgr;
    pixel_shader_manager_ = pixel_shader_mgr;
    shader_program_manager_ = shader_program_mgr;
    render_backend_ = render_backend;
    
    if (!state_manager_ || !vertex_shader_manager_ || 
        !pixel_shader_manager_ || !shader_program_manager_) {
        DX8GL_ERROR("RenderThread initialization failed - null manager pointers");
        return false;
    }
    
    // Start the render thread
    running_ = true;
    stop_requested_ = false;
    render_thread_ = std::thread(&RenderThread::render_thread_func, this);
    
    DX8GL_INFO("RenderThread initialized successfully");
    return true;
}

void RenderThread::submit(std::unique_ptr<CommandBuffer> command_buffer) {
    if (!command_buffer || command_buffer->empty()) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (stop_requested_) {
            DX8GL_WARNING("Attempting to submit command buffer after stop requested");
            return;
        }
        
        size_t command_count = command_buffer->get_command_count();
        size_t buffer_size = command_buffer->size();
        
        command_queue_.push(std::move(command_buffer));
        commands_pending_++;
        
        DX8GL_DEBUG("Command buffer submitted: %zu commands, %zu bytes (queue size: %zu)",
                   command_count, buffer_size, command_queue_.size());
    }
    
    // Wake up the render thread
    queue_cv_.notify_one();
}

void RenderThread::wait_for_idle() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    if (!running_) {
        return;
    }
    
    DX8GL_DEBUG("Waiting for render thread to idle...");
    
    // Wait until queue is empty and no commands are being processed
    idle_cv_.wait(lock, [this] { 
        return command_queue_.empty() && commands_pending_ == 0;
    });
    
    DX8GL_DEBUG("Render thread is idle");
}

void RenderThread::flush() {
    // Force immediate processing by waking the render thread
    queue_cv_.notify_one();
    
    // Then wait for completion
    wait_for_idle();
}

size_t RenderThread::get_pending_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return command_queue_.size();
}

bool RenderThread::is_idle() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return command_queue_.empty() && commands_pending_ == 0;
}

void RenderThread::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (!running_) {
            return;
        }
        
        DX8GL_INFO("Stopping render thread...");
        stop_requested_ = true;
    }
    
    // Wake up the render thread so it can see the stop request
    queue_cv_.notify_one();
    
    // Wait for the thread to finish
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
    
    running_ = false;
    DX8GL_INFO("Render thread stopped");
}

void RenderThread::render_thread_func() {
    DX8GL_INFO("Render thread started");
    
    // Make OpenGL context current in this thread
    if (render_backend_) {
        if (!render_backend_->make_current()) {
            DX8GL_ERROR("Failed to make OpenGL context current in render thread");
            running_ = false;
            return;
        }
        owns_context_ = true;
        DX8GL_DEBUG("OpenGL context made current in render thread");
        
        // OpenGL functions should already be loaded by OSMesa at this point
        // The render backend initialization ensures all required functions are available
    }
    
    while (true) {
        std::unique_ptr<CommandBuffer> command_buffer;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for work or stop signal
            queue_cv_.wait(lock, [this] {
                return !command_queue_.empty() || stop_requested_;
            });
            
            // Check for stop request
            if (stop_requested_ && command_queue_.empty()) {
                break;
            }
            
            // Get next command buffer
            if (!command_queue_.empty()) {
                command_buffer = std::move(command_queue_.front());
                command_queue_.pop();
            }
        }
        
        // Execute command buffer outside of lock
        if (command_buffer) {
            size_t command_count = command_buffer->get_command_count();
            
            DX8GL_DEBUG("Executing command buffer: %zu commands", command_count);
            
            try {
                // Execute the command buffer with all managers
                command_buffer->execute(*state_manager_,
                                      vertex_shader_manager_,
                                      pixel_shader_manager_,
                                      shader_program_manager_);
                
                commands_processed_++;
                
                DX8GL_DEBUG("Command buffer executed successfully");
            } catch (const std::exception& e) {
                DX8GL_ERROR("Exception during command buffer execution: %s", e.what());
            }
            
            // Update pending count and notify waiters
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (commands_pending_ > 0) {
                    commands_pending_--;
                }
                
                // Notify anyone waiting for idle
                if (command_queue_.empty() && commands_pending_ == 0) {
                    idle_cv_.notify_all();
                }
            }
        }
    }
    
    // Process any remaining commands before stopping
    while (!command_queue_.empty()) {
        auto command_buffer = std::move(command_queue_.front());
        command_queue_.pop();
        
        if (command_buffer) {
            try {
                command_buffer->execute(*state_manager_,
                                      vertex_shader_manager_,
                                      pixel_shader_manager_,
                                      shader_program_manager_);
                commands_processed_++;
            } catch (const std::exception& e) {
                DX8GL_ERROR("Exception during final command buffer execution: %s", e.what());
            }
        }
    }
    
    DX8GL_INFO("Render thread exiting - processed %zu commands total", 
               commands_processed_.load());
}

} // namespace dx8gl