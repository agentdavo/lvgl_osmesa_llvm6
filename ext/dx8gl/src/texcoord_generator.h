#pragma once

#include <string>
#include "d3d8_types.h"
#include "d3d8_constants.h"
#include "GL/gl.h"
#include "GL/glext.h"

namespace dx8gl {

// Texture coordinate generation modes from DirectX 8
enum TexCoordGenMode {
    TEXGEN_PASSTHRU = 0,                    // D3DTSS_TCI_PASSTHRU
    TEXGEN_CAMERASPACENORMAL = 1,          // D3DTSS_TCI_CAMERASPACENORMAL  
    TEXGEN_CAMERASPACEPOSITION = 2,        // D3DTSS_TCI_CAMERASPACEPOSITION
    TEXGEN_CAMERASPACEREFLECTIONVECTOR = 3 // D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
};

// Texture transform flags
enum TexTransformFlags {
    TEXTRANSFORM_DISABLE = 0,    // D3DTTFF_DISABLE
    TEXTRANSFORM_COUNT1 = 1,     // D3DTTFF_COUNT1
    TEXTRANSFORM_COUNT2 = 2,     // D3DTTFF_COUNT2
    TEXTRANSFORM_COUNT3 = 3,     // D3DTTFF_COUNT3
    TEXTRANSFORM_COUNT4 = 4,     // D3DTTFF_COUNT4
    TEXTRANSFORM_PROJECTED = 256 // D3DTTFF_PROJECTED
};

// Texture coordinate generation utilities
class TexCoordGenerator {
public:
    // Generate GLSL code for texture coordinate generation
    static std::string generate_texcoord_code(int texture_stage, 
                                             DWORD texcoord_index,
                                             DWORD transform_flags);
    
    // Get the texture coordinate generation mode from D3DTSS_TEXCOORDINDEX
    static TexCoordGenMode get_texgen_mode(DWORD texcoord_index);
    
    // Get the texture coordinate index (which set of UVs to use)
    static int get_texcoord_index(DWORD texcoord_index);
    
    // Generate vertex shader code for all texture stages
    static std::string generate_vertex_texcoord_code(const DWORD* texcoord_indices,
                                                     const DWORD* transform_flags,
                                                     int num_stages,
                                                     bool has_normals);
    
    // Generate fragment shader code for texture coordinate usage
    static std::string generate_fragment_texcoord_code(int num_stages);
    
private:
    // Generate code for specific texgen modes
    static std::string generate_spheremap_code(int stage);
    static std::string generate_reflection_code(int stage);
    static std::string generate_camera_normal_code(int stage);
    static std::string generate_camera_position_code(int stage);
    
    // Generate texture transform code
    static std::string generate_transform_code(int stage, 
                                              DWORD transform_flags,
                                              const std::string& input_coords);
};

// Shader snippet templates for texture coordinate generation
namespace TexGenTemplates {
    
    // Sphere mapping calculation
    constexpr const char* SPHERE_MAP_VERTEX = R"(
    // Sphere map generation for stage %d
    vec3 sphere_normal_%d = normalize(v_normal);
    vec3 sphere_eye_%d = normalize(v_position);
    vec3 sphere_r_%d = reflect(sphere_eye_%d, sphere_normal_%d);
    float sphere_m_%d = 2.0 * sqrt(sphere_r_%d.x * sphere_r_%d.x + 
                                   sphere_r_%d.y * sphere_r_%d.y + 
                                   (sphere_r_%d.z + 1.0) * (sphere_r_%d.z + 1.0));
    v_texcoord%d = vec2(sphere_r_%d.x / sphere_m_%d + 0.5, 
                        sphere_r_%d.y / sphere_m_%d + 0.5);
)";

    // Reflection vector calculation
    constexpr const char* REFLECTION_VECTOR_VERTEX = R"(
    // Reflection vector generation for stage %d
    vec3 refl_normal_%d = normalize(v_normal);
    vec3 refl_eye_%d = normalize(v_position);
    vec3 refl_vec_%d = reflect(refl_eye_%d, refl_normal_%d);
    // Convert reflection vector to texture coordinates
    v_texcoord%d = refl_vec_%d.xy * 0.5 + 0.5;
)";

    // Camera space normal as texture coordinates
    constexpr const char* CAMERA_NORMAL_VERTEX = R"(
    // Camera space normal for stage %d
    vec3 cam_normal_%d = normalize((u_view_matrix * vec4(v_normal, 0.0)).xyz);
    v_texcoord%d = cam_normal_%d.xy * 0.5 + 0.5;
)";

    // Camera space position as texture coordinates  
    constexpr const char* CAMERA_POSITION_VERTEX = R"(
    // Camera space position for stage %d
    vec4 cam_pos_%d = u_view_matrix * vec4(v_position, 1.0);
    v_texcoord%d = cam_pos_%d.xy / cam_pos_%d.w;
)";

    // Texture transform application
    constexpr const char* TEXTURE_TRANSFORM = R"(
    // Apply texture transform for stage %d
    vec4 tex_coord_%d = vec4(%s, 0.0, 1.0);
    tex_coord_%d = u_texture_matrix%d * tex_coord_%d;
)";

    // Projected texture coordinates
    constexpr const char* PROJECTED_COORDS = R"(
    v_texcoord%d = tex_coord_%d.xy / tex_coord_%d.w;
)";

    // Non-projected texture coordinates
    constexpr const char* STANDARD_COORDS = R"(
    v_texcoord%d = tex_coord_%d.xy;
)";
}

// Integration with shader generator
class ShaderGeneratorTexCoordExtension {
public:
    // Modify vertex shader to include texture coordinate generation
    static void inject_texcoord_generation(std::string& vertex_shader,
                                          const DWORD* texcoord_indices,
                                          const DWORD* transform_flags,
                                          int num_stages);
    
    // Add required uniforms for texture coordinate generation
    static std::string get_texcoord_uniforms(const DWORD* transform_flags,
                                            int num_stages);
    
    // Add required varyings
    static std::string get_texcoord_varyings(int num_stages);
};

} // namespace dx8gl