#include "dx8gl.h"
#include "d3d8_interface.h"
#include "d3d8_device.h"
#include "logger.h"
#include "osmesa_context.h"
#include "blue_screen.h"
#include "osmesa_gl_loader.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <cstring>
#include <cstdlib>

namespace dx8gl {

// Global state
static std::atomic<bool> g_initialized{false};
static std::mutex g_init_mutex;

// Thread-local error string
static thread_local std::string g_last_error;

void set_error(const std::string& error) {
    g_last_error = error;
    DX8GL_ERROR("%s", error.c_str());
}

} // namespace dx8gl

namespace dx8gl {

// Internal C++ implementation
extern "C" IDirect3D8* Direct3DCreate8_CPP(UINT SDKVersion) {
    if (SDKVersion != D3D_SDK_VERSION) {
        dx8gl::set_error("Invalid SDK version");
        return nullptr;
    }
    
    // Initialize dx8gl if needed
    if (!dx8gl::g_initialized) {
        std::lock_guard<std::mutex> lock(dx8gl::g_init_mutex);
        if (!dx8gl::g_initialized) {
            dx8gl::Logger::instance();  // Initialize logging
            DX8GL_INFO("Direct3DCreate8 called with SDK version %u", SDKVersion);
            
            DX8GL_INFO("dx8gl configured for OSMesa-only software rendering (no EGL overhead)");
            
            dx8gl::g_initialized = true;
        }
    }
    
    // Create and return IDirect3D8 interface
    auto d3d8 = new dx8gl::Direct3D8();
    if (!d3d8->initialize()) {
        delete d3d8;
        return nullptr;
    }
    
    DX8GL_INFO("Created IDirect3D8 interface at %p", d3d8);
    return d3d8;
}

} // namespace dx8gl

