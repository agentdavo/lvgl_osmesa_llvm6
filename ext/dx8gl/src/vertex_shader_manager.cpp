#include "vertex_shader_manager.h"
#include "dx8_shader_translator.h"
#include "shader_bytecode_disassembler.h"
#include "shader_binary_cache.h"
#include "d3d8.h"
#include "logger.h"
#include <sstream>
#include <cstring>

// Define missing DirectX error codes if not already defined
#ifndef D3DERR_MOREDATA
#define D3DERR_MOREDATA 0x887600A4L
#endif

namespace dx8gl {

VertexShaderManager::VertexShaderManager() 
    : current_shader_(nullptr), next_handle_(1) {
    memset(shader_constants_, 0, sizeof(shader_constants_));
    memset(constant_dirty_, 0, sizeof(constant_dirty_));
}

VertexShaderManager::~VertexShaderManager() {
    cleanup();
}

bool VertexShaderManager::initialize() {
    DX8GL_INFO("Initializing vertex shader manager");
    
    // Set identity matrix as default for constants
    // c0-c3 could be world matrix, c4-c7 view matrix, etc.
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            shader_constants_[i * 16 + j * 4 + j] = 1.0f; // Identity matrix
        }
    }
    
    return true;
}

void VertexShaderManager::cleanup() {
    DX8GL_INFO("Cleaning up vertex shader manager");
    
    // Delete all shaders
    for (auto& pair : shaders_) {
        VertexShaderInfo* shader = pair.second.get();
        if (shader && shader->gl_program) {
            glDeleteProgram(shader->gl_program);
        }
        if (shader && shader->gl_shader) {
            glDeleteShader(shader->gl_shader);
        }
    }
    
    shaders_.clear();
    current_shader_ = nullptr;
}

