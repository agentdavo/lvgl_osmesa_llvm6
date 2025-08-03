#include "shader_program_manager.h"
#include "vertex_shader_manager.h"
#include "pixel_shader_manager.h"
#include "logger.h"
#include <cstring>
#include <vector>

namespace dx8gl {

ShaderProgramManager::ShaderProgramManager() 
    : vertex_shader_manager_(nullptr),
      pixel_shader_manager_(nullptr),
      current_program_(nullptr),
      current_valid_(false) {
    current_key_.vertex_shader_handle = 0;
    current_key_.pixel_shader_handle = 0;
}

ShaderProgramManager::~ShaderProgramManager() {
    cleanup();
}

bool ShaderProgramManager::initialize(VertexShaderManager* vertex_mgr, PixelShaderManager* pixel_mgr) {
    if (!vertex_mgr || !pixel_mgr) {
        DX8GL_ERROR("ShaderProgramManager: Invalid shader managers provided");
        return false;
    }
    
    vertex_shader_manager_ = vertex_mgr;
    pixel_shader_manager_ = pixel_mgr;
    
    DX8GL_INFO("ShaderProgramManager initialized");
    return true;
}

void ShaderProgramManager::cleanup() {
    // Delete all cached programs
    for (auto& pair : program_cache_) {
        if (pair.second && pair.second->program) {
            glDeleteProgram(pair.second->program);
        }
    }
    program_cache_.clear();
    
    current_program_ = nullptr;
    current_valid_ = false;
}

GLuint ShaderProgramManager::get_current_program() {
    // Get current shader handles from managers
    auto* vs_info = vertex_shader_manager_->get_current_shader();
    DWORD vs_handle = vs_info ? vs_info->handle : 0;
    DWORD ps_handle = pixel_shader_manager_->get_current_shader_handle();
    
    // Check if program needs to be updated
    if (current_valid_ && 
        current_key_.vertex_shader_handle == vs_handle &&
        current_key_.pixel_shader_handle == ps_handle) {
        return current_program_ ? current_program_->program : 0;
    }
    
    // Update current key
    current_key_.vertex_shader_handle = vs_handle;
    current_key_.pixel_shader_handle = ps_handle;
    
    // Look for existing program in cache
    auto it = program_cache_.find(current_key_);
    if (it != program_cache_.end()) {
        current_program_ = it->second.get();
        current_valid_ = true;
        return current_program_->program;
    }
    
    // Create new program
    auto new_program = create_program(vs_handle, ps_handle);
    if (!new_program || !new_program->program) {
        current_valid_ = false;
        current_program_ = nullptr;
        return 0;
    }
    
    // Add to cache and set as current
    current_program_ = new_program.get();
    program_cache_[current_key_] = std::move(new_program);
    current_valid_ = true;
    
    return current_program_->program;
}

void ShaderProgramManager::apply_shader_state() {
    GLuint program = get_current_program();
    if (!program) {
        DX8GL_ERROR("ShaderProgramManager: No valid program to apply");
        return;
    }
    
    // Use the program
    glUseProgram(program);
    
    // Apply uniforms
    if (current_program_) {
        apply_uniforms(current_program_);
    }
}

void ShaderProgramManager::invalidate_current_program() {
    current_valid_ = false;
}

std::unique_ptr<ShaderProgramManager::ShaderProgram> 
ShaderProgramManager::create_program(DWORD vs_handle, DWORD ps_handle) {
    auto program = std::make_unique<ShaderProgram>();
    
    // Get shader info from managers
    auto* vs_info = vertex_shader_manager_->get_current_shader();
    if (!vs_info || vs_info->handle != vs_handle) {
        DX8GL_ERROR("ShaderProgramManager: Vertex shader handle mismatch");
        return nullptr;
    }
    
    // For now, if there's no pixel shader, we need to create a simple one
    if (ps_handle == 0) {
        // Create a simple pass-through pixel shader
        GLuint ps_shader = create_default_pixel_shader();
        if (!ps_shader) {
            DX8GL_ERROR("ShaderProgramManager: Failed to create default pixel shader");
            return nullptr;
        }
        
        // Get the vertex shader
        GLuint vs_shader = vs_info->gl_shader;
        if (!vs_shader) {
            DX8GL_ERROR("ShaderProgramManager: Vertex shader not compiled");
            glDeleteShader(ps_shader);
            return nullptr;
        }
        
        // Link with the vertex shader
        program->program = link_shaders(vs_shader, ps_shader);
        glDeleteShader(ps_shader); // Clean up the shader object after linking
        
        if (!program->program) {
            return nullptr;
        }
        
        cache_uniform_locations(program.get());
        DX8GL_INFO("ShaderProgramManager: Created program with default pixel shader");
        return program;
    }
    
    // Get OpenGL shader objects
    GLuint vs_shader = vs_info->gl_shader;
    if (!vs_shader) {
        DX8GL_ERROR("ShaderProgramManager: Vertex shader not compiled");
        return nullptr;
    }
    
    // Get pixel shader from pixel shader manager
    GLuint ps_shader = 0;
    if (ps_handle != 0) {
        ps_shader = pixel_shader_manager_->get_current_gl_shader();
        if (!ps_shader) {
            DX8GL_ERROR("ShaderProgramManager: Pixel shader not compiled");
            return nullptr;
        }
    }
    
    // Link shaders into program
    program->program = link_shaders(vs_shader, ps_shader);
    if (!program->program) {
        return nullptr;
    }
    
    // Cache uniform locations
    cache_uniform_locations(program.get());
    
    return program;
}

GLuint ShaderProgramManager::link_shaders(GLuint vertex_shader, GLuint pixel_shader) {
    if (!vertex_shader) {
        DX8GL_ERROR("ShaderProgramManager: No vertex shader provided for linking");
        return 0;
    }
    
    // Log shader objects being linked
    DX8GL_INFO("ShaderProgramManager: Linking vertex shader %u with pixel shader %u", vertex_shader, pixel_shader);
    
    // Get and log vertex shader source for debugging
    GLint vs_length = 0;
    glGetShaderiv(vertex_shader, GL_SHADER_SOURCE_LENGTH, &vs_length);
    if (vs_length > 0) {
        std::vector<char> vs_source(vs_length);
        glGetShaderSource(vertex_shader, vs_length, nullptr, vs_source.data());
        DX8GL_INFO("Vertex shader source:\n%s", vs_source.data());
    }
    
    // Get and log pixel shader source for debugging if provided
    if (pixel_shader) {
        GLint ps_length = 0;
        glGetShaderiv(pixel_shader, GL_SHADER_SOURCE_LENGTH, &ps_length);
        if (ps_length > 0) {
            std::vector<char> ps_source(ps_length);
            glGetShaderSource(pixel_shader, ps_length, nullptr, ps_source.data());
            DX8GL_INFO("Pixel shader source:\n%s", ps_source.data());
        }
    }
    
    GLuint program = glCreateProgram();
    if (!program) {
        DX8GL_ERROR("ShaderProgramManager: Failed to create program object");
        return 0;
    }
    
    // Attach vertex shader
    glAttachShader(program, vertex_shader);
    
    // Attach pixel shader if provided
    if (pixel_shader) {
        glAttachShader(program, pixel_shader);
    }
    
    // Bind standard attribute locations before linking
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, "a_normal");
    glBindAttribLocation(program, 2, "a_color");
    glBindAttribLocation(program, 3, "a_texcoord0");
    glBindAttribLocation(program, 4, "a_texcoord1");
    glBindAttribLocation(program, 5, "a_texcoord2");
    glBindAttribLocation(program, 6, "a_texcoord3");
    
