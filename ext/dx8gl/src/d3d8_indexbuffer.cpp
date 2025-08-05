#include "d3d8_indexbuffer.h"
#include "d3d8_device.h"
#include "osmesa_context.h"
#include <cstring>

// Include OpenGL headers - order is important!
#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/osmesa.h>
// MUST include osmesa_gl_loader.h AFTER GL headers to override function definitions
#include "osmesa_gl_loader.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace dx8gl {

Direct3DIndexBuffer8::Direct3DIndexBuffer8(Direct3DDevice8* device, UINT length,
                                           DWORD usage, D3DFORMAT format, D3DPOOL pool)
    : ref_count_(1)
    , device_(device)
    , length_(length)
    , usage_(usage)
    , format_(format)
    , pool_(pool)
    , priority_(0)
    , ibo_(0)
    , locked_(false)
    , lock_buffer_(nullptr)
    , lock_offset_(0)
    , lock_size_(0)
    , lock_flags_(0) {
    
    // Determine GL type and size from format
    switch (format) {
        case D3DFMT_INDEX16:
            gl_type_ = GL_UNSIGNED_SHORT;
            index_size_ = 2;
            break;
        case D3DFMT_INDEX32:
            gl_type_ = GL_UNSIGNED_INT;
            index_size_ = 4;
            break;
        default:
            DX8GL_ERROR("Invalid index buffer format: %d", format);
            gl_type_ = GL_UNSIGNED_SHORT;
            index_size_ = 2;
            break;
    }
    
    // Add reference to device
    device_->AddRef();
    
    DX8GL_DEBUG("Direct3DIndexBuffer8 created: length=%u, format=%d, index_size=%u",
                length, format, index_size_);
}

Direct3DIndexBuffer8::~Direct3DIndexBuffer8() {
    DX8GL_DEBUG("Direct3DIndexBuffer8 destructor");
    
    // Unregister from device
    if (device_) {
        device_->unregister_index_buffer(this);
    }
    
    // Clean up OpenGL resources
    if (ibo_ && gl_DeleteBuffers) {
        gl_DeleteBuffers(1, &ibo_);
    }
    
    // Clean up lock buffer
    if (lock_buffer_) {
        free(lock_buffer_);
    }
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool Direct3DIndexBuffer8::initialize() {
    // For managed/system memory pools, we don't create IBO immediately
    if (pool_ == D3DPOOL_SYSTEMMEM || pool_ == D3DPOOL_SCRATCH) {
        // Just allocate system memory
        lock_buffer_ = malloc(length_);
        if (!lock_buffer_) {
            DX8GL_ERROR("Failed to allocate system memory for index buffer");
            return false;
        }
        memset(lock_buffer_, 0, length_);
        return true;
    }
    
    // Ensure OSMesa context is current before creating OpenGL resources
    if (device_ && device_->get_osmesa_context()) {
        if (!device_->get_osmesa_context()->make_current()) {
            DX8GL_ERROR("Failed to make OSMesa context current for index buffer creation");
            return false;
        }
    }
    
    // Clear any existing GL errors
    GLenum clear_error;
    while ((clear_error = glGetError()) != GL_NO_ERROR) {
        DX8GL_DEBUG("Cleared existing GL error: 0x%04x", clear_error);
    }
    
    // For DEFAULT pool, create IBO
#ifdef DX8GL_HAS_OSMESA
    // Skip VAO creation for OpenGL 2.1 compatibility
    DX8GL_DEBUG("Using OpenGL 2.1 compatibility - no VAO needed");
#endif
    
    if (gl_GenBuffers) {
        gl_GenBuffers(1, &ibo_);
    } else {
        DX8GL_ERROR("gl_GenBuffers is NULL!");
        return false;
    }
    GLenum gen_error = glGetError();
    DX8GL_DEBUG("glGenBuffers returned ibo=%u, GL error=0x%04x", ibo_, gen_error);
    
// No VAO cleanup needed for OpenGL 2.1
    
    if (gen_error != GL_NO_ERROR || !ibo_) {
        DX8GL_ERROR("Failed to generate index buffer object: GL error 0x%04x", gen_error);
        return false;
    }
    
    // Determine usage hint
    GLenum gl_usage = GL_STATIC_DRAW;
    if (usage_ & D3DUSAGE_DYNAMIC) {
        gl_usage = GL_DYNAMIC_DRAW;
    } else if (usage_ & D3DUSAGE_WRITEONLY) {
        gl_usage = GL_STREAM_DRAW;
    }
    
    // Allocate IBO storage
    if (gl_BindBuffer && gl_BufferData) {
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
        gl_BufferData(GL_ELEMENT_ARRAY_BUFFER, length_, nullptr, gl_usage);
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        DX8GL_ERROR("gl_BindBuffer or gl_BufferData is NULL!");
        if (gl_DeleteBuffers) gl_DeleteBuffers(1, &ibo_);
        ibo_ = 0;
        return false;
    }
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("Failed to allocate IBO storage: 0x%04x", error);
        if (gl_DeleteBuffers) gl_DeleteBuffers(1, &ibo_);
        ibo_ = 0;
        return false;
    }
    
    DX8GL_DEBUG("Created IBO %u with %u bytes", ibo_, length_);
    return true;
}

