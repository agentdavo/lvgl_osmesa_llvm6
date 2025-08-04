#ifndef DX8GL_OSMESA_BACKEND_H
#define DX8GL_OSMESA_BACKEND_H

#include "render_backend.h"
#include <cstdint>
#include <memory>

#ifdef DX8GL_HAS_OSMESA
#include <GL/osmesa.h>

namespace dx8gl {

/**
 * OSMesa implementation of the DX8RenderBackend interface
 * 
 * This backend uses OSMesa for software rendering, providing
 * an off-screen rendering context without requiring a display.
 */
class DX8OSMesaBackend : public DX8RenderBackend {
public:
    DX8OSMesaBackend();
    ~DX8OSMesaBackend() override;
    
    // DX8RenderBackend interface implementation
    bool initialize(int width, int height) override;
    bool make_current() override;
    void* get_framebuffer(int& width, int& height, int& format) override;
    bool resize(int width, int height) override;
    void shutdown() override;
    DX8BackendType get_type() const override { return DX8_BACKEND_OSMESA; }
    bool has_extension(const char* extension) const override;
    
    // OSMesa-specific methods
    const char* get_error() const;
    void show_blue_screen(const char* error_msg = nullptr);

private:
    OSMesaContext context_;  // OSMesa context handle
    void* framebuffer_;
    int width_;
    int height_;
    bool initialized_;
    
    // Error message buffer
    mutable char error_buffer_[256];
};

} // namespace dx8gl

#else
// Stub implementation when OSMesa is not available
namespace dx8gl {
class DX8OSMesaBackend : public DX8RenderBackend {
public:
    DX8OSMesaBackend() {}
    ~DX8OSMesaBackend() override {}
    bool initialize(int width, int height) override { return false; }
    bool make_current() override { return false; }
    void* get_framebuffer(int& width, int& height, int& format) override { 
        width = height = format = 0;
        return nullptr; 
    }
    bool resize(int width, int height) override { return false; }
    void shutdown() override {}
    DX8BackendType get_type() const override { return DX8_BACKEND_OSMESA; }
    bool has_extension(const char* extension) const override { return false; }
    const char* get_error() const { return "OSMesa not compiled in"; }
};
} // namespace dx8gl
#endif

#endif // DX8GL_OSMESA_BACKEND_H