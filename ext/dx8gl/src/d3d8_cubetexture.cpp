#include "d3d8_cubetexture.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include <cstring>
#include <algorithm>

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

Direct3DCubeTexture8::Direct3DCubeTexture8(Direct3DDevice8* device, UINT edge_length, UINT levels,
                                           DWORD usage, D3DFORMAT format, D3DPOOL pool)
    : ref_count_(1)
    , device_(device)
    , edge_length_(edge_length)
    , levels_(levels)
    , usage_(usage)
    , format_(format)
    , pool_(pool)
    , priority_(0)
    , lod_(0)
    , gl_texture_(0) {
    
    // If levels is 0, calculate number of mip levels
    if (levels_ == 0) {
        UINT size = edge_length_;
        levels_ = 1;
        while (size > 1) {
            size /= 2;
            levels_++;
        }
    }
    
    // Initialize face info
    for (int face = 0; face < 6; face++) {
        faces_[face].surfaces.resize(levels_, nullptr);
        faces_[face].locked.resize(levels_, false);
        faces_[face].lock_buffers.resize(levels_, nullptr);
        faces_[face].lock_flags.resize(levels_, 0);
    }
    
    // Add reference to device
    device_->AddRef();
    
    DX8GL_DEBUG("Direct3DCubeTexture8 created: edge=%u, levels=%u, format=0x%08x, pool=%d",
                edge_length, levels_, format, pool);
}

Direct3DCubeTexture8::~Direct3DCubeTexture8() {
    DX8GL_DEBUG("Direct3DCubeTexture8 destructor");
    
    // Unregister from device
    if (device_) {
        // TODO: Add cube texture tracking to device
        // device_->unregister_cube_texture(this);
    }
    
    // Release all surfaces
    for (int face = 0; face < 6; face++) {
        for (auto* surface : faces_[face].surfaces) {
            if (surface) {
                surface->Release();
            }
        }
        for (auto* buffer : faces_[face].lock_buffers) {
            if (buffer) {
                free(buffer);
            }
        }
    }
    
    // Release OpenGL resources
    if (gl_texture_) {
        glDeleteTextures(1, &gl_texture_);
    }
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool Direct3DCubeTexture8::initialize() {
    // For system memory pools, we don't create GL texture immediately
    if (pool_ == D3DPOOL_SYSTEMMEM || pool_ == D3DPOOL_SCRATCH) {
        return true;
    }
    
    // Create OpenGL cube map texture
    glGenTextures(1, &gl_texture_);
    if (!gl_texture_) {
        DX8GL_ERROR("Failed to generate cube texture");
        return false;
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, gl_texture_);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error binding cube texture: 0x%04x", error);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Get GL format info
    GLenum internal_format, format, type;
    if (!get_gl_format(format_, internal_format, format, type)) {
        DX8GL_ERROR("Unsupported cube texture format: 0x%08x", format_);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Allocate storage for all faces and mip levels
    UINT mip_size = edge_length_;
    
    for (UINT level = 0; level < levels_; level++) {
        // Allocate each cube face at this mip level
        for (int face = 0; face < 6; face++) {
            GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            
            glTexImage2D(target, level, internal_format, mip_size, mip_size,
                        0, format, type, nullptr);
            
            GLenum tex_error = glGetError();
            if (tex_error != GL_NO_ERROR) {
                DX8GL_ERROR("OpenGL error in glTexImage2D for cube face %d level %u: 0x%04x", 
                           face, level, tex_error);
                glDeleteTextures(1, &gl_texture_);
                gl_texture_ = 0;
                return false;
            }
        }
        
        // Next mip level size
        mip_size = std::max(1u, mip_size / 2);
    }
    
    // Set default texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, 
                   (levels_ > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    DX8GL_DEBUG("Created cube texture %u with %u levels", gl_texture_, levels_);
    return true;
}

// IUnknown methods
HRESULT Direct3DCubeTexture8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || 
        IsEqualGUID(*riid, IID_IDirect3DResource8) ||
        IsEqualGUID(*riid, IID_IDirect3DBaseTexture8) ||
        IsEqualGUID(*riid, IID_IDirect3DCubeTexture8)) {
        *ppvObj = static_cast<IDirect3DCubeTexture8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DCubeTexture8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DCubeTexture8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DCubeTexture8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DCubeTexture8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT Direct3DCubeTexture8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DCubeTexture8::SetPrivateData(REFGUID refguid, const void* pData,
                                              DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DCubeTexture8::GetPrivateData(REFGUID refguid, void* pData,
                                              DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DCubeTexture8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

DWORD Direct3DCubeTexture8::SetPriority(DWORD PriorityNew) {
    DWORD old = priority_;
    priority_ = PriorityNew;
    return old;
}

DWORD Direct3DCubeTexture8::GetPriority() {
    return priority_;
}

void Direct3DCubeTexture8::PreLoad() {
    // TODO: Implement texture preloading
}

D3DRESOURCETYPE Direct3DCubeTexture8::GetType() {
    return D3DRTYPE_CUBETEXTURE;
}

// IDirect3DBaseTexture8 methods
DWORD Direct3DCubeTexture8::SetLOD(DWORD LODNew) {
    if (pool_ != D3DPOOL_MANAGED) {
        return 0;
    }
    
    DWORD old_lod = lod_;
    lod_ = std::min(LODNew, levels_ - 1);
    
    if (gl_texture_) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, gl_texture_);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, lod_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    
    return old_lod;
}

DWORD Direct3DCubeTexture8::GetLOD() {
    if (pool_ != D3DPOOL_MANAGED) {
        return 0;
    }
    return lod_;
}

DWORD Direct3DCubeTexture8::GetLevelCount() {
    return levels_;
}

// IDirect3DCubeTexture8 methods
HRESULT Direct3DCubeTexture8::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    UINT mip_size = edge_length_ >> Level;
    if (mip_size == 0) mip_size = 1;
    
    pDesc->Format = format_;
    pDesc->Type = D3DRTYPE_SURFACE;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = 0;  // Not meaningful for surfaces
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    pDesc->Width = mip_size;
    pDesc->Height = mip_size;
    
    return D3D_OK;
}

HRESULT Direct3DCubeTexture8::GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, 
                                                 IDirect3DSurface8** ppCubeMapSurface) {
    if (!ppCubeMapSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    if (FaceType < D3DCUBEMAP_FACE_POSITIVE_X || FaceType > D3DCUBEMAP_FACE_NEGATIVE_Z) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    int face_index = static_cast<int>(FaceType);
    
    // Create surface if it doesn't exist
    if (!faces_[face_index].surfaces[Level]) {
        UINT mip_size = edge_length_ >> Level;
        if (mip_size == 0) mip_size = 1;
        
        // Create surface for cube texture face
        // Note: We pass nullptr for texture since this is a cube texture, not a regular texture
        auto surface = new Direct3DSurface8(device_, mip_size, mip_size, 
                                          format_, usage_);
        if (!surface->initialize()) {
            surface->Release();
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        
        faces_[face_index].surfaces[Level] = surface;
    }
    
    *ppCubeMapSurface = faces_[face_index].surfaces[Level];
    (*ppCubeMapSurface)->AddRef();
    
    return D3D_OK;
}

HRESULT Direct3DCubeTexture8::LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, 
                                       D3DLOCKED_RECT* pLockedRect,
                                       const RECT* pRect, DWORD Flags) {
    if (!pLockedRect) {
        return D3DERR_INVALIDCALL;
    }
    
    if (FaceType < D3DCUBEMAP_FACE_POSITIVE_X || FaceType > D3DCUBEMAP_FACE_NEGATIVE_Z) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    int face_index = static_cast<int>(FaceType);
    
    if (faces_[face_index].locked[Level]) {
        DX8GL_ERROR("Cube face %d level %u already locked", face_index, Level);
        return D3DERR_INVALIDCALL;
    }
    
    UINT mip_size = edge_length_ >> Level;
    if (mip_size == 0) mip_size = 1;
    
    // Calculate bytes per pixel
    UINT bytes_per_pixel = 4;  // Default to RGBA
    switch (format_) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
            bytes_per_pixel = 2;
            break;
        case D3DFMT_R8G8B8:
            bytes_per_pixel = 3;
            break;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bytes_per_pixel = 4;
            break;
        case D3DFMT_DXT1:
            bytes_per_pixel = 0;  // Special handling needed
            break;
        case D3DFMT_DXT3:
        case D3DFMT_DXT5:
            bytes_per_pixel = 0;  // Special handling needed
            break;
    }
    
    // Calculate pitch and allocate buffer
    pLockedRect->Pitch = mip_size * bytes_per_pixel;
    UINT buffer_size = pLockedRect->Pitch * mip_size;
    
    if (!faces_[face_index].lock_buffers[Level]) {
        faces_[face_index].lock_buffers[Level] = malloc(buffer_size);
        if (!faces_[face_index].lock_buffers[Level]) {
            return E_OUTOFMEMORY;
        }
        memset(faces_[face_index].lock_buffers[Level], 0, buffer_size);
    }
    
    // If locking for read and we have a GL texture, download the data
    if ((Flags & D3DLOCK_READONLY) && gl_texture_) {
        GLenum internal_format, format, type;
        if (get_gl_format(format_, internal_format, format, type)) {
            // Save current state
            GLint current_active_texture;
            glGetIntegerv(GL_ACTIVE_TEXTURE, &current_active_texture);
            glActiveTexture(GL_TEXTURE0);
            
            GLint current_binding;
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &current_binding);
            
            // Bind our texture
            glBindTexture(GL_TEXTURE_CUBE_MAP, gl_texture_);
            
            // Get the face data
            GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index;
            glGetTexImage(target, Level, format, type, faces_[face_index].lock_buffers[Level]);
            
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                DX8GL_WARNING("OpenGL error downloading cube face %d level %u: 0x%04x", 
                           face_index, Level, error);
            }
            
            // Restore previous state
            glBindTexture(GL_TEXTURE_CUBE_MAP, current_binding);
            glActiveTexture(current_active_texture);
        }
    }
    
    pLockedRect->pBits = faces_[face_index].lock_buffers[Level];
    faces_[face_index].locked[Level] = true;
    faces_[face_index].lock_flags[Level] = Flags;
    
    DX8GL_TRACE("Locked cube face %d level %u with flags 0x%08x", face_index, Level, Flags);
    return D3D_OK;
}

HRESULT Direct3DCubeTexture8::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) {
    if (FaceType < D3DCUBEMAP_FACE_POSITIVE_X || FaceType > D3DCUBEMAP_FACE_NEGATIVE_Z) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    int face_index = static_cast<int>(FaceType);
    
    if (!faces_[face_index].locked[Level]) {
        DX8GL_ERROR("Cube face %d level %u not locked", face_index, Level);
        return D3DERR_INVALIDCALL;
    }
    
    // Upload data to OpenGL if we have a texture and it wasn't locked as read-only
    if (gl_texture_ && faces_[face_index].lock_buffers[Level] && 
        !(faces_[face_index].lock_flags[Level] & D3DLOCK_READONLY)) {
        UINT mip_size = edge_length_ >> Level;
        if (mip_size == 0) mip_size = 1;
        
        GLenum internal_format, format, type;
        if (get_gl_format(format_, internal_format, format, type)) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, gl_texture_);
            
            GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index;
            glTexSubImage2D(target, Level, 0, 0, mip_size, mip_size,
                           format, type, faces_[face_index].lock_buffers[Level]);
            
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                DX8GL_ERROR("OpenGL error uploading cube face %d level %u: 0x%04x", 
                           face_index, Level, error);
            }
            
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
    }
    
    faces_[face_index].locked[Level] = false;
    faces_[face_index].lock_flags[Level] = 0;
    
    DX8GL_TRACE("Unlocked cube face %d level %u", face_index, Level);
    return D3D_OK;
}

HRESULT Direct3DCubeTexture8::AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT* pDirtyRect) {
    // Track dirty regions for managed textures
    if (pool_ == D3DPOOL_MANAGED) {
        // TODO: Implement dirty region tracking
        DX8GL_TRACE("AddDirtyRect called for cube face %d", FaceType);
    }
    return D3D_OK;
}

// Internal methods
void Direct3DCubeTexture8::bind(UINT sampler) const {
    if (gl_texture_) {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(GL_TEXTURE_CUBE_MAP, gl_texture_);
    }
}

void Direct3DCubeTexture8::release_gl_resources() {
    DX8GL_DEBUG("Releasing GL resources for cube texture %u (pool=%d)", gl_texture_, pool_);
    
    if (gl_texture_) {
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
    }
}

bool Direct3DCubeTexture8::recreate_gl_resources() {
    DX8GL_DEBUG("Recreating GL resources for cube texture (pool=%d, size=%u, levels=%u)", 
                pool_, edge_length_, levels_);
    
    // Only D3DPOOL_DEFAULT resources need recreation
    if (pool_ != D3DPOOL_DEFAULT) {
        DX8GL_WARN("Attempted to recreate non-default pool cube texture");
        return true; // Not an error, just not needed
    }
    
    // Release old resources first
    release_gl_resources();
    
    // Recreate the cube texture
    return initialize();
}

GLenum Direct3DCubeTexture8::get_cube_face_target(D3DCUBEMAP_FACES face) {
    switch (face) {
        case D3DCUBEMAP_FACE_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case D3DCUBEMAP_FACE_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case D3DCUBEMAP_FACE_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case D3DCUBEMAP_FACE_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case D3DCUBEMAP_FACE_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case D3DCUBEMAP_FACE_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }
}

bool Direct3DCubeTexture8::get_gl_format(D3DFORMAT format, GLenum& internal_format, 
                                          GLenum& format_out, GLenum& type) {
    switch (format) {
        case D3DFMT_R8G8B8:
            internal_format = GL_RGB8;
            format_out = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8R8G8B8:
            internal_format = GL_RGBA8;
            format_out = GL_BGRA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_X8R8G8B8:
            internal_format = GL_RGB8;
            format_out = GL_BGRA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_R5G6B5:
            internal_format = GL_RGB;
            format_out = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            return true;
            
        case D3DFMT_X1R5G5B5:
            internal_format = GL_RGB5;
            format_out = GL_BGRA;
            type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
            
        case D3DFMT_A1R5G5B5:
            internal_format = GL_RGB5_A1;
            format_out = GL_BGRA;
            type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
            
        case D3DFMT_A4R4G4B4:
            internal_format = GL_RGBA4;
            format_out = GL_BGRA;
            type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            return true;
            
        case D3DFMT_DXT1:
            internal_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            format_out = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            type = 0;
            return true;
            
        case D3DFMT_DXT3:
            internal_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            format_out = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            type = 0;
            return true;
            
        case D3DFMT_DXT5:
            internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            format_out = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            type = 0;
            return true;
            
        default:
            DX8GL_ERROR("Unsupported cube texture format: 0x%08x", format);
            return false;
    }
}

} // namespace dx8gl