#include "fvf_utils.h"
#include "logger.h"
#include "d3d8_constants.h"

namespace dx8gl {

UINT FVFParser::get_vertex_size(DWORD fvf) {
    UINT size = 0;
    
    // Position
    switch (fvf & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ:
            size += 3 * sizeof(float);
            break;
        case D3DFVF_XYZRHW:
            size += 4 * sizeof(float);
            break;
        case D3DFVF_XYZB1:
            size += 4 * sizeof(float);  // XYZ + 1 blend weight
            break;
        case D3DFVF_XYZB2:
            size += 5 * sizeof(float);  // XYZ + 2 blend weights
            break;
        case D3DFVF_XYZB3:
            size += 6 * sizeof(float);  // XYZ + 3 blend weights
            break;
        case D3DFVF_XYZB4:
            size += 7 * sizeof(float);  // XYZ + 4 blend weights
            break;
        case D3DFVF_XYZB5:
            size += 8 * sizeof(float);  // XYZ + 5 blend weights
            break;
    }
    
    // Normal
    if (fvf & D3DFVF_NORMAL) {
        size += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf & D3DFVF_PSIZE) {
        size += sizeof(float);
    }
    
    // Diffuse color
    if (fvf & D3DFVF_DIFFUSE) {
        size += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        size += sizeof(DWORD);
    }
    
    // Texture coordinates
    int tex_count = get_texcoord_count(fvf);
    for (int i = 0; i < tex_count; i++) {
        size += get_texcoord_size(fvf, i) * sizeof(float);
    }
    
    return size;
}

std::vector<VertexAttribute> FVFParser::parse_fvf(DWORD fvf) {
    std::vector<VertexAttribute> attributes;
    UINT offset = 0;
    GLuint location = 0;
    
    // Position - always at location 0
    switch (fvf & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ:
            attributes.push_back({3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
            offset += 3 * sizeof(float);
            break;
        case D3DFVF_XYZRHW:
            attributes.push_back({4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
            offset += 4 * sizeof(float);
            break;
        case D3DFVF_XYZB1:
        case D3DFVF_XYZB2:
        case D3DFVF_XYZB3:
        case D3DFVF_XYZB4:
        case D3DFVF_XYZB5:
            {
                // Position
                attributes.push_back({3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
                offset += 3 * sizeof(float);
                
                // Blend weights
                int blend_count = ((fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZ) / 2;
                if (blend_count > 0) {
                    attributes.push_back({blend_count, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
                    offset += blend_count * sizeof(float);
                }
            }
            break;
    }
    
    // Normal - typically at location 1
    if (fvf & D3DFVF_NORMAL) {
        attributes.push_back({3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
        offset += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf & D3DFVF_PSIZE) {
        attributes.push_back({1, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
        offset += sizeof(float);
    }
    
    // Diffuse color - typically at location 2
    if (fvf & D3DFVF_DIFFUSE) {
        attributes.push_back({4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(offset), location++});
        offset += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        attributes.push_back({4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(offset), location++});
        offset += sizeof(DWORD);
    }
    
    // Texture coordinates - typically start at location 3
    // DirectX 8 supports up to 8 texture coordinate sets
    int tex_count = get_texcoord_count(fvf);
    for (int i = 0; i < tex_count; i++) {
        int coord_size = get_texcoord_size(fvf, i);
        attributes.push_back({coord_size, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset), location++});
        offset += coord_size * sizeof(float);
    }
    
    return attributes;
}

void FVFParser::setup_vertex_attributes(DWORD fvf, GLuint program, 
                                       UINT stride, const void* base_offset) {
    // Get standard attribute locations from shader
    GLint position_loc = glGetAttribLocation(program, "a_position");
    GLint normal_loc = glGetAttribLocation(program, "a_normal");
    GLint color_loc = glGetAttribLocation(program, "a_color");
    
    // Get texture coordinate locations for all 8 possible sets
    GLint texcoord_locs[8];
    const char* texcoord_names[8] = {
        "a_texcoord0", "a_texcoord1", "a_texcoord2", "a_texcoord3",
        "a_texcoord4", "a_texcoord5", "a_texcoord6", "a_texcoord7"
    };
    for (int i = 0; i < 8; i++) {
        texcoord_locs[i] = glGetAttribLocation(program, texcoord_names[i]);
    }
    
    UINT offset = 0;
    const uint8_t* base = static_cast<const uint8_t*>(base_offset);
    
    // Position
    if (position_loc >= 0) {
        switch (fvf & D3DFVF_POSITION_MASK) {
            case D3DFVF_XYZ:
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stride, base + offset);
                offset += 3 * sizeof(float);
                break;
            case D3DFVF_XYZRHW:
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 4, GL_FLOAT, GL_FALSE, stride, base + offset);
                offset += 4 * sizeof(float);
                break;
            default:
                // Handle blended vertices
                if ((fvf & D3DFVF_POSITION_MASK) >= D3DFVF_XYZB1) {
                    glEnableVertexAttribArray(position_loc);
                    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stride, base + offset);
                    offset += 3 * sizeof(float);
                    
                    // Skip blend weights for now
                    int blend_count = ((fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZ) / 2;
                    offset += blend_count * sizeof(float);
                }
                break;
        }
    } else {
        // Skip position data if shader doesn't use it
        switch (fvf & D3DFVF_POSITION_MASK) {
            case D3DFVF_XYZ: offset += 3 * sizeof(float); break;
            case D3DFVF_XYZRHW: offset += 4 * sizeof(float); break;
            default:
                if ((fvf & D3DFVF_POSITION_MASK) >= D3DFVF_XYZB1) {
                    offset += 3 * sizeof(float);
                    int blend_count = ((fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZ) / 2;
                    offset += blend_count * sizeof(float);
                }
                break;
        }
    }
    
    // Normal
    if (fvf & D3DFVF_NORMAL) {
        if (normal_loc >= 0) {
            glEnableVertexAttribArray(normal_loc);
            glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, stride, base + offset);
        }
        offset += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf & D3DFVF_PSIZE) {
        // GL ES 2.0 doesn't support gl_PointSize from vertex shader in most implementations
        offset += sizeof(float);
    }
    
    // Diffuse color
    if (fvf & D3DFVF_DIFFUSE) {
        if (color_loc >= 0) {
            glEnableVertexAttribArray(color_loc);
            glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, base + offset);
        }
        offset += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        // Most shaders combine this with diffuse or ignore it
        offset += sizeof(DWORD);
    }
    
    // Texture coordinates - support all 8 texture coordinate sets
    int tex_count = get_texcoord_count(fvf);
    for (int i = 0; i < tex_count && i < 8; i++) {
        int coord_size = get_texcoord_size(fvf, i);
        
        if (texcoord_locs[i] >= 0) {
            glEnableVertexAttribArray(texcoord_locs[i]);
            glVertexAttribPointer(texcoord_locs[i], coord_size, GL_FLOAT, GL_FALSE, stride, base + offset);
            DX8GL_DEBUG("Enabled texture coordinate set %d with %d components at offset %u", 
                        i, coord_size, offset);
        }
        
        offset += coord_size * sizeof(float);
    }
}

int FVFParser::get_texcoord_size(DWORD fvf, int stage) {
    // Default is 2D texture coordinates
    int size = 2;
    
    // Check if this stage has a specific size set
    DWORD tex_format = (fvf >> (16 + stage * 2)) & 0x3;
    
    switch (tex_format) {
        case D3DFVF_TEXTUREFORMAT1: size = 1; break;
        case D3DFVF_TEXTUREFORMAT2: size = 2; break;
        case D3DFVF_TEXTUREFORMAT3: size = 3; break;
        case D3DFVF_TEXTUREFORMAT4: size = 4; break;
    }
    
    return size;
}

} // namespace dx8gl