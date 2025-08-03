#include "texcoord_generator.h"
#include "logger.h"
#include <sstream>
#include <iomanip>

namespace dx8gl {

TexCoordGenMode TexCoordGenerator::get_texgen_mode(DWORD texcoord_index) {
    // Extract the texture coordinate generation mode from the upper bits
    DWORD mode = texcoord_index & 0xFFFF0000;
    
    switch (mode) {
        case D3DTSS_TCI_PASSTHRU:
            return TEXGEN_PASSTHRU;
        case D3DTSS_TCI_CAMERASPACENORMAL:
            return TEXGEN_CAMERASPACENORMAL;
        case D3DTSS_TCI_CAMERASPACEPOSITION:
            return TEXGEN_CAMERASPACEPOSITION;
        case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
            return TEXGEN_CAMERASPACEREFLECTIONVECTOR;
        default:
            DX8GL_WARNING("Unknown texture coordinate generation mode: 0x%08X", mode);
            return TEXGEN_PASSTHRU;
    }
}

int TexCoordGenerator::get_texcoord_index(DWORD texcoord_index) {
    // Extract the texture coordinate set index from the lower bits
    return texcoord_index & 0x0000FFFF;
}

std::string TexCoordGenerator::generate_texcoord_code(int texture_stage, 
                                                      DWORD texcoord_index,
                                                      DWORD transform_flags) {
    std::ostringstream code;
    
    TexCoordGenMode mode = get_texgen_mode(texcoord_index);
    int coord_index = get_texcoord_index(texcoord_index);
    
    // Generate the base texture coordinate
    std::string base_coords;
    
    switch (mode) {
        case TEXGEN_PASSTHRU:
            // Use the specified texture coordinate set
            if (coord_index < 8) {
                base_coords = "a_texcoord" + std::to_string(coord_index);
            } else {
                DX8GL_WARNING("Invalid texture coordinate index: %d", coord_index);
                base_coords = "vec2(0.0, 0.0)";
            }
            code << "    // Pass through texture coordinates\n";
            break;
            
        case TEXGEN_CAMERASPACENORMAL:
            code << generate_camera_normal_code(texture_stage);
            return code.str(); // Already includes assignment to v_texcoord
            
        case TEXGEN_CAMERASPACEPOSITION:
            code << generate_camera_position_code(texture_stage);
            return code.str();
            
        case TEXGEN_CAMERASPACEREFLECTIONVECTOR:
            code << generate_reflection_code(texture_stage);
            return code.str();
    }
    
    // Apply texture transform if needed
    if (transform_flags != D3DTTFF_DISABLE) {
        code << generate_transform_code(texture_stage, transform_flags, base_coords);
    } else {
        // No transform, just pass through
        code << "    v_texcoord" << texture_stage << " = " << base_coords << ";\n";
    }
    
    return code.str();
}

std::string TexCoordGenerator::generate_vertex_texcoord_code(const DWORD* texcoord_indices,
                                                             const DWORD* transform_flags,
                                                             int num_stages,
                                                             bool has_normals) {
    std::ostringstream code;
    
    // Check if we need any special generation modes
    bool needs_view_position = false;
    bool needs_view_normal = false;
    
    for (int i = 0; i < num_stages; i++) {
        TexCoordGenMode mode = get_texgen_mode(texcoord_indices[i]);
        if (mode == TEXGEN_CAMERASPACEPOSITION || 
            mode == TEXGEN_CAMERASPACEREFLECTIONVECTOR) {
            needs_view_position = true;
        }
        if (mode == TEXGEN_CAMERASPACENORMAL || 
            mode == TEXGEN_CAMERASPACEREFLECTIONVECTOR) {
            needs_view_normal = true;
        }
    }
    
    // Generate helper variables if needed
    if (needs_view_position) {
        code << "    // Calculate view space position\n";
        code << "    vec4 view_pos = u_view_matrix * u_world_matrix * vec4(a_position, 1.0);\n";
        code << "    vec3 v_position = view_pos.xyz / view_pos.w;\n";
        code << "\n";
    }
    
    if (needs_view_normal && has_normals) {
        code << "    // Calculate view space normal\n";
        code << "    vec3 v_normal = normalize((u_normal_matrix * vec4(a_normal, 0.0)).xyz);\n";
        code << "\n";
    }
    
    // Generate texture coordinates for each stage
    code << "    // Generate texture coordinates\n";
    for (int i = 0; i < num_stages; i++) {
        code << generate_texcoord_code(i, texcoord_indices[i], transform_flags[i]);
    }
    
    return code.str();
}

std::string TexCoordGenerator::generate_fragment_texcoord_code(int num_stages) {
    std::ostringstream code;
    
    // Fragment shader just uses the interpolated texture coordinates
    // This is a placeholder for potential future enhancements
    
    return code.str();
}

std::string TexCoordGenerator::generate_spheremap_code(int stage) {
    std::ostringstream code;
    
    // Use the template with stage number filled in
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), TexGenTemplates::SPHERE_MAP_VERTEX,
             stage, stage, stage, stage, stage, stage, stage, stage, stage, 
             stage, stage, stage, stage, stage, stage, stage, stage, stage);
    
    code << buffer;
    return code.str();
}

