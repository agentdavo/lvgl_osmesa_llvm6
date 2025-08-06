#ifndef DX8GL_RENDER_THREAD_H
#define DX8GL_RENDER_THREAD_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>
#include "command_buffer.h"
#include "logger.h"

namespace dx8gl {

// Forward declarations
class StateManager;
class VertexShaderManager;
class PixelShaderManager;
class ShaderProgramManager;
class DX8RenderBackend;

/**
 * RenderThread - Dedicated thread for sequential OpenGL command execution
 * 
 * This class ensures all OpenGL commands are executed in order on a single thread
 * that owns the OpenGL context, preventing context thrashing and race conditions.
 */
class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    // Non-copyable, non-movable
    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;
    RenderThread(RenderThread&&) = delete;
    RenderThread& operator=(RenderThread&&) = delete;

    /**
     * Initialize the render thread with required managers
     * Must be called before submitting any command buffers
     */
    bool initialize(StateManager* state_manager,
                   VertexShaderManager* vertex_shader_mgr,
                   PixelShaderManager* pixel_shader_mgr,
                   ShaderProgramManager* shader_program_mgr,
                   DX8RenderBackend* render_backend);

    /**
     * Submit a command buffer for execution
     * Command buffers are executed sequentially in submission order
     */
    void submit(std::unique_ptr<CommandBuffer> command_buffer);

    /**
     * Wait for all pending command buffers to complete
     * Blocks until the render queue is empty
     */
    void wait_for_idle();

    /**
     * Flush all pending commands immediately
     * Forces execution of all queued command buffers
     */
    void flush();

    /**
     * Get the number of pending command buffers
     */
    size_t get_pending_count() const;

    /**
     * Check if the render thread is idle (no pending commands)
     */
    bool is_idle() const;

    /**
     * Stop the render thread
     * Waits for all pending commands to complete before returning
     */
    void stop();

private:
    // Worker thread function
    void render_thread_func();

    // Thread and synchronization
    std::thread render_thread_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::condition_variable idle_cv_;
    
    // Command buffer queue
    std::queue<std::unique_ptr<CommandBuffer>> command_queue_;
    
    // State
    std::atomic<bool> running_;
    std::atomic<bool> stop_requested_;
    std::atomic<size_t> commands_processed_;
    std::atomic<size_t> commands_pending_;
    
    // Manager pointers (not owned)
    StateManager* state_manager_;
    VertexShaderManager* vertex_shader_manager_;
    PixelShaderManager* pixel_shader_manager_;
    ShaderProgramManager* shader_program_manager_;
    DX8RenderBackend* render_backend_;
    
    // OpenGL context ownership flag
    bool owns_context_;
};

} // namespace dx8gl

#endif // DX8GL_RENDER_THREAD_H