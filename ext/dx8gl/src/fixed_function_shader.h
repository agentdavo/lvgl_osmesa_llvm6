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