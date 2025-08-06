#ifndef DX8GL_CUBE_TEXTURE_SUPPORT_H
#define DX8GL_CUBE_TEXTURE_SUPPORT_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <string>

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"
#endif

namespace dx8gl {

/**
 * Enhanced cube texture support for all backends
 */
class CubeTextureSupport {
public:
    // OpenGL cube map face mapping
    static GLenum get_gl_cube_face(D3DCUBEMAP_FACES face);
    
    // Cube map coordinate generation for fixed function
    static std::string generate_cube_texcoord_glsl(int texture_unit);
    
    // WGSL cube texture support
    static std::string generate_cube_sampler_wgsl(int texture_unit);
    static std::string generate_cube_sampling_wgsl(const std::string& sampler_name,
                                                   const std::string& coord_expr);
    
    // Cube map face orientation helpers
    struct FaceOrientation {
        float rotation_angle;  // Rotation needed for proper orientation
        bool flip_horizontal;
        bool flip_vertical;
    };
    
    static FaceOrientation get_face_orientation(D3DCUBEMAP_FACES face);
    
    // Environment mapping helpers
    static std::string generate_reflection_vector_glsl(const std::string& normal,
                                                       const std::string& view_dir);
    static std::string generate_reflection_vector_wgsl(const std::string& normal,
                                                       const std::string& view_dir);
    
    // Format conversion for cube faces
    static bool convert_face_data(const void* src_data, void* dst_data,
                                 D3DFORMAT d3d_format, GLenum gl_format,
                                 uint32_t width, uint32_t height,
                                 D3DCUBEMAP_FACES face);
    
#ifdef DX8GL_HAS_WEBGPU
    // WebGPU cube texture creation
    static WGpuTexture create_webgpu_cube_texture(WGpuDevice device,
                                                  uint32_t size,
                                                  uint32_t mip_levels,
                                                  WGpuTextureFormat format);
    
    // WebGPU cube texture view creation
    static WGpuTextureView create_cube_texture_view(WGpuTexture texture);
    
    // WebGPU cube sampler creation
    static WGpuSampler create_cube_sampler(WGpuDevice device,
                                          WGpuFilterMode min_filter,
                                          WGpuFilterMode mag_filter,
                                          WGpuMipmapFilterMode mipmap_filter);
#endif
};

/**
 * Cube texture shader integration
 */
class CubeTextureShaderGenerator {
public:
    // Generate GLSL declarations for cube textures
    static std::string generate_glsl_declarations(int max_cube_textures);
    
    // Generate GLSL sampling function
    static std::string generate_glsl_sampling_function();
    
    // Generate WGSL declarations for cube textures
    static std::string generate_wgsl_declarations(int max_cube_textures);
    
    // Generate WGSL sampling function
    static std::string generate_wgsl_sampling_function();
    
    // Generate environment mapping shader code
    struct EnvironmentMapConfig {
        bool use_reflection;
        bool use_refraction;
        float refraction_index;
        bool use_fresnel;
        float fresnel_power;
    };
    
    static std::string generate_environment_mapping_glsl(const EnvironmentMapConfig& config);
    static std::string generate_environment_mapping_wgsl(const EnvironmentMapConfig& config);
    
    // Generate cube map coordinate transformation
    static std::string generate_cubemap_transform_glsl(const std::string& input_coord,
                                                       const std::string& transform_matrix);
    static std::string generate_cubemap_transform_wgsl(const std::string& input_coord,
                                                       const std::string& transform_matrix);
};

/**
 * Cube texture state management
 */
class CubeTextureState {
public:
    // Track active cube textures
    struct CubeTextureBinding {
        uint32_t texture_id;     // GL texture ID or WebGPU handle
        uint32_t sampler_unit;   // Texture unit index
        bool is_cube_map;        // True for cube, false for 2D
        D3DTEXTUREFILTERTYPE min_filter;
        D3DTEXTUREFILTERTYPE mag_filter;
        D3DTEXTUREFILTERTYPE mip_filter;
        D3DTEXTUREADDRESS address_u;
        D3DTEXTUREADDRESS address_v;
        D3DTEXTUREADDRESS address_w;
    };
    
    // Set cube texture binding
    static void set_cube_texture(uint32_t stage, const CubeTextureBinding& binding);
    
    // Get cube texture binding
    static const CubeTextureBinding* get_cube_texture(uint32_t stage);
    
    // Clear cube texture binding
    static void clear_cube_texture(uint32_t stage);
    
    // Check if stage has cube texture
    static bool has_cube_texture(uint32_t stage);
    
    // Generate shader defines based on cube texture state
    static std::string generate_shader_defines();
    
private:
    static CubeTextureBinding cube_textures_[8];  // Max 8 texture stages
    static uint32_t active_cube_texture_mask_;
};

/**
 * Cube texture coordinate generation modes
 */
enum CubeTexGenMode {
    CUBE_TEXGEN_NONE = 0,
    CUBE_TEXGEN_REFLECTION_MAP,    // Reflection mapping
    CUBE_TEXGEN_NORMAL_MAP,         // Normal-based mapping
    CUBE_TEXGEN_SPHERE_MAP,         // Spherical environment mapping
    CUBE_TEXGEN_CAMERA_SPACE,       // Camera space normal
    CUBE_TEXGEN_OBJECT_SPACE        // Object space position
};

/**
 * Cube texture coordinate generator
 */
class CubeTexCoordGenerator {
public:
    // Set texgen mode for a texture stage
    static void set_texgen_mode(uint32_t stage, CubeTexGenMode mode);
    
    // Get texgen mode for a texture stage
    static CubeTexGenMode get_texgen_mode(uint32_t stage);
    
    // Generate GLSL code for texgen
    static std::string generate_texgen_glsl(uint32_t stage,
                                           const std::string& position,
                                           const std::string& normal,
                                           const std::string& view_matrix);
    
    // Generate WGSL code for texgen
    static std::string generate_texgen_wgsl(uint32_t stage,
                                           const std::string& position,
                                           const std::string& normal,
                                           const std::string& view_matrix);
    
private:
    static CubeTexGenMode texgen_modes_[8];
};

/**
 * Dynamic cube texture support (for render-to-cube-texture)
 */
class DynamicCubeTexture {
public:
    // Create framebuffer for rendering to cube face
    static GLuint create_cube_face_framebuffer(GLuint cube_texture,
                                              D3DCUBEMAP_FACES face,
                                              uint32_t mip_level);
    
    // Setup viewport for cube face rendering
    static void setup_cube_face_viewport(uint32_t size, uint32_t mip_level);
    
    // Setup view matrix for cube face
    static void setup_cube_face_view_matrix(D3DCUBEMAP_FACES face,
                                           const float* cube_center,
                                           float* view_matrix);
    
    // Setup projection matrix for cube rendering (90 degree FOV)
    static void setup_cube_projection_matrix(float near_plane,
                                            float far_plane,
                                            float* proj_matrix);
    
#ifdef DX8GL_HAS_WEBGPU
    // WebGPU render to cube face
    static WGpuRenderPassEncoder begin_cube_face_render_pass(
        WGpuCommandEncoder encoder,
        WGpuTextureView cube_face_view,
        const WGpuColor* clear_color);
#endif
};

} // namespace dx8gl

#endif // DX8GL_CUBE_TEXTURE_SUPPORT_H