#ifndef DX8GL_D3D8_INDEXBUFFER_H
#define DX8GL_D3D8_INDEXBUFFER_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <mutex>
#include "logger.h"
#include "private_data.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;

class Direct3DIndexBuffer8 : public IDirect3DIndexBuffer8 {
public:
    Direct3DIndexBuffer8(Direct3DDevice8* device, UINT length, DWORD usage,
                         D3DFORMAT format, D3DPOOL pool);
    virtual ~Direct3DIndexBuffer8();

    // Initialize the index buffer
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

    // IDirect3DIndexBuffer8 methods
    HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock,
                                   BYTE** ppbData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE Unlock() override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* pDesc) override;

    // Internal methods
    GLuint get_ibo() const { return ibo_; }
    UINT get_length() const { return length_; }
    D3DFORMAT get_format() const { return format_; }
    GLenum get_gl_type() const { return gl_type_; }
    UINT get_index_size() const { return index_size_; }
    UINT get_index_count() const { return length_ / index_size_; }
    
    // Bind this buffer to OpenGL
    void bind() const;

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    
    // Buffer properties
    UINT length_;
    DWORD usage_;
    D3DFORMAT format_;
    D3DPOOL pool_;
    DWORD priority_;
    
    // Index format info
    GLenum gl_type_;
    UINT index_size_;
    
    // OpenGL resources
    GLuint ibo_;
    
    // Lock state
    mutable std::mutex lock_mutex_;
    bool locked_;
    void* lock_buffer_;
    UINT lock_offset_;
    UINT lock_size_;
    DWORD lock_flags_;
    
    // Private data storage
    PrivateDataManager private_data_manager_;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_INDEXBUFFER_H