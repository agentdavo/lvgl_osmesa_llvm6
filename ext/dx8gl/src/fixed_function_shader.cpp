#include "fixed_function_shader.h"
#include "logger.h"
#include "d3d8.h"
#include <sstream>
#include <iomanip>
#include <vector>

#ifdef DX8GL_HAS_OSMESA
// MUST include osmesa_gl_loader.h AFTER GL headers to override function definitions
#include "osmesa_gl_loader.h"
#endif

namespace dx8gl {

uint64_t FixedFunctionState::get_hash() const {
    uint64_t hash = 0;
    
    // Pack state into hash
    hash |= (uint64_t)(lighting_enabled ? 1 : 0) << 0;
    hash |= (uint64_t)(fog_enabled ? 1 : 0) << 1;
    hash |= (uint64_t)(alpha_test_enabled ? 1 : 0) << 2;
    hash |= (uint64_t)(fog_mode & 0x7) << 3;
    hash |= (uint64_t)(alpha_func & 0x7) << 6;
    hash |= (uint64_t)(num_active_lights & 0xF) << 9;
    
    // Pack texture enables
    for (int i = 0; i < 8; i++) {
        if (texture_enabled[i]) {
            hash |= (uint64_t)1 << (13 + i);
        }
    }
    
    // Include vertex format in hash
    hash |= ((uint64_t)vertex_format & 0xFFFFFFFF) << 21;
    
    return hash;
}

FixedFunctionShader::FixedFunctionShader() {
    DX8GL_INFO("Created fixed function shader generator");
}

FixedFunctionShader::~FixedFunctionShader() {
    // Clean up all cached programs
    for (auto& pair : shader_cache_) {
        if (pair.second.program) {
            glDeleteProgram(pair.second.program);
        }
    }
}

GLuint FixedFunctionShader::get_program(const FixedFunctionState& state) {
    uint64_t hash = state.get_hash();
    
    // Check cache first
    auto it = shader_cache_.find(hash);
    if (it != shader_cache_.end()) {
        return it->second.program;
    }
    
    // Generate new shader
    std::string vs_source = generate_vertex_shader(state);
    std::string fs_source = generate_fragment_shader(state);
    
    DX8GL_INFO("Generated vertex shader for FVF 0x%x:\n%s", state.vertex_format, vs_source.c_str());
    DX8GL_INFO("Generated fragment shader:\n%s", fs_source.c_str());
    
    // Debug: Save shaders to files for inspection
    static int shader_debug_count = 0;
    if (shader_debug_count < 3) {
        char filename[256];
        snprintf(filename, sizeof(filename), "dx8gl_shader_%02d.vert", shader_debug_count);
        FILE* fp = fopen(filename, "w");
        if (fp) {
            fprintf(fp, "%s", vs_source.c_str());
            fclose(fp);
            DX8GL_INFO("Saved vertex shader to %s", filename);
        }
        
        snprintf(filename, sizeof(filename), "dx8gl_shader_%02d.frag", shader_debug_count);
        fp = fopen(filename, "w");
        if (fp) {
            fprintf(fp, "%s", fs_source.c_str());
            fclose(fp);
            DX8GL_INFO("Saved fragment shader to %s", filename);
        }
        shader_debug_count++;
    }
    
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_source);
    if (!vs) return 0;
    
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_source);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }
    
    GLuint program = link_program(vs, fs);
    
    // Shaders can be deleted after linking
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    if (program) {
        // Cache the program and its uniform locations
        CachedProgram cached;
        cached.program = program;
        
        // First add to cache so cache_uniform_locations can find it
        shader_cache_[hash] = cached;
        
        // Now cache the uniform locations
        cache_uniform_locations(program);
        
        DX8GL_INFO("Created fixed function shader program %u for state hash 0x%016llx", 
                   program, (unsigned long long)hash);
    }
    
    return program;
}

