#include "vao_manager.h"
#include "fvf_utils.h"
#include "logger.h"
#include "osmesa_gl_loader.h"

namespace dx8gl {

static VAOManager* g_vao_manager = nullptr;

VAOManager& get_vao_manager() {
    if (!g_vao_manager) {
        g_vao_manager = new VAOManager();
    }
    return *g_vao_manager;
}

VAOManager::VAOManager() 
    : current_vao_(0) {
    DX8GL_INFO("VAOManager initialized");
}

VAOManager::~VAOManager() {
    clear_cache();
}

GLuint VAOManager::get_vao(DWORD fvf, GLuint program, GLuint vbo, UINT stride) {
    VAOKey key = { fvf, program, vbo };
    
    DX8GL_INFO("VAOManager::get_vao called with FVF 0x%x, program %u, VBO %u, stride %u", 
               fvf, program, vbo, stride);
    
    // Check if we already have a VAO for this combination
    auto it = vao_cache_.find(key);
    if (it != vao_cache_.end()) {
        current_vao_ = it->second->vao;
        DX8GL_INFO("Found cached VAO %u", current_vao_);
        return current_vao_;
    }
    
    // Create new VAO
    DX8GL_INFO("Creating new VAO");
    auto cached = std::make_unique<CachedVAO>();
    cached->key = key;
    cached->stride = stride;
    
    glGenVertexArrays(1, &cached->vao);
    glBindVertexArray(cached->vao);
    
    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Setup vertex attributes
    setup_vertex_attributes(fvf, program, stride);
    
    // Unbind VAO (but keep VBO bound as it's part of VAO state)
    glBindVertexArray(0);
    
    current_vao_ = cached->vao;
    GLuint vao = cached->vao;
    vao_cache_[key] = std::move(cached);
    
    DX8GL_DEBUG("Created VAO %u for FVF 0x%x, program %u, VBO %u", 
                vao, fvf, program, vbo);
    
    return vao;
}

void VAOManager::clear_cache() {
    for (auto& pair : vao_cache_) {
        if (pair.second->vao) {
            glDeleteVertexArrays(1, &pair.second->vao);
        }
    }
    vao_cache_.clear();
    current_vao_ = 0;
}

void VAOManager::setup_vertex_attributes(DWORD fvf, GLuint program, UINT stride) {
    // Get standard attribute locations from shader
    GLint position_loc = glGetAttribLocation(program, "a_position");
    GLint normal_loc = glGetAttribLocation(program, "a_normal");
    GLint color_loc = glGetAttribLocation(program, "a_color");
    GLint texcoord0_loc = glGetAttribLocation(program, "a_texcoord0");
    GLint texcoord1_loc = glGetAttribLocation(program, "a_texcoord1");
    
    DX8GL_INFO("VAO setup for FVF 0x%x: position_loc=%d, normal_loc=%d, color_loc=%d, texcoord0_loc=%d",
               fvf, position_loc, normal_loc, color_loc, texcoord0_loc);
    
    // IMPORTANT: Disable all possible attributes first to prevent Mesa crash
    // This ensures attributes from previous VAOs don't remain enabled
    if (position_loc >= 0) glDisableVertexAttribArray(position_loc);
    if (normal_loc >= 0) glDisableVertexAttribArray(normal_loc);
    if (color_loc >= 0) glDisableVertexAttribArray(color_loc);
    if (texcoord0_loc >= 0) glDisableVertexAttribArray(texcoord0_loc);
    if (texcoord1_loc >= 0) glDisableVertexAttribArray(texcoord1_loc);
    
    UINT offset = 0;
    
    // Position
    if (position_loc >= 0) {
        switch (fvf & D3DFVF_POSITION_MASK) {
            case D3DFVF_XYZ:
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stride, 
                                    reinterpret_cast<const void*>(offset));
                offset += 3 * sizeof(float);
                break;
            case D3DFVF_XYZRHW:
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 4, GL_FLOAT, GL_FALSE, stride, 
                                    reinterpret_cast<const void*>(offset));
                offset += 4 * sizeof(float);
                break;
            default:
                // Handle blended vertices
                if ((fvf & D3DFVF_POSITION_MASK) >= D3DFVF_XYZB1) {
                    glEnableVertexAttribArray(position_loc);
                    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stride, 
                                        reinterpret_cast<const void*>(offset));
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
            glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, stride, 
                                reinterpret_cast<const void*>(offset));
        }
        offset += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf & D3DFVF_PSIZE) {
        offset += sizeof(float);
    }
    
    // Diffuse color
    if (fvf & D3DFVF_DIFFUSE) {
        if (color_loc >= 0) {
            glEnableVertexAttribArray(color_loc);
            glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, 
                                reinterpret_cast<const void*>(offset));
        }
        offset += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        offset += sizeof(DWORD);
    }
    
    // Texture coordinates
    int tex_count = FVFParser::get_texcoord_count(fvf);
    for (int i = 0; i < tex_count; i++) {
        int coord_size = FVFParser::get_texcoord_size(fvf, i);
        
        GLint texcoord_loc = -1;
        if (i == 0) {
            texcoord_loc = texcoord0_loc;
        } else if (i == 1) {
            texcoord_loc = texcoord1_loc;
        }
        // TODO: Support more texture coordinates
        
        if (texcoord_loc >= 0) {
            glEnableVertexAttribArray(texcoord_loc);
            glVertexAttribPointer(texcoord_loc, coord_size, GL_FLOAT, GL_FALSE, stride, 
                                reinterpret_cast<const void*>(offset));
        }
        
        offset += coord_size * sizeof(float);
    }
}

} // namespace dx8gl