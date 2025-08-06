#include "pixel_shader_manager.h"
#include "d3d8.h"
#include "logger.h"
#include "dx8_shader_translator.h"
#include <sstream>
#include <cstring>

// Define missing DirectX error codes if not already defined
#ifndef D3DERR_MOREDATA
#define D3DERR_MOREDATA 0x887600A4L
#endif

namespace dx8gl {

PixelShaderManager::PixelShaderManager() 
    : current_shader_(nullptr), next_handle_(1) {
    memset(shader_constants_, 0, sizeof(shader_constants_));
    memset(constant_dirty_, 0, sizeof(constant_dirty_));
}

PixelShaderManager::~PixelShaderManager() {
    cleanup();
}

bool PixelShaderManager::initialize() {
    DX8GL_INFO("Initializing pixel shader manager");
    
    // Set default constants
    for (int i = 0; i < MAX_PIXEL_SHADER_CONSTANTS; i++) {
        shader_constants_[i * 4 + 0] = 1.0f; // R
        shader_constants_[i * 4 + 1] = 1.0f; // G
        shader_constants_[i * 4 + 2] = 1.0f; // B
        shader_constants_[i * 4 + 3] = 1.0f; // A
    }
    
    return true;
}

void PixelShaderManager::cleanup() {
    DX8GL_INFO("Cleaning up pixel shader manager");
    
    std::lock_guard<std::mutex> lock(shader_mutex_);
    
    // Delete all shaders
    for (auto& pair : shaders_) {
        PixelShaderInfo* shader = pair.second.get();
        if (shader->gl_program) {
            glDeleteProgram(shader->gl_program);
        }
        if (shader->gl_shader) {
            glDeleteShader(shader->gl_shader);
        }
    }
    
    shaders_.clear();
    current_shader_ = nullptr;
}

HRESULT PixelShaderManager::create_pixel_shader(const DWORD* pFunction, DWORD* pHandle) {
    if (!pFunction || !pHandle) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("Creating pixel shader");
    
    // Create new shader info
    auto shader_info = std::make_unique<PixelShaderInfo>();
    shader_info->handle = next_handle_++;
    
    // Store function bytecode
    const DWORD* func_ptr = pFunction;
    if (*func_ptr == 0xFFFF0101 || *func_ptr == 0xFFFF0102 || *func_ptr == 0xFFFF0103 || *func_ptr == 0xFFFF0104) { 
        // ps_1_1, ps_1_2, ps_1_3, ps_1_4 version tokens
        func_ptr++;
        while (*func_ptr != 0x0000FFFF) { // End token
            shader_info->function_bytecode.push_back(*func_ptr);
            func_ptr++;
        }
        shader_info->function_bytecode.push_back(0x0000FFFF); // Add end token
    } else {
        DX8GL_ERROR("Unsupported pixel shader version");
        return D3DERR_INVALIDCALL;
    }
    
    // For now, create a simple pass-through fragment shader
    shader_info->glsl_source = generate_simple_pixel_shader(shader_info.get());
    
    // Compile the shader
    if (!compile_pixel_shader(shader_info.get())) {
        DX8GL_ERROR("Failed to compile pixel shader");
        return D3DERR_INVALIDCALL;
    }
    
    *pHandle = shader_info->handle;
    
    // Store the shader
    DWORD handle = shader_info->handle;
    {
        std::lock_guard<std::mutex> lock(shader_mutex_);
        shaders_[handle] = std::move(shader_info);
    }
    
    DX8GL_INFO("Created pixel shader with handle %d", handle);
    return S_OK;
}

HRESULT PixelShaderManager::delete_pixel_shader(DWORD Handle) {
    std::lock_guard<std::mutex> lock(shader_mutex_);
    
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    PixelShaderInfo* shader = it->second.get();
    
    // Don't delete if it's currently active
    if (current_shader_ == shader) {
        current_shader_ = nullptr;
    }
    
    // Delete OpenGL resources
    if (shader->gl_program) {
        glDeleteProgram(shader->gl_program);
    }
    if (shader->gl_shader) {
        glDeleteShader(shader->gl_shader);
    }
    
    shaders_.erase(it);
    
    DX8GL_INFO("Deleted pixel shader handle %d", Handle);
    return S_OK;
}

HRESULT PixelShaderManager::set_pixel_shader(DWORD Handle) {
    if (Handle == 0) {
        // Disable pixel shader
        std::lock_guard<std::mutex> lock(shader_mutex_);
        current_shader_ = nullptr;
        DX8GL_INFO("Disabled pixel shader");
        return S_OK;
    }
    
    std::lock_guard<std::mutex> lock(shader_mutex_);
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    current_shader_ = it->second.get();
    DX8GL_INFO("Set pixel shader handle %d", Handle);
    return S_OK;
}

HRESULT PixelShaderManager::set_pixel_shader_constant(DWORD Register, const void* pConstantData, DWORD ConstantCount) {
    if (!pConstantData || Register + ConstantCount > MAX_PIXEL_SHADER_CONSTANTS) {
        return D3DERR_INVALIDCALL;
    }
    
    const float* data = (const float*)pConstantData;
    for (DWORD i = 0; i < ConstantCount; i++) {
        memcpy(&shader_constants_[(Register + i) * 4], &data[i * 4], 4 * sizeof(float));
        constant_dirty_[Register + i] = true;
    }
    
    return S_OK;
}

HRESULT PixelShaderManager::get_pixel_shader_constant(DWORD Register, void* pConstantData, DWORD ConstantCount) {
    if (!pConstantData || Register + ConstantCount > MAX_PIXEL_SHADER_CONSTANTS) {
        return D3DERR_INVALIDCALL;
    }
    
    float* data = (float*)pConstantData;
    for (DWORD i = 0; i < ConstantCount; i++) {
        memcpy(&data[i * 4], &shader_constants_[(Register + i) * 4], 4 * sizeof(float));
    }
    
    return S_OK;
}

HRESULT PixelShaderManager::get_pixel_shader_function(DWORD Handle, void* pData, DWORD* pSizeOfData) {
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    const auto& function = it->second->function_bytecode;
    DWORD required_size = function.size() * sizeof(DWORD);
    
    if (!pData) {
        *pSizeOfData = required_size;
        return S_OK;
    }
    
    if (*pSizeOfData < required_size) {
        return D3DERR_MOREDATA;
    }
    
    memcpy(pData, function.data(), required_size);
    *pSizeOfData = required_size;
    
    return S_OK;
}

void PixelShaderManager::apply_shader_state() {
    // This function is not used when ShaderProgramManager is active
    // The ShaderProgramManager will handle applying all shader state
    // including pixel shader constants
    
    // Just log a warning if this is called
    if (current_shader_) {
        DX8GL_WARNING("PixelShaderManager::apply_shader_state called but should be handled by ShaderProgramManager");
    }
}

DWORD PixelShaderManager::get_current_shader_handle() const {
    std::lock_guard<std::mutex> lock(shader_mutex_);
    return current_shader_ ? current_shader_->handle : 0;
}

GLuint PixelShaderManager::get_current_gl_shader() const {
    std::lock_guard<std::mutex> lock(shader_mutex_);
    return current_shader_ ? current_shader_->gl_shader : 0;
}

bool PixelShaderManager::compile_pixel_shader(PixelShaderInfo* shader_info) {
    // Create fragment shader
    shader_info->gl_shader = create_gl_shader(shader_info->glsl_source);
    if (!shader_info->gl_shader) {
        return false;
    }
    
    // Don't create a program here - the ShaderProgramManager will handle linking
    // vertex and pixel shaders together
    shader_info->gl_program = 0;
    
    return true;
}

GLuint PixelShaderManager::create_gl_shader(const std::string& glsl_source) {
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* source = glsl_source.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        DX8GL_ERROR("Fragment shader compilation failed: %s", log);
        DX8GL_ERROR("Shader source:\n%s", glsl_source.c_str());
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

void PixelShaderManager::cache_uniform_locations(PixelShaderInfo* shader_info) {
    // Cache constant uniforms - use ps_ prefix for pixel shader constants
    for (int i = 0; i < MAX_PIXEL_SHADER_CONSTANTS; i++) {
        std::string name = "ps_c" + std::to_string(i);
        GLint location = glGetUniformLocation(shader_info->gl_program, name.c_str());
        if (location >= 0) {
            shader_info->constant_locations[i] = location;
        }
    }
    
    // Cache texture uniforms - use s0-s3 for DirectX compatibility
    for (int i = 0; i < 4; i++) {
        std::string name = "s" + std::to_string(i);
        shader_info->texture_locations[i] = glGetUniformLocation(shader_info->gl_program, name.c_str());
    }
}

std::string PixelShaderManager::generate_simple_pixel_shader(const PixelShaderInfo* shader_info) {
    UNUSED(shader_info);
    
    std::ostringstream frag;
    frag << "#version 100\n";
    frag << "precision mediump float;\n\n";
    
    // Add varying inputs - match vertex shader output type (vec4)
    frag << "varying vec4 v_texcoord0;\n\n";
    
    // Add uniforms for constants - use ps_ prefix for pixel shader constants
    for (int i = 0; i < MAX_PIXEL_SHADER_CONSTANTS; i++) {
        frag << "uniform vec4 ps_c" << i << ";\n";
    }
    
    // Add texture uniforms - use s0-s3 for DirectX compatibility
    for (int i = 0; i < 4; i++) {
        frag << "uniform sampler2D s" << i << ";\n";
    }
    
    frag << "\nvoid main() {\n";
    frag << "    vec4 color = texture2D(s0, v_texcoord0.xy);\n";
    frag << "    color *= ps_c0; // Apply constant color modulation\n";
    frag << "    gl_FragColor = color;\n";
    frag << "}\n";
    
    return frag.str();
}

bool PixelShaderManager::get_pixel_shader_bytecode(DWORD Handle, std::vector<DWORD>& bytecode) const {
    std::lock_guard<std::mutex> lock(shader_mutex_);
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return false;
    }
    
    bytecode = it->second->function_bytecode;
    return true;
}

} // namespace dx8gl