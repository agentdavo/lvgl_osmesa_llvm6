#ifndef DX8GL_EGL_SURFACELESS_CONTEXT_H
#define DX8GL_EGL_SURFACELESS_CONTEXT_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "gl3_headers.h"

namespace dx8gl {

class EGLSurfacelessContext {
public:
    EGLSurfacelessContext();
    ~EGLSurfacelessContext();

    bool initialize(int width, int height);
    bool makeCurrent();
    bool swapBuffers();
    void shutdown();
    
    bool resize(int width, int height);
    void* getFramebuffer() const { return framebuffer_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    const char* getError() const { return error_buffer_; }
    bool isInitialized() const { return initialized_; }

private:
    bool checkExtensions();
    
    EGLDisplay display_;
    EGLContext context_;
    EGLConfig config_;
    
    // Framebuffer for reading pixels
    GLuint fbo_;
    GLuint color_texture_;
    GLuint depth_renderbuffer_;
    void* framebuffer_;
    
    int width_;
    int height_;
    bool initialized_;
    char error_buffer_[256];
};

} // namespace dx8gl

#endif // DX8GL_EGL_SURFACELESS_CONTEXT_H