// IUnknown methods
HRESULT Direct3DIndexBuffer8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DIndexBuffer8)) {
        *ppvObj = static_cast<IDirect3DIndexBuffer8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DIndexBuffer8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DIndexBuffer8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DIndexBuffer8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DIndexBuffer8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT Direct3DIndexBuffer8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DIndexBuffer8::SetPrivateData(REFGUID refguid, const void* pData,
                                              DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DIndexBuffer8::GetPrivateData(REFGUID refguid, void* pData,
                                              DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DIndexBuffer8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

DWORD Direct3DIndexBuffer8::SetPriority(DWORD PriorityNew) {
    DWORD old = priority_;
    priority_ = PriorityNew;
    return old;
}

DWORD Direct3DIndexBuffer8::GetPriority() {
    return priority_;
}

void Direct3DIndexBuffer8::PreLoad() {
    // No-op for index buffers
}

D3DRESOURCETYPE Direct3DIndexBuffer8::GetType() {
    return D3DRTYPE_INDEXBUFFER;
}

// IDirect3DIndexBuffer8 methods
HRESULT Direct3DIndexBuffer8::Lock(UINT OffsetToLock, UINT SizeToLock,
                                   BYTE** ppbData, DWORD Flags) {
    if (!ppbData) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (locked_) {
        DX8GL_ERROR("Index buffer already locked");
        return D3DERR_INVALIDCALL;
    }
    
    // Handle size 0 = lock entire buffer
    if (SizeToLock == 0) {
        SizeToLock = length_ - OffsetToLock;
    }
    
    // Validate parameters
    if (OffsetToLock + SizeToLock > length_) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("Lock IB: offset=%u, size=%u, flags=0x%08x", 
                OffsetToLock, SizeToLock, Flags);
    
    // For system memory buffers, return direct pointer
    if (pool_ == D3DPOOL_SYSTEMMEM || pool_ == D3DPOOL_SCRATCH) {
        *ppbData = static_cast<BYTE*>(lock_buffer_) + OffsetToLock;
        locked_ = true;
        lock_offset_ = OffsetToLock;
        lock_size_ = SizeToLock;
        lock_flags_ = Flags;
        return D3D_OK;
    }
    
    // For IBO, we need to allocate temporary buffer
    if (!lock_buffer_) {
        lock_buffer_ = malloc(length_);
        if (!lock_buffer_) {
            return E_OUTOFMEMORY;
        }
    }
    
    // ES 2.0 doesn't support buffer mapping, so we keep data in CPU memory
    // No need to read back data since we maintain it in lock_buffer_
    
    *ppbData = static_cast<BYTE*>(lock_buffer_) + OffsetToLock;
    locked_ = true;
    lock_offset_ = OffsetToLock;
    lock_size_ = SizeToLock;
    lock_flags_ = Flags;
    
    return D3D_OK;
}

HRESULT Direct3DIndexBuffer8::Unlock() {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (!locked_) {
        DX8GL_ERROR("Index buffer not locked");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("Unlock IB");
    
    // For IBO, upload the data
    if (ibo_ && !(lock_flags_ & D3DLOCK_READONLY) && gl_BindBuffer && gl_BufferData && gl_BufferSubData) {
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
        
        if (lock_flags_ & D3DLOCK_DISCARD) {
            // Discard entire buffer and reupload
            gl_BufferData(GL_ELEMENT_ARRAY_BUFFER, length_, lock_buffer_,
                        (usage_ & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        } else {
            // Update only the locked region
            gl_BufferSubData(GL_ELEMENT_ARRAY_BUFFER, lock_offset_, lock_size_,
                           static_cast<BYTE*>(lock_buffer_) + lock_offset_);
        }
        
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    locked_ = false;
    lock_offset_ = 0;
    lock_size_ = 0;
    lock_flags_ = 0;
    
    return D3D_OK;
}

HRESULT Direct3DIndexBuffer8::GetDesc(D3DINDEXBUFFER_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    pDesc->Format = format_;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = length_;
    
    return D3D_OK;
}

// Internal methods
void Direct3DIndexBuffer8::bind() const {
    if (ibo_ && gl_BindBuffer) {
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    }
}

void Direct3DIndexBuffer8::release_gl_resources() {
    DX8GL_DEBUG("Releasing GL resources for index buffer %u (pool=%d)", ibo_, pool_);
    
    if (ibo_) {
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }
}

bool Direct3DIndexBuffer8::recreate_gl_resources() {
    DX8GL_DEBUG("Recreating GL resources for index buffer (pool=%d, size=%u, usage=0x%x)", 
                pool_, length_, usage_);
    
    // Only D3DPOOL_DEFAULT resources need recreation
    if (pool_ != D3DPOOL_DEFAULT) {
        DX8GL_WARN("Attempted to recreate non-default pool index buffer");
        return true; // Not an error, just not needed
    }
    
    // Release old resources first
    release_gl_resources();
    
    // Determine buffer usage
    GLenum gl_usage = GL_STATIC_DRAW;
    if (usage_ & D3DUSAGE_DYNAMIC) {
        gl_usage = GL_DYNAMIC_DRAW;
    } else if (usage_ & D3DUSAGE_WRITEONLY) {
        gl_usage = GL_STREAM_DRAW;
    }
    
    // Create new IBO
    glGenBuffers(1, &ibo_);
    if (!ibo_) {
        DX8GL_ERROR("Failed to generate index buffer");
        return false;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, length_, nullptr, gl_usage);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error creating index buffer: 0x%04x", error);
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
        return false;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    DX8GL_DEBUG("Successfully recreated index buffer %u", ibo_);
    return true;
}

} // namespace dx8gl