    // Link the program
    glLinkProgram(program);
    
    // Check link status
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        char info_log[1024];
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
        DX8GL_ERROR("ShaderProgramManager: Program link failed: %s", info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    DX8GL_INFO("ShaderProgramManager: Successfully linked program %u", program);
    return program;
}

void ShaderProgramManager::cache_uniform_locations(ShaderProgram* program) {
    if (!program || !program->program) return;
    
    // Standard matrix uniforms
    program->u_world_matrix = glGetUniformLocation(program->program, "u_world_matrix");
    program->u_view_matrix = glGetUniformLocation(program->program, "u_view_matrix");
    program->u_projection_matrix = glGetUniformLocation(program->program, "u_projection_matrix");
    program->u_world_view_proj_matrix = glGetUniformLocation(program->program, "u_world_view_proj_matrix");
    
    // Vertex shader constants (c0-c95)
    for (int i = 0; i < 96; i++) {
        char name[32];
        snprintf(name, sizeof(name), "c%d", i);
        program->u_vs_constants[i] = glGetUniformLocation(program->program, name);
    }
    
    // Pixel shader constants (ps_c0-ps_c7)
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "ps_c%d", i);
        program->u_ps_constants[i] = glGetUniformLocation(program->program, name);
    }
    
    // Texture samplers (s0-s7)
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "s%d", i);
        program->u_textures[i] = glGetUniformLocation(program->program, name);
        
        // Set texture unit if location is valid
        if (program->u_textures[i] != -1) {
            glUseProgram(program->program);
            glUniform1i(program->u_textures[i], i);
        }
    }
}

GLuint ShaderProgramManager::create_default_pixel_shader() {
    const char* source = R"(
        #version 100
        precision highp float;
        
        varying vec4 v_color0;
        varying vec4 v_texcoord0;
        
        uniform sampler2D s0;
        
        void main() {
            // Simple pass-through that uses texture if available
            vec4 texColor = texture2D(s0, v_texcoord0.xy);
            gl_FragColor = v_color0 * texColor;
        }
    )";
    
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        DX8GL_ERROR("Default pixel shader compilation failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

void ShaderProgramManager::apply_uniforms(ShaderProgram* program) {
    if (!program || !program->program) return;
    
    // Apply vertex shader constants (c0-c95)
    float vs_constants[96 * 4]; // 96 constants, 4 floats each
    for (int i = 0; i < 96; i++) {
        if (program->u_vs_constants[i] != -1) {
            // Get the constant value from vertex shader manager
            HRESULT hr = vertex_shader_manager_->get_vertex_shader_constant(i, &vs_constants[i * 4], 1);
            if (SUCCEEDED(hr)) {
                glUniform4fv(program->u_vs_constants[i], 1, &vs_constants[i * 4]);
            }
        }
    }
    
    // Apply pixel shader constants (ps_c0-ps_c7)
    float ps_constants[8 * 4]; // 8 constants, 4 floats each
    for (int i = 0; i < 8; i++) {
        if (program->u_ps_constants[i] != -1) {
            // Get the constant value from pixel shader manager
            HRESULT hr = pixel_shader_manager_->get_pixel_shader_constant(i, &ps_constants[i * 4], 1);
            if (SUCCEEDED(hr)) {
                glUniform4fv(program->u_ps_constants[i], 1, &ps_constants[i * 4]);
            }
        }
    }
    
    // Note: Matrix uniforms (u_world_matrix, etc.) would typically be set from device state
    // but DirectX 8 shaders use constants for matrices instead
}

} // namespace dx8gl