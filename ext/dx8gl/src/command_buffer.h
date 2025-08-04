#ifndef DX8GL_COMMAND_BUFFER_H
#define DX8GL_COMMAND_BUFFER_H

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "d3d8.h"
#include "logger.h"

namespace dx8gl {

// Forward declarations
class StateManager;
class VertexShaderManager;
class PixelShaderManager;
class ShaderProgramManager;

// Command types
enum class CommandType : uint8_t {
    // State changes
    SET_RENDER_STATE,
    SET_TEXTURE_STAGE_STATE,
    SET_TRANSFORM,
    SET_MATERIAL,
    SET_LIGHT,
    SET_VIEWPORT,
    SET_SCISSOR_RECT,
    
    // Resource bindings
    SET_TEXTURE,
    SET_VERTEX_SHADER,
    SET_PIXEL_SHADER,
    SET_STREAM_SOURCE,
    SET_INDICES,
    
    // Drawing
    DRAW_PRIMITIVE,
    DRAW_INDEXED_PRIMITIVE,
    DRAW_PRIMITIVE_UP,
    DRAW_INDEXED_PRIMITIVE_UP,
    
    // Clear operations
    CLEAR,
    
    // Synchronization
    FENCE,
    FLUSH,
};

// Base command structure
struct Command {
    CommandType type;
    uint32_t size;  // Total size including this header
};

// State change commands
struct SetRenderStateCmd : Command {
    D3DRENDERSTATETYPE state;
    DWORD value;
};

struct SetTextureStageStateCmd : Command {
    DWORD stage;
    D3DTEXTURESTAGESTATETYPE type;
    DWORD value;
};

struct SetTransformCmd : Command {
    D3DTRANSFORMSTATETYPE state;
    D3DMATRIX matrix;
};

struct SetMaterialCmd : Command {
    D3DMATERIAL8 material;
};

struct SetLightCmd : Command {
    DWORD index;
    D3DLIGHT8 light;
    BOOL enable;
};

struct SetViewportCmd : Command {
    D3DVIEWPORT8 viewport;
};

struct SetScissorRectCmd : Command {
    RECT rect;
    BOOL enable;
};

// Resource binding commands
struct SetTextureCmd : Command {
    DWORD stage;
    uintptr_t texture;  // Pointer to texture object
};

struct SetStreamSourceCmd : Command {
    UINT stream;
    uintptr_t vertex_buffer;  // Pointer to vertex buffer
    UINT stride;
};

struct SetIndicesCmd : Command {
    uintptr_t index_buffer;  // Pointer to index buffer
    UINT base_vertex_index;
};

// Drawing commands
struct DrawPrimitiveCmd : Command {
    D3DPRIMITIVETYPE primitive_type;
    UINT start_vertex;
    UINT primitive_count;
};

struct DrawIndexedPrimitiveCmd : Command {
    D3DPRIMITIVETYPE primitive_type;
    UINT min_index;
    UINT num_vertices;
    UINT start_index;
    UINT primitive_count;
};

struct DrawPrimitiveUPCmd : Command {
    D3DPRIMITIVETYPE primitive_type;
    UINT primitive_count;
    UINT vertex_stride;
    DWORD fvf;  // Store FVF with the command to avoid state timing issues
    // Vertex data follows immediately after this structure
};

struct DrawIndexedPrimitiveUPCmd : Command {
    D3DPRIMITIVETYPE primitive_type;
    UINT min_vertex_index;
    UINT num_vertices;
    UINT primitive_count;
    D3DFORMAT index_format;
    UINT vertex_stride;
    DWORD fvf;  // Store FVF with the command to avoid state timing issues
    // Index data follows, then vertex data
};

// Clear command
struct ClearCmd : Command {
    DWORD count;
    DWORD flags;
    D3DCOLOR color;
    float z;
    DWORD stencil;
    // D3DRECT array follows if count > 0
};

// Command buffer for batching rendering commands
class CommandBuffer {
public:
    explicit CommandBuffer(size_t initial_size = 64 * 1024);
    ~CommandBuffer();

    // Reset buffer for reuse
    void reset();
    
    // Get current size
    size_t size() const { return write_pos_; }
    
    // Get buffer data
    const uint8_t* data() const { return buffer_.data(); }
    
    // Check if buffer is empty
    bool empty() const { return write_pos_ == 0; }
    
    // Reserve space for a command
    template<typename T>
    T* allocate_command() {
        static_assert(std::is_base_of<Command, T>::value, "T must derive from Command");
        
        size_t cmd_size = sizeof(T);
        ensure_space(cmd_size);
        
        T* cmd = reinterpret_cast<T*>(buffer_.data() + write_pos_);
        cmd->type = get_command_type<T>();
        cmd->size = static_cast<uint32_t>(cmd_size);
        
        write_pos_ += cmd_size;
        command_count_++;
        
        return cmd;
    }
    
