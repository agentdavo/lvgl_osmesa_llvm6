#include "egl_surfaceless_context.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>
#include <cassert>

namespace dx8gl {

EGLSurfacelessContext::EGLSurfacelessContext()
    : display_(EGL_NO_DISPLAY)
    , context_(EGL_NO_CONTEXT)
    , config_(nullptr)
    , fbo_(0)
    , color_texture_(0)
    , depth_renderbuffer_(0)
    , framebuffer_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false) {
    error_buffer_[0] = '\0';
}

EGLSurfacelessContext::~EGLSurfacelessContext() {
    shutdown();
}

bool EGLSurfacelessContext::checkExtensions() {
    // Check client extensions first
    const char* clientExtensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (clientExtensions) {
        DX8GL_INFO("EGL Client Extensions: %s", clientExtensions);
    }
    
    // Check display extensions
    const char* displayExtensions = eglQueryString(display_, EGL_EXTENSIONS);
    if (!displayExtensions) {
        DX8GL_ERROR("Failed to query EGL display extensions");
        return false;
    }
    
    DX8GL_INFO("EGL Display Extensions: %s", displayExtensions);
    
    // Check for required extensions - based on the WPEFramework example
    bool has_no_config = strstr(displayExtensions, "EGL_KHR_no_config_context") != nullptr;
    bool has_configless = strstr(displayExtensions, "EGL_MESA_configless_context") != nullptr;
    bool has_surfaceless = strstr(displayExtensions, "EGL_KHR_surfaceless_context") != nullptr;
    
    DX8GL_INFO("EGL_KHR_no_config_context: %s", has_no_config ? "YES" : "NO");
    DX8GL_INFO("EGL_MESA_configless_context: %s", has_configless ? "YES" : "NO");
    DX8GL_INFO("EGL_KHR_surfaceless_context: %s", has_surfaceless ? "YES" : "NO");
    
    // According to the example, all three should be present
    if (!has_no_config) {
        DX8GL_WARN("EGL_KHR_no_config_context not found (may still work with MESA_configless_context)");
    }
    
    if (!has_configless) {
        DX8GL_WARN("EGL_MESA_configless_context not found (may still work with KHR_no_config_context)");
    }
    
    // At least one configless extension is required
    if (!has_no_config && !has_configless) {
        snprintf(error_buffer_, sizeof(error_buffer_),
                 "Neither EGL_KHR_no_config_context nor EGL_MESA_configless_context is supported");
        return false;
    }
    
    // Surfaceless context is required
    if (!has_surfaceless) {
        snprintf(error_buffer_, sizeof(error_buffer_),
                 "EGL_KHR_surfaceless_context is not supported");
        return false;
    }
    
    return true;
}

bool EGLSurfacelessContext::initialize(int width, int height) {
    if (initialized_) {
        return true;
    }
    
    DX8GL_INFO("Initializing EGL surfaceless context %dx%d", width, height);
    
    // Get default display (surfaceless)
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to get EGL display");
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(display_, &major, &minor)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to initialize EGL");
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    DX8GL_INFO("EGL version %d.%d", major, minor);
    
    // Check required extensions
    if (!checkExtensions()) {
        DX8GL_ERROR("%s", error_buffer_);
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Bind OpenGL ES API
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to bind OpenGL ES API");
        DX8GL_ERROR("%s", error_buffer_);
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Choose config (configless context doesn't need this, but we'll try anyway)
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    EGLint num_configs;
    if (!eglChooseConfig(display_, config_attribs, &config_, 1, &num_configs) || num_configs == 0) {
        // This is okay for configless context
        DX8GL_INFO("No EGL config chosen (using configless context)");
        config_ = nullptr;
    }
    
    // Create context
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs);
    if (context_ == EGL_NO_CONTEXT) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to create EGL context");
        DX8GL_ERROR("%s", error_buffer_);
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Make context current with no surface (surfaceless)
    if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make context current");
        DX8GL_ERROR("%s", error_buffer_);
        eglDestroyContext(display_, context_);
        eglTerminate(display_);
        context_ = EGL_NO_CONTEXT;
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Log GL info
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    DX8GL_INFO("GL Vendor: %s", vendor ? vendor : "Unknown");
    DX8GL_INFO("GL Renderer: %s", renderer ? renderer : "Unknown");
    DX8GL_INFO("GL Version: %s", version ? version : "Unknown");
    
    // Create FBO for rendering
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    
    // Create color texture
    glGenTextures(1, &color_texture_);
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_, 0);
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &depth_renderbuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    // Use separate depth and stencil for GLES2 compatibility
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_);
    
    // Check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Framebuffer incomplete: 0x%04X", status);
        DX8GL_ERROR("%s", error_buffer_);
        shutdown();
        return false;
    }
    
    // Allocate framebuffer for reading pixels
    size_t buffer_size = width * height * 4; // RGBA
    framebuffer_ = malloc(buffer_size);
    if (!framebuffer_) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to allocate framebuffer (%zu bytes)", buffer_size);
        DX8GL_ERROR("%s", error_buffer_);
        shutdown();
        return false;
    }
    
    width_ = width;
    height_ = height;
    initialized_ = true;
    
    DX8GL_INFO("EGL surfaceless context initialized successfully");
    return true;
}

bool EGLSurfacelessContext::makeCurrent() {
    if (!initialized_) {
        return false;
    }
    
    if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        DX8GL_ERROR("Failed to make EGL context current");
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    return true;
}

bool EGLSurfacelessContext::swapBuffers() {
    if (!initialized_) {
        return false;
    }
    
    // Read framebuffer for potential use
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_);
    glFinish();
    
    return true;
}

bool EGLSurfacelessContext::resize(int width, int height) {
    if (!initialized_ || (width == width_ && height == height_)) {
        return true;
    }
    
    DX8GL_INFO("Resizing EGL surfaceless context from %dx%d to %dx%d", width_, height_, width, height);
    
    // Resize color texture
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Resize depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    
    // Reallocate framebuffer
    size_t buffer_size = width * height * 4; // RGBA
    void* new_buffer = realloc(framebuffer_, buffer_size);
    if (!new_buffer) {
        DX8GL_ERROR("Failed to reallocate framebuffer");
        return false;
    }
    
    framebuffer_ = new_buffer;
    width_ = width;
    height_ = height;
    
    glViewport(0, 0, width, height);
    
    return true;
}

void EGLSurfacelessContext::shutdown() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (fbo_) {
            glDeleteFramebuffers(1, &fbo_);
            fbo_ = 0;
        }
        
        if (color_texture_) {
            glDeleteTextures(1, &color_texture_);
            color_texture_ = 0;
        }
        
        if (depth_renderbuffer_) {
            glDeleteRenderbuffers(1, &depth_renderbuffer_);
            depth_renderbuffer_ = 0;
        }
        
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
    
    if (framebuffer_) {
        free(framebuffer_);
        framebuffer_ = nullptr;
    }
    
    width_ = 0;
    height_ = 0;
    initialized_ = false;
}

} // namespace dx8gl