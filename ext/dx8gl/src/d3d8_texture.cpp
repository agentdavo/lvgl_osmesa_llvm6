#include "d3d8_texture.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include <algorithm>
#include <cmath>

namespace dx8gl {

Direct3DTexture8::Direct3DTexture8(Direct3DDevice8* device, UINT width, UINT height,
                                   UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
    : ref_count_(1)
    , device_(device)
    , width_(width)
    , height_(height)
    , levels_(levels)
    , usage_(usage)
    , format_(format)
    , pool_(pool)
    , priority_(0)
    , lod_(0)
    , gl_texture_(0) {
    
    // Add reference to device
    device_->AddRef();
    
    // Calculate actual mip levels if requested
    if (levels_ == 0) {
        levels_ = calculate_mip_levels(width_, height_);
    }
    
    DX8GL_DEBUG("Direct3DTexture8 created: %ux%u, levels=%u, format=%d, pool=%d",
                width, height, levels_, format, pool);
}

Direct3DTexture8::~Direct3DTexture8() {
    DX8GL_DEBUG("Direct3DTexture8 destructor");
    
    // Unregister from device tracking
    if (device_) {
        device_->unregister_texture(this);
    }
    
    // Release all surfaces
    for (auto surface : surfaces_) {
        if (surface) {
            surface->Release();
        }
    }
    surfaces_.clear();
    
    // Delete OpenGL texture
    if (gl_texture_) {
        glDeleteTextures(1, &gl_texture_);
    }
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool Direct3DTexture8::initialize() {
    // OSMesa context is always current
    
    // Create OpenGL texture
    glGenTextures(1, &gl_texture_);
    if (!gl_texture_) {
        DX8GL_ERROR("Failed to generate texture");
        return false;
    }
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, gl_texture_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error after binding texture: 0x%04x", error);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Convert format
    GLenum internal_format, format, type;
    if (!get_gl_format(format_, internal_format, format, type)) {
        DX8GL_ERROR("Unsupported texture format: %d", format_);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Create surface objects for each mip level
    surfaces_.resize(levels_);
    UINT mip_width = width_;
    UINT mip_height = height_;
    
    for (UINT level = 0; level < levels_; level++) {
        // Create surface
        auto surface = new Direct3DSurface8(this, level, mip_width, mip_height,
                                          format_, usage_);
        if (!surface->initialize()) {
            surface->Release();
            return false;
        }
        surfaces_[level] = surface;
        
        // Allocate texture storage
        glTexImage2D(GL_TEXTURE_2D, level, internal_format, mip_width, mip_height,
                    0, format, type, nullptr);
        
        GLenum tex_error = glGetError();
        if (tex_error != GL_NO_ERROR) {
            DX8GL_ERROR("OpenGL error in glTexImage2D for level %u (size %ux%u, format 0x%x): 0x%04x", 
                       level, mip_width, mip_height, internal_format, tex_error);
            return false;
        }
        
        // Next mip level dimensions
        mip_width = std::max(1u, mip_width / 2);
        mip_height = std::max(1u, mip_height / 2);
    }
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   levels_ > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error during texture creation: 0x%04x", error);
        return false;
    }
    
    DX8GL_DEBUG("Created texture %u with %u mip levels", gl_texture_, levels_);
    return true;
}

// IUnknown methods
HRESULT Direct3DTexture8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DTexture8)) {
        *ppvObj = static_cast<IDirect3DTexture8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DTexture8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DTexture8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DTexture8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DTexture8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT Direct3DTexture8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DTexture8::SetPrivateData(REFGUID refguid, const void* pData,
                                         DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DTexture8::GetPrivateData(REFGUID refguid, void* pData,
                                         DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DTexture8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

DWORD Direct3DTexture8::SetPriority(DWORD PriorityNew) {
    DWORD old = priority_;
    priority_ = PriorityNew;
    return old;
}

DWORD Direct3DTexture8::GetPriority() {
    return priority_;
}

void Direct3DTexture8::PreLoad() {
    // No-op - texture is always resident in OpenGL
}

D3DRESOURCETYPE Direct3DTexture8::GetType() {
    return D3DRTYPE_TEXTURE;
}

// IDirect3DBaseTexture8 methods
DWORD Direct3DTexture8::SetLOD(DWORD LODNew) {
    DWORD old = lod_;
    lod_ = std::min(LODNew, levels_ - 1);
    
    // GL_TEXTURE_BASE_LEVEL not available in ES 2.0, skip for now
    // TODO: Handle LOD in ES 2.0 compatible way
    (void)gl_texture_;
    
    return old;
}

DWORD Direct3DTexture8::GetLOD() {
    return lod_;
}

DWORD Direct3DTexture8::GetLevelCount() {
    return levels_;
}

// IDirect3DTexture8 methods
HRESULT Direct3DTexture8::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) {
    if (!pDesc || Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    if (surfaces_[Level]) {
        return surfaces_[Level]->GetDesc(pDesc);
    }
    
    return D3DERR_INVALIDCALL;
}

HRESULT Direct3DTexture8::GetSurfaceLevel(UINT Level, IDirect3DSurface8** ppSurfaceLevel) {
    if (!ppSurfaceLevel || Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppSurfaceLevel = surfaces_[Level];
    if (*ppSurfaceLevel) {
        (*ppSurfaceLevel)->AddRef();
        return D3D_OK;
    }
    
    return D3DERR_INVALIDCALL;
}

HRESULT Direct3DTexture8::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect,
                                   const RECT* pRect, DWORD Flags) {
    if (!pLockedRect || Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    if (surfaces_[Level]) {
        return surfaces_[Level]->LockRect(pLockedRect, pRect, Flags);
    }
    
    return D3DERR_INVALIDCALL;
}

HRESULT Direct3DTexture8::UnlockRect(UINT Level) {
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    if (surfaces_[Level]) {
        return surfaces_[Level]->UnlockRect();
    }
    
    return D3DERR_INVALIDCALL;
}

HRESULT Direct3DTexture8::AddDirtyRect(const RECT* pDirtyRect) {
    // TODO: Track dirty regions for managed textures
    return D3D_OK;
}

// Internal methods
void Direct3DTexture8::bind(UINT sampler) const {
    glActiveTexture(GL_TEXTURE0 + sampler);
    glBindTexture(GL_TEXTURE_2D, gl_texture_);
}

UINT Direct3DTexture8::calculate_mip_levels(UINT width, UINT height) const {
    UINT size = std::max(width, height);
    UINT levels = 1;
    
    while (size > 1) {
        size /= 2;
        levels++;
    }
    
    return levels;
}

bool Direct3DTexture8::get_gl_format(D3DFORMAT d3d_format, GLenum& internal_format,
                                     GLenum& format, GLenum& type) {
    // Use the same format conversion as Direct3DSurface8
    return Direct3DSurface8::get_gl_format(d3d_format, internal_format, format, type);
}

} // namespace dx8gl