#include "webgpu_backend.h"
#include "logger.h"
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
    , buffer_mapped_(false) {
    error_buffer_[0] = '\0';
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
    
    // Ensure we have the latest frame data by reading back from GPU
    if (!readback_buffer_) {
        width = height = format = 0;
        return nullptr;
    }
    
    // Map the readback buffer to get the latest frame data
    buffer_mapped_ = false;
    wgpu_buffer_map_async(readback_buffer_, WGPU_MAP_MODE_READ, 0, 
                         width_ * height_ * 4, buffer_map_callback, this);
    
    // Wait for the mapping to complete (this would be async in a real implementation)
    // For now, we'll use a simple polling approach
    int timeout = 100; // 100ms timeout
    while (!buffer_mapped_ && timeout-- > 0) {
#ifdef __EMSCRIPTEN__
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
    }
    
    width = width_;
    height = height_;
    format = framebuffer_->get_gl_format();
    return framebuffer_->get_data();
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
    DX8GL_INFO("Setting up WebGPU offscreen canvas");
    
#ifdef __EMSCRIPTEN__
    // Create an OffscreenCanvas for WebGPU rendering
    OffscreenCanvasId canvas_id = 1; // Use a fixed ID for simplicity
    
    // In a real implementation, you would create the OffscreenCanvas
    // This is a simplified version that assumes the canvas setup is handled elsewhere
    canvas_context_ = wgpu_offscreen_canvas_get_webgpu_context(canvas_id);
    
    if (!canvas_context_) {
        strncpy(error_buffer_, "Failed to get WebGPU canvas context", sizeof(error_buffer_) - 1);
        return false;
    }
    
    // Configure the canvas context
    WGpuCanvasConfiguration config = {};
    config.device = device_;
    config.format = WGPU_TEXTURE_FORMAT_BGRA8_UNORM;
    config.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
    config.alphaMode = WGPU_CANVAS_ALPHA_MODE_OPAQUE;
    
    wgpu_canvas_context_configure(canvas_context_, &config);
    
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
    } else {
        DX8GL_ERROR("Failed to map WebGPU buffer: status=%d", status);
        backend->buffer_mapped_ = false;
    }
}

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU