#include "osmesa_backend.h"
#include "logger.h"
#include "osmesa_gl_loader.h"
#include "blue_screen.h"
#include <cstring>
#include <cstdlib>

#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>

namespace dx8gl {

DX8OSMesaBackend::DX8OSMesaBackend()
    : context_(nullptr)
    , framebuffer_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false) {
    error_buffer_[0] = '\0';
}

DX8OSMesaBackend::~DX8OSMesaBackend() {
    shutdown();
}

bool DX8OSMesaBackend::initialize(int width, int height) {
    if (initialized_) {
        return true;
    }
    
    DX8GL_INFO("Initializing OSMesa backend %dx%d", width, height);
    
    // Try to create OpenGL 4.5 Core context first
    DX8GL_INFO("Attempting to create OpenGL 4.5 Core context with OSMesaCreateContextAttribs");
    
    // Define context attributes for OpenGL 4.5 Core
    const int attribs[] = {
        OSMESA_FORMAT, OSMESA_RGBA,
        OSMESA_DEPTH_BITS, 32,
        OSMESA_STENCIL_BITS, 8,
        OSMESA_ACCUM_BITS, 16,
        OSMESA_PROFILE, OSMESA_CORE_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION, 4,
        OSMESA_CONTEXT_MINOR_VERSION, 5,
        0  // End of attributes
    };
    
    context_ = OSMesaCreateContextAttribs(attribs, NULL);
    
    if (!context_) {
        DX8GL_WARNING("OSMesaCreateContextAttribs failed, trying legacy OSMesaCreateContextExt");
        // Fall back to legacy context creation
        context_ = OSMesaCreateContextExt(OSMESA_RGBA, 24, 8, 0, NULL);
        if (!context_) {
            DX8GL_WARNING("OSMesaCreateContextExt failed, trying OSMesaCreateContext");
            context_ = OSMesaCreateContext(OSMESA_RGBA, NULL);
            if (!context_) {
                snprintf(error_buffer_, sizeof(error_buffer_), 
                         "Failed to create OSMesa context");
                DX8GL_ERROR("%s", error_buffer_);
                return false;
            }
        }
    }
    
    // Allocate framebuffer - use GL_UNSIGNED_BYTE format (standard for OSMesa)
    size_t buffer_size = width * height * 4 * sizeof(GLubyte); // RGBA bytes
    framebuffer_ = malloc(buffer_size);
    if (!framebuffer_) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to allocate framebuffer (%zu bytes)", buffer_size);
        DX8GL_ERROR("%s", error_buffer_);
        OSMesaDestroyContext(context_);
        context_ = nullptr;
        return false;
    }
    
    // Clear to black
    memset(framebuffer_, 0, buffer_size);
    
    // Make context current with our framebuffer - use GL_UNSIGNED_BYTE format
    if (!OSMesaMakeCurrent(context_, framebuffer_, GL_UNSIGNED_BYTE, width, height)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make OSMesa context current");
        DX8GL_ERROR("%s", error_buffer_);
        free(framebuffer_);
        framebuffer_ = nullptr;
        OSMesaDestroyContext(context_);
        context_ = nullptr;
        return false;
    }
    
    width_ = width;
    height_ = height;
    initialized_ = true;
    
    // Query OSMesa information
    int ctx_width, ctx_height, max_width, max_height;
    OSMesaGetIntegerv(OSMESA_WIDTH, &ctx_width);
    OSMesaGetIntegerv(OSMESA_HEIGHT, &ctx_height);
    OSMesaGetIntegerv(OSMESA_MAX_WIDTH, &max_width);
    OSMesaGetIntegerv(OSMESA_MAX_HEIGHT, &max_height);
    
    // Query actual context profile
    GLint profile_mask = 0;
    GLint major_version = 0;
    GLint minor_version = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    
    DX8GL_INFO("OSMesa version: %d.%d.%d", OSMESA_MAJOR_VERSION, OSMESA_MINOR_VERSION, OSMESA_PATCH_VERSION);
    DX8GL_INFO("OSMesa context: %dx%d (max: %dx%d)", ctx_width, ctx_height, max_width, max_height);
    DX8GL_INFO("Actual OpenGL context: version %d.%d, profile mask=0x%x", major_version, minor_version, profile_mask);
    
    // Query comprehensive OpenGL info
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    
    DX8GL_INFO("=== OSMesa Backend OpenGL Capabilities ===");
    DX8GL_INFO("OpenGL vendor: %s", vendor ? vendor : "Unknown");
    DX8GL_INFO("OpenGL renderer: %s", renderer ? renderer : "Unknown");
    DX8GL_INFO("OpenGL version: %s", version ? version : "Unknown");
    DX8GL_INFO("GLSL version: %s", glsl_version ? glsl_version : "Unknown");
    
    // Initialize OpenGL function pointers via OSMesaGetProcAddress
    DX8GL_INFO("Initializing OpenGL function pointers via OSMesaGetProcAddress");
    if (!InitializeOSMesaGL()) {
        DX8GL_ERROR("Failed to initialize OpenGL function pointers via OSMesaGetProcAddress");
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to load OpenGL functions via OSMesaGetProcAddress");
        // Don't fail initialization, some functions might not be needed
    }
    
    return true;
}