std::string TexCoordGenerator::generate_reflection_code(int stage) {
    std::ostringstream code;
    
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), TexGenTemplates::REFLECTION_VECTOR_VERTEX,
             stage, stage, stage, stage, stage, stage, stage, stage);
    
    code << buffer;
    return code.str();
}

std::string TexCoordGenerator::generate_camera_normal_code(int stage) {
    std::ostringstream code;
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), TexGenTemplates::CAMERA_NORMAL_VERTEX,
             stage, stage, stage, stage);
    
    code << buffer;
    return code.str();
}

std::string TexCoordGenerator::generate_camera_position_code(int stage) {
    std::ostringstream code;
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), TexGenTemplates::CAMERA_POSITION_VERTEX,
             stage, stage, stage, stage, stage);
    
    code << buffer;
    return code.str();
}

std::string TexCoordGenerator::generate_transform_code(int stage, 
                                                      DWORD transform_flags,
                                                      const std::string& input_coords) {
    std::ostringstream code;
    
    // Extract the coordinate count
    int coord_count = transform_flags & 0xFF;
    bool projected = (transform_flags & D3DTTFF_PROJECTED) != 0;
    
    if (coord_count == D3DTTFF_DISABLE) {
        // No transform
        code << "    v_texcoord" << stage << " = " << input_coords << ";\n";
        return code.str();
    }
    
    // Generate texture transform code
    char buffer[512];
    snprintf(buffer, sizeof(buffer), TexGenTemplates::TEXTURE_TRANSFORM,
             stage, input_coords.c_str(), stage, stage, stage);
    code << buffer;
    
    // Apply projection if needed
    if (projected) {
        snprintf(buffer, sizeof(buffer), TexGenTemplates::PROJECTED_COORDS,
                 stage, stage, stage);
    } else {
        snprintf(buffer, sizeof(buffer), TexGenTemplates::STANDARD_COORDS,
                 stage, stage);
    }
    code << buffer;
    
    return code.str();
}

void ShaderGeneratorTexCoordExtension::inject_texcoord_generation(std::string& vertex_shader,
                                                                  const DWORD* texcoord_indices,
                                                                  const DWORD* transform_flags,
                                                                  int num_stages) {
    // This would modify an existing vertex shader to add texture coordinate generation
    // For now, this is a placeholder - the actual implementation would need to
    // parse the shader and inject code at the appropriate location
    DX8GL_DEBUG("Texture coordinate generation injection not yet implemented");
}

std::string ShaderGeneratorTexCoordExtension::get_texcoord_uniforms(const DWORD* transform_flags,
                                                                   int num_stages) {
    std::ostringstream uniforms;
    
    // Add texture matrices for stages that use transforms
    for (int i = 0; i < num_stages; i++) {
        if (transform_flags[i] != D3DTTFF_DISABLE) {
            uniforms << "uniform mat4 u_texture_matrix" << i << ";\n";
        }
    }
    
    return uniforms.str();
}

std::string ShaderGeneratorTexCoordExtension::get_texcoord_varyings(int num_stages) {
    std::ostringstream varyings;
    
    for (int i = 0; i < num_stages; i++) {
        varyings << "varying vec2 v_texcoord" << i << ";\n";
    }
    
    return varyings.str();
}

} // namespace dx8gl