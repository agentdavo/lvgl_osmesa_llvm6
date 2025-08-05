#ifndef DX8GL_EGL_BACKEND_H
#define DX8GL_EGL_BACKEND_H

#include "render_backend.h"
#include "offscreen_framebuffer.h"
#include <cstdint>
#include <memory>

#ifdef DX8GL_HAS_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace dx8gl {

/**
 * EGL implementation of the DX8RenderBackend interface
 * 
 * This backend uses EGL with surfaceless context extension for
 * hardware-accelerated off-screen rendering without a display surface.
 */
class DX8EGLBackend : public DX8RenderBackend {
public:
    DX8EGLBackend();
    ~DX8EGLBackend() override;
    
    // DX8RenderBackend interface implementation
    bool initialize(int width, int height) override;
    bool make_current() override;
    void* get_framebuffer(int& width, int& height, int& format) override;
    bool resize(int width, int height) override;
    void shutdown() override;
    DX8BackendType get_type() const override { return DX8GL_BACKEND_EGL; }
    bool has_extension(const char* extension) const override;
    
    // EGL-specific methods
    const char* get_error() const;

private:
    EGLDisplay display_;
    EGLContext context_;
    EGLConfig config_;
    EGLSurface surface_;
    
    // Offscreen framebuffer resources
    unsigned int framebuffer_id_;
    unsigned int color_texture_id_;
    unsigned int depth_renderbuffer_id_;
    
    // CPU-side framebuffer helper
    std::unique_ptr<OffscreenFramebuffer> framebuffer_;
    int width_;
    int height_;
    bool initialized_;
    
    // Error message buffer
    mutable char error_buffer_[256];
    
    // Helper methods
    bool create_offscreen_framebuffer(int width, int height);
    void destroy_offscreen_framebuffer();
    bool read_framebuffer_data();
};

} // namespace dx8gl

#else
// Stub implementation when EGL is not available
namespace dx8gl {
class DX8EGLBackend : public DX8RenderBackend {
public:
    DX8EGLBackend() {}
    ~DX8EGLBackend() override {}
    bool initialize(int width, int height) override { return false; }
    bool make_current() override { return false; }
    void* get_framebuffer(int& width, int& height, int& format) override { 
        width = height = format = 0;
        return nullptr; 
    }
    bool resize(int width, int height) override { return false; }
    void shutdown() override {}
    DX8BackendType get_type() const override { return DX8GL_BACKEND_EGL; }
    bool has_extension(const char* extension) const override { return false; }
    const char* get_error() const { return "EGL not compiled in"; }
};
} // namespace dx8gl
#endif

#endif // DX8GL_EGL_BACKEND_H