HRESULT VertexShaderManager::create_vertex_shader(const DWORD* pDeclaration, const DWORD* pFunction,
                                                 DWORD* pHandle, DWORD Usage) {
    UNUSED(Usage);
    
    if (!pDeclaration || !pFunction || !pHandle) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("Creating vertex shader");
    
    // Create new shader info
    auto shader_info = std::make_unique<VertexShaderInfo>();
    shader_info->handle = next_handle_++;
    
    // Parse vertex declaration
    if (!parse_vertex_declaration(pDeclaration, shader_info.get())) {
        DX8GL_ERROR("Failed to parse vertex declaration");
        return D3DERR_INVALIDCALL;
    }
    
    // Store function bytecode (including version token)
    const DWORD* func_ptr = pFunction;
    if (*func_ptr == 0xFFFE0101) { // vs_1_1 version token
        shader_info->function_bytecode.push_back(*func_ptr); // Include version token
        func_ptr++;
        while (*func_ptr != 0x0000FFFF) { // End token
            shader_info->function_bytecode.push_back(*func_ptr);
            func_ptr++;
        }
        shader_info->function_bytecode.push_back(0x0000FFFF); // Add end token
    } else {
        DX8GL_ERROR("Unsupported vertex shader version");
        return D3DERR_INVALIDCALL;
    }
    
    // Compute bytecode hash for caching
    if (g_shader_binary_cache) {
        bytecode_hash_ = ShaderBinaryCache::compute_bytecode_hash(shader_info->function_bytecode.data(),
                                                                  shader_info->function_bytecode.size());
        DX8GL_INFO("Vertex shader bytecode hash: %s", bytecode_hash_.c_str());
    }
    
    // Try to disassemble bytecode and use the translator
    bool can_translate = false;
    std::string assembly_source;
    
    // Disassemble the bytecode to DirectX assembly
    if (shader_info->function_bytecode.size() > 0) {
        // First, try to disassemble the bytecode
        if (ShaderBytecodeDisassembler::disassemble(shader_info->function_bytecode, assembly_source)) {
            DX8GL_INFO("Successfully disassembled vertex shader bytecode");
            DX8GL_DEBUG("Disassembled shader:\n%s", assembly_source.c_str());
            
            // Now translate the assembly to GLSL
            DX8ShaderTranslator translator;
            std::string error_msg;
            
            if (translator.parse_shader(assembly_source, error_msg)) {
                shader_info->glsl_source = translator.generate_glsl();
                can_translate = true;
                DX8GL_INFO("Successfully translated vertex shader to GLSL");
            } else {
                DX8GL_WARNING("Shader translation failed: %s", error_msg.c_str());
            }
        } else {
            DX8GL_WARNING("Failed to disassemble vertex shader bytecode");
        }
    }
    
    // Fallback to simple pass-through shader if translation failed
    if (!can_translate) {
        std::ostringstream glsl;
        // Target OpenGL ES 3.0 / OpenGL 3.3 Core
        const char* version_str = (const char*)glGetString(GL_VERSION);
        bool is_es = version_str && strstr(version_str, "ES") != nullptr;
        
        if (is_es) {
            glsl << "#version 300 es\n";
            glsl << "precision highp float;\n\n";
        } else {
            glsl << "#version 330 core\n\n";
        }
        
        // Add attributes based on declaration (modern GLSL uses 'in')
        for (const auto& attr : shader_info->attributes) {
            if (attr.usage == 0) { // Position
                glsl << "in vec4 a_position;\n";
            } else if (attr.usage == 10) { // Color
                glsl << "in vec4 a_color;\n";
            } else if (attr.usage == 8) { // Texture coordinates
                glsl << "in vec2 a_texcoord" << attr.usage_index << ";\n";
            }
        }
        
        // Add uniforms for constants
        glsl << "uniform mat4 u_mvp_matrix;\n";
        glsl << "uniform mat4 u_world_matrix;\n";
        for (int i = 0; i < 16; i++) {
            glsl << "uniform vec4 c" << i << ";\n";
        }
        
        // Add varyings (modern GLSL uses 'out')
        glsl << "out vec4 v_color;\n";
        glsl << "out vec2 v_texcoord0;\n";
        
        // Main function
        glsl << "void main() {\n";
        glsl << "    gl_Position = u_mvp_matrix * a_position;\n";
        glsl << "    v_color = a_color;\n";
        glsl << "    v_texcoord0 = a_texcoord0;\n";
        glsl << "}\n";
        
        shader_info->glsl_source = glsl.str();
        DX8GL_INFO("Using fallback pass-through vertex shader");
    }
    
    // Compile the shader
    if (!compile_vertex_shader(shader_info.get())) {
        DX8GL_ERROR("Failed to compile vertex shader");
        return D3DERR_INVALIDCALL;
    }
    
    *pHandle = shader_info->handle;
    
    // Store the shader
    DWORD handle = shader_info->handle;
    shaders_[handle] = std::move(shader_info);
    
    DX8GL_INFO("Created vertex shader with handle %d", handle);
    return S_OK;
}

HRESULT VertexShaderManager::delete_vertex_shader(DWORD Handle) {
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    VertexShaderInfo* shader = it->second.get();
    
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
    
    DX8GL_INFO("Deleted vertex shader handle %d", Handle);
    return S_OK;
}

HRESULT VertexShaderManager::set_vertex_shader(DWORD Handle) {
    if (Handle == 0) {
        // Disable vertex shader
        current_shader_ = nullptr;
        DX8GL_INFO("Disabled vertex shader");
        return S_OK;
    }
    
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    current_shader_ = it->second.get();
    DX8GL_INFO("Set vertex shader handle %d", Handle);
    return S_OK;
}

HRESULT VertexShaderManager::set_vertex_shader_constant(DWORD Register, const void* pConstantData, DWORD ConstantCount) {
    if (!pConstantData || Register + ConstantCount > MAX_VERTEX_SHADER_CONSTANTS) {
        return D3DERR_INVALIDCALL;
    }
    
    const float* data = (const float*)pConstantData;
    for (DWORD i = 0; i < ConstantCount; i++) {
        memcpy(&shader_constants_[(Register + i) * 4], &data[i * 4], 4 * sizeof(float));
        constant_dirty_[Register + i] = true;
    }
    
    return S_OK;
}

