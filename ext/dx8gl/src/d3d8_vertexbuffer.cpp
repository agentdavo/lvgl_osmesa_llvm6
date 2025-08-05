#include "d3d8_vertexbuffer.h"
#include "d3d8_device.h"
#include "osmesa_context.h"
#include <cstring>
#include <algorithm>

// Include OpenGL headers
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

Direct3DVertexBuffer8::Direct3DVertexBuffer8(Direct3DDevice8* device, UINT length, 
                                             DWORD usage, DWORD fvf, D3DPOOL pool)
    : ref_count_(1)
    , device_(device)
    , length_(length)
    , usage_(usage)
    , fvf_(fvf)
    , pool_(pool)
    , priority_(0)
    , vbo_(0)
    , current_buffer_version_(0)  // For buffer orphaning optimization
    , locked_(false)
    , lock_buffer_(nullptr)
    , lock_offset_(0)
    , lock_size_(0)
    , lock_flags_(0)
    , has_position_(false)
    , has_rhw_(false)
    , has_normal_(false)
    , has_diffuse_(false)
    , has_specular_(false)
    , num_texcoords_(0) {
    
    // Calculate stride from FVF
    stride_ = calculate_fvf_stride(fvf);
    
    // Parse FVF attributes
    parse_fvf_attributes();
    
    // Add reference to device
    device_->AddRef();
    
    DX8GL_DEBUG("Direct3DVertexBuffer8 created: length=%u, fvf=0x%08x, stride=%u",
                length, fvf, stride_);
}

