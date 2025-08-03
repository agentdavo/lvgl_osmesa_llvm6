#include "osmesa_context.h"
#include "logger.h"
#include "osmesa_gl_loader.h"
#include <cstring>
#include <cstdlib>

#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>

namespace dx8gl {

DX8OSMesaContext::DX8OSMesaContext()
    : context_(nullptr)
    , framebuffer_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false) {
    error_buffer_[0] = '\0';
}

DX8OSMesaContext::~DX8OSMesaContext() {
    shutdown();
}

bool DX8OSMesaContext::initialize(int width, int height) {
    if (initialized_) {
        return true;
    }
    
    DX8GL_INFO("Initializing OSMesa context %dx%d", width, height);
    
    // Try to create OpenGL 3.3 Core context first
    DX8GL_INFO("Attempting to create OpenGL 3.3 Core context with OSMesaCreateContextAttribs");
    
    // Define context attributes for OpenGL 3.3 Core
    const int attribs[] = {
        OSMESA_FORMAT, OSMESA_RGBA,
        OSMESA_DEPTH_BITS, 24,
        OSMESA_STENCIL_BITS, 8,
        OSMESA_ACCUM_BITS, 0,
        OSMESA_PROFILE, OSMESA_CORE_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION, 3,
        OSMESA_CONTEXT_MINOR_VERSION, 3,
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
    
    DX8GL_INFO("=== OSMesa OpenGL Capabilities ===");
    DX8GL_INFO("OpenGL vendor: %s", vendor ? vendor : "Unknown");
    DX8GL_INFO("OpenGL renderer: %s", renderer ? renderer : "Unknown");
    DX8GL_INFO("OpenGL version: %s", version ? version : "Unknown");
    DX8GL_INFO("GLSL version: %s", glsl_version ? glsl_version : "Unknown");
    
    // Query OpenGL limits and capabilities
    GLint max_texture_size, max_texture_units, max_vertex_attribs, max_fragment_uniforms;
    GLint max_vertex_uniforms, max_varying_vectors, max_renderbuffer_size;
    GLint max_viewport[2], max_combined_texture_units;
    
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &max_fragment_uniforms);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &max_vertex_uniforms);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &max_varying_vectors);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_texture_units);
    
    DX8GL_INFO("=== OpenGL Limits ===");
    DX8GL_INFO("Max texture size: %d", max_texture_size);
    DX8GL_INFO("Max texture units: %d", max_texture_units);
    DX8GL_INFO("Max combined texture units: %d", max_combined_texture_units);
    DX8GL_INFO("Max vertex attributes: %d", max_vertex_attribs);
    DX8GL_INFO("Max vertex uniforms: %d", max_vertex_uniforms);
    DX8GL_INFO("Max fragment uniforms: %d", max_fragment_uniforms);
    DX8GL_INFO("Max varying vectors: %d", max_varying_vectors);
    DX8GL_INFO("Max renderbuffer size: %d", max_renderbuffer_size);
    DX8GL_INFO("Max viewport: %dx%d", max_viewport[0], max_viewport[1]);
    
    // Query extensions
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    DX8GL_INFO("=== OpenGL Extensions ===");
    if (extensions) {
        // Count extensions
        int ext_count = 0;
        const char* ptr = extensions;
        while (*ptr) {
            if (*ptr == ' ') ext_count++;
            ptr++;
        }
        ext_count++; // Last extension doesn't have trailing space
        
        DX8GL_INFO("Extension count: %d", ext_count);
        
        // Log key extensions for DirectX 8 compatibility
        const char* key_extensions[] = {
            "GL_ARB_framebuffer_object",
            "GL_ARB_vertex_buffer_object", 
            "GL_ARB_pixel_buffer_object",
            "GL_ARB_texture_non_power_of_two",
            "GL_ARB_vertex_shader",
            "GL_ARB_fragment_shader",
            "GL_ARB_shading_language_100",
            "GL_EXT_framebuffer_object",
            "GL_EXT_blend_equation_separate",
            "GL_EXT_blend_func_separate",
            "GL_EXT_texture_compression_s3tc",
            "GL_OES_standard_derivatives",
            "GL_OES_vertex_array_object"
        };
        
        DX8GL_INFO("=== Key Extensions for DirectX 8 Compatibility ===");
        for (size_t i = 0; i < sizeof(key_extensions) / sizeof(key_extensions[0]); i++) {
            if (strstr(extensions, key_extensions[i])) {
                DX8GL_INFO("✓ %s", key_extensions[i]);
            } else {
                DX8GL_INFO("✗ %s", key_extensions[i]);
            }
        }
        
        // Log all extensions (truncated for readability)
        DX8GL_INFO("=== All Extensions (first 1000 chars) ===");
        char ext_buffer[1001];
        strncpy(ext_buffer, extensions, 1000);
        ext_buffer[1000] = '\0';
        DX8GL_INFO("%s%s", ext_buffer, strlen(extensions) > 1000 ? "..." : "");
    } else {
        DX8GL_INFO("No extensions available or GL_EXTENSIONS query failed");
    }
    
    DX8GL_INFO("=== OSMesa Context Analysis Complete ===");
    
    // Initialize OpenGL function pointers via OSMesaGetProcAddress
    DX8GL_INFO("Initializing OpenGL function pointers via OSMesaGetProcAddress");
    if (!InitializeOSMesaGL()) {
        DX8GL_ERROR("Failed to initialize OpenGL function pointers via OSMesaGetProcAddress");
        snprintf(error_buffer_, sizeof(error_buffer_), 
                 "Failed to load OpenGL functions via OSMesaGetProcAddress");
        // Don't fail initialization, some functions might not be needed
        // return false;
    }
    
    return true;
}

void DX8OSMesaContext::shutdown() {
    if (!initialized_) {
        return;
    }
    
    DX8GL_INFO("Shutting down OSMesa context");
    
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

bool DX8OSMesaContext::make_current() {
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

bool DX8OSMesaContext::resize(int width, int height) {
    if (!initialized_) {
        return false;
    }
    
    if (width == width_ && height == height_) {
        return true;
    }
    
    DX8GL_INFO("Resizing OSMesa context from %dx%d to %dx%d", 
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

const char* DX8OSMesaContext::get_error() const {
    return error_buffer_[0] ? error_buffer_ : "No error";
}

} // namespace dx8gl

#endif // DX8GL_HAS_OSMESA