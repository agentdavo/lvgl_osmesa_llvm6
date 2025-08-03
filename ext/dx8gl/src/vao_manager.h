#ifndef DX8GL_VAO_MANAGER_H
#define DX8GL_VAO_MANAGER_H

#include "gl3_headers.h"
#include "d3d8_types.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace dx8gl {

// Forward declarations
class Direct3DVertexBuffer8;

struct VAOKey {
    DWORD fvf;
    GLuint program;
    GLuint vbo;
    
    bool operator==(const VAOKey& other) const {
        return fvf == other.fvf && program == other.program && vbo == other.vbo;
    }
};

struct VAOKeyHash {
    std::size_t operator()(const VAOKey& key) const {
        std::size_t h1 = std::hash<DWORD>{}(key.fvf);
        std::size_t h2 = std::hash<GLuint>{}(key.program);
        std::size_t h3 = std::hash<GLuint>{}(key.vbo);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class VAOManager {
public:
    VAOManager();
    ~VAOManager();
    
    // Get or create a VAO for the given FVF, program and VBO combination
    GLuint get_vao(DWORD fvf, GLuint program, GLuint vbo, UINT stride);
    
    // Clear all cached VAOs
    void clear_cache();
    
    // Setup vertex attributes for the current VAO
    void setup_vertex_attributes(DWORD fvf, GLuint program, UINT stride);
    
private:
    struct CachedVAO {
        GLuint vao;
        VAOKey key;
        UINT stride;
    };
    
    std::unordered_map<VAOKey, std::unique_ptr<CachedVAO>, VAOKeyHash> vao_cache_;
    GLuint current_vao_;
};

// Global VAO manager instance
VAOManager& get_vao_manager();

} // namespace dx8gl

#endif // DX8GL_VAO_MANAGER_H