#include "egl_backend.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>

#ifdef DX8GL_HAS_EGL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace dx8gl {

DX8EGLBackend::DX8EGLBackend()
    : display_(EGL_NO_DISPLAY)
    , context_(EGL_NO_CONTEXT)
    , config_(nullptr)
    , surface_(EGL_NO_SURFACE)
    , framebuffer_id_(0)
    , color_texture_id_(0)
    , depth_renderbuffer_id_(0)
    , framebuffer_data_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false) {
    error_buffer_[0] = '\0';
}

DX8EGLBackend::~DX8EGLBackend() {
    shutdown();
}

bool DX8EGLBackend::initialize(int width, int height) {
    if (initialized_) {
        return true;
    }
    
    DX8GL_INFO("Initializing EGL backend %dx%d", width, height);
    
    // Get default display
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
    
    DX8GL_INFO("EGL version: %d.%d", major, minor);
    
    // Bind OpenGL ES API
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to bind OpenGL ES API");
        DX8GL_ERROR("%s", error_buffer_);
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Choose config that supports pbuffer - try ES2 first for wider compatibility
    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,  // Try ES 2.0 for better compatibility
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
        EGLint error = eglGetError();
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to choose EGL config (error: 0x%x)", error);
        DX8GL_ERROR("%s", error_buffer_);
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    DX8GL_INFO("Found %d matching EGL configs", num_configs);
    
    // Create context
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,  // OpenGL ES 2.0 for better compatibility
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
    
    // Check for surfaceless context support
    const char* extensions = eglQueryString(display_, EGL_EXTENSIONS);
    bool has_surfaceless = extensions && strstr(extensions, "EGL_KHR_surfaceless_context");
    
    if (has_surfaceless) {
        DX8GL_INFO("EGL_KHR_surfaceless_context is supported, using surfaceless rendering");
        surface_ = EGL_NO_SURFACE;
    } else {
        // Try to create a surface - first pbuffer, then window surface fallback
        const EGLint pbuffer_attribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        
        surface_ = eglCreatePbufferSurface(display_, config_, pbuffer_attribs);
        if (surface_ == EGL_NO_SURFACE) {
            // Clear error and try window surface
            eglGetError();
            DX8GL_WARNING("Failed to create pbuffer surface, trying window surface fallback");
            
            surface_ = eglCreateWindowSurface(display_, config_, (EGLNativeWindowType)NULL, NULL);
        }
        
        if (eglGetError() != EGL_SUCCESS) {
            snprintf(error_buffer_, sizeof(error_buffer_), 
                     "Failed to create EGL surface");
            DX8GL_ERROR("%s", error_buffer_);
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
            eglTerminate(display_);
            display_ = EGL_NO_DISPLAY;
            return false;
        }
        
        DX8GL_INFO("Successfully created EGL surface");
    }
    
    // Make context current with the created surface
    eglMakeCurrent(display_, surface_, surface_, context_);
    if (eglGetError() != EGL_SUCCESS) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make EGL context current");
        DX8GL_ERROR("%s", error_buffer_);
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
        eglDestroyContext(display_, context_);
        context_ = EGL_NO_CONTEXT;
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Turn off vsync to maximize performance
    eglSwapInterval(display_, 0);
    
    // Create offscreen framebuffer
    if (!create_offscreen_framebuffer(width, height)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display_, context_);
        context_ = EGL_NO_CONTEXT;
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        return false;
    }
    
    // Query OpenGL info
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    
    DX8GL_INFO("=== EGL Backend OpenGL Capabilities ===");
    DX8GL_INFO("OpenGL vendor: %s", vendor ? vendor : "Unknown");
    DX8GL_INFO("OpenGL renderer: %s", renderer ? renderer : "Unknown");
    DX8GL_INFO("OpenGL version: %s", version ? version : "Unknown");
    DX8GL_INFO("GLSL version: %s", glsl_version ? glsl_version : "Unknown");
    
    width_ = width;
    height_ = height;
    initialized_ = true;
    
    return true;
}

bool DX8EGLBackend::create_offscreen_framebuffer(int width, int height) {
    DX8GL_INFO("Creating offscreen framebuffer %dx%d", width, height);
    
    // Check for GL errors before we start
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        DX8GL_WARNING("GL error before framebuffer creation: 0x%x", err);
    }
    
    // Generate framebuffer
    glGenFramebuffers(1, &framebuffer_id_);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        DX8GL_ERROR("glGenFramebuffers failed with error: 0x%x", err);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    
    // Create color texture
    glGenTextures(1, &color_texture_id_);
    glBindTexture(GL_TEXTURE_2D, color_texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_id_, 0);
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &depth_renderbuffer_id_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_id_);
    // Use separate depth and stencil for ES 2.0 compatibility
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_id_);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char* status_str = "Unknown";
        switch(status) {
            case 0: status_str = "Invalid enum (GL function not loaded?)"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: status_str = "Incomplete attachment"; break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: status_str = "Missing attachment"; break;
            case GL_FRAMEBUFFER_UNSUPPORTED: status_str = "Unsupported"; break;
            default: break;
        }
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Framebuffer incomplete: 0x%x (%s)", status, status_str);
        DX8GL_ERROR("%s", error_buffer_);
        destroy_offscreen_framebuffer();
        return false;
    }
    
    // Allocate CPU-side buffer for readback
    size_t buffer_size = width * height * 4; // RGBA
    framebuffer_data_ = malloc(buffer_size);
    if (!framebuffer_data_) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to allocate framebuffer data (%zu bytes)", buffer_size);
        DX8GL_ERROR("%s", error_buffer_);
        destroy_offscreen_framebuffer();
        return false;
    }
    
    // Clear to black
    memset(framebuffer_data_, 0, buffer_size);
    
    DX8GL_INFO("Created offscreen framebuffer %dx%d", width, height);
    return true;
}

void DX8EGLBackend::destroy_offscreen_framebuffer() {
    if (framebuffer_id_) {
        glDeleteFramebuffers(1, &framebuffer_id_);
        framebuffer_id_ = 0;
    }
    
    if (color_texture_id_) {
        glDeleteTextures(1, &color_texture_id_);
        color_texture_id_ = 0;
    }
    
    if (depth_renderbuffer_id_) {
        glDeleteRenderbuffers(1, &depth_renderbuffer_id_);
        depth_renderbuffer_id_ = 0;
    }
    
    if (framebuffer_data_) {
        free(framebuffer_data_);
        framebuffer_data_ = nullptr;
    }
}

bool DX8EGLBackend::read_framebuffer_data() {
    if (!framebuffer_id_ || !framebuffer_data_) {
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_data_);
    
    return true;
}

void DX8EGLBackend::shutdown() {
    if (!initialized_) {
        return;
    }
    
    DX8GL_INFO("Shutting down EGL backend");
    
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        destroy_offscreen_framebuffer();
        
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
    
    width_ = 0;
    height_ = 0;
    initialized_ = false;
}

bool DX8EGLBackend::make_current() {
    if (!initialized_ || display_ == EGL_NO_DISPLAY || context_ == EGL_NO_CONTEXT || surface_ == EGL_NO_SURFACE) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Context not initialized");
        return false;
    }
    
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make EGL context current");
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    // Bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    
    return true;
}

void* DX8EGLBackend::get_framebuffer(int& width, int& height, int& format) {
    width = width_;
    height = height_;
    format = GL_RGBA;
    
    // Read framebuffer data from GPU
    if (initialized_ && framebuffer_data_) {
        read_framebuffer_data();
    }
    
    return framebuffer_data_;
}

bool DX8EGLBackend::resize(int width, int height) {
    if (!initialized_) {
        return false;
    }
    
    if (width == width_ && height == height_) {
        return true;
    }
    
    DX8GL_INFO("Resizing EGL backend from %dx%d to %dx%d", 
               width_, height_, width, height);
    
    // Make context current
    if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make context current for resize");
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    // Destroy old framebuffer
    destroy_offscreen_framebuffer();
    
    // Create new framebuffer
    if (!create_offscreen_framebuffer(width, height)) {
        return false;
    }
    
    width_ = width;
    height_ = height;
    
    return true;
}

bool DX8EGLBackend::has_extension(const char* extension) const {
    if (!initialized_ || !extension) {
        return false;
    }
    
    // Check EGL extensions
    const char* egl_extensions = eglQueryString(display_, EGL_EXTENSIONS);
    if (egl_extensions && strstr(egl_extensions, extension)) {
        return true;
    }
    
    // Check OpenGL extensions
    const char* gl_extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (gl_extensions && strstr(gl_extensions, extension)) {
        return true;
    }
    
    return false;
}

const char* DX8EGLBackend::get_error() const {
    return error_buffer_[0] ? error_buffer_ : "No error";
}

} // namespace dx8gl

#endif // DX8GL_HAS_EGL