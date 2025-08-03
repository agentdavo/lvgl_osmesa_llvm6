#include "shader_generator.h"
#include "logger.h"
#include <sstream>
#include <future>
#include <cstring>
#include <GL/gl.h>

namespace dx8gl {

ShaderGenerator::ShaderGenerator() {
    DX8GL_INFO("ShaderGenerator initialized");
}

ShaderGenerator::~ShaderGenerator() {
    clear_cache();
}

ShaderProgram* ShaderGenerator::get_shader_for_state(const RenderState& render_state,
                                                     DWORD fvf, int active_textures) {
    // Build feature flags
    uint32_t features = SHADER_FEATURE_NONE;
    
    if (render_state.lighting) {
        features |= SHADER_FEATURE_LIGHTING;
    }
    if (render_state.fog_enable) {
        features |= SHADER_FEATURE_FOG;
    }
    if (active_textures > 0) {
        features |= SHADER_FEATURE_TEXTURE;
        if (active_textures > 1) {
            features |= SHADER_FEATURE_MULTI_TEXTURE;
        }
    }
    if (fvf & D3DFVF_DIFFUSE) {
        features |= SHADER_FEATURE_VERTEX_COLOR;
    }
    if (render_state.alpha_test_enable) {
        features |= SHADER_FEATURE_ALPHA_TEST;
    }
    if (render_state.specular_enable) {
        features |= SHADER_FEATURE_SPECULAR;
    }
    
    // Build cache key
    ShaderKey key;
    key.features = features;
    key.fvf = fvf;
    key.alpha_func = render_state.alpha_func;
    key.fog_mode = render_state.fog_vertex_mode;
    key.num_textures = active_textures;
    
    // Check cache
    auto it = shader_cache_.find(key);
    if (it != shader_cache_.end()) {
        return it->second.get();
    }
    
    // Generate new shader
    auto program = std::make_unique<ShaderProgram>();
    
    std::string vs_source = generate_vertex_shader(features, fvf);
    std::string fs_source = generate_fragment_shader(features, render_state);
    
    // Compile shaders directly
    program->vertex_shader = compile_shader(GL_VERTEX_SHADER, vs_source);
    program->fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs_source);
    
    if (!program->vertex_shader || !program->fragment_shader) {
        return nullptr;
    }
    
    program->program = glCreateProgram();
    glAttachShader(program->program, program->vertex_shader);
    glAttachShader(program->program, program->fragment_shader);
    
    // Bind attribute locations
    glBindAttribLocation(program->program, 0, "a_position");
    glBindAttribLocation(program->program, 1, "a_normal");
    glBindAttribLocation(program->program, 2, "a_color");
    glBindAttribLocation(program->program, 3, "a_texcoord0");
    glBindAttribLocation(program->program, 4, "a_texcoord1");
    
    if (!link_program(program.get())) {
        return nullptr;
    }
    
    cache_uniform_locations(program.get());
    
    // Insert into cache
    auto* result = program.get();
    shader_cache_[key] = std::move(program);
    
    DX8GL_DEBUG("Created shader program %u (features=0x%x, fvf=0x%x)",
                result->program, features, fvf);
    
    return result;
}

void ShaderGenerator::clear_cache() {
    for (auto& pair : shader_cache_) {
        auto& program = pair.second;
        if (program->program) {
            glDeleteProgram(program->program);
        }
        if (program->vertex_shader) {
            glDeleteShader(program->vertex_shader);
        }
        if (program->fragment_shader) {
            glDeleteShader(program->fragment_shader);
        }
    }
    shader_cache_.clear();
}

