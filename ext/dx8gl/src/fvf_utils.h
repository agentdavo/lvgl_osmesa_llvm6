#ifndef DX8GL_FVF_UTILS_H
#define DX8GL_FVF_UTILS_H

#include "d3d8_types.h"
#include "d3d8_constants.h"
#include "gl3_headers.h"
#include <vector>

namespace dx8gl {

// Structure to describe a vertex attribute
struct VertexAttribute {
    GLint size;         // Number of components (1-4)
    GLenum type;        // GL_FLOAT, GL_UNSIGNED_BYTE, etc.
    GLboolean normalized;
    GLsizei offset;     // Offset in bytes from start of vertex
    GLuint location;    // Shader attribute location
};

// FVF parsing utility class
class FVFParser {
public:
    // Parse FVF and return vertex size
    static UINT get_vertex_size(DWORD fvf);
    
    // Parse FVF and return attribute list
    static std::vector<VertexAttribute> parse_fvf(DWORD fvf);
    
    // Set up vertex attributes for current shader
    static void setup_vertex_attributes(DWORD fvf, GLuint program, 
                                       UINT stride, const void* base_offset = nullptr);
    
    // Get number of texture coordinates
    static int get_texcoord_count(DWORD fvf) {
        return (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    }
    
    // Check FVF flags
    static bool has_position(DWORD fvf) { return (fvf & D3DFVF_POSITION_MASK) != 0; }
    static bool has_rhw(DWORD fvf) { return (fvf & D3DFVF_XYZRHW) != 0; }
    static bool has_normal(DWORD fvf) { return (fvf & D3DFVF_NORMAL) != 0; }
    static bool has_diffuse(DWORD fvf) { return (fvf & D3DFVF_DIFFUSE) != 0; }
    static bool has_specular(DWORD fvf) { return (fvf & D3DFVF_SPECULAR) != 0; }
    static bool has_psize(DWORD fvf) { return (fvf & D3DFVF_PSIZE) != 0; }
    
    // Get texture coordinate size for a specific stage
    static int get_texcoord_size(DWORD fvf, int stage);
};

} // namespace dx8gl

#endif // DX8GL_FVF_UTILS_H