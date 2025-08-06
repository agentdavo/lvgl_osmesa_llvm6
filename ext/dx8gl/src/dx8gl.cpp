#include "dx8gl.h"
#include "d3d8_interface.h"
#include "d3d8_device.h"
#include "logger.h"
#ifdef DX8GL_HAS_OSMESA
#include "osmesa_context.h"
#include "osmesa_gl_loader.h"
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "render_backend.h"
#include "blue_screen.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>

// Device structure definition (in global namespace to match header)
struct dx8gl_device {
    std::unique_ptr<dx8gl::DX8RenderBackend> backend;
    dx8gl_stats stats;
    std::string last_error;
    bool initialized;
    
    dx8gl_device() : initialized(false) {
        // Initialize stats
        std::memset(&stats, 0, sizeof(stats));
    }
};

namespace dx8gl {

// Global state
static std::atomic<bool> g_initialized{false};
static std::mutex g_init_mutex;
static std::unique_ptr<DX8RenderBackend> g_render_backend;
static DX8BackendType g_selected_backend = DX8GL_BACKEND_OSMESA;
static std::vector<std::unique_ptr<::dx8gl_device>> g_devices;
static std::mutex g_devices_mutex;

// Context tracking
static thread_local dx8gl_context* g_current_context = nullptr;
static std::mutex g_context_mutex;

// Accessor for render backend
DX8RenderBackend* get_render_backend() {
    return g_render_backend.get();
}

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
        dx8gl_error result = dx8gl_init(nullptr);
        if (result != DX8GL_SUCCESS) {
            DX8GL_ERROR("Failed to initialize dx8gl: %d", result);
            return nullptr;
        }
    }
    
    DX8GL_INFO("Direct3DCreate8 called with SDK version %u", SDKVersion);
    
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
    
    // Check command line arguments for backend selection
    const char* args = std::getenv("DX8GL_ARGS");
    if (args) {
        if (std::strstr(args, "--backend=egl")) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_EGL;
            DX8GL_INFO("Selected EGL backend from command line");
        } else if (std::strstr(args, "--backend=osmesa")) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_OSMESA;
            DX8GL_INFO("Selected OSMesa backend from command line");
        } else if (std::strstr(args, "--backend=webgpu")) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_WEBGPU;
            DX8GL_INFO("Selected WebGPU backend from command line");
        } else if (std::strstr(args, "--backend=auto")) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_DEFAULT;
            DX8GL_INFO("Selected auto backend from command line");
        }
    }
    
    // Apply configuration if provided
    if (config) {
        if (config->enable_logging) {
            dx8gl::Logger::instance().set_level(dx8gl::LogLevel::DEBUG);
        }
        
        if (config->log_callback) {
            dx8gl::Logger::instance().set_callback(config->log_callback);
            DX8GL_INFO("Custom log callback registered");
        }
        
        // Backend selection from config
        if (config->backend_type != DX8GL_BACKEND_DEFAULT) {
            dx8gl::g_selected_backend = config->backend_type;
            DX8GL_INFO("Selected backend %d from config", config->backend_type);
        }
    }
    
    // Check for backend selection via environment variable
    const char* backend_env = std::getenv("DX8GL_BACKEND");
    if (backend_env) {
        if (std::strcmp(backend_env, "egl") == 0 || std::strcmp(backend_env, "EGL") == 0) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_EGL;
            DX8GL_INFO("Selected EGL backend from environment");
        } else if (std::strcmp(backend_env, "osmesa") == 0 || std::strcmp(backend_env, "OSMESA") == 0) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_OSMESA;
            DX8GL_INFO("Selected OSMesa backend from environment");
        } else if (std::strcmp(backend_env, "webgpu") == 0 || std::strcmp(backend_env, "WEBGPU") == 0) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_WEBGPU;
            DX8GL_INFO("Selected WebGPU backend from environment");
        } else if (std::strcmp(backend_env, "auto") == 0 || std::strcmp(backend_env, "AUTO") == 0) {
            dx8gl::g_selected_backend = DX8GL_BACKEND_DEFAULT;
            DX8GL_INFO("Selected auto backend from environment");
        } else {
            DX8GL_WARNING("Unknown backend in DX8GL_BACKEND: %s", backend_env);
        }
    }
    
    // Create render backend with fallback support
    dx8gl::g_render_backend = dx8gl::create_render_backend(dx8gl::g_selected_backend);
    if (!dx8gl::g_render_backend) {
        // If specific backend failed and it wasn't DEFAULT, try fallback
        if (dx8gl::g_selected_backend != DX8GL_BACKEND_DEFAULT && 
            dx8gl::g_selected_backend != DX8GL_BACKEND_OSMESA) {
            DX8GL_WARNING("Failed to create requested backend %d, trying fallback chain", dx8gl::g_selected_backend);
            dx8gl::g_render_backend = dx8gl::create_render_backend(DX8GL_BACKEND_DEFAULT);
        }
        
        if (!dx8gl::g_render_backend) {
            dx8gl::set_error("Failed to create any render backend");
            return DX8GL_ERROR_INTERNAL;
        }
    }
    
    // Initialize the backend with default size (will be resized by device)
    if (!dx8gl::g_render_backend->initialize(800, 600)) {
        // Try fallback if initialization fails
        if (dx8gl::g_selected_backend != DX8GL_BACKEND_DEFAULT && 
            dx8gl::g_selected_backend != DX8GL_BACKEND_OSMESA) {
            DX8GL_WARNING("Failed to initialize backend %d, trying fallback", dx8gl::g_selected_backend);
            dx8gl::g_render_backend = dx8gl::create_render_backend(DX8GL_BACKEND_DEFAULT);
            
            if (dx8gl::g_render_backend && dx8gl::g_render_backend->initialize(800, 600)) {
                DX8GL_INFO("Successfully initialized fallback backend");
            } else {
                dx8gl::set_error("Failed to initialize any render backend");
                dx8gl::g_render_backend.reset();
                return DX8GL_ERROR_INTERNAL;
            }
        } else {
            dx8gl::set_error("Failed to initialize render backend");
            dx8gl::g_render_backend.reset();
            return DX8GL_ERROR_INTERNAL;
        }
    }
    
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
    
    if (dx8gl::g_render_backend) {
        dx8gl::g_render_backend->shutdown();
        dx8gl::g_render_backend.reset();
    }
    
    // OSMesa mode doesn't need shared context
    dx8gl::g_initialized = false;
}

dx8gl_error dx8gl_create_device(dx8gl_device** device) {
    if (!device) {
        return DX8GL_ERROR_INVALID_PARAMETER;
    }
    
    if (!dx8gl::g_initialized) {
        dx8gl::set_error("dx8gl not initialized");
        return DX8GL_ERROR_NOT_INITIALIZED;
    }
    
    // Create new device
    auto new_device = std::make_unique<::dx8gl_device>();
    
    // Create backend for this device
    new_device->backend = dx8gl::create_render_backend(dx8gl::g_selected_backend);
    if (!new_device->backend) {
        dx8gl::set_error("Failed to create render backend for device");
        return DX8GL_ERROR_INTERNAL;
    }
    
    // Initialize the backend with default size
    if (!new_device->backend->initialize(800, 600)) {
        dx8gl::set_error("Failed to initialize render backend for device");
        return DX8GL_ERROR_INTERNAL;
    }
    
    new_device->initialized = true;
    
    // Store device in global list
    {
        std::lock_guard<std::mutex> lock(dx8gl::g_devices_mutex);
        dx8gl::g_devices.push_back(std::move(new_device));
        *device = dx8gl::g_devices.back().get();
    }
    
    DX8GL_INFO("Created dx8gl device at %p", *device);
    return DX8GL_SUCCESS;
}

void dx8gl_destroy_device(dx8gl_device* device) {
    if (!device) {
        return;
    }
    
    DX8GL_INFO("Destroying dx8gl device at %p", device);
    
    // Find and remove device from global list
    {
        std::lock_guard<std::mutex> lock(dx8gl::g_devices_mutex);
        dx8gl::g_devices.erase(
            std::remove_if(dx8gl::g_devices.begin(), dx8gl::g_devices.end(),
                [device](const std::unique_ptr<::dx8gl_device>& ptr) {
                    return ptr.get() == device;
                }),
            dx8gl::g_devices.end()
        );
    }
}

dx8gl_error dx8gl_get_caps(dx8gl_device* device, dx8gl_caps* caps) {
    if (!device || !caps) {
        return DX8GL_ERROR_INVALID_PARAMETER;
    }
    
    if (!device->initialized || !device->backend) {
        dx8gl::set_error("Device not initialized");
        return DX8GL_ERROR_NOT_INITIALIZED;
    }
    
    // Initialize with reasonable defaults for WebGPU or other backends
    GLint max_texture_size = 4096;
    GLint max_texture_units = 8;
    GLint max_vertex_attribs = 16;
    GLint max_vertex_uniform_components = 384;  // 96 vec4s
    GLint max_fragment_uniform_components = 32;  // 8 vec4s
    GLint max_renderbuffer_size = 4096;
    GLint max_color_attachments = 4;
    
#ifdef DX8GL_HAS_OSMESA
    // Make backend context current to query OpenGL capabilities
    if (!device->backend->make_current()) {
        dx8gl::set_error("Failed to make backend context current");
        return DX8GL_ERROR_INTERNAL;
    }
    
    // Query actual OpenGL capabilities
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform_components);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_fragment_uniform_components);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
    
    // Check for any OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_WARNING("OpenGL error while querying capabilities: 0x%04x", error);
        // Continue with reasonable defaults
    }
#endif
    
    // Fill in capabilities structure
    caps->max_texture_size = static_cast<uint32_t>(std::max(max_texture_size, 1024));
    caps->max_texture_units = static_cast<uint32_t>(std::max(max_texture_units, 4));
    caps->max_anisotropy = 1; // Software rendering typically doesn't support anisotropic filtering
    
    // DirectX 8 shader versions (dx8gl supports up to these)
    caps->max_vertex_shader_version = 0x0101;  // vs_1_1
    caps->max_pixel_shader_version = 0x0104;   // ps_1_4
    
    // Calculate shader constants based on uniform components
    // DirectX 8 vs_1_1 supports up to 96 constants (float4), ps_1_4 supports 8
    caps->max_vertex_shader_constants = std::min(96u, static_cast<uint32_t>(max_vertex_uniform_components / 4));
    caps->max_pixel_shader_constants = std::min(8u, static_cast<uint32_t>(max_fragment_uniform_components / 4));
    
    // Rendering limits
    caps->max_primitives_per_call = 65535; // Reasonable limit for software rendering
    caps->max_vertex_index = 65535;        // 16-bit indices for compatibility
    caps->max_render_targets = std::min(static_cast<uint32_t>(max_color_attachments), 4u);
    
    // Feature support (check OpenGL extensions)
    const char* gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* gl_extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    
    // Modern OpenGL (3.3+) supports these by default
    caps->supports_npot_textures = true;
    caps->supports_cubemaps = true;
    
    // Check for compressed texture support
    caps->supports_compressed_textures = (gl_extensions && 
        (strstr(gl_extensions, "GL_EXT_texture_compression_s3tc") != nullptr ||
         strstr(gl_extensions, "GL_ARB_texture_compression") != nullptr));
    
    // Volume textures are less common in OpenGL ES, conservative default
    caps->supports_volume_textures = (gl_extensions && 
        strstr(gl_extensions, "GL_OES_texture_3D") != nullptr);
    
    DX8GL_INFO("Queried OpenGL capabilities:");
    DX8GL_INFO("  Max texture size: %u", caps->max_texture_size);
    DX8GL_INFO("  Max texture units: %u", caps->max_texture_units);
    DX8GL_INFO("  Max vertex constants: %u", caps->max_vertex_shader_constants);
    DX8GL_INFO("  Max pixel constants: %u", caps->max_pixel_shader_constants);
    DX8GL_INFO("  NPOT textures: %s", caps->supports_npot_textures ? "yes" : "no");
    DX8GL_INFO("  Compressed textures: %s", caps->supports_compressed_textures ? "yes" : "no");
    DX8GL_INFO("  Cube maps: %s", caps->supports_cubemaps ? "yes" : "no");
    DX8GL_INFO("  Volume textures: %s", caps->supports_volume_textures ? "yes" : "no");
    
    return DX8GL_SUCCESS;
}

dx8gl_error dx8gl_get_stats(dx8gl_device* device, dx8gl_stats* stats) {
    if (!device || !stats) {
        return DX8GL_ERROR_INVALID_PARAMETER;
    }
    
    if (!device->initialized) {
        dx8gl::set_error("Device not initialized");
        return DX8GL_ERROR_NOT_INITIALIZED;
    }
    
    // Copy current stats
    *stats = device->stats;
    return DX8GL_SUCCESS;
}

void dx8gl_reset_stats(dx8gl_device* device) {
    if (!device || !device->initialized) {
        return;
    }
    
    // Reset all statistics counters
    std::memset(&device->stats, 0, sizeof(device->stats));
    DX8GL_DEBUG("Reset statistics for device %p", device);
}

const char* dx8gl_get_error_string(void) {
    return dx8gl::g_last_error.c_str();
}

const char* dx8gl_get_version_string(void) {
    return DX8GL_VERSION_STRING;
}

#ifdef DX8GL_HAS_OSMESA
// Context management (OSMesa-only)
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
        // Allow setting current context to null
        dx8gl::g_current_context = nullptr;
        return true;
    }
    
    auto ctx = reinterpret_cast<dx8gl::DX8OSMesaContext*>(context);
    if (!ctx->make_current()) {
        return false;
    }
    
    // Update current context tracking
    dx8gl::g_current_context = context;
    
    DX8GL_DEBUG("Made context %p current", context);
    return true;
}

dx8gl_context* dx8gl_context_get_current(void) {
    return dx8gl::g_current_context;
}

void dx8gl_context_get_size(dx8gl_context* context, uint32_t* width, uint32_t* height) {
    if (!context || !width || !height) {
        return;
    }
    
    auto ctx = reinterpret_cast<dx8gl::DX8OSMesaContext*>(context);
    *width = static_cast<uint32_t>(ctx->get_width());
    *height = static_cast<uint32_t>(ctx->get_height());
}
#endif // DX8GL_HAS_OSMESA

// Get framebuffer from device for display (dx8gl extension)
void* dx8gl_get_framebuffer(IDirect3DDevice8* device, int* width, int* height) {
    if (!device) {
        // Try to use global backend if no device provided
        if (!dx8gl::g_render_backend) {
            return nullptr;
        }
        int w, h, format;
        void* fb = dx8gl::g_render_backend->get_framebuffer(w, h, format);
        if (width) *width = w;
        if (height) *height = h;
        return fb;
    }
    
    // Use device's method
    auto d3d8_device = static_cast<dx8gl::Direct3DDevice8*>(device);
    int format;
    return d3d8_device->get_framebuffer(width, height, &format);
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
    
    // Get framebuffer from the device (which uses the render backend)
    int fb_width = 0, fb_height = 0, format = 0;
    void* framebuffer = device->get_framebuffer(&fb_width, &fb_height, &format);
    if (!framebuffer) {
        return nullptr;
    }
    
#ifdef DX8GL_HAS_OSMESA
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
#endif
    
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