HRESULT VertexShaderManager::get_vertex_shader_constant(DWORD Register, void* pConstantData, DWORD ConstantCount) {
    if (!pConstantData || Register + ConstantCount > MAX_VERTEX_SHADER_CONSTANTS) {
        return D3DERR_INVALIDCALL;
    }
    
    float* data = (float*)pConstantData;
    for (DWORD i = 0; i < ConstantCount; i++) {
        memcpy(&data[i * 4], &shader_constants_[(Register + i) * 4], 4 * sizeof(float));
    }
    
    return S_OK;
}

HRESULT VertexShaderManager::get_vertex_shader_declaration(DWORD Handle, void* pData, DWORD* pSizeOfData) {
    auto it = shaders_.find(Handle);
    if (it == shaders_.end()) {
        return D3DERR_INVALIDCALL;
    }
    
    const auto& declaration = it->second->declaration;
    DWORD required_size = declaration.size() * sizeof(DWORD);
    
    if (!pData) {
        *pSizeOfData = required_size;
        return S_OK;
    }
    
    if (*pSizeOfData < required_size) {
        return D3DERR_MOREDATA;
    }
    
    memcpy(pData, declaration.data(), required_size);
    *pSizeOfData = required_size;
    
    return S_OK;
}

HRESULT VertexShaderManager::get_vertex_shader_function(DWORD Handle, void* pData, DWORD* pSizeOfData) {
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

void VertexShaderManager::apply_shader_state() {
    if (!current_shader_ || !current_shader_->gl_program) {
        // Use fixed function pipeline
        glUseProgram(0);
        return;
    }
    
    // Use the vertex shader program
    glUseProgram(current_shader_->gl_program);
    
    // Update dirty constants
    for (int i = 0; i < MAX_VERTEX_SHADER_CONSTANTS; i++) {
        if (constant_dirty_[i]) {
            auto it = current_shader_->constant_locations.find(i);
            if (it != current_shader_->constant_locations.end()) {
                glUniform4fv(it->second, 1, &shader_constants_[i * 4]);
            }
            constant_dirty_[i] = false;
        }
    }
}

bool VertexShaderManager::compile_vertex_shader(VertexShaderInfo* shader_info) {
    // Create vertex shader
    shader_info->gl_shader = create_gl_shader(shader_info->glsl_source);
    if (!shader_info->gl_shader) {
        return false;
    }
    
    // Don't create a program here - the ShaderProgramManager will handle linking
    // vertex and pixel shaders together
    shader_info->gl_program = 0;
    
    return true;
    
    // OLD CODE - Keep for reference but skip execution
    if (false) {
        // Generate and compile fragment shader
        std::string frag_source = generate_fragment_shader(shader_info);
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* frag_src = frag_source.c_str();
        glShaderSource(fragment_shader, 1, &frag_src, nullptr);
        glCompileShader(fragment_shader);
        
        GLint compiled;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[512];
            glGetShaderInfoLog(fragment_shader, sizeof(log), nullptr, log);
            DX8GL_ERROR("Fragment shader compilation failed: %s", log);
            glDeleteShader(fragment_shader);
            return false;
        }
        
        // Create program
        shader_info->gl_program = glCreateProgram();
        glAttachShader(shader_info->gl_program, shader_info->gl_shader);
        glAttachShader(shader_info->gl_program, fragment_shader);
        
        // Bind attribute locations based on declaration
        int attr_index = 0;
        for (const auto& attr : shader_info->attributes) {
            if (attr.usage == 0) { // Position
                glBindAttribLocation(shader_info->gl_program, attr_index, "a_position");
            } else if (attr.usage == 10) { // Color
                glBindAttribLocation(shader_info->gl_program, attr_index, "a_color");
            } else if (attr.usage == 8) { // Texture coordinates
                std::string name = "a_texcoord" + std::to_string(attr.usage_index);
                glBindAttribLocation(shader_info->gl_program, attr_index, name.c_str());
            }
            attr_index++;
        }
        
        glLinkProgram(shader_info->gl_program);
        
        GLint linked;
        glGetProgramiv(shader_info->gl_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(shader_info->gl_program, sizeof(log), nullptr, log);
            DX8GL_ERROR("Shader program linking failed: %s", log);
            glDeleteShader(fragment_shader);
            return false;
        }
        
        glDeleteShader(fragment_shader); // Can delete after linking
        
        // Cache uniform locations
        cache_uniform_locations(shader_info);
        
        return true;
    } // End of if (false) block
}