void DX8OSMesaBackend::shutdown() {
    if (!initialized_) {
        return;
    }
    
    DX8GL_INFO("Shutting down OSMesa backend");
    
    if (context_) {
        OSMesaDestroyContext(context_);
        context_ = nullptr;
    }
    
    if (framebuffer_) {
        free(framebuffer_);
        framebuffer_ = nullptr;
    }
    
    width_ = 0;
    height_ = 0;
    initialized_ = false;
}

bool DX8OSMesaBackend::make_current() {
    if (!initialized_ || !context_ || !framebuffer_) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Context not initialized");
        return false;
    }
    
    if (!OSMesaMakeCurrent(context_, framebuffer_, GL_UNSIGNED_BYTE, width_, height_)) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to make OSMesa context current");
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    return true;
}

void* DX8OSMesaBackend::get_framebuffer(int& width, int& height, int& format) {
    width = width_;
    height = height_;
    format = GL_RGBA;  // OSMesa uses RGBA format
    return framebuffer_;
}

bool DX8OSMesaBackend::resize(int width, int height) {
    if (!initialized_) {
        return false;
    }
    
    if (width == width_ && height == height_) {
        return true;
    }
    
    DX8GL_INFO("Resizing OSMesa backend from %dx%d to %dx%d", 
               width_, height_, width, height);
    
    // Allocate new framebuffer - use GL_UNSIGNED_BYTE format
    size_t buffer_size = width * height * 4 * sizeof(GLubyte); // RGBA bytes
    void* new_framebuffer = malloc(buffer_size);
    if (!new_framebuffer) {
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to allocate new framebuffer (%zu bytes)", buffer_size);
        DX8GL_ERROR("%s", error_buffer_);
        return false;
    }
    
    // Clear to black
    memset(new_framebuffer, 0, buffer_size);
    
    // Free old framebuffer
    if (framebuffer_) {
        free(framebuffer_);
    }
    
    framebuffer_ = new_framebuffer;
    width_ = width;
    height_ = height;
    
    // Update context with new framebuffer
    return make_current();
}

bool DX8OSMesaBackend::has_extension(const char* extension) const {
    if (!initialized_ || !extension) {
        return false;
    }
    
    // Use modern OpenGL 3.0+ method for Core profile
    GLint ext_count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
    
    for (GLint i = 0; i < ext_count; i++) {
        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (ext && strcmp(ext, extension) == 0) {
            return true;
        }
    }
    
    return false;
}

const char* DX8OSMesaBackend::get_error() const {
    return error_buffer_[0] ? error_buffer_ : "No error";
}

void DX8OSMesaBackend::show_blue_screen(const char* error_msg) {
    if (!framebuffer_ || !initialized_) {
        return;
    }
    
    DX8GL_ERROR("Showing blue screen due to error: %s", error_msg ? error_msg : "Unknown error");
    
    // Fill framebuffer with blue screen
    BlueScreen::fill_framebuffer(framebuffer_, width_, height_, error_msg);
    
    // Make sure the blue screen is visible by flushing GL commands
    if (context_ && OSMesaGetCurrentContext() == context_) {
        glFinish();
    }
}

} // namespace dx8gl

#endif // DX8GL_HAS_OSMESA