std::string ShaderGenerator::generate_vertex_shader(uint32_t features, DWORD fvf) {
    std::ostringstream ss;
    
    // Header - use GLSL 3.30 core or ES 3.00
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "OpenGL ES");
    
    if (is_es) {
        ss << "#version 300 es\n";
        ss << "precision highp float;\n\n";
    } else {
        ss << "#version 330 core\n\n";
    }
    
    // Attributes - use 'in' instead of 'attribute'
    ss << "in vec3 a_position;\n";
    
    if (features & SHADER_FEATURE_LIGHTING) {
        ss << "in vec3 a_normal;\n";
    }
    
    if (fvf & D3DFVF_DIFFUSE) {
        ss << "in vec4 a_color;\n";
    }
    
    int tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int i = 0; i < tex_count; i++) {
        ss << "in vec2 a_texcoord" << i << ";\n";
    }
    
    // Uniforms
    ss << "\nuniform mat4 u_mvp_matrix;\n";
    
    if (features & SHADER_FEATURE_LIGHTING) {
        ss << "uniform mat4 u_world_matrix;\n";
        ss << "uniform mat4 u_normal_matrix;\n";
    }
    
    // Varyings - use 'out' instead of 'varying'
    if (features & SHADER_FEATURE_LIGHTING) {
        ss << "\nout vec3 v_position;\n";
        ss << "out vec3 v_normal;\n";
    }
    
    if (fvf & D3DFVF_DIFFUSE) {
        ss << "out vec4 v_color;\n";
    }
    
    for (int i = 0; i < tex_count; i++) {
        ss << "out vec2 v_texcoord" << i << ";\n";
    }
    
    // Main function
    ss << "\nvoid main() {\n";
    ss << "    gl_Position = u_mvp_matrix * vec4(a_position, 1.0);\n";
    
    if (features & SHADER_FEATURE_LIGHTING) {
        ss << "    v_position = (u_world_matrix * vec4(a_position, 1.0)).xyz;\n";
        ss << "    v_normal = normalize((u_normal_matrix * vec4(a_normal, 0.0)).xyz);\n";
    }
    
    if (fvf & D3DFVF_DIFFUSE) {
        ss << "    v_color = a_color;\n";
    }
    
    for (int i = 0; i < tex_count; i++) {
        ss << "    v_texcoord" << i << " = a_texcoord" << i << ";\n";
    }
    
    ss << "}\n";
    
    return ss.str();
}

std::string ShaderGenerator::generate_fragment_shader(uint32_t features, const RenderState& state) {
    std::ostringstream ss;
    
    // Header - use GLSL 3.30 core or ES 3.00
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "OpenGL ES");
    
    if (is_es) {
        ss << "#version 300 es\n";
        ss << "precision mediump float;\n\n";
    } else {
        ss << "#version 330 core\n\n";
    }
    
    // Varyings - use 'in' instead of 'varying'
    if (features & SHADER_FEATURE_LIGHTING) {
        ss << "in vec3 v_position;\n";
        ss << "in vec3 v_normal;\n";
    }
    
    if (features & SHADER_FEATURE_VERTEX_COLOR) {
        ss << "in vec4 v_color;\n";
    }
    
    int num_textures = 0;
    if (features & SHADER_FEATURE_TEXTURE) {
        num_textures = (features & SHADER_FEATURE_MULTI_TEXTURE) ? 2 : 1;
        for (int i = 0; i < num_textures; i++) {
            ss << "in vec2 v_texcoord" << i << ";\n";
        }
    }
    
    // Output color
    ss << "\nout vec4 FragColor;\n";
    
    // Uniforms
    if (features & SHADER_FEATURE_TEXTURE) {
        for (int i = 0; i < num_textures; i++) {
            ss << "uniform sampler2D u_texture" << i << ";\n";
        }
    }
    
    if (features & SHADER_FEATURE_ALPHA_TEST) {
        ss << "uniform float u_alpha_ref;\n";
    }
    
    // Main function
    ss << "\nvoid main() {\n";
    
    // Base color
    ss << "    vec4 color = vec4(1.0);\n";
    
    // Vertex color
    if (features & SHADER_FEATURE_VERTEX_COLOR) {
        ss << "    color *= v_color;\n";
    }
    
    // Texture sampling - use 'texture' instead of 'texture2D'
    if (features & SHADER_FEATURE_TEXTURE) {
        ss << "    color *= texture(u_texture0, v_texcoord0);\n";
        
        if (features & SHADER_FEATURE_MULTI_TEXTURE) {
            // TODO: Implement proper texture stage operations
            ss << "    color *= texture(u_texture1, v_texcoord1);\n";
        }
    }
    
    // Alpha test
    if (features & SHADER_FEATURE_ALPHA_TEST) {
        switch (state.alpha_func) {
            case D3DCMP_NEVER:
                ss << "    discard;\n";
                break;
            case D3DCMP_LESS:
                ss << "    if (color.a >= u_alpha_ref) discard;\n";
                break;
            case D3DCMP_EQUAL:
                ss << "    if (color.a != u_alpha_ref) discard;\n";
                break;
            case D3DCMP_LESSEQUAL:
                ss << "    if (color.a > u_alpha_ref) discard;\n";
                break;
            case D3DCMP_GREATER:
                ss << "    if (color.a <= u_alpha_ref) discard;\n";
                break;
            case D3DCMP_NOTEQUAL:
                ss << "    if (color.a == u_alpha_ref) discard;\n";
                break;
            case D3DCMP_GREATEREQUAL:
                ss << "    if (color.a < u_alpha_ref) discard;\n";
                break;
            case D3DCMP_ALWAYS:
            default:
                break;
        }
    }
    
    ss << "    FragColor = color;\n";
    ss << "}\n";
    
    return ss.str();
}

GLuint ShaderGenerator::compile_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    if (!shader) {
        DX8GL_ERROR("Failed to create shader");
        return 0;
    }
    
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        
        std::string log(log_length, '\0');
        glGetShaderInfoLog(shader, log_length, nullptr, &log[0]);
        
        DX8GL_ERROR("Shader compilation failed: %s", log.c_str());
        DX8GL_DEBUG("Shader source:\n%s", source.c_str());
        
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool ShaderGenerator::link_program(ShaderProgram* program) {
    glLinkProgram(program->program);
    
    GLint status;
    glGetProgramiv(program->program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint log_length;
        glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &log_length);
        
        std::string log(log_length, '\0');
        glGetProgramInfoLog(program->program, log_length, nullptr, &log[0]);
        
        DX8GL_ERROR("Program linking failed: %s", log.c_str());
        return false;
    }
    
    return true;
}

void ShaderGenerator::cache_uniform_locations(ShaderProgram* program) {
    // Matrix uniforms
    program->u_mvp_matrix = glGetUniformLocation(program->program, "u_mvp_matrix");
    program->u_world_matrix = glGetUniformLocation(program->program, "u_world_matrix");
    program->u_view_matrix = glGetUniformLocation(program->program, "u_view_matrix");
    program->u_projection_matrix = glGetUniformLocation(program->program, "u_projection_matrix");
    program->u_normal_matrix = glGetUniformLocation(program->program, "u_normal_matrix");
    
    // Material uniforms
    program->u_material_diffuse = glGetUniformLocation(program->program, "u_material_diffuse");
    program->u_material_ambient = glGetUniformLocation(program->program, "u_material_ambient");
    program->u_material_specular = glGetUniformLocation(program->program, "u_material_specular");
    program->u_material_emissive = glGetUniformLocation(program->program, "u_material_emissive");
    program->u_material_power = glGetUniformLocation(program->program, "u_material_power");
    
    // Fog uniforms
    program->u_fog_color = glGetUniformLocation(program->program, "u_fog_color");
    program->u_fog_start = glGetUniformLocation(program->program, "u_fog_start");
    program->u_fog_end = glGetUniformLocation(program->program, "u_fog_end");
    program->u_fog_density = glGetUniformLocation(program->program, "u_fog_density");
    
    // Alpha test uniform
    program->u_alpha_ref = glGetUniformLocation(program->program, "u_alpha_ref");
    
    // Texture uniforms
    char name[32];
    for (int i = 0; i < 8; i++) {
        snprintf(name, sizeof(name), "u_texture%d", i);
        program->u_texture[i] = glGetUniformLocation(program->program, name);
        
        // Light uniforms
        snprintf(name, sizeof(name), "u_light_enabled[%d]", i);
        program->u_light_enabled[i] = glGetUniformLocation(program->program, name);
        
        snprintf(name, sizeof(name), "u_light_position[%d]", i);
        program->u_light_position[i] = glGetUniformLocation(program->program, name);
        
        snprintf(name, sizeof(name), "u_light_direction[%d]", i);
        program->u_light_direction[i] = glGetUniformLocation(program->program, name);
        
        snprintf(name, sizeof(name), "u_light_diffuse[%d]", i);
        program->u_light_diffuse[i] = glGetUniformLocation(program->program, name);
        
        snprintf(name, sizeof(name), "u_light_specular[%d]", i);
        program->u_light_specular[i] = glGetUniformLocation(program->program, name);
        
        snprintf(name, sizeof(name), "u_light_ambient[%d]", i);
        program->u_light_ambient[i] = glGetUniformLocation(program->program, name);
    }
}

} // namespace dx8gl