    // Allocate command with variable data
    template<typename T>
    T* allocate_command_with_data(size_t extra_data_size) {
        static_assert(std::is_base_of<Command, T>::value, "T must derive from Command");
        
        size_t cmd_size = sizeof(T) + extra_data_size;
        ensure_space(cmd_size);
        
        T* cmd = reinterpret_cast<T*>(buffer_.data() + write_pos_);
        cmd->type = get_command_type<T>();
        cmd->size = static_cast<uint32_t>(cmd_size);
        
        write_pos_ += cmd_size;
        command_count_++;
        
        return cmd;
    }
    
    // Get pointer to data after a command
    template<typename T>
    void* get_command_data(T* cmd) {
        return reinterpret_cast<uint8_t*>(cmd) + sizeof(T);
    }
    
    // Execute all commands in the buffer
    void execute(StateManager& state_manager,
                VertexShaderManager* vertex_shader_mgr = nullptr,
                PixelShaderManager* pixel_shader_mgr = nullptr,
                ShaderProgramManager* shader_program_mgr = nullptr);
    
    // Get statistics
    size_t get_command_count() const { return command_count_; }
    size_t get_capacity() const { return buffer_.size(); }
    
    // Helper methods for immediate mode drawing
    void draw_primitive_up(D3DPRIMITIVETYPE primitive_type, UINT start_vertex,
                          UINT primitive_count, const void* vertex_data,
                          size_t vertex_data_size, UINT vertex_stride, DWORD fvf);
    
    void draw_indexed_primitive_up(D3DPRIMITIVETYPE primitive_type, UINT min_vertex_index,
                                  UINT num_vertices, UINT primitive_count,
                                  const void* index_data, size_t index_data_size,
                                  D3DFORMAT index_format, const void* vertex_data,
                                  size_t vertex_data_size, UINT vertex_stride, DWORD fvf);

private:
    void ensure_space(size_t size);
    
    template<typename T>
    CommandType get_command_type();
    
    std::vector<uint8_t> buffer_;
    size_t write_pos_;
    size_t command_count_;
};

// Command buffer pool for efficient allocation
class CommandBufferPool {
public:
    CommandBufferPool();
    ~CommandBufferPool();
    
    // Get a command buffer from the pool
    std::unique_ptr<CommandBuffer> acquire();
    
    // Return a command buffer to the pool
    void release(std::unique_ptr<CommandBuffer> buffer);
    
    // Clear all pooled buffers
    void clear();
    
    // Get pool statistics
    size_t get_pool_size() const;
    size_t get_total_allocated() const;

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<CommandBuffer>> pool_;
    std::atomic<size_t> total_allocated_;
};

// Template specializations for command type mapping
template<> inline CommandType CommandBuffer::get_command_type<SetRenderStateCmd>() { 
    return CommandType::SET_RENDER_STATE; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetTextureStageStateCmd>() { 
    return CommandType::SET_TEXTURE_STAGE_STATE; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetTransformCmd>() { 
    return CommandType::SET_TRANSFORM; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetMaterialCmd>() { 
    return CommandType::SET_MATERIAL; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetLightCmd>() { 
    return CommandType::SET_LIGHT; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetViewportCmd>() { 
    return CommandType::SET_VIEWPORT; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetScissorRectCmd>() { 
    return CommandType::SET_SCISSOR_RECT; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetTextureCmd>() { 
    return CommandType::SET_TEXTURE; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetStreamSourceCmd>() { 
    return CommandType::SET_STREAM_SOURCE; 
}
template<> inline CommandType CommandBuffer::get_command_type<SetIndicesCmd>() { 
    return CommandType::SET_INDICES; 
}
template<> inline CommandType CommandBuffer::get_command_type<DrawPrimitiveCmd>() { 
    return CommandType::DRAW_PRIMITIVE; 
}
template<> inline CommandType CommandBuffer::get_command_type<DrawIndexedPrimitiveCmd>() { 
    return CommandType::DRAW_INDEXED_PRIMITIVE; 
}
template<> inline CommandType CommandBuffer::get_command_type<DrawPrimitiveUPCmd>() { 
    return CommandType::DRAW_PRIMITIVE_UP; 
}
template<> inline CommandType CommandBuffer::get_command_type<DrawIndexedPrimitiveUPCmd>() { 
    return CommandType::DRAW_INDEXED_PRIMITIVE_UP; 
}
template<> inline CommandType CommandBuffer::get_command_type<ClearCmd>() { 
    return CommandType::CLEAR; 
}

} // namespace dx8gl

#endif // DX8GL_COMMAND_BUFFER_H