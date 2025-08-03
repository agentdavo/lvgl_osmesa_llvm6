#ifndef DX8GL_OSMESA_CONTEXT_H
#define DX8GL_OSMESA_CONTEXT_H

#include <cstdint>
#include <memory>

#ifdef DX8GL_HAS_OSMESA
#include <GL/osmesa.h>

namespace dx8gl {

class DX8OSMesaContext {
public:
    DX8OSMesaContext();
    ~DX8OSMesaContext();
    
    bool initialize(int width, int height);
    void shutdown();
    
    bool make_current();
    void* get_framebuffer() const { return framebuffer_; }
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    
    // Resize the context and framebuffer
    bool resize(int width, int height);
    
    // Get the current error string
    const char* get_error() const;

private:
    OSMesaContext context_;  // OSMesa context handle
    void* framebuffer_;
    int width_;
    int height_;
    bool initialized_;
    
    // Error message
    mutable char error_buffer_[256];
};

} // namespace dx8gl

#else
// Stub implementation when OSMesa is not available
namespace dx8gl {
class DX8OSMesaContext {
public:
    DX8OSMesaContext() {}
    ~DX8OSMesaContext() {}
    bool initialize(int width, int height) { return false; }
    void shutdown() {}
    bool make_current() { return false; }
    void* get_framebuffer() const { return nullptr; }
    int get_width() const { return 0; }
    int get_height() const { return 0; }
    bool resize(int width, int height) { return false; }
    const char* get_error() const { return "OSMesa not compiled in"; }
};
} // namespace dx8gl
#endif

#endif // DX8GL_OSMESA_CONTEXT_H