std::string FixedFunctionShader::generate_vertex_shader(const FixedFunctionState& state) {
    std::stringstream ss;
    
    // Target OpenGL ES 3.0 / OpenGL 3.3 Core
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "ES") != nullptr;
    
    if (is_es) {
        // OpenGL ES 3.0
        ss << "#version 300 es\n";
        ss << "precision highp float;\n\n";
    } else {
        // OpenGL 3.3 Core
        ss << "#version 330 core\n\n";
    }
    
    // Attributes based on FVF (modern GLSL uses 'in')
    bool has_rhw = (state.vertex_format & D3DFVF_XYZRHW) != 0;
    if (has_rhw) {
        ss << "in vec4 a_position;  // XYZRHW - pre-transformed screen coordinates\n";
    } else {
        ss << "in vec3 a_position;  // XYZ - world coordinates\n";
    }
    
    if (state.vertex_format & D3DFVF_NORMAL) {
        ss << "in vec3 a_normal;\n";
    }
    
    if (state.vertex_format & D3DFVF_DIFFUSE) {
        ss << "in vec4 a_color;\n";
    }
    
    // Texture coordinates
    int tex_count = (state.vertex_format & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int i = 0; i < tex_count; i++) {
        ss << "in vec2 a_texcoord" << i << ";\n";
    }
    
    // Uniforms
    ss << "\nuniform mat4 u_world;\n";
    ss << "uniform mat4 u_view;\n";
    ss << "uniform mat4 u_projection;\n";
    ss << "uniform mat4 u_worldViewProj;\n";
    
    if (has_rhw) {
        ss << "uniform vec2 u_viewport_size;  // Viewport width and height for screen-to-NDC conversion\n";
    }
    
    if (state.lighting_enabled && (state.vertex_format & D3DFVF_NORMAL)) {
        ss << "uniform mat3 u_normalMatrix;\n";
    }
    
    // Varyings (modern GLSL uses 'out')
    if (state.vertex_format & D3DFVF_DIFFUSE) {
        ss << "\nout vec4 v_color;\n";
    }
    
    for (int i = 0; i < tex_count; i++) {
        ss << "out vec2 v_texcoord" << i << ";\n";
    }
    
    if (state.lighting_enabled && (state.vertex_format & D3DFVF_NORMAL)) {
        ss << "out vec3 v_normal;\n";
        ss << "out vec3 v_worldPos;\n";
    }
    
    // Main function
    ss << "\nvoid main() {\n";
    
    if (has_rhw) {
        // XYZRHW vertices are pre-transformed screen coordinates
        // Convert from screen coordinates to OpenGL NDC [-1,1]
        ss << "    // Convert screen coordinates to NDC\n";
        ss << "    float x_ndc = (a_position.x / u_viewport_size.x) * 2.0 - 1.0;\n";
        ss << "    float y_ndc = 1.0 - (a_position.y / u_viewport_size.y) * 2.0;  // Flip Y\n";
        ss << "    gl_Position = vec4(x_ndc, y_ndc, a_position.z, a_position.w);\n";
        ss << "    vec4 worldPos = vec4(a_position.xyz, 1.0);  // Use as-is for lighting\n";
    } else {
        // XYZ vertices need standard matrix transformations
        ss << "    vec4 worldPos = u_world * vec4(a_position, 1.0);\n";
        ss << "    gl_Position = u_worldViewProj * vec4(a_position, 1.0);\n";
    }
    
    if (state.vertex_format & D3DFVF_DIFFUSE) {
        ss << "    v_color = a_color;\n";
    }
    
    for (int i = 0; i < tex_count; i++) {
        if (has_rhw) {
            // Flip U coordinate for XYZRHW vertices to fix mirrored text
            ss << "    v_texcoord" << i << " = vec2(1.0 - a_texcoord" << i << ".x, a_texcoord" << i << ".y);\n";
        } else {
            ss << "    v_texcoord" << i << " = a_texcoord" << i << ";\n";
        }
    }
    
    if (state.lighting_enabled && (state.vertex_format & D3DFVF_NORMAL)) {
        ss << "    v_normal = normalize(u_normalMatrix * a_normal);\n";
        ss << "    v_worldPos = worldPos.xyz;\n";
    }
    
    ss << "}\n";
    
    return ss.str();
}

