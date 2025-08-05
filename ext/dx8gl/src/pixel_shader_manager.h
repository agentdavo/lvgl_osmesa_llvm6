#ifndef DX8GL_PIXEL_SHADER_MANAGER_H
#define DX8GL_PIXEL_SHADER_MANAGER_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace dx8gl {

/**
 * @brief Manages DirectX 8 pixel shaders and their OpenGL ES equivalents
 */
class PixelShaderManager {
public:
    PixelShaderManager();
    ~PixelShaderManager();

    // Lifecycle
    bool initialize();
    void cleanup();

    // Shader management
    HRESULT create_pixel_shader(const DWORD* pFunction, DWORD* pHandle);
    HRESULT delete_pixel_shader(DWORD Handle);
    HRESULT set_pixel_shader(DWORD Handle);
    
    // Constants
    HRESULT set_pixel_shader_constant(DWORD Register, const void* pConstantData, DWORD ConstantCount);
    HRESULT get_pixel_shader_constant(DWORD Register, void* pConstantData, DWORD ConstantCount);
    
    // Shader info retrieval
    HRESULT get_pixel_shader_function(DWORD Handle, void* pData, DWORD* pSizeOfData);
    
    // State application
    void apply_shader_state();
    
    // Current shader info
    DWORD get_current_shader_handle() const;
    GLuint get_current_gl_shader() const;
    
    // Get shader bytecode for caching purposes
    bool get_pixel_shader_bytecode(DWORD Handle, std::vector<DWORD>& bytecode) const;

private:
    static const int MAX_PIXEL_SHADER_CONSTANTS = 8; // DirectX 8 pixel shader constant limit

    struct PixelShaderInfo {
        DWORD handle;
        std::vector<DWORD> function_bytecode;
        std::string glsl_source;
        GLuint gl_shader;
        GLuint gl_program;
        std::unordered_map<int, GLint> constant_locations;
        GLint texture_locations[4]; // Support up to 4 texture units
    };

    // Compilation helpers
    bool compile_pixel_shader(PixelShaderInfo* shader_info);
    GLuint create_gl_shader(const std::string& glsl_source);
    void cache_uniform_locations(PixelShaderInfo* shader_info);
    std::string generate_simple_pixel_shader(const PixelShaderInfo* shader_info);

    // Member variables
    std::unordered_map<DWORD, std::unique_ptr<PixelShaderInfo>> shaders_;
    PixelShaderInfo* current_shader_;
    DWORD next_handle_;
    
    // Shader constants (c0-c7 for pixel shaders)
    float shader_constants_[MAX_PIXEL_SHADER_CONSTANTS * 4];
    bool constant_dirty_[MAX_PIXEL_SHADER_CONSTANTS];
};

} // namespace dx8gl

#endif // DX8GL_PIXEL_SHADER_MANAGER_H