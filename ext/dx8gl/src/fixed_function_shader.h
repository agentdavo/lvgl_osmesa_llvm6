#ifndef DX8GL_FIXED_FUNCTION_SHADER_H
#define DX8GL_FIXED_FUNCTION_SHADER_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include "gl3_headers.h"
#include "d3d8_types.h"
#include "d3d8_constants.h"

namespace dx8gl {

// Fixed function pipeline state that affects shader generation
struct FixedFunctionState {
    bool lighting_enabled = false;
    bool texture_enabled[8] = {false};
    int num_active_lights = 0;
    bool fog_enabled = false;
    D3DFOGMODE fog_mode = D3DFOG_NONE;
    bool alpha_test_enabled = false;
    D3DCMPFUNC alpha_func = D3DCMP_ALWAYS;
    DWORD vertex_format = 0;  // FVF flags
    
    // Texture operations for each stage
    DWORD color_op[8] = {D3DTOP_DISABLE};
    DWORD alpha_op[8] = {D3DTOP_DISABLE};
    
    // Bump mapping parameters
    float bump_env_mat[8][4] = {};    // BUMPENVMAT00, 01, 10, 11 for each stage
    float bump_env_lscale[8] = {0.0f}; // BUMPENVLSCALE for each stage  
    float bump_env_loffset[8] = {0.0f}; // BUMPENVLOFFSET for each stage
    
    // Generate a hash key for shader caching
    uint64_t get_hash() const;
};

class FixedFunctionShader {
public:
    FixedFunctionShader();
    ~FixedFunctionShader();
    
    // Generate or retrieve a shader program for the given state
    GLuint get_program(const FixedFunctionState& state);
    
    // Get uniform locations for a program
    struct UniformLocations {
        GLint world_matrix = -1;
        GLint view_matrix = -1;
        GLint projection_matrix = -1;
        GLint world_view_proj_matrix = -1;
        GLint normal_matrix = -1;
        GLint viewport_size = -1;  // For XYZRHW coordinate conversion
        
        GLint material_ambient = -1;
        GLint material_diffuse = -1;
        GLint material_specular = -1;
        GLint material_emissive = -1;
        GLint material_power = -1;
        
        GLint light_position[8] = {-1};
        GLint light_direction[8] = {-1};
        GLint light_ambient[8] = {-1};
        GLint light_diffuse[8] = {-1};
        GLint light_specular[8] = {-1};
        GLint light_range[8] = {-1};
        GLint light_falloff[8] = {-1};
        GLint light_attenuation[8] = {-1};
        GLint light_theta[8] = {-1};
        GLint light_phi[8] = {-1};
        GLint light_type[8] = {-1};
        
        GLint ambient_light = -1;
        GLint fog_color = -1;
        GLint fog_params = -1;  // start, end, density
        GLint alpha_ref = -1;
        
        GLint texture_sampler[8] = {-1};
        GLint texture_matrix[8] = {-1};
        GLint texture_factor = -1;  // For D3DTA_TFACTOR
        
        // Bump mapping uniforms
        GLint bump_env_mat[8] = {-1};      // 2x2 matrix as vec4
        GLint bump_env_lscale[8] = {-1};   // Luminance scale
        GLint bump_env_loffset[8] = {-1};  // Luminance offset
    };
    
    const UniformLocations* get_uniform_locations(GLuint program) const;
    
private:
    std::string generate_vertex_shader(const FixedFunctionState& state);
    std::string generate_fragment_shader(const FixedFunctionState& state);
    GLuint compile_shader(GLenum type, const std::string& source);
    GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);
    void cache_uniform_locations(GLuint program);
    
    struct CachedProgram {
        GLuint program;
        UniformLocations uniforms;
    };
    
    std::unordered_map<uint64_t, CachedProgram> shader_cache_;
};

} // namespace dx8gl

#endif // DX8GL_FIXED_FUNCTION_SHADER_H