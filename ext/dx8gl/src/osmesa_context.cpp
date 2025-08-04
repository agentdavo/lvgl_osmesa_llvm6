#include "osmesa_context.h"
#include "logger.h"
#include "osmesa_gl_loader.h"
#include "blue_screen.h"
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
    
    // Enable OpenGL debug output for error detection
    if (glDebugMessageCallback) {
        DX8GL_INFO("=== Enabling OpenGL Debug Output ===");
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure messages are delivered immediately
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
                                 GLsizei length, const GLchar* message, const void* userParam) {
            const char* sourceStr = "UNKNOWN";
            switch (source) {
                case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "WINDOW_SYSTEM"; break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "SHADER_COMPILER"; break;
                case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "THIRD_PARTY"; break;
                case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "APPLICATION"; break;
                case GL_DEBUG_SOURCE_OTHER: sourceStr = "OTHER"; break;
            }
            
            const char* typeStr = "UNKNOWN";
            switch (type) {
                case GL_DEBUG_TYPE_ERROR: typeStr = "ERROR"; break;
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED"; break;
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "UNDEFINED"; break;
                case GL_DEBUG_TYPE_PORTABILITY: typeStr = "PORTABILITY"; break;
                case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "PERFORMANCE"; break;
                case GL_DEBUG_TYPE_MARKER: typeStr = "MARKER"; break;
                case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "PUSH_GROUP"; break;
                case GL_DEBUG_TYPE_POP_GROUP: typeStr = "POP_GROUP"; break;
                case GL_DEBUG_TYPE_OTHER: typeStr = "OTHER"; break;
            }
            
            const char* severityStr = "UNKNOWN";
            switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH: severityStr = "HIGH"; break;
                case GL_DEBUG_SEVERITY_MEDIUM: severityStr = "MEDIUM"; break;
                case GL_DEBUG_SEVERITY_LOW: severityStr = "LOW"; break;
                case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "NOTIFICATION"; break;
            }
            
            if (type == GL_DEBUG_TYPE_ERROR) {
                DX8GL_ERROR("OpenGL ERROR [%s/%s/%s]: %s", sourceStr, typeStr, severityStr, message);
            } else if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
                DX8GL_WARN("OpenGL WARNING [%s/%s/%s]: %s", sourceStr, typeStr, severityStr, message);
            } else {
                DX8GL_INFO("OpenGL DEBUG [%s/%s/%s]: %s", sourceStr, typeStr, severityStr, message);
            }
        }, nullptr);
        
        // Filter out low-priority notifications to reduce noise
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        DX8GL_INFO("OpenGL debug output enabled with filtering");
    } else {
        DX8GL_WARN("OpenGL debug output not available (glDebugMessageCallback not found)");
    }
    
    // Query extensions (use modern OpenGL 3.0+ method for Core profile)
    DX8GL_INFO("=== OpenGL Extensions ===");
    GLint ext_count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
    
    if (ext_count > 0) {
        DX8GL_INFO("Extension count: %d", ext_count);
        
        // Log key extensions for DirectX 8 compatibility (OpenGL 3.3 Core profile)
        const char* key_extensions[] = {
            "GL_ARB_framebuffer_object",
            "GL_ARB_vertex_buffer_object", 
            "GL_ARB_pixel_buffer_object",
            "GL_ARB_texture_non_power_of_two",
            "GL_ARB_vertex_shader",
            "GL_ARB_fragment_shader",
            "GL_ARB_get_program_binary",        // Replaces GL_OES_get_program_binary
            "GL_EXT_framebuffer_object",
            "GL_EXT_blend_equation_separate",
            "GL_EXT_blend_func_separate", 
            "GL_EXT_texture_compression_s3tc",
            // Note: GL_OES_standard_derivatives -> Core in GLSL 3.30+ (dFdx, dFdy, fwidth built-in)
            // Note: GL_OES_vertex_array_object -> Core in OpenGL 3.3+ (VAO core functionality)
        };
        
        // Check for key extensions using modern OpenGL 3.0+ method
        DX8GL_INFO("=== Key Extensions for DirectX 8 Compatibility ===");
        for (size_t i = 0; i < sizeof(key_extensions) / sizeof(key_extensions[0]); i++) {
            bool found = false;
            for (GLint j = 0; j < ext_count; j++) {
                const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, j);
                if (ext && strcmp(ext, key_extensions[i]) == 0) {
                    found = true;
                    break;
                }
            }
            DX8GL_INFO("%s %s", found ? "✓" : "✗", key_extensions[i]);
        }
        
        // Log first 20 extensions for debugging
        DX8GL_INFO("=== Sample Extensions (first 20) ===");
        int log_count = ext_count < 20 ? ext_count : 20;
        for (int i = 0; i < log_count; i++) {
            const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
            if (ext) {
                DX8GL_INFO("  %s", ext);
            }
        }
        if (ext_count > 20) {
            DX8GL_INFO("  ... and %d more extensions", ext_count - 20);
        }
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

void DX8OSMesaContext::show_blue_screen(const char* error_msg) {
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