Direct3DVertexBuffer8::~Direct3DVertexBuffer8() {
    DX8GL_DEBUG("Direct3DVertexBuffer8 destructor");
    
    // Unregister from device
    if (device_) {
        device_->unregister_vertex_buffer(this);
    }
    
    // Clean up OpenGL resources
    if (vbo_ && gl_DeleteBuffers) {
        gl_DeleteBuffers(1, &vbo_);
    }
    
    // Clean up buffer versions for orphaning
    for (auto& version : buffer_versions_) {
        if (version.vbo && gl_DeleteBuffers) {
            gl_DeleteBuffers(1, &version.vbo);
        }
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

bool Direct3DVertexBuffer8::initialize() {
    // For managed/system memory pools, we don't create VBO immediately
    if (pool_ == D3DPOOL_SYSTEMMEM || pool_ == D3DPOOL_SCRATCH) {
        // Just allocate system memory
        lock_buffer_ = malloc(length_);
        if (!lock_buffer_) {
            DX8GL_ERROR("Failed to allocate system memory for vertex buffer");
            return false;
        }
        memset(lock_buffer_, 0, length_);
        return true;
    }
    
    // Ensure OSMesa context is current before creating OpenGL resources
    if (device_ && device_->get_osmesa_context()) {
        DX8GL_DEBUG("Making OSMesa context current for vertex buffer creation");
        if (!device_->get_osmesa_context()->make_current()) {
            DX8GL_ERROR("Failed to make OSMesa context current for vertex buffer creation");
            return false;
        }
        DX8GL_DEBUG("OSMesa context made current successfully");
    } else {
        DX8GL_WARNING("No OSMesa context available for vertex buffer creation");
    }
    
    // Debug: print GL version and current context
    const char* version = (const char*)glGetString(GL_VERSION);
    DX8GL_DEBUG("OpenGL version: %s", version ? version : "NULL");
    
    // Check if we have a current context
#ifdef DX8GL_HAS_OSMESA
    OSMesaContext current = OSMesaGetCurrentContext();
    DX8GL_DEBUG("Current OSMesa context: %p", current);
#else
    DX8GL_DEBUG("OSMesa not available - using default context");
#endif
    
    // Clear any existing GL errors
    GLenum clear_error;
    int error_count = 0;
    while ((clear_error = glGetError()) != GL_NO_ERROR) {
        DX8GL_DEBUG("Cleared existing GL error #%d: 0x%04x", ++error_count, clear_error);
        if (error_count > 10) {
            DX8GL_ERROR("Too many GL errors to clear, something is wrong!");
            break;
        }
    }
    DX8GL_DEBUG("Cleared %d GL errors total", error_count);
    
    // For DEFAULT pool, create VBO
#ifdef DX8GL_HAS_OSMESA
    // Debug: Check if function pointer is loaded
    DX8GL_DEBUG("gl_GenBuffers function pointer: %p", (void*)gl_GenBuffers);
    if (!gl_GenBuffers) {
        DX8GL_ERROR("gl_GenBuffers function pointer is NULL! OSMesa GL functions not loaded properly.");
        return false;
    }
    
    // Skip VAO creation for OpenGL 2.1 compatibility
    DX8GL_DEBUG("Using OpenGL 2.1 compatibility - no VAO needed");
#endif
    
    // Double-check we have a current context
    OSMesaContext current_ctx = OSMesaGetCurrentContext();
    if (!current_ctx) {
        DX8GL_ERROR("No current OSMesa context when creating vertex buffer!");
        return false;
    }
    DX8GL_DEBUG("Current OSMesa context confirmed: %p", current_ctx);
    
    // Also check if we can do a simple GL operation
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLenum clear_test = glGetError();
    DX8GL_DEBUG("glClearColor test error: 0x%04x", clear_test);
    
    DX8GL_DEBUG("Calling glGenBuffers(1, &vbo_)...");
    
    // First ensure no prior errors
    GLenum prior_error;
    int prior_error_count = 0;
    while ((prior_error = glGetError()) != GL_NO_ERROR) {
        DX8GL_DEBUG("Cleared prior error #%d before glGenBuffers: 0x%04x", ++prior_error_count, prior_error);
        if (prior_error_count > 10) {
            DX8GL_ERROR("Too many prior errors before glGenBuffers!");
            break;
        }
    }
    DX8GL_DEBUG("Cleared %d prior errors before glGenBuffers", prior_error_count);
    
    // Use ARB version if available
    if (gl_GenBuffers) {
        gl_GenBuffers(1, &vbo_);
    } else {
        DX8GL_ERROR("gl_GenBuffers is NULL!");
        return false;
    }
    GLenum gen_error = glGetError();
    DX8GL_DEBUG("glGenBuffers returned vbo=%u, GL error=0x%04x", vbo_, gen_error);
    
// No VAO cleanup needed for OpenGL 2.1
    
    if (gen_error != GL_NO_ERROR || !vbo_) {
        DX8GL_ERROR("Failed to generate vertex buffer object: GL error 0x%04x", gen_error);
        return false;
    }
    
    // Determine usage hint
    GLenum gl_usage = GL_STATIC_DRAW;
    if (usage_ & D3DUSAGE_DYNAMIC) {
        gl_usage = GL_DYNAMIC_DRAW;
    } else if (usage_ & D3DUSAGE_WRITEONLY) {
        gl_usage = GL_STREAM_DRAW;
    }
    
    // Allocate VBO storage
    if (gl_BindBuffer && gl_BufferData) {
        gl_BindBuffer(GL_ARRAY_BUFFER, vbo_);
        gl_BufferData(GL_ARRAY_BUFFER, length_, nullptr, gl_usage);
        gl_BindBuffer(GL_ARRAY_BUFFER, 0);
    } else {
        DX8GL_ERROR("gl_BindBuffer or gl_BufferData is NULL!");
        if (gl_DeleteBuffers) gl_DeleteBuffers(1, &vbo_);
        vbo_ = 0;
        return false;
    }
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("Failed to allocate VBO storage: 0x%04x", error);
        if (gl_DeleteBuffers) gl_DeleteBuffers(1, &vbo_);
        vbo_ = 0;
        return false;
    }
    
    // For DYNAMIC buffers, create additional versions for orphaning
    if (usage_ & D3DUSAGE_DYNAMIC) {
        buffer_versions_.reserve(MAX_BUFFER_VERSIONS);
        for (int i = 0; i < MAX_BUFFER_VERSIONS; ++i) {
            BufferVersion version = {0, false};
            if (gl_GenBuffers) gl_GenBuffers(1, &version.vbo);
            if (version.vbo && gl_BindBuffer && gl_BufferData) {
                gl_BindBuffer(GL_ARRAY_BUFFER, version.vbo);
                gl_BufferData(GL_ARRAY_BUFFER, length_, nullptr, GL_DYNAMIC_DRAW);
                gl_BindBuffer(GL_ARRAY_BUFFER, 0);
                
                error = glGetError();
                if (error != GL_NO_ERROR) {
                    DX8GL_WARNING("Failed to allocate orphan buffer %d: 0x%04x", i, error);
                    if (gl_DeleteBuffers) gl_DeleteBuffers(1, &version.vbo);
                    version.vbo = 0;
                }
            }
            buffer_versions_.push_back(version);
        }
        DX8GL_DEBUG("Created %zu orphan buffers for DYNAMIC usage", buffer_versions_.size());
    }
    
    DX8GL_DEBUG("Created VBO %u with %u bytes", vbo_, length_);
    return true;
}

// IUnknown methods
HRESULT Direct3DVertexBuffer8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DVertexBuffer8)) {
        *ppvObj = static_cast<IDirect3DVertexBuffer8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DVertexBuffer8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DVertexBuffer8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DVertexBuffer8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DVertexBuffer8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT Direct3DVertexBuffer8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DVertexBuffer8::SetPrivateData(REFGUID refguid, const void* pData,
                                               DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DVertexBuffer8::GetPrivateData(REFGUID refguid, void* pData,
                                               DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DVertexBuffer8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

DWORD Direct3DVertexBuffer8::SetPriority(DWORD PriorityNew) {
    DWORD old = priority_;
    priority_ = PriorityNew;
    return old;
}

DWORD Direct3DVertexBuffer8::GetPriority() {
    return priority_;
}

void Direct3DVertexBuffer8::PreLoad() {
    // No-op for vertex buffers
}

D3DRESOURCETYPE Direct3DVertexBuffer8::GetType() {
    return D3DRTYPE_VERTEXBUFFER;
}

// IDirect3DVertexBuffer8 methods
HRESULT Direct3DVertexBuffer8::Lock(UINT OffsetToLock, UINT SizeToLock,
                                    BYTE** ppbData, DWORD Flags) {
    if (!ppbData) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (locked_) {
        DX8GL_ERROR("Vertex buffer already locked");
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
    
    DX8GL_TRACE("Lock VB: offset=%u, size=%u, flags=0x%08x", 
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
    
    // For VBO, we need to allocate temporary buffer
    if (!lock_buffer_) {
        lock_buffer_ = malloc(length_);
        if (!lock_buffer_) {
            return E_OUTOFMEMORY;
        }
    }
    
    // Handle buffer orphaning for DYNAMIC buffers with DISCARD flag
    if ((usage_ & D3DUSAGE_DYNAMIC) && (Flags & D3DLOCK_DISCARD) && !buffer_versions_.empty()) {
        // Find next available buffer version
        int next_version = (current_buffer_version_ + 1) % buffer_versions_.size();
        int attempts = 0;
        
        while (buffer_versions_[next_version].in_use && attempts < buffer_versions_.size()) {
            next_version = (next_version + 1) % buffer_versions_.size();
            attempts++;
        }
        
        // If we found an available buffer, use it
        if (!buffer_versions_[next_version].in_use && buffer_versions_[next_version].vbo) {
            // Mark current buffer as in use (GPU might still be using it)
            if (current_buffer_version_ < buffer_versions_.size()) {
                buffer_versions_[current_buffer_version_].in_use = true;
            }
            
            // Switch to the new buffer
            current_buffer_version_ = next_version;
            vbo_ = buffer_versions_[current_buffer_version_].vbo;
            buffer_versions_[current_buffer_version_].in_use = false; // Will be marked in use on unlock
            
            DX8GL_TRACE("Buffer orphaning: switched to buffer version %d (VBO %u)", 
                        current_buffer_version_, vbo_);
        } else {
            DX8GL_TRACE("Buffer orphaning: all buffers in use, falling back to regular update");
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

HRESULT Direct3DVertexBuffer8::Unlock() {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (!locked_) {
        DX8GL_ERROR("Vertex buffer not locked");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("Unlock VB");
    
    // For VBO, upload the data
    if (vbo_ && !(lock_flags_ & D3DLOCK_READONLY) && gl_BindBuffer && gl_BufferData && gl_BufferSubData) {
        gl_BindBuffer(GL_ARRAY_BUFFER, vbo_);
        
        if (lock_flags_ & D3DLOCK_DISCARD) {
            // Discard entire buffer and reupload
            gl_BufferData(GL_ARRAY_BUFFER, length_, lock_buffer_,
                        (usage_ & D3DUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            
            // For orphaned buffers, mark older versions as available after a delay
            if ((usage_ & D3DUSAGE_DYNAMIC) && !buffer_versions_.empty()) {
                // Simple strategy: mark all other buffers as not in use
                // In a real implementation, you'd track frames and only mark after 2-3 frames
                for (size_t i = 0; i < buffer_versions_.size(); ++i) {
                    if (i != current_buffer_version_) {
                        buffer_versions_[i].in_use = false;
                    }
                }
            }
        } else {
            // Update only the locked region
            gl_BufferSubData(GL_ARRAY_BUFFER, lock_offset_, lock_size_,
                           static_cast<BYTE*>(lock_buffer_) + lock_offset_);
        }
        
        gl_BindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    locked_ = false;
    lock_offset_ = 0;
    lock_size_ = 0;
    lock_flags_ = 0;
    
    return D3D_OK;
}

HRESULT Direct3DVertexBuffer8::GetDesc(D3DVERTEXBUFFER_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    pDesc->Format = D3DFMT_VERTEXDATA;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = length_;
    pDesc->FVF = fvf_;
    
    return D3D_OK;
}

// Internal methods
void Direct3DVertexBuffer8::bind() const {
    if (vbo_ && gl_BindBuffer) {
        gl_BindBuffer(GL_ARRAY_BUFFER, vbo_);
    }
}

UINT Direct3DVertexBuffer8::calculate_fvf_stride(DWORD fvf) {
    UINT stride = 0;
    
    // Position
    switch (fvf & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ:
            stride += 3 * sizeof(float);
            break;
        case D3DFVF_XYZRHW:
            stride += 4 * sizeof(float);
            break;
        case D3DFVF_XYZB1:
            stride += 4 * sizeof(float);
            break;
        case D3DFVF_XYZB2:
            stride += 5 * sizeof(float);
            break;
        case D3DFVF_XYZB3:
            stride += 6 * sizeof(float);
            break;
        case D3DFVF_XYZB4:
            stride += 7 * sizeof(float);
            break;
        case D3DFVF_XYZB5:
            stride += 8 * sizeof(float);
            break;
    }
    
    // Normal
    if (fvf & D3DFVF_NORMAL) {
        stride += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf & D3DFVF_PSIZE) {
        stride += sizeof(float);
    }
    
    // Diffuse color
    if (fvf & D3DFVF_DIFFUSE) {
        stride += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        stride += sizeof(DWORD);
    }
    
    // Texture coordinates
    int tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int i = 0; i < tex_count; i++) {
        int coord_size = (fvf >> (16 + i * 2)) & 3;
        switch (coord_size) {
            case D3DFVF_TEXTUREFORMAT1: stride += 1 * sizeof(float); break;
            case D3DFVF_TEXTUREFORMAT2: stride += 2 * sizeof(float); break;
            case D3DFVF_TEXTUREFORMAT3: stride += 3 * sizeof(float); break;
            case D3DFVF_TEXTUREFORMAT4: stride += 4 * sizeof(float); break;
        }
    }
    
    return stride;
}

void Direct3DVertexBuffer8::parse_fvf_attributes() {
    attributes_.clear();
    UINT offset = 0;
    
    // Position
    switch (fvf_ & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ:
            has_position_ = true;
            has_rhw_ = false;
            attributes_.push_back({3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset)});
            offset += 3 * sizeof(float);
            break;
        case D3DFVF_XYZRHW:
            has_position_ = true;
            has_rhw_ = true;
            attributes_.push_back({4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset)});
            offset += 4 * sizeof(float);
            break;
        // Handle other position formats...
    }
    
    // Normal
    if (fvf_ & D3DFVF_NORMAL) {
        has_normal_ = true;
        attributes_.push_back({3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset)});
        offset += 3 * sizeof(float);
    }
    
    // Point size
    if (fvf_ & D3DFVF_PSIZE) {
        attributes_.push_back({1, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset)});
        offset += sizeof(float);
    }
    
    // Diffuse color
    if (fvf_ & D3DFVF_DIFFUSE) {
        has_diffuse_ = true;
        attributes_.push_back({4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(offset)});
        offset += sizeof(DWORD);
    }
    
    // Specular color
    if (fvf_ & D3DFVF_SPECULAR) {
        has_specular_ = true;
        attributes_.push_back({4, GL_UNSIGNED_BYTE, GL_TRUE, static_cast<GLsizei>(offset)});
        offset += sizeof(DWORD);
    }
    
    // Texture coordinates
    num_texcoords_ = (fvf_ & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int i = 0; i < num_texcoords_; i++) {
        int coord_size = (fvf_ >> (16 + i * 2)) & 3;
        GLint size = 2;  // Default
        switch (coord_size) {
            case D3DFVF_TEXTUREFORMAT1: size = 1; break;
            case D3DFVF_TEXTUREFORMAT2: size = 2; break;
            case D3DFVF_TEXTUREFORMAT3: size = 3; break;
            case D3DFVF_TEXTUREFORMAT4: size = 4; break;
        }
        attributes_.push_back({size, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(offset)});
        offset += size * sizeof(float);
    }
}

void Direct3DVertexBuffer8::release_gl_resources() {
    DX8GL_DEBUG("Releasing GL resources for vertex buffer %u (pool=%d)", vbo_, pool_);
    
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    
    // Release all buffer versions for dynamic buffers
    for (auto& version : buffer_versions_) {
        if (version.vbo) {
            glDeleteBuffers(1, &version.vbo);
            version.vbo = 0;
        }
    }
    buffer_versions_.clear();
    current_buffer_version_ = 0;
}

bool Direct3DVertexBuffer8::recreate_gl_resources() {
    DX8GL_DEBUG("Recreating GL resources for vertex buffer (pool=%d, size=%u, usage=0x%x)", 
                pool_, length_, usage_);
    
    // Only D3DPOOL_DEFAULT resources need recreation
    if (pool_ != D3DPOOL_DEFAULT) {
        DX8GL_WARN("Attempted to recreate non-default pool vertex buffer");
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
    
    // For dynamic buffers, create multiple versions for orphaning
    if (usage_ & D3DUSAGE_DYNAMIC) {
        buffer_versions_.resize(MAX_BUFFER_VERSIONS);
        for (int i = 0; i < MAX_BUFFER_VERSIONS; i++) {
            glGenBuffers(1, &buffer_versions_[i].vbo);
            if (!buffer_versions_[i].vbo) {
                DX8GL_ERROR("Failed to generate dynamic vertex buffer version %d", i);
                release_gl_resources();
                return false;
            }
            
            glBindBuffer(GL_ARRAY_BUFFER, buffer_versions_[i].vbo);
            glBufferData(GL_ARRAY_BUFFER, length_, nullptr, gl_usage);
            
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                DX8GL_ERROR("OpenGL error creating dynamic buffer version %d: 0x%04x", i, error);
                release_gl_resources();
                return false;
            }
            
            buffer_versions_[i].in_use = false;
        }
        vbo_ = buffer_versions_[0].vbo;
        current_buffer_version_ = 0;
        DX8GL_DEBUG("Created %d versions of dynamic vertex buffer", MAX_BUFFER_VERSIONS);
    } else {
        // Create single VBO for static buffer
        glGenBuffers(1, &vbo_);
        if (!vbo_) {
            DX8GL_ERROR("Failed to generate vertex buffer");
            return false;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, length_, nullptr, gl_usage);
        
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            DX8GL_ERROR("OpenGL error creating vertex buffer: 0x%04x", error);
            glDeleteBuffers(1, &vbo_);
            vbo_ = 0;
            return false;
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    DX8GL_DEBUG("Successfully recreated vertex buffer %u", vbo_);
    return true;
}

} // namespace dx8gl