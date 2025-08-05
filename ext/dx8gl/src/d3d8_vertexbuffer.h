#ifndef DX8GL_D3D8_VERTEXBUFFER_H
#define DX8GL_D3D8_VERTEXBUFFER_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include "logger.h"
#include "private_data.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;

class Direct3DVertexBuffer8 : public IDirect3DVertexBuffer8 {
public:
    Direct3DVertexBuffer8(Direct3DDevice8* device, UINT length, DWORD usage, 
                          DWORD fvf, D3DPOOL pool);
    virtual ~Direct3DVertexBuffer8();

    // Initialize the vertex buffer
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3DResource8 methods
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData, 
                                             DWORD SizeOfData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, 
                                             DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override;
    DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override;
    DWORD STDMETHODCALLTYPE GetPriority() override;
    void STDMETHODCALLTYPE PreLoad() override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override;

    // IDirect3DVertexBuffer8 methods
    HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock, 
                                   BYTE** ppbData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE Unlock() override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* pDesc) override;

    // Internal methods
    GLuint get_vbo() const { return vbo_; }
    UINT get_length() const { return length_; }
    DWORD get_fvf() const { return fvf_; }
    UINT get_stride() const { return stride_; }
    
    // Bind this buffer to OpenGL
    void bind() const;
    
    // Get vertex count based on stride
    UINT get_vertex_count() const { 
        return stride_ > 0 ? length_ / stride_ : 0; 
    }
    
    // Device reset support
    D3DPOOL get_pool() const { return pool_; }
    void release_gl_resources();
    bool recreate_gl_resources();

private:
    // Calculate stride from FVF
    static UINT calculate_fvf_stride(DWORD fvf);
    
    // Parse FVF to determine vertex attributes
    void parse_fvf_attributes();

    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    
    // Buffer properties
    UINT length_;
    DWORD usage_;
    DWORD fvf_;
    D3DPOOL pool_;
    UINT stride_;
    DWORD priority_;
    
    // OpenGL resources
    GLuint vbo_;
    
    // Buffer orphaning support for DYNAMIC buffers
    // This optimization creates multiple VBO versions to avoid GPU stalls
    // when updating buffers with D3DLOCK_DISCARD flag
    static constexpr int MAX_BUFFER_VERSIONS = 3;
    struct BufferVersion {
        GLuint vbo;
        bool in_use;  // Tracks if GPU might still be using this buffer
    };
    std::vector<BufferVersion> buffer_versions_;
    int current_buffer_version_;
    
    // Lock state
    mutable std::mutex lock_mutex_;
    bool locked_;
    void* lock_buffer_;
    UINT lock_offset_;
    UINT lock_size_;
    DWORD lock_flags_;
    
    // Vertex attribute information
    struct VertexAttribute {
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLsizei offset;
    };
    
    bool has_position_;
    bool has_rhw_;
    bool has_normal_;
    bool has_diffuse_;
    bool has_specular_;
    int num_texcoords_;
    std::vector<VertexAttribute> attributes_;
    
    // Private data storage
    PrivateDataManager private_data_manager_;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_VERTEXBUFFER_H