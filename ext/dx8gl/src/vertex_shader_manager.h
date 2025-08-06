#ifndef DX8GL_VERTEX_SHADER_MANAGER_H
#define DX8GL_VERTEX_SHADER_MANAGER_H

#include "d3d8_types.h"
#include "gl3_headers.h"
#include "dx8_shader_translator.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace dx8gl {

// Vertex shader storage
struct VertexShaderInfo {
    DWORD handle;
    std::vector<DWORD> declaration;
    std::vector<DWORD> function_bytecode;
    std::string glsl_source;
    GLuint gl_shader;
    GLuint gl_program;
    
    // Uniform locations
    std::unordered_map<int, GLint> constant_locations;
    GLint mvp_matrix_location;
    GLint world_matrix_location;
    
    // Vertex attribute info
    struct VertexAttribute {
        int stream;
        int offset;
        int type;
        int usage;
        int usage_index;
    };
    std::vector<VertexAttribute> attributes;
    
    VertexShaderInfo() : handle(0), gl_shader(0), gl_program(0), 
                        mvp_matrix_location(-1), world_matrix_location(-1) {}
};

class VertexShaderManager {
public:
    VertexShaderManager();
    ~VertexShaderManager();
    
    // Initialize/cleanup
    bool initialize();
    void cleanup();
    
    // Vertex shader management
    HRESULT create_vertex_shader(const DWORD* pDeclaration, const DWORD* pFunction, 
                                DWORD* pHandle, DWORD Usage);
    HRESULT delete_vertex_shader(DWORD Handle);
    HRESULT set_vertex_shader(DWORD Handle);
    
    // Shader constants
    HRESULT set_vertex_shader_constant(DWORD Register, const void* pConstantData, DWORD ConstantCount);
    HRESULT get_vertex_shader_constant(DWORD Register, void* pConstantData, DWORD ConstantCount);
    
    // Shader info retrieval
    HRESULT get_vertex_shader_declaration(DWORD Handle, void* pData, DWORD* pSizeOfData);
    HRESULT get_vertex_shader_function(DWORD Handle, void* pData, DWORD* pSizeOfData);
    
    // Current shader info
    VertexShaderInfo* get_current_shader() { return current_shader_; }
    bool is_using_vertex_shader() const { return current_shader_ != nullptr; }
    
    // Apply shader state for rendering
    void apply_shader_state();
    
private:
    // Shader compilation
    bool compile_vertex_shader(VertexShaderInfo* shader_info);
    bool parse_vertex_declaration(const DWORD* pDeclaration, VertexShaderInfo* shader_info);
    GLuint create_gl_shader(const std::string& glsl_source);
    GLuint create_gl_program(GLuint vertex_shader);
    void cache_uniform_locations(VertexShaderInfo* shader_info);
    
    // Generate simple fragment shader for vertex shader programs
    std::string generate_fragment_shader(const VertexShaderInfo* shader_info);
    
    // Member variables
    std::unordered_map<DWORD, std::unique_ptr<VertexShaderInfo>> shaders_;
    VertexShaderInfo* current_shader_;
    DWORD next_handle_;
    
    // Shader constants (c0-c95 in DirectX 8)
    static const int MAX_VERTEX_SHADER_CONSTANTS = 96;
    float shader_constants_[MAX_VERTEX_SHADER_CONSTANTS * 4]; // 4 floats per constant
    bool constant_dirty_[MAX_VERTEX_SHADER_CONSTANTS];
    
    DX8ShaderTranslator translator_;
    
    // Cache support
    std::string bytecode_hash_;
    
    // Thread safety
    mutable std::mutex shader_mutex_;  // Protects shaders_ map and current_shader_
};

} // namespace dx8gl

#endif // DX8GL_VERTEX_SHADER_MANAGER_H