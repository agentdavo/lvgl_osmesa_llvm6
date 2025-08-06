#include "webgpu_backend.h"
#include "logger.h"
#include "state_manager.h"
#include "cube_texture_support.h"
#include <cstring>
#include <cstdlib>

#ifndef __EMSCRIPTEN__
#include <thread>
#include <chrono>
#endif

#ifdef DX8GL_HAS_WEBGPU

namespace dx8gl {

DX8WebGPUBackend::DX8WebGPUBackend()
    : adapter_(nullptr)
    , device_(nullptr)
    , queue_(nullptr)
    , canvas_context_(nullptr)
    , render_texture_(nullptr)
    , render_texture_view_(nullptr)
    , readback_buffer_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false)
    , adapter_ready_(false)
    , device_ready_(false)
    , buffer_mapped_(false)
    , canvas_id_(1)  // Default canvas ID
    , canvas_created_(false)
    , framebuffer_ready_(false)
    , framebuffer_callback_(nullptr)
    , framebuffer_callback_user_data_(nullptr)
    , current_pipeline_(nullptr) {
    error_buffer_[0] = '\0';
    state_mapper_ = std::make_unique<WebGPUStateMapper>();
}

DX8WebGPUBackend::~DX8WebGPUBackend() {
    shutdown();
}

bool DX8WebGPUBackend::initialize(int width, int height) {
    if (initialized_) {
        DX8GL_INFO("WebGPU backend already initialized");
        return true;
    }
    
    DX8GL_INFO("Initializing WebGPU backend %dx%d", width, height);
    
    width_ = width;
    height_ = height;
    
    // Step 1: Create WebGPU adapter
    if (!create_adapter()) {
        DX8GL_ERROR("Failed to create WebGPU adapter: %s", error_buffer_);
        return false;
    }
    
    // Step 2: Create WebGPU device
    if (!create_device()) {
        DX8GL_ERROR("Failed to create WebGPU device: %s", error_buffer_);
        return false;
    }
    
    // Step 3: Setup offscreen canvas for rendering
    if (!setup_offscreen_canvas()) {
        DX8GL_ERROR("Failed to setup offscreen canvas: %s", error_buffer_);
        return false;
    }
    
    // Step 4: Create rendering resources
    if (!create_render_resources()) {
        DX8GL_ERROR("Failed to create render resources: %s", error_buffer_);
        return false;
    }
    
    // Step 5: Setup framebuffer readback
    if (!setup_readback_buffer()) {
        DX8GL_ERROR("Failed to setup readback buffer: %s", error_buffer_);
        return false;
    }
    
    initialized_ = true;
    DX8GL_INFO("WebGPU backend initialized successfully");
    return true;
}