bool VertexShaderManager::parse_vertex_declaration(const DWORD* pDeclaration, VertexShaderInfo* shader_info) {
    const DWORD* decl = pDeclaration;
    
    while (*decl != 0xFFFFFFFF) { // End token
        DWORD token = *decl;
        decl++;
        
        if ((token & 0xFF) == 0) { // Stream declaration
            VertexShaderInfo::VertexAttribute attr;
            attr.stream = (token >> 4) & 0xF;
            attr.offset = (token >> 8) & 0xFF;
            attr.type = (token >> 16) & 0xFF;
            attr.usage = (token >> 24) & 0xF;
            attr.usage_index = (token >> 28) & 0xF;
            
            shader_info->attributes.push_back(attr);
            shader_info->declaration.push_back(token);
        }
    }
    
    shader_info->declaration.push_back(0xFFFFFFFF); // End token
    
    return true;
}

GLuint VertexShaderManager::create_gl_shader(const std::string& glsl_source) {
    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    const char* source = glsl_source.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        DX8GL_ERROR("Vertex shader compilation failed: %s", log);
        DX8GL_ERROR("Shader source:\n%s", glsl_source.c_str());
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

void VertexShaderManager::cache_uniform_locations(VertexShaderInfo* shader_info) {
    // Cache standard uniforms
    shader_info->mvp_matrix_location = glGetUniformLocation(shader_info->gl_program, "u_mvp_matrix");
    shader_info->world_matrix_location = glGetUniformLocation(shader_info->gl_program, "u_world_matrix");
    
    // Cache constant uniforms
    for (int i = 0; i < 16; i++) {
        std::string name = "c" + std::to_string(i);
        GLint location = glGetUniformLocation(shader_info->gl_program, name.c_str());
        if (location >= 0) {
            shader_info->constant_locations[i] = location;
        }
    }
}

std::string VertexShaderManager::generate_fragment_shader(const VertexShaderInfo* shader_info) {
    UNUSED(shader_info);
    
    std::ostringstream frag;
    // Target OpenGL ES 3.0 / OpenGL 3.3 Core
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "ES") != nullptr;
    
    if (is_es) {
        frag << "#version 300 es\n";
        frag << "precision mediump float;\n\n";
    } else {
        frag << "#version 330 core\n\n";
    }
    
    // Check if the vertex shader uses v_color0 (DirectX shader) or v_color (fixed function)
    // For now, assume DirectX shaders use v_color0 and v_color1 for dual color
    if (shader_info->glsl_source.find("v_color0") != std::string::npos) {
        frag << "in vec4 v_color0;\n";
        frag << "in vec4 v_color1;\n";
    } else {
        frag << "in vec4 v_color;\n";
    }
    
    // Always declare texture coordinate varyings (modern GLSL uses 'in')
    frag << "in vec4 v_texcoord0;\n";
    
    // Output variable (modern GLSL requires explicit output)
    frag << "out vec4 fragColor;\n";
    
    frag << "uniform sampler2D u_texture0;\n\n";
    frag << "void main() {\n";
    
    // Use appropriate color varying (modern GLSL uses 'texture' instead of 'texture2D')
    if (shader_info->glsl_source.find("v_color0") != std::string::npos) {
        frag << "    fragColor = v_color0 * texture(u_texture0, v_texcoord0.xy);\n";
    } else {
        frag << "    fragColor = v_color * texture(u_texture0, v_texcoord0.xy);\n";
    }
    
    frag << "}\n";
    
    return frag.str();
}

} // namespace dx8gl