std::string FixedFunctionShader::generate_fragment_shader(const FixedFunctionState& state) {
    std::stringstream ss;
    
    // Target OpenGL ES 3.0 / OpenGL 3.3 Core
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "ES") != nullptr;
    
    if (is_es) {
        // OpenGL ES 3.0
        ss << "#version 300 es\n";
        ss << "precision mediump float;\n\n";
    } else {
        // OpenGL 3.3 Core
        ss << "#version 330 core\n\n";
    }
    
    // Varyings (modern GLSL uses 'in')
    if (state.vertex_format & D3DFVF_DIFFUSE) {
        ss << "in vec4 v_color;\n";
    }
    
    int tex_count = (state.vertex_format & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int i = 0; i < tex_count; i++) {
        ss << "in vec2 v_texcoord" << i << ";\n";
    }
    
    if (state.lighting_enabled && (state.vertex_format & D3DFVF_NORMAL)) {
        ss << "in vec3 v_normal;\n";
        ss << "in vec3 v_worldPos;\n";
    }
    
    // Uniforms
    if (state.lighting_enabled) {
        ss << "\nuniform vec4 u_materialAmbient;\n";
        ss << "uniform vec4 u_materialDiffuse;\n";
        ss << "uniform vec4 u_materialSpecular;\n";
        ss << "uniform vec4 u_materialEmissive;\n";
        ss << "uniform float u_materialPower;\n";
        ss << "uniform vec4 u_ambientLight;\n";
        
        // Light uniforms (simplified for now)
        for (int i = 0; i < state.num_active_lights; i++) {
            ss << "uniform vec3 u_lightPos" << i << ";\n";
            ss << "uniform vec4 u_lightDiffuse" << i << ";\n";
        }
    }
    
    // Texture samplers
    for (int i = 0; i < tex_count; i++) {
        if (state.texture_enabled[i]) {
            ss << "uniform sampler2D u_texture" << i << ";\n";
        }
    }
    
    if (state.alpha_test_enabled) {
        ss << "uniform float u_alphaRef;\n";
    }
    
    // Output variable (modern GLSL requires explicit output)
    ss << "\nout vec4 fragColor;\n";
    
    // Main function
    ss << "\nvoid main() {\n";
    ss << "    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);\n";
    
    // Start with vertex color if available
    if (state.vertex_format & D3DFVF_DIFFUSE) {
        // DirectX stores ARGB as 0xAARRGGBB which in little-endian memory becomes BGRA bytes
        // OpenGL reads this as BGRA, so we need to swizzle to RGBA
        ss << "    color = v_color.bgra;\n";
    }
    
    // Apply lighting
    if (state.lighting_enabled && (state.vertex_format & D3DFVF_NORMAL)) {
        ss << "    vec3 normal = normalize(v_normal);\n";
        ss << "    vec3 lightColor = u_ambientLight.rgb * u_materialAmbient.rgb;\n";
        
        // Simple diffuse lighting for each light
        for (int i = 0; i < state.num_active_lights; i++) {
            ss << "    {\n";
            ss << "        vec3 lightDir = normalize(u_lightPos" << i << " - v_worldPos);\n";
            ss << "        float diff = max(dot(normal, lightDir), 0.0);\n";
            ss << "        lightColor += diff * u_lightDiffuse" << i << ".rgb * u_materialDiffuse.rgb;\n";
            ss << "    }\n";
        }
        
        ss << "    color.rgb *= lightColor;\n";
    }
    
    // Apply textures
    for (int i = 0; i < tex_count; i++) {
        if (state.texture_enabled[i]) {
            ss << "    color *= texture(u_texture" << i << ", v_texcoord" << i << ");\n";
        }
    }
    
    // Alpha test
    if (state.alpha_test_enabled) {
        // For now, just implement GREATER
        ss << "    if (color.a <= u_alphaRef) discard;\n";
    }
    
    // Output final color
    ss << "    fragColor = color;\n";
    ss << "}\n";
    
    return ss.str();
}

