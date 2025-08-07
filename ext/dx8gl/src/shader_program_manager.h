#ifndef DX8GL_SHADER_PROGRAM_MANAGER_H
#define DX8GL_SHADER_PROGRAM_MANAGER_H

#include "d3d8_types.h"
#include "gl3_headers.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

namespace dx8gl {

class VertexShaderManager;
class PixelShaderManager;

/**
 * @brief Manages combined vertex and pixel shader programs
 * 
 * DirectX 8 allows separate vertex and pixel shaders, but OpenGL requires
 * them to be linked into a single program. This manager handles the
 * combination and caching of shader programs.
 */
class ShaderProgramManager {
public:
    ShaderProgramManager();
    ~ShaderProgramManager();
    
    // Initialize with references to shader managers
    bool initialize(VertexShaderManager* vertex_mgr, PixelShaderManager* pixel_mgr, 
                   class ShaderConstantManager* constant_mgr = nullptr);
    void cleanup();
    
    // Get or create a program for current vertex/pixel shader combination
    GLuint get_current_program();
    
    // Apply the current shader program and update uniforms
    void apply_shader_state();
    
    // Invalidate cache when shaders change
    void invalidate_current_program();
    
private:
    // Key for caching shader programs
    struct ProgramKey {
        DWORD vertex_shader_handle;
        DWORD pixel_shader_handle;
        
        bool operator==(const ProgramKey& other) const {
            return vertex_shader_handle == other.vertex_shader_handle &&
                   pixel_shader_handle == other.pixel_shader_handle;
        }
    };
    
    struct ProgramKeyHash {
        std::size_t operator()(const ProgramKey& key) const {
            return std::hash<DWORD>()(key.vertex_shader_handle) ^
                   (std::hash<DWORD>()(key.pixel_shader_handle) << 1);
        }
    };
    
    struct ShaderProgram {
        GLuint program;
        
        // Cached uniform locations
        std::unordered_map<std::string, GLint> uniform_locations;
        
        // Standard uniforms
        GLint u_world_matrix;
        GLint u_view_matrix;
        GLint u_projection_matrix;
        GLint u_world_view_proj_matrix;
        
        // Vertex shader constants (c0-c95)
        GLint u_vs_constants[96];
        
        // Pixel shader constants (c0-c7)
        GLint u_ps_constants[8];
        
        // Texture samplers
        GLint u_textures[8];
        
        ShaderProgram() : program(0), u_world_matrix(-1), u_view_matrix(-1),
                         u_projection_matrix(-1), u_world_view_proj_matrix(-1) {
            std::fill(std::begin(u_vs_constants), std::end(u_vs_constants), -1);
            std::fill(std::begin(u_ps_constants), std::end(u_ps_constants), -1);
            std::fill(std::begin(u_textures), std::end(u_textures), -1);
        }
    };
    
    // Create a new program from vertex and pixel shaders
    std::unique_ptr<ShaderProgram> create_program(DWORD vs_handle, DWORD ps_handle);
    
    // Link vertex and pixel shaders into a program
    GLuint link_shaders(GLuint vertex_shader, GLuint pixel_shader);
    
    // Cache uniform locations for a program
    void cache_uniform_locations(ShaderProgram* program);
    
    // Apply uniforms to the current program
    void apply_uniforms(ShaderProgram* program);
    
    // Create a default pixel shader for vertex-only programs
    GLuint create_default_pixel_shader();
    
    // Member variables
    VertexShaderManager* vertex_shader_manager_;
    PixelShaderManager* pixel_shader_manager_;
    class ShaderConstantManager* shader_constant_manager_;
    
    // Cache of compiled programs
    std::unordered_map<ProgramKey, std::unique_ptr<ShaderProgram>, ProgramKeyHash> program_cache_;
    
    // Current program
    ProgramKey current_key_;
    ShaderProgram* current_program_;
    bool current_valid_;
    
    // Cache support
    GLuint default_pixel_shader_;
    
    // Thread safety
    mutable std::mutex cache_mutex_;  // Protects program_cache_ and current state
};

} // namespace dx8gl

#endif // DX8GL_SHADER_PROGRAM_MANAGER_H