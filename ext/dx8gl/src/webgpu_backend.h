#ifndef DX8GL_WEBGPU_BACKEND_H
#define DX8GL_WEBGPU_BACKEND_H

#include "render_backend.h"
#include "offscreen_framebuffer.h"

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"
#include <memory>
#include <vector>

namespace dx8gl {

/**
 * WebGPU rendering backend implementation
 * 
 * This backend uses WebGPU to render to an offscreen canvas,
 * providing modern GPU acceleration that works on web platforms
 * as well as native desktop platforms with WebGPU support.
 */
class DX8WebGPUBackend : public DX8RenderBackend {
public:
    DX8WebGPUBackend();
    virtual ~DX8WebGPUBackend();
    
    // DX8RenderBackend interface implementation
    virtual bool initialize(int width, int height) override;
    virtual bool make_current() override;
    virtual void* get_framebuffer(int& width, int& height, int& format) override;
    virtual bool resize(int width, int height) override;
    virtual void shutdown() override;
    virtual DX8BackendType get_type() const override { return DX8GL_BACKEND_WEBGPU; }
    virtual bool has_extension(const char* extension) const override;
    
    // WebGPU-specific methods
    WGpuDevice get_device() const { return device_; }
    WGpuQueue get_queue() const { return queue_; }
    WGpuTexture get_render_texture() const { return render_texture_; }
    WGpuCanvasContext get_canvas_context() const { return canvas_context_; }

private:
    // WebGPU objects
    WGpuAdapter adapter_;
    WGpuDevice device_;
    WGpuQueue queue_;
    WGpuCanvasContext canvas_context_;
    
    // Rendering resources
    WGpuTexture render_texture_;
    WGpuTextureView render_texture_view_;
    WGpuBuffer readback_buffer_;
    
    // Framebuffer data
    std::unique_ptr<OffscreenFramebuffer> framebuffer_;
    int width_;
    int height_;
    bool initialized_;
    
    // Error handling
    char error_buffer_[512];
    
    // Private helper methods
    bool create_adapter();
    bool create_device();
    bool setup_offscreen_canvas();
    bool create_render_resources();
    bool setup_readback_buffer();
    void cleanup_resources();
    
    // WebGPU callback handlers
    static void adapter_callback(WGpuRequestAdapterStatus status, WGpuAdapter adapter, const char* message, void* user_data);
    static void device_callback(WGpuRequestDeviceStatus status, WGpuDevice device, const char* message, void* user_data);
    static void buffer_map_callback(WGpuBufferMapAsyncStatus status, void* user_data);
    
    // Synchronization for async operations
    bool adapter_ready_;
    bool device_ready_;
    bool buffer_mapped_;
};

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU

#endif // DX8GL_WEBGPU_BACKEND_H