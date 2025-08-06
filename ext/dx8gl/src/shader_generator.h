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
        
        // Texture stage operations for each active texture
        struct TextureStageOps {
            DWORD color_op;
            DWORD color_arg1;
            DWORD color_arg2;
            DWORD alpha_op;
            DWORD alpha_arg1;
            DWORD alpha_arg2;
        } tex_stages[8];
        
        bool operator==(const ShaderKey& other) const {
            if (features != other.features ||
                fvf != other.fvf ||
                alpha_func != other.alpha_func ||
                fog_mode != other.fog_mode ||
                num_textures != other.num_textures) {
                return false;
            }
            
            // Compare texture stage operations for active textures
            for (int i = 0; i < num_textures; i++) {
                if (tex_stages[i].color_op != other.tex_stages[i].color_op ||
                    tex_stages[i].color_arg1 != other.tex_stages[i].color_arg1 ||
                    tex_stages[i].color_arg2 != other.tex_stages[i].color_arg2 ||
                    tex_stages[i].alpha_op != other.tex_stages[i].alpha_op ||
                    tex_stages[i].alpha_arg1 != other.tex_stages[i].alpha_arg1 ||
                    tex_stages[i].alpha_arg2 != other.tex_stages[i].alpha_arg2) {
                    return false;
                }
            }
            
            return true;
        }
    };
    
    struct ShaderKeyHash {
        size_t operator()(const ShaderKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.features);
            size_t h2 = std::hash<DWORD>()(key.fvf);
            size_t h3 = std::hash<int>()(key.alpha_func);
            size_t h4 = std::hash<int>()(key.fog_mode);
            size_t h5 = std::hash<int>()(key.num_textures);
            size_t result = h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
            
            // Hash texture stage operations
            for (int i = 0; i < key.num_textures; i++) {
                size_t stage_hash = std::hash<DWORD>()(key.tex_stages[i].color_op) ^
                                   (std::hash<DWORD>()(key.tex_stages[i].color_arg1) << 1) ^
                                   (std::hash<DWORD>()(key.tex_stages[i].color_arg2) << 2) ^
                                   (std::hash<DWORD>()(key.tex_stages[i].alpha_op) << 3) ^
                                   (std::hash<DWORD>()(key.tex_stages[i].alpha_arg1) << 4) ^
                                   (std::hash<DWORD>()(key.tex_stages[i].alpha_arg2) << 5);
                result ^= (stage_hash << ((i + 5) * 4));
            }
            
            return result;
        }
    };
    
    // Shader cache
    std::unordered_map<ShaderKey, std::unique_ptr<ShaderProgram>, ShaderKeyHash> shader_cache_;
};

} // namespace dx8gl

#endif // DX8GL_SHADER_GENERATOR_H