bool DX8WebGPUBackend::make_current() {
    if (!initialized_) {
        strncpy(error_buffer_, "Backend not initialized", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // WebGPU doesn't have a concept of "current context" like OpenGL
    // The device and queue are always accessible once created
    return true;
}

void* DX8WebGPUBackend::get_framebuffer(int& width, int& height, int& format) {
    if (!initialized_ || !framebuffer_) {
        width = height = format = 0;
        return nullptr;
    }
    
    // If framebuffer is ready from a previous async operation, return it
    if (framebuffer_ready_) {
        width = width_;
        height = height_;
        format = framebuffer_->get_gl_format();
        return framebuffer_->get_data();
    }
    
    // For backward compatibility, we still support synchronous readback
    // but it's not recommended in worker contexts
    if (!readback_buffer_) {
        width = height = format = 0;
        return nullptr;
    }
    
    DX8GL_WARNING("Synchronous framebuffer readback is deprecated in WebGPU backend. "
                  "Use request_framebuffer_async() instead for better performance.");
    
    // Initiate mapping but don't block for too long
    buffer_mapped_ = false;
    wgpu_buffer_map_async(readback_buffer_, WGPU_MAP_MODE_READ, 0, 
                         width_ * height_ * 4, buffer_map_callback, this);
    
    // Very short wait - just a few attempts
    int attempts = 5;
    while (!buffer_mapped_ && attempts-- > 0) {
#ifdef __EMSCRIPTEN__
        // In workers, we can't block for long
        emscripten_sleep(1);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
    }
    
    if (buffer_mapped_) {
        // Use framebuffer helper's read_from_gpu method
        framebuffer_->read_from_gpu([this](void* dest, size_t size) {
            const void* mapped_data = wgpu_buffer_get_const_mapped_range(readback_buffer_, 0, WGPU_WHOLE_MAP_SIZE);
            if (mapped_data) {
                memcpy(dest, mapped_data, size);
                return true;
            }
            return false;
        });
        
        // Unmap the buffer
        wgpu_buffer_unmap(readback_buffer_);
        framebuffer_ready_ = true;
    }
    
    if (framebuffer_ready_) {
        width = width_;
        height = height_;
        format = framebuffer_->get_gl_format();
        return framebuffer_->get_data();
    } else {
        // Return null if data is not ready yet
        width = height = format = 0;
        return nullptr;
    }
}

bool DX8WebGPUBackend::resize(int width, int height) {
    if (!initialized_) {
        strncpy(error_buffer_, "Backend not initialized", sizeof(error_buffer_) - 1);
        return false;
    }
    
    if (width == width_ && height == height_) {
        return true; // No change needed
    }
    
    DX8GL_INFO("Resizing WebGPU backend from %dx%d to %dx%d", width_, height_, width, height);
    
#ifdef __EMSCRIPTEN__
    // Resize the OffscreenCanvas if it exists
    if (offscreen_canvas_is_valid(canvas_id_)) {
        DX8GL_INFO("Resizing OffscreenCanvas %d to %dx%d", canvas_id_, width, height);
        offscreen_canvas_set_size(canvas_id_, width, height);
    }
#endif
    
    // Cleanup old resources
    if (render_texture_view_) {
        wgpu_object_destroy(render_texture_view_);
        render_texture_view_ = nullptr;
    }
    if (render_texture_) {
        wgpu_object_destroy(render_texture_);
        render_texture_ = nullptr;
    }
    if (readback_buffer_) {
        wgpu_object_destroy(readback_buffer_);
        readback_buffer_ = nullptr;
    }
    
    // Update dimensions
    width_ = width;
    height_ = height;
    
    // Recreate resources with new dimensions
    if (!create_render_resources()) {
        DX8GL_ERROR("Failed to recreate render resources after resize");
        return false;
    }
    
    if (!setup_readback_buffer()) {
        DX8GL_ERROR("Failed to recreate readback buffer after resize");
        return false;
    }
    
    // Resize framebuffer helper
    if (!framebuffer_) {
        framebuffer_ = std::make_unique<OffscreenFramebuffer>(width_, height_, PixelFormat::RGBA8, true);
    } else {
        framebuffer_->resize(width_, height_);
    }
    
    DX8GL_INFO("WebGPU backend resized successfully");
    return true;
}

void DX8WebGPUBackend::shutdown() {
    if (!initialized_) {
        return;
    }
    
    DX8GL_INFO("Shutting down WebGPU backend");
    
    cleanup_resources();
    
    initialized_ = false;
    adapter_ready_ = false;
    device_ready_ = false;
    buffer_mapped_ = false;
    
    DX8GL_INFO("WebGPU backend shutdown complete");
}

bool DX8WebGPUBackend::transfer_canvas_control(const char* canvas_selector) {
#ifdef __EMSCRIPTEN__
    if (!canvas_selector) {
        strncpy(error_buffer_, "Canvas selector is null", sizeof(error_buffer_) - 1);
        return false;
    }
    
    DX8GL_INFO("Transferring control from HTML canvas '%s' to OffscreenCanvas ID %d", 
               canvas_selector, canvas_id_);
    
    // Transfer control from the HTML canvas to an OffscreenCanvas
    canvas_transfer_control_to_offscreen(canvas_selector, canvas_id_);
    
    // Verify the transfer was successful
    if (!offscreen_canvas_is_valid(canvas_id_)) {
        strncpy(error_buffer_, "Failed to transfer canvas control to offscreen", sizeof(error_buffer_) - 1);
        return false;
    }
    
    canvas_created_ = true;  // Mark that we now own the canvas
    DX8GL_INFO("Successfully transferred canvas control to OffscreenCanvas");
    return true;
#else
    // Not applicable for non-Emscripten builds
    return true;
#endif
}

void DX8WebGPUBackend::request_framebuffer_async(FramebufferReadyCallback callback, void* user_data) {
    if (!initialized_ || !framebuffer_ || !readback_buffer_) {
        // Call callback immediately with failure
        if (callback) {
            callback(nullptr, 0, 0, 0, user_data);
        }
        return;
    }
    
    // Store the callback for later
    framebuffer_callback_ = callback;
    framebuffer_callback_user_data_ = user_data;
    framebuffer_ready_ = false;
    
    // Initiate the async buffer mapping
    buffer_mapped_ = false;
    wgpu_buffer_map_async(readback_buffer_, WGPU_MAP_MODE_READ, 0,
                         width_ * height_ * 4, buffer_map_callback, this);
    
    DX8GL_INFO("Initiated async framebuffer readback");
}

bool DX8WebGPUBackend::has_extension(const char* extension) const {
    if (!initialized_ || !extension) {
        return false;
    }
    
    // WebGPU has different feature/extension model than OpenGL
    // For now, we'll return false for all OpenGL extensions
    // In a real implementation, you might want to map some common
    // OpenGL extensions to WebGPU features
    return false;
}

bool DX8WebGPUBackend::create_adapter() {
    DX8GL_INFO("Requesting WebGPU adapter");
    
    WGpuRequestAdapterOptions options = {};
    options.powerPreference = WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE;
    options.forceFallbackAdapter = WGPU_FALSE;
    
    adapter_ready_ = false;
    wgpu_request_adapter(&options, adapter_callback, this);
    
    // Wait for adapter creation (this would be async in a real implementation)
    int timeout = 5000; // 5 second timeout
    while (!adapter_ready_ && timeout-- > 0) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
    }
    
    if (!adapter_ready_ || !adapter_) {
        strncpy(error_buffer_, "Failed to obtain WebGPU adapter", sizeof(error_buffer_) - 1);
        return false;
    }
    
    DX8GL_INFO("WebGPU adapter created successfully");
    return true;
}

bool DX8WebGPUBackend::create_device() {
    if (!adapter_) {
        strncpy(error_buffer_, "No adapter available", sizeof(error_buffer_) - 1);
        return false;
    }
    
    DX8GL_INFO("Requesting WebGPU device");
    
    WGpuDeviceDescriptor descriptor = {};
    descriptor.label = "dx8gl WebGPU Device";
    
    device_ready_ = false;
    wgpu_adapter_request_device(adapter_, &descriptor, device_callback, this);
    
    // Wait for device creation
    int timeout = 5000; // 5 second timeout
    while (!device_ready_ && timeout-- > 0) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
    }
    
    if (!device_ready_ || !device_) {
        strncpy(error_buffer_, "Failed to obtain WebGPU device", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Get the device queue
    queue_ = wgpu_device_get_queue(device_);
    if (!queue_) {
        strncpy(error_buffer_, "Failed to obtain device queue", sizeof(error_buffer_) - 1);
        return false;
    }
    
    DX8GL_INFO("WebGPU device and queue created successfully");
    return true;
}

bool DX8WebGPUBackend::setup_offscreen_canvas() {
    DX8GL_INFO("Setting up WebGPU offscreen canvas (ID: %d)", canvas_id_);
    
#ifdef __EMSCRIPTEN__
    // First check if the canvas is already valid
    if (!offscreen_canvas_is_valid(canvas_id_)) {
        // Create the OffscreenCanvas with the specified size
        DX8GL_INFO("Creating OffscreenCanvas with ID %d, size %dx%d", canvas_id_, width_, height_);
        offscreen_canvas_create(canvas_id_, width_, height_);
        canvas_created_ = true;
        
        // Verify the canvas was created successfully
        if (!offscreen_canvas_is_valid(canvas_id_)) {
            strncpy(error_buffer_, "Failed to create OffscreenCanvas", sizeof(error_buffer_) - 1);
            return false;
        }
        
        // Determine the threading model and dispatch the canvas accordingly
#ifdef __EMSCRIPTEN_WASM_WORKERS__
        // Wasm Workers mode - post to current worker thread
        DX8GL_INFO("Using Wasm Workers mode - posting canvas to current worker");
        emscripten_wasm_worker_t current_worker = emscripten_wasm_worker_self_id();
        if (current_worker != 0) {
            // We're in a worker thread, no need to post (canvas is already here)
            DX8GL_INFO("Already in worker thread %d", current_worker);
        }
#elif defined(__EMSCRIPTEN_PTHREADS__)
        // Pthreads mode - post to current pthread
        DX8GL_INFO("Using Pthreads mode - posting canvas to current pthread");
        pthread_t current_thread = pthread_self();
        if (current_thread != pthread_self()) {  // Check if we're not on main thread
            // We're in a pthread, canvas should already be accessible
            DX8GL_INFO("Already in pthread %p", (void*)current_thread);
        }
#else
        // Main thread or no threading - canvas is already accessible
        DX8GL_INFO("No threading model detected - canvas accessible on main thread");
#endif
    } else {
        DX8GL_INFO("OffscreenCanvas with ID %d already exists", canvas_id_);
    }
    
    // Get the WebGPU context from the OffscreenCanvas
    canvas_context_ = wgpu_offscreen_canvas_get_webgpu_context(canvas_id_);
    
    if (!canvas_context_) {
        strncpy(error_buffer_, "Failed to get WebGPU canvas context", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Validate the canvas one more time
    if (!offscreen_canvas_is_valid(canvas_id_)) {
        strncpy(error_buffer_, "OffscreenCanvas became invalid after getting context", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Configure the canvas context
    WGpuCanvasConfiguration config = {};
    config.device = device_;
    config.format = WGPU_TEXTURE_FORMAT_BGRA8_UNORM;
    config.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
    config.alphaMode = WGPU_CANVAS_ALPHA_MODE_OPAQUE;
    
    wgpu_canvas_context_configure(canvas_context_, &config);
    
    DX8GL_INFO("OffscreenCanvas configured successfully");
    
#else
    // For native platforms, we don't need an actual canvas
    // We'll render to a texture directly
    canvas_context_ = nullptr;
#endif
    
    DX8GL_INFO("Offscreen canvas setup complete");
    return true;
}

bool DX8WebGPUBackend::create_render_resources() {
    DX8GL_INFO("Creating WebGPU render resources");
    
    // Create render texture
    WGpuTextureDescriptor texture_desc = {};
    texture_desc.label = "dx8gl Render Texture";
    texture_desc.size.width = width_;
    texture_desc.size.height = height_;
    texture_desc.size.depthOrArrayLayers = 1;
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
    texture_desc.format = WGPU_TEXTURE_FORMAT_RGBA8_UNORM;
    texture_desc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;
    
    render_texture_ = wgpu_device_create_texture(device_, &texture_desc);
    if (!render_texture_) {
        strncpy(error_buffer_, "Failed to create render texture", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Create render texture view
    WGpuTextureViewDescriptor view_desc = {};
    view_desc.label = "dx8gl Render Texture View";
    view_desc.format = WGPU_TEXTURE_FORMAT_RGBA8_UNORM;
    view_desc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    
    render_texture_view_ = wgpu_texture_create_view(render_texture_, &view_desc);
    if (!render_texture_view_) {
        strncpy(error_buffer_, "Failed to create render texture view", sizeof(error_buffer_) - 1);
        return false;
    }
    
    DX8GL_INFO("Render resources created successfully");
    return true;
}

bool DX8WebGPUBackend::setup_readback_buffer() {
    DX8GL_INFO("Setting up framebuffer readback buffer");
    
    size_t buffer_size = width_ * height_ * 4; // RGBA8
    
    WGpuBufferDescriptor buffer_desc = {};
    buffer_desc.label = "dx8gl Readback Buffer";
    buffer_desc.size = buffer_size;
    buffer_desc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ;
    buffer_desc.mappedAtCreation = WGPU_FALSE;
    
    readback_buffer_ = wgpu_device_create_buffer(device_, &buffer_desc);
    if (!readback_buffer_) {
        strncpy(error_buffer_, "Failed to create readback buffer", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Initialize framebuffer helper
    framebuffer_ = std::make_unique<OffscreenFramebuffer>(width_, height_, PixelFormat::RGBA8, true);
    framebuffer_->clear(0.0f, 0.0f, 0.0f, 1.0f);
    
    DX8GL_INFO("Readback buffer setup complete");
    return true;
}

void DX8WebGPUBackend::cleanup_resources() {
    // Destroy WebGPU objects in reverse order of creation
    if (readback_buffer_) {
        wgpu_object_destroy(readback_buffer_);
        readback_buffer_ = nullptr;
    }
    
    if (render_texture_view_) {
        wgpu_object_destroy(render_texture_view_);
        render_texture_view_ = nullptr;
    }
    
    if (render_texture_) {
        wgpu_object_destroy(render_texture_);
        render_texture_ = nullptr;
    }
    
    if (canvas_context_) {
        wgpu_object_destroy(canvas_context_);
        canvas_context_ = nullptr;
    }
    
#ifdef __EMSCRIPTEN__
    // Clean up the OffscreenCanvas if we created it
    if (canvas_created_ && offscreen_canvas_is_valid(canvas_id_)) {
        DX8GL_INFO("Destroying OffscreenCanvas with ID %d", canvas_id_);
        offscreen_canvas_destroy(canvas_id_);
        canvas_created_ = false;
    }
#endif
    
    if (queue_) {
        wgpu_object_destroy(queue_);
        queue_ = nullptr;
    }
    
    if (device_) {
        wgpu_object_destroy(device_);
        device_ = nullptr;
    }
    
    if (adapter_) {
        wgpu_object_destroy(adapter_);
        adapter_ = nullptr;
    }
    
    framebuffer_.reset();
}

void DX8WebGPUBackend::apply_render_state(const RenderState& render_state) {
    if (!initialized_ || !state_mapper_) {
        DX8GL_ERROR("Cannot apply render state: backend not initialized");
        return;
    }
    
    // Check if render state has changed
    if (std::memcmp(&cached_render_state_, &render_state, sizeof(RenderState)) == 0) {
        return; // No change needed
    }
    
    cached_render_state_ = render_state;
    
    // Create pipeline state key from current states
    WebGPUStateMapper::PipelineStateKey key = {};
    
    // Blend state
    key.blend_enabled = render_state.alpha_blend_enable != FALSE;
    key.src_blend = WebGPUStateMapper::d3d_to_wgpu_blend_factor(
        static_cast<D3DBLEND>(render_state.src_blend));
    key.dst_blend = WebGPUStateMapper::d3d_to_wgpu_blend_factor(
        static_cast<D3DBLEND>(render_state.dest_blend));
    key.blend_op = WebGPUStateMapper::d3d_to_wgpu_blend_op(
        static_cast<D3DBLENDOP>(render_state.blend_op));
    key.src_blend_alpha = WebGPUStateMapper::d3d_to_wgpu_blend_factor(
        static_cast<D3DBLEND>(render_state.src_blend_alpha));
    key.dst_blend_alpha = WebGPUStateMapper::d3d_to_wgpu_blend_factor(
        static_cast<D3DBLEND>(render_state.dest_blend_alpha));
    key.blend_op_alpha = WebGPUStateMapper::d3d_to_wgpu_blend_op(
        static_cast<D3DBLENDOP>(render_state.blend_op_alpha));
    
    // Depth state
    key.depth_test_enabled = render_state.z_enable != FALSE;
    key.depth_write_enabled = render_state.z_write_enable != FALSE;
    key.depth_compare = WebGPUStateMapper::d3d_to_wgpu_compare_func(
        static_cast<D3DCMPFUNC>(render_state.z_func));
    key.depth_bias = static_cast<int32_t>(render_state.z_bias);
    key.depth_bias_slope_scale = render_state.slope_scale_depth_bias;
    key.depth_bias_clamp = 0.0f; // D3D8 doesn't have depth bias clamp
    
    // Stencil state
    key.stencil_enabled = render_state.stencil_enable != FALSE;
    key.stencil_compare = WebGPUStateMapper::d3d_to_wgpu_compare_func(
        static_cast<D3DCMPFUNC>(render_state.stencil_func));
    key.stencil_fail_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
        static_cast<D3DSTENCILOP>(render_state.stencil_fail));
    key.stencil_depth_fail_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
        static_cast<D3DSTENCILOP>(render_state.stencil_z_fail));
    key.stencil_pass_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
        static_cast<D3DSTENCILOP>(render_state.stencil_pass));
    key.stencil_read_mask = render_state.stencil_mask;
    key.stencil_write_mask = render_state.stencil_write_mask;
    
    // Two-sided stencil (if enabled)
    if (render_state.two_sided_stencil_mode) {
        key.stencil_back_compare = WebGPUStateMapper::d3d_to_wgpu_compare_func(
            static_cast<D3DCMPFUNC>(render_state.ccw_stencil_func));
        key.stencil_back_fail_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
            static_cast<D3DSTENCILOP>(render_state.ccw_stencil_fail));
        key.stencil_back_depth_fail_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
            static_cast<D3DSTENCILOP>(render_state.ccw_stencil_z_fail));
        key.stencil_back_pass_op = WebGPUStateMapper::d3d_to_wgpu_stencil_op(
            static_cast<D3DSTENCILOP>(render_state.ccw_stencil_pass));
    } else {
        // Use same stencil ops for both faces
        key.stencil_back_compare = key.stencil_compare;
        key.stencil_back_fail_op = key.stencil_fail_op;
        key.stencil_back_depth_fail_op = key.stencil_depth_fail_op;
        key.stencil_back_pass_op = key.stencil_pass_op;
    }
    
    // Rasterizer state
    key.cull_mode = WebGPUStateMapper::d3d_to_wgpu_cull_mode(
        static_cast<D3DCULL>(render_state.cull_mode));
    key.polygon_mode = (render_state.fill_mode == D3DFILL_WIREFRAME) ? 
        WGPU_POLYGON_MODE_LINE : WGPU_POLYGON_MODE_FILL;
    key.front_face = WGPU_FRONT_FACE_CCW; // D3D8 uses CCW
    
    // Multisample state
    key.sample_count = (render_state.multisample_antialias != FALSE) ? 4 : 1;
    key.alpha_to_coverage_enabled = false; // D3D8 doesn't have this
    
    // Look up or create pipeline for this state combination
    auto pipeline = state_mapper_->get_or_create_pipeline(device_, key);
    if (pipeline != current_pipeline_) {
        current_pipeline_ = pipeline;
        DX8GL_INFO("Switched to pipeline for state key (blend=%d, depth=%d, stencil=%d)",
                   key.blend_enabled, key.depth_test_enabled, key.stencil_enabled);
    }
    
    // Update texture samplers for all stages
    for (uint32_t stage = 0; stage < 8; ++stage) {
        auto sampler_desc = state_mapper_->create_sampler_descriptor(render_state, stage);
        if (sampler_desc) {
            // Create or update sampler for this stage
            WGpuSampler sampler = wgpu_device_create_sampler(device_, sampler_desc);
            state_mapper_->set_sampler(stage, sampler);
            delete sampler_desc;
        }
    }
}

void DX8WebGPUBackend::apply_transform_state(const TransformState& transform_state) {
    if (!initialized_ || !state_mapper_) {
        DX8GL_ERROR("Cannot apply transform state: backend not initialized");
        return;
    }
    
    // Check if transform state has changed
    if (std::memcmp(&cached_transform_state_, &transform_state, sizeof(TransformState)) == 0) {
        return; // No change needed
    }
    
    cached_transform_state_ = transform_state;
    
    // Transform state affects vertex shader constants and viewport/scissor
    // This would typically update uniform buffers or push constants
    
    // Set viewport
    if (transform_state.viewport_set) {
        DX8GL_INFO("Setting viewport: x=%d, y=%d, w=%d, h=%d, minZ=%f, maxZ=%f",
                   transform_state.viewport.x, transform_state.viewport.y,
                   transform_state.viewport.width, transform_state.viewport.height,
                   transform_state.viewport.min_z, transform_state.viewport.max_z);
        
        // Store viewport for use in render pass
        // WebGPU sets viewport per render pass, not globally
    }
    
    // Update transform matrices as uniform data
    // This would be passed to shaders via uniform buffers
    if (transform_state.world_transform_set) {
        // Update world transform uniform
        state_mapper_->set_transform_matrix(WebGPUStateMapper::TRANSFORM_WORLD, 
                                           transform_state.world_transform);
    }
    
    if (transform_state.view_transform_set) {
        // Update view transform uniform
        state_mapper_->set_transform_matrix(WebGPUStateMapper::TRANSFORM_VIEW,
                                           transform_state.view_transform);
    }
    
    if (transform_state.projection_transform_set) {
        // Update projection transform uniform
        state_mapper_->set_transform_matrix(WebGPUStateMapper::TRANSFORM_PROJECTION,
                                           transform_state.projection_transform);
    }
    
    // Update texture transforms
    for (uint32_t stage = 0; stage < 8; ++stage) {
        if (transform_state.texture_transform_set[stage]) {
            state_mapper_->set_transform_matrix(
                static_cast<WebGPUStateMapper::TransformType>(
                    WebGPUStateMapper::TRANSFORM_TEXTURE0 + stage),
                transform_state.texture_transform[stage]);
        }
    }
    
    // Update lighting state
    if (transform_state.lighting_enabled != cached_transform_state_.lighting_enabled) {
        state_mapper_->set_lighting_enabled(transform_state.lighting_enabled);
    }
    
    // Update material properties
    if (std::memcmp(&transform_state.material, &cached_transform_state_.material, 
                    sizeof(transform_state.material)) != 0) {
        state_mapper_->set_material(transform_state.material);
    }
    
    // Update light properties
    for (uint32_t i = 0; i < 8; ++i) {
        if (transform_state.light_enabled[i]) {
            if (std::memcmp(&transform_state.lights[i], &cached_transform_state_.lights[i],
                           sizeof(transform_state.lights[i])) != 0) {
                state_mapper_->set_light(i, transform_state.lights[i]);
            }
        }
    }
}

// Static callback handlers
void DX8WebGPUBackend::adapter_callback(WGpuRequestAdapterStatus status, WGpuAdapter adapter, const char* message, void* user_data) {
    DX8WebGPUBackend* backend = static_cast<DX8WebGPUBackend*>(user_data);
    
    if (status == WGPU_REQUEST_ADAPTER_STATUS_SUCCESS && adapter) {
        backend->adapter_ = adapter;
        backend->adapter_ready_ = true;
        DX8GL_INFO("WebGPU adapter obtained successfully");
    } else {
        DX8GL_ERROR("Failed to obtain WebGPU adapter: %s", message ? message : "Unknown error");
        if (message) {
            strncpy(backend->error_buffer_, message, sizeof(backend->error_buffer_) - 1);
        }
        backend->adapter_ready_ = true; // Mark as ready even on failure to unblock waiting
    }
}

void DX8WebGPUBackend::device_callback(WGpuRequestDeviceStatus status, WGpuDevice device, const char* message, void* user_data) {
    DX8WebGPUBackend* backend = static_cast<DX8WebGPUBackend*>(user_data);
    
    if (status == WGPU_REQUEST_DEVICE_STATUS_SUCCESS && device) {
        backend->device_ = device;
        backend->device_ready_ = true;
        DX8GL_INFO("WebGPU device obtained successfully");
    } else {
        DX8GL_ERROR("Failed to obtain WebGPU device: %s", message ? message : "Unknown error");
        if (message) {
            strncpy(backend->error_buffer_, message, sizeof(backend->error_buffer_) - 1);
        }
        backend->device_ready_ = true; // Mark as ready even on failure to unblock waiting
    }
}

void DX8WebGPUBackend::buffer_map_callback(WGpuBufferMapAsyncStatus status, void* user_data) {
    DX8WebGPUBackend* backend = static_cast<DX8WebGPUBackend*>(user_data);
    
    if (status == WGPU_BUFFER_MAP_ASYNC_STATUS_SUCCESS) {
        backend->buffer_mapped_ = true;
        backend->framebuffer_ready_ = true;
        
        // If there's an async callback registered, process it now
        if (backend->framebuffer_callback_ && backend->framebuffer_) {
            // Copy data from the mapped buffer to the framebuffer
            backend->framebuffer_->read_from_gpu([backend](void* dest, size_t size) {
                const void* mapped_data = wgpu_buffer_get_const_mapped_range(
                    backend->readback_buffer_, 0, WGPU_WHOLE_MAP_SIZE);
                if (mapped_data) {
                    memcpy(dest, mapped_data, size);
                    return true;
                }
                return false;
            });
            
            // Unmap the buffer
            wgpu_buffer_unmap(backend->readback_buffer_);
            
            // Call the user callback with the framebuffer data
            backend->framebuffer_callback_(
                backend->framebuffer_->get_data(),
                backend->width_,
                backend->height_,
                backend->framebuffer_->get_gl_format(),
                backend->framebuffer_callback_user_data_
            );
            
            // Clear the callback
            backend->framebuffer_callback_ = nullptr;
            backend->framebuffer_callback_user_data_ = nullptr;
        }
    } else {
        DX8GL_ERROR("Failed to map WebGPU buffer: status=%d", status);
        backend->buffer_mapped_ = false;
        backend->framebuffer_ready_ = false;
        
        // Call the callback with null to indicate failure
        if (backend->framebuffer_callback_) {
            backend->framebuffer_callback_(
                nullptr, 0, 0, 0,
                backend->framebuffer_callback_user_data_
            );
            backend->framebuffer_callback_ = nullptr;
            backend->framebuffer_callback_user_data_ = nullptr;
        }
    }
}

// Cube texture support methods
WGpuTexture DX8WebGPUBackend::create_cube_texture(uint32_t size, uint32_t mip_levels, WGpuTextureFormat format) {
    if (!device_) {
        DX8GL_ERROR("Cannot create cube texture: device not initialized");
        return nullptr;
    }
    
    return CubeTextureSupport::create_webgpu_cube_texture(device_, size, mip_levels, format);
}

WGpuTextureView DX8WebGPUBackend::create_cube_texture_view(WGpuTexture cube_texture) {
    if (!cube_texture) {
        DX8GL_ERROR("Cannot create cube texture view: invalid texture");
        return nullptr;
    }
    
    return CubeTextureSupport::create_cube_texture_view(cube_texture);
}

WGpuSampler DX8WebGPUBackend::create_cube_sampler(WGpuFilterMode min_filter, 
                                                  WGpuFilterMode mag_filter, 
                                                  WGpuMipmapFilterMode mipmap_filter) {
    if (!device_) {
        DX8GL_ERROR("Cannot create cube sampler: device not initialized");
        return nullptr;
    }
    
    return CubeTextureSupport::create_cube_sampler(device_, min_filter, mag_filter, mipmap_filter);
}

bool DX8WebGPUBackend::update_cube_face(WGpuTexture cube_texture, D3DCUBEMAP_FACES face, 
                                        uint32_t mip_level, const void* data, 
                                        uint32_t data_size, uint32_t row_pitch) {
    if (!cube_texture || !data) {
        DX8GL_ERROR("Cannot update cube face: invalid parameters");
        return false;
    }
    
    if (!queue_) {
        DX8GL_ERROR("Cannot update cube face: queue not initialized");
        return false;
    }
    
    // Calculate the face index (0-5)
    uint32_t face_index = static_cast<uint32_t>(face);
    if (face_index > 5) {
        DX8GL_ERROR("Invalid cube face index: %u", face_index);
        return false;
    }
    
    // Write texture data
    WGpuImageCopyTexture destination = {};
    destination.texture = cube_texture;
    destination.mipLevel = mip_level;
    destination.origin.x = 0;
    destination.origin.y = 0;
    destination.origin.z = face_index;  // Z coordinate selects the cube face
    destination.aspect = WGPU_TEXTURE_ASPECT_ALL;
    
    WGpuTextureDataLayout data_layout = {};
    data_layout.offset = 0;
    data_layout.bytesPerRow = row_pitch;
    data_layout.rowsPerImage = 0;  // Not used for 2D textures
    
    // Calculate texture size at this mip level from data_size and row_pitch
    // Assuming square cube faces
    uint32_t mip_size = row_pitch / 4;  // Assuming RGBA format (4 bytes per pixel)
    if (mip_size == 0) mip_size = 1;
    
    WGpuExtent3D write_size = {};
    write_size.width = mip_size;
    write_size.height = mip_size;
    write_size.depthOrArrayLayers = 1;
    
    wgpu_queue_write_texture(queue_, &destination, data, data_size, &data_layout, &write_size);
    
    DX8GL_TRACE("Updated cube texture face %u, mip level %u", face, mip_level);
    return true;
}

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU