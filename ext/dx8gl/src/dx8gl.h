#ifndef DX8GL_H
#define DX8GL_H

/**
 * @file dx8gl.h
 * @brief DirectX 8.1 to OpenGL ES 1.1 Renderer
 * 
 * Main header file for the dx8gl library. This provides a complete
 * DirectX 8.1 compatible API that renders using OpenGL ES 1.1,
 * with full vertex shader 1.1 and pixel shader 1.3 support.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
#define DX8GL_VERSION_MAJOR 1
#define DX8GL_VERSION_MINOR 0
#define DX8GL_VERSION_PATCH 0
#define DX8GL_VERSION_STRING "1.0.0"

/* Export macros */
#ifdef _WIN32
    #ifdef DX8GL_BUILDING_DLL
        #define DX8GL_API __declspec(dllexport)
    #else
        #define DX8GL_API __declspec(dllimport)
    #endif
#else
    #define DX8GL_API __attribute__((visibility("default")))
#endif

/* Forward declarations */
typedef struct dx8gl_device dx8gl_device;
typedef struct dx8gl_context dx8gl_context;
typedef struct dx8gl_config dx8gl_config;
typedef struct Framebuffer Framebuffer;

/* Context management */
DX8GL_API dx8gl_context* dx8gl_context_create(void);
DX8GL_API dx8gl_context* dx8gl_context_create_with_size(uint32_t width, uint32_t height);
DX8GL_API void dx8gl_context_destroy(dx8gl_context* context);
DX8GL_API bool dx8gl_context_make_current(dx8gl_context* context);
DX8GL_API dx8gl_context* dx8gl_context_get_current(void);
DX8GL_API Framebuffer* dx8gl_context_get_framebuffer(dx8gl_context* context);
DX8GL_API void dx8gl_context_get_size(dx8gl_context* context, uint32_t* width, uint32_t* height);

/* System GL integration */
DX8GL_API uint32_t dx8gl_context_get_framebuffer_id(dx8gl_context* context);
DX8GL_API uint32_t dx8gl_context_get_color_texture_id(dx8gl_context* context);

/* Error codes */
typedef enum dx8gl_error {
    DX8GL_SUCCESS = 0,
    DX8GL_ERROR_INVALID_PARAMETER = -1,
    DX8GL_ERROR_OUT_OF_MEMORY = -2,
    DX8GL_ERROR_NOT_INITIALIZED = -3,
    DX8GL_ERROR_ALREADY_INITIALIZED = -4,
    DX8GL_ERROR_PLUGIN_LOAD_FAILED = -5,
    DX8GL_ERROR_SHADER_COMPILE_FAILED = -6,
    DX8GL_ERROR_PIPELINE_ERROR = -7,
    DX8GL_ERROR_NOT_SUPPORTED = -8,
    DX8GL_ERROR_INTERNAL = -99
} dx8gl_error;

/* Device capabilities */
typedef struct dx8gl_caps {
    /* Shader support */
    uint32_t max_vertex_shader_version;
    uint32_t max_pixel_shader_version;
    uint32_t max_vertex_shader_constants;
    uint32_t max_pixel_shader_constants;
    
    /* Texture support */
    uint32_t max_texture_size;
    uint32_t max_texture_units;
    uint32_t max_anisotropy;
    
    /* Rendering limits */
    uint32_t max_primitives_per_call;
    uint32_t max_vertex_index;
    uint32_t max_render_targets;
    
    /* Feature flags */
    bool supports_npot_textures;
    bool supports_compressed_textures;
    bool supports_cubemaps;
    bool supports_volume_textures;
} dx8gl_caps;

/* Backend type enumeration */
typedef enum dx8gl_backend_type {
    DX8GL_BACKEND_DEFAULT = 0,  /* Auto-select best available */
    DX8GL_BACKEND_OSMESA = 1,   /* OSMesa software rendering */
    DX8GL_BACKEND_EGL = 2       /* EGL surfaceless context */
} dx8gl_backend_type;

/* Configuration */
struct dx8gl_config {
    /* Backend selection */
    dx8gl_backend_type backend_type;
    
    /* Rendering options */
    bool enable_multithreading;
    uint32_t worker_thread_count;
    uint32_t tile_size;
    
    /* Memory options */
    size_t command_buffer_size;
    size_t shader_cache_size;
    bool enable_memory_tracking;
    
    /* Plugin options */
    const char* plugin_path;
    bool auto_load_plugins;
    const char** plugin_list;
    size_t plugin_count;
    
    /* Debug options */
    bool enable_validation;
    bool enable_profiling;
    bool enable_logging;
    void (*log_callback)(const char* message);
};

/* Statistics */
typedef struct dx8gl_stats {
    /* Frame statistics */
    uint64_t frame_count;
    double fps;
    double frame_time_ms;
    
    /* Draw statistics */
    uint64_t draw_calls;
    uint64_t primitives_rendered;
    uint64_t vertices_processed;
    
    /* Shader statistics */
    uint64_t shader_switches;
    uint64_t shader_instructions;
    
    /* Memory statistics */
    size_t memory_allocated;
    size_t memory_peak;
    
    /* Cache statistics */
    uint64_t texture_cache_hits;
    uint64_t texture_cache_misses;
    uint64_t shader_cache_hits;
    uint64_t shader_cache_misses;
} dx8gl_stats;

/**
 * Initialize the dx8gl library
 * @param config Configuration options (can be NULL for defaults)
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_init(const dx8gl_config* config);

/**
 * Shutdown the dx8gl library
 */
DX8GL_API void dx8gl_shutdown(void);

/**
 * Create a rendering device
 * @param device Output device handle
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_create_device(dx8gl_device** device);

/**
 * Destroy a rendering device
 * @param device Device to destroy
 */
DX8GL_API void dx8gl_destroy_device(dx8gl_device* device);

/**
 * Get device capabilities
 * @param device Device handle
 * @param caps Output capabilities
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_get_caps(dx8gl_device* device, dx8gl_caps* caps);

/**
 * Get current statistics
 * @param device Device handle
 * @param stats Output statistics
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_get_stats(dx8gl_device* device, dx8gl_stats* stats);

/**
 * Reset statistics counters
 * @param device Device handle
 */
DX8GL_API void dx8gl_reset_stats(dx8gl_device* device);

/**
 * Get last error message
 * @return Error message string
 */
DX8GL_API const char* dx8gl_get_error_string(void);

/**
 * Get version string
 * @return Version string
 */
DX8GL_API const char* dx8gl_get_version_string(void);

/* Plugin management */

/**
 * Load a plugin
 * @param path Plugin file path
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_load_plugin(const char* path);

/**
 * Unload a plugin
 * @param name Plugin name
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_unload_plugin(const char* name);

/**
 * List loaded plugins
 * @param names Output array of plugin names
 * @param count Input/output count
 * @return Error code
 */
DX8GL_API dx8gl_error dx8gl_list_plugins(const char** names, size_t* count);

/* DirectX 8 compatibility layer */
#ifdef DX8GL_D3D8_COMPAT
    #include "dx8gl/d3d8_compat.h"
#endif

/* Additional DirectX 8 functions */

/* Forward declarations for D3D8 types */
typedef struct IDirect3D8 IDirect3D8;
typedef struct IDirect3DDevice8 IDirect3DDevice8;
typedef uint32_t UINT;

/**
 * Create DirectX 8 interface (stub implementation)
 * @param SDKVersion SDK version
 * @return DirectX 8 interface or NULL
 */
DX8GL_API IDirect3D8* Direct3DCreate8(UINT SDKVersion);

/**
 * Get framebuffer from device for display (dx8gl extension)
 * @param device DirectX 8 device
 * @param width Output width
 * @param height Output height
 * @return Framebuffer data or NULL
 */
DX8GL_API void* dx8gl_get_framebuffer(IDirect3DDevice8* device, int* width, int* height);

/**
 * Get shared framebuffer for LVGL integration
 * @param width Output width
 * @param height Output height  
 * @param frame_number Output frame number
 * @param updated Output updated flag
 * @return Shared framebuffer data or NULL
 */
DX8GL_API void* dx8gl_get_shared_framebuffer(int* width, int* height, int* frame_number, bool* updated);

#ifdef __cplusplus
}

// C++ only functions
namespace dx8gl {
    class DX8RenderBackend;
    DX8RenderBackend* get_render_backend();
}
#endif

#endif /* DX8GL_H */