extern "C" {

// dx8gl specific functions
dx8gl_error dx8gl_init(const dx8gl_config* config) {
    std::lock_guard<std::mutex> lock(dx8gl::g_init_mutex);
    
    if (dx8gl::g_initialized) {
        return DX8GL_ERROR_ALREADY_INITIALIZED;
    }
    
    dx8gl::Logger::instance();  // Initialize logging
    
    // Apply configuration if provided
    if (config) {
        if (config->enable_logging) {
            dx8gl::Logger::instance().set_level(dx8gl::LogLevel::DEBUG);
        }
        
        if (config->log_callback) {
            // TODO: Implement custom log callback support
        }
    }
    
    // Check for OSMesa mode before initializing SDL3
    const char* use_osmesa = std::getenv("DX8GL_USE_OSMESA");
    bool osmesa_mode = (use_osmesa && std::strcmp(use_osmesa, "1") == 0);
    
    // OSMesa mode doesn't need SDL initialization
    
    DX8GL_INFO("dx8gl initialized");
    dx8gl::g_initialized = true;
    return DX8GL_SUCCESS;
}

void dx8gl_shutdown(void) {
    std::lock_guard<std::mutex> lock(dx8gl::g_init_mutex);
    
    if (!dx8gl::g_initialized) {
        return;
    }
    
    DX8GL_INFO("dx8gl shutting down");
    
    // OSMesa mode doesn't need shared context
    dx8gl::g_initialized = false;
}

dx8gl_error dx8gl_create_device(dx8gl_device** device) {
    if (!device) {
        return DX8GL_ERROR_INVALID_PARAMETER;
    }
    
    // TODO: Implement device creation
    return DX8GL_ERROR_NOT_SUPPORTED;
}

void dx8gl_destroy_device(dx8gl_device* device) {
    // TODO: Implement device destruction
}

dx8gl_error dx8gl_get_caps(dx8gl_device* device, dx8gl_caps* caps) {
    if (!device || !caps) {
        return DX8GL_ERROR_INVALID_PARAMETER;
    }
    
    // TODO: Query actual OpenGL ES capabilities
    caps->max_texture_size = 4096;
    caps->max_texture_units = 8;
    caps->max_vertex_shader_version = 0x0101;  // vs_1_1
    caps->max_pixel_shader_version = 0x0104;   // ps_1_4
    caps->max_vertex_shader_constants = 96;
    caps->max_pixel_shader_constants = 8;
    caps->supports_npot_textures = true;
    caps->supports_compressed_textures = true;
    caps->supports_cubemaps = true;
    caps->supports_volume_textures = false;
    
    return DX8GL_SUCCESS;
}

const char* dx8gl_get_error_string(void) {
    return dx8gl::g_last_error.c_str();
}

const char* dx8gl_get_version_string(void) {
    return DX8GL_VERSION_STRING;
}

// Context management
dx8gl_context* dx8gl_context_create(void) {
    return dx8gl_context_create_with_size(800, 600);
}

dx8gl_context* dx8gl_context_create_with_size(uint32_t width, uint32_t height) {
    auto ctx = new dx8gl::DX8OSMesaContext();
    
    if (!ctx->initialize(width, height)) {
        delete ctx;
        return nullptr;
    }
    
    return reinterpret_cast<dx8gl_context*>(ctx);
}

void dx8gl_context_destroy(dx8gl_context* context) {
    if (context) {
        auto ctx = reinterpret_cast<dx8gl::DX8OSMesaContext*>(context);
        delete ctx;
    }
}

bool dx8gl_context_make_current(dx8gl_context* context) {
    if (!context) {
        return false;
    }
    
    auto ctx = reinterpret_cast<dx8gl::DX8OSMesaContext*>(context);
    return ctx->make_current();
}

dx8gl_context* dx8gl_context_get_current(void) {
    // TODO: Implement current context tracking
    return nullptr;
}

void dx8gl_context_get_size(dx8gl_context* context, uint32_t* width, uint32_t* height) {
    if (!context || !width || !height) {
        return;
    }
    
    auto ctx = reinterpret_cast<dx8gl::DX8OSMesaContext*>(context);
    *width = static_cast<uint32_t>(ctx->get_width());
    *height = static_cast<uint32_t>(ctx->get_height());
}

// Shared framebuffer for integration
void* dx8gl_get_shared_framebuffer(int* width, int* height, int* frame_number, bool* updated) {
    static int fb_frame = 0;
    static std::mutex fb_mutex;
    static bool showing_blue_screen = false;
    
    std::lock_guard<std::mutex> lock(fb_mutex);
    
    // Get the global device instance
    auto device = dx8gl::get_global_device();
    if (!device) {
        return nullptr;
    }
    
    // Get OSMesa framebuffer from the device
    void* framebuffer = device->get_osmesa_framebuffer();
    if (!framebuffer) {
        return nullptr;
    }
    
    // Get dimensions from device
    int fb_width = 0, fb_height = 0;
    device->get_osmesa_dimensions(&fb_width, &fb_height);
    
    // Check for OpenGL errors and show blue screen if needed
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR && !showing_blue_screen) {
        DX8GL_ERROR("OpenGL error detected in dx8gl_get_shared_framebuffer: 0x%04X", gl_error);
        
        // Show blue screen
        const char* error_msg = nullptr;
        switch (gl_error) {
            case GL_INVALID_ENUM: error_msg = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error_msg = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: error_msg = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: error_msg = "GL_OUT_OF_MEMORY"; break;
            default: error_msg = "GL_ERROR"; break;
        }
        
        // Fill framebuffer with blue screen
        dx8gl::BlueScreen::fill_framebuffer(framebuffer, fb_width, fb_height, error_msg);
        showing_blue_screen = true;
        
        // Clear remaining errors
        while (glGetError() != GL_NO_ERROR) {}
    }
    
    // Update output parameters
    if (width) *width = fb_width;
    if (height) *height = fb_height;
    if (frame_number) *frame_number = fb_frame;
    if (updated) *updated = device->was_frame_presented();
    
    // Increment frame counter if a frame was presented
    if (device->was_frame_presented()) {
        fb_frame++;
        device->reset_frame_presented();
    }
    
    return framebuffer;
}

// Plugin management stubs
dx8gl_error dx8gl_load_plugin(const char* path) {
    return DX8GL_ERROR_NOT_SUPPORTED;
}

dx8gl_error dx8gl_unload_plugin(const char* name) {
    return DX8GL_ERROR_NOT_SUPPORTED;
}

dx8gl_error dx8gl_list_plugins(const char** names, size_t* count) {
    if (count) {
        *count = 0;
    }
    return DX8GL_SUCCESS;
}

} // extern "C"