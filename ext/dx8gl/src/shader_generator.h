#ifndef DX8GL_SHADER_GENERATOR_H
#define DX8GL_SHADER_GENERATOR_H

#include <string>
#include <unordered_map>
#include "state_manager.h"
#include "texcoord_generator.h"

namespace dx8gl {

// Shader feature flags
enum ShaderFeatures {
    SHADER_FEATURE_NONE = 0,
    SHADER_FEATURE_LIGHTING = (1 << 0),
    SHADER_FEATURE_FOG = (1 << 1),
    SHADER_FEATURE_TEXTURE = (1 << 2),
    SHADER_FEATURE_VERTEX_COLOR = (1 << 3),
    SHADER_FEATURE_ALPHA_TEST = (1 << 4),
    SHADER_FEATURE_SPECULAR = (1 << 5),
    SHADER_FEATURE_MULTI_TEXTURE = (1 << 6),
};

// Shader program handle
struct ShaderProgram {
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
    
    // Uniform locations (cached)
    GLint u_mvp_matrix;
    GLint u_world_matrix;
    GLint u_view_matrix;
    GLint u_projection_matrix;
    GLint u_normal_matrix;
    
    // Lighting uniforms
    GLint u_light_enabled[8];
    GLint u_light_position[8];
    GLint u_light_direction[8];
    GLint u_light_diffuse[8];
    GLint u_light_specular[8];
    GLint u_light_ambient[8];
    
    // Material uniforms
    GLint u_material_diffuse;
    GLint u_material_ambient;
    GLint u_material_specular;
    GLint u_material_emissive;
    GLint u_material_power;
    
    // Fog uniforms
    GLint u_fog_color;
    GLint u_fog_start;
    GLint u_fog_end;
    GLint u_fog_density;
    
    // Texture uniforms
    GLint u_texture[8];
    
    // Alpha test uniforms
    GLint u_alpha_ref;
    
    ShaderProgram() : program(0), vertex_shader(0), fragment_shader(0) {
        // Initialize all uniform locations to -1
        u_mvp_matrix = -1;
        u_world_matrix = -1;
        u_view_matrix = -1;
        u_projection_matrix = -1;
        u_normal_matrix = -1;
        u_material_diffuse = -1;
        u_material_ambient = -1;
        u_material_specular = -1;
        u_material_emissive = -1;
        u_material_power = -1;
        u_fog_color = -1;
        u_fog_start = -1;
        u_fog_end = -1;
        u_fog_density = -1;
        u_alpha_ref = -1;
        
        for (int i = 0; i < 8; i++) {
            u_light_enabled[i] = -1;
            u_light_position[i] = -1;
            u_light_direction[i] = -1;
            u_light_diffuse[i] = -1;
            u_light_specular[i] = -1;
            u_light_ambient[i] = -1;
            u_texture[i] = -1;
        }
    }
};

class ShaderGenerator {
public:
    ShaderGenerator();
    ~ShaderGenerator();
    
    // Get or create shader program for current state
    ShaderProgram* get_shader_for_state(const RenderState& render_state,
                                        DWORD fvf, int active_textures);
    
    // Clear shader cache
    void clear_cache();
    
private:
    // Generate shader source
    std::string generate_vertex_shader(uint32_t features, DWORD fvf);
    std::string generate_fragment_shader(uint32_t features, const RenderState& state);
    
    // Compile shader
    GLuint compile_shader(GLenum type, const std::string& source);
    
    // Link program
    bool link_program(ShaderProgram* program);
    
    // Cache uniform locations
    void cache_uniform_locations(ShaderProgram* program);
    
    // Shader cache key
    struct ShaderKey {
        uint32_t features;
        DWORD fvf;
        D3DCMPFUNC alpha_func;
        D3DFOGMODE fog_mode;
        int num_textures;
        
        bool operator==(const ShaderKey& other) const {
            return features == other.features &&
                   fvf == other.fvf &&
                   alpha_func == other.alpha_func &&
                   fog_mode == other.fog_mode &&
                   num_textures == other.num_textures;
        }
    };
    
    struct ShaderKeyHash {
        size_t operator()(const ShaderKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.features);
            size_t h2 = std::hash<DWORD>()(key.fvf);
            size_t h3 = std::hash<int>()(key.alpha_func);
            size_t h4 = std::hash<int>()(key.fog_mode);
            size_t h5 = std::hash<int>()(key.num_textures);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
        }
    };
    
    // Shader cache
    std::unordered_map<ShaderKey, std::unique_ptr<ShaderProgram>, ShaderKeyHash> shader_cache_;
};

} // namespace dx8gl

#endif // DX8GL_SHADER_GENERATOR_H