GLuint FixedFunctionShader::compile_shader(GLenum type, const std::string& source) {
    DX8GL_INFO("compile_shader called with type=0x%04x", type);
    
    // Clear any previous GL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Check if function pointer is loaded
    if (!glCreateShader) {
        DX8GL_ERROR("glCreateShader function pointer is NULL!");
        return 0;
    }
    
    GLuint shader = glCreateShader(type);
    GLenum error = glGetError();
    
    if (error != GL_NO_ERROR || !shader) {
        DX8GL_ERROR("glCreateShader failed: shader=%u, GL error 0x%04x", shader, error);
        return 0;
    }
    
    DX8GL_DEBUG("Created %s shader object %u", 
                (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", shader);
    
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("glShaderSource failed: GL error 0x%04x", error);
        glDeleteShader(shader);
        return 0;
    }
    
    glCompileShader(shader);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("glCompileShader failed: GL error 0x%04x", error);
        glDeleteShader(shader);
        return 0;
    }
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            std::vector<char> log(len);
            glGetShaderInfoLog(shader, len, nullptr, log.data());
            DX8GL_ERROR("Shader compilation failed: %s", log.data());
        } else {
            DX8GL_ERROR("Shader compilation failed with no error log");
        }
        DX8GL_ERROR("Failed shader source:\n%s", source.c_str());
        glDeleteShader(shader);
        return 0;
    }
    
    DX8GL_DEBUG("Successfully compiled %s shader %u", 
                (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", shader);
    return shader;
}

GLuint FixedFunctionShader::link_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    if (!program) {
        DX8GL_ERROR("Failed to create program");
        return 0;
    }
    
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    
    // Bind attribute locations based on our convention
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, "a_normal");
    glBindAttribLocation(program, 2, "a_color");
    glBindAttribLocation(program, 3, "a_texcoord0");
    glBindAttribLocation(program, 4, "a_texcoord1");
    
    DX8GL_INFO("Binding attributes: position=0, normal=1, color=2, texcoord0=3, texcoord1=4");
    
    glLinkProgram(program);
    
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            std::vector<char> log(len);
            glGetProgramInfoLog(program, len, nullptr, log.data());
            DX8GL_ERROR("Program linking failed: %s", log.data());
        }
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

void FixedFunctionShader::cache_uniform_locations(GLuint program) {
    if (shader_cache_.empty()) return;
    
    // Find the cached program with this GLuint
    CachedProgram* cached_prog = nullptr;
    for (auto& pair : shader_cache_) {
        if (pair.second.program == program) {
            cached_prog = &pair.second;
            break;
        }
    }
    
    if (!cached_prog) return;
    auto& uniforms = cached_prog->uniforms;
    
    // Matrix uniforms
    uniforms.world_matrix = glGetUniformLocation(program, "u_world");
    uniforms.view_matrix = glGetUniformLocation(program, "u_view");
    uniforms.projection_matrix = glGetUniformLocation(program, "u_projection");
    uniforms.world_view_proj_matrix = glGetUniformLocation(program, "u_worldViewProj");
    uniforms.normal_matrix = glGetUniformLocation(program, "u_normalMatrix");
    uniforms.viewport_size = glGetUniformLocation(program, "u_viewport_size");
    
    // Material uniforms
    uniforms.material_ambient = glGetUniformLocation(program, "u_materialAmbient");
    uniforms.material_diffuse = glGetUniformLocation(program, "u_materialDiffuse");
    uniforms.material_specular = glGetUniformLocation(program, "u_materialSpecular");
    uniforms.material_emissive = glGetUniformLocation(program, "u_materialEmissive");
    uniforms.material_power = glGetUniformLocation(program, "u_materialPower");
    
    // Light uniforms
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "u_lightPos%d", i);
        uniforms.light_position[i] = glGetUniformLocation(program, name);
        
        snprintf(name, sizeof(name), "u_lightDiffuse%d", i);
        uniforms.light_diffuse[i] = glGetUniformLocation(program, name);
    }
    
    uniforms.ambient_light = glGetUniformLocation(program, "u_ambientLight");
    uniforms.alpha_ref = glGetUniformLocation(program, "u_alphaRef");
    
    // Texture samplers
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "u_texture%d", i);
        uniforms.texture_sampler[i] = glGetUniformLocation(program, name);
    }
}

const FixedFunctionShader::UniformLocations* FixedFunctionShader::get_uniform_locations(GLuint program) const {
    for (const auto& pair : shader_cache_) {
        if (pair.second.program == program) {
            return &pair.second.uniforms;
        }
    }
    return nullptr;
}

} // namespace dx8gl