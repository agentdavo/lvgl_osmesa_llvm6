#include "d3d8_volumetexture.h"
#include "d3d8_device.h"
#include "d3d8_volume.h"
#include <cstring>
#include <algorithm>
#include <sstream>

// Include OpenGL headers - order is important!
#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/osmesa.h>
#include "osmesa_gl_loader.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace dx8gl {

Direct3DVolumeTexture8::Direct3DVolumeTexture8(Direct3DDevice8* device, UINT width, UINT height, UINT depth,
                                               UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
    : ref_count_(1)
    , device_(device)
    , width_(width)
    , height_(height)
    , depth_(depth)
    , levels_(levels)
    , usage_(usage)
    , format_(format)
    , pool_(pool)
    , priority_(0)
    , lod_(0)
    , gl_texture_(0)
    , has_dirty_regions_(false) {
    
    // If levels is 0, calculate number of mip levels
    if (levels_ == 0) {
        UINT size = std::max({width_, height_, depth_});
        levels_ = 1;
        while (size > 1) {
            size /= 2;
            levels_++;
        }
    }
    
    // Initialize volume info
    volumes_.resize(levels_);
    for (UINT i = 0; i < levels_; i++) {
        volumes_[i].volume = nullptr;
        volumes_[i].locked = false;
        volumes_[i].lock_buffer = nullptr;
        volumes_[i].lock_flags = 0;
    }
    
    // Initialize dirty tracking
    level_fully_dirty_.resize(levels_, false);
    
    // Add reference to device
    device_->AddRef();
    
    // Register with device for proper cleanup on device reset
    device_->register_volume_texture(this);
    
    DX8GL_DEBUG("Direct3DVolumeTexture8 created: %ux%ux%u, levels=%u, format=0x%08x, pool=%d",
                width, height, depth, levels_, format, pool);
}

Direct3DVolumeTexture8::~Direct3DVolumeTexture8() {
    DX8GL_DEBUG("Direct3DVolumeTexture8 destructor");
    
    // Unregister from device
    if (device_) {
        device_->unregister_volume_texture(this);
    }
    
    // Release all volumes
    for (auto& vol_info : volumes_) {
        if (vol_info.volume) {
            vol_info.volume->Release();
        }
        if (vol_info.lock_buffer) {
            free(vol_info.lock_buffer);
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

bool Direct3DVolumeTexture8::initialize() {
    // For system memory pools, we don't create GL texture immediately
    if (pool_ == D3DPOOL_SYSTEMMEM || pool_ == D3DPOOL_SCRATCH) {
        return true;
    }
    
    // Check if 3D textures are supported
    GLint max_3d_texture_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    if (max_3d_texture_size == 0) {
        DX8GL_ERROR("3D textures not supported by OpenGL implementation");
        return false;
    }
    
    if (width_ > (UINT)max_3d_texture_size || height_ > (UINT)max_3d_texture_size || 
        depth_ > (UINT)max_3d_texture_size) {
        DX8GL_ERROR("Volume texture dimensions exceed maximum: %dx%dx%d (max: %d)",
                   width_, height_, depth_, max_3d_texture_size);
        return false;
    }
    
    // Create OpenGL 3D texture
    glGenTextures(1, &gl_texture_);
    if (!gl_texture_) {
        DX8GL_ERROR("Failed to generate 3D texture");
        return false;
    }
    
    glBindTexture(GL_TEXTURE_3D, gl_texture_);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error binding 3D texture: 0x%04x", error);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Get GL format info
    GLenum internal_format, format, type;
    if (!get_gl_format(format_, internal_format, format, type)) {
        DX8GL_ERROR("Unsupported volume texture format: 0x%08x", format_);
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
        return false;
    }
    
    // Allocate storage for all mip levels
    UINT mip_width = width_;
    UINT mip_height = height_;
    UINT mip_depth = depth_;
    
    for (UINT level = 0; level < levels_; level++) {
        glTexImage3D(GL_TEXTURE_3D, level, internal_format, 
                    mip_width, mip_height, mip_depth,
                    0, format, type, nullptr);
        
        GLenum tex_error = glGetError();
        if (tex_error != GL_NO_ERROR) {
            DX8GL_ERROR("OpenGL error in glTexImage3D for level %u: 0x%04x", level, tex_error);
            glDeleteTextures(1, &gl_texture_);
            gl_texture_ = 0;
            return false;
        }
        
        // Next mip level size
        mip_width = std::max(1u, mip_width / 2);
        mip_height = std::max(1u, mip_height / 2);
        mip_depth = std::max(1u, mip_depth / 2);
    }
    
    // Set default texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, 
                   (levels_ > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    DX8GL_DEBUG("Created 3D texture %u with %u levels", gl_texture_, levels_);
    return true;
}

// IUnknown methods
HRESULT Direct3DVolumeTexture8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(riid, IID_IUnknown) || 
        IsEqualGUID(riid, IID_IDirect3DResource8) ||
        IsEqualGUID(riid, IID_IDirect3DBaseTexture8) ||
        IsEqualGUID(riid, IID_IDirect3DVolumeTexture8)) {
        *ppvObj = static_cast<IDirect3DVolumeTexture8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DVolumeTexture8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DVolumeTexture8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DVolumeTexture8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DVolumeTexture8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT Direct3DVolumeTexture8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DVolumeTexture8::SetPrivateData(REFGUID refguid, const void* pData,
                                               DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DVolumeTexture8::GetPrivateData(REFGUID refguid, void* pData,
                                               DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DVolumeTexture8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

DWORD Direct3DVolumeTexture8::SetPriority(DWORD PriorityNew) {
    DWORD old = priority_;
    priority_ = PriorityNew;
    return old;
}

DWORD Direct3DVolumeTexture8::GetPriority() {
    return priority_;
}

void Direct3DVolumeTexture8::PreLoad() {
    DX8GL_TRACE("Direct3DVolumeTexture8::PreLoad() - texture %u", gl_texture_);
    
    if (gl_texture_) {
        glBindTexture(GL_TEXTURE_3D, gl_texture_);
        
        // Set texture parameters to ensure proper sampling
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, 
                       levels_ > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        CHECK_GL_ERROR("Volume texture PreLoad");
    }
}

D3DRESOURCETYPE Direct3DVolumeTexture8::GetType() {
    return D3DRTYPE_VOLUMETEXTURE;
}

// IDirect3DBaseTexture8 methods
DWORD Direct3DVolumeTexture8::SetLOD(DWORD LODNew) {
    if (pool_ != D3DPOOL_MANAGED) {
        return 0;
    }
    
    DWORD old_lod = lod_;
    lod_ = std::min(LODNew, levels_ - 1);
    
    if (gl_texture_) {
        glBindTexture(GL_TEXTURE_3D, gl_texture_);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, lod_);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    
    return old_lod;
}

DWORD Direct3DVolumeTexture8::GetLOD() {
    if (pool_ != D3DPOOL_MANAGED) {
        return 0;
    }
    return lod_;
}

DWORD Direct3DVolumeTexture8::GetLevelCount() {
    return levels_;
}

// IDirect3DVolumeTexture8 methods
HRESULT Direct3DVolumeTexture8::GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    UINT mip_width, mip_height, mip_depth;
    VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, Level,
                                            mip_width, mip_height, mip_depth);
    
    pDesc->Format = format_;
    pDesc->Type = D3DRTYPE_VOLUME;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = calculate_texture_size(Level);
    pDesc->Width = mip_width;
    pDesc->Height = mip_height;
    pDesc->Depth = mip_depth;
    
    return D3D_OK;
}

HRESULT Direct3DVolumeTexture8::GetVolumeLevel(UINT Level, IDirect3DVolume8** ppVolumeLevel) {
    if (!ppVolumeLevel) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    // Create volume if it doesn't exist
    if (!volumes_[Level].volume) {
        UINT mip_width, mip_height, mip_depth;
        VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, Level,
                                                mip_width, mip_height, mip_depth);
        
        auto volume = new Direct3DVolume8(device_, mip_width, mip_height, mip_depth,
                                         format_, usage_, pool_);
        if (!volume->initialize()) {
            volume->Release();
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        
        volumes_[Level].volume = volume;
    }
    
    *ppVolumeLevel = volumes_[Level].volume;
    (*ppVolumeLevel)->AddRef();
    
    return D3D_OK;
}

HRESULT Direct3DVolumeTexture8::LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume,
                                        const D3DBOX* pBox, DWORD Flags) {
    if (!pLockedVolume) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    if (volumes_[Level].locked) {
        DX8GL_ERROR("Volume level %u already locked", Level);
        return D3DERR_INVALIDCALL;
    }
    
    UINT mip_width, mip_height, mip_depth;
    VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, Level,
                                            mip_width, mip_height, mip_depth);
    
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
        case D3DFMT_L8:
        case D3DFMT_A8:
            bytes_per_pixel = 1;
            break;
        case D3DFMT_A8L8:
            bytes_per_pixel = 2;
            break;
    }
    
    // Determine lock region
    D3DBOX lock_box;
    if (pBox) {
        lock_box = *pBox;
        // Clamp to mip level dimensions
        lock_box.Right = std::min(lock_box.Right, mip_width);
        lock_box.Bottom = std::min(lock_box.Bottom, mip_height);
        lock_box.Back = std::min(lock_box.Back, mip_depth);
    } else {
        lock_box.Left = 0;
        lock_box.Top = 0;
        lock_box.Front = 0;
        lock_box.Right = mip_width;
        lock_box.Bottom = mip_height;
        lock_box.Back = mip_depth;
    }
    
    // Calculate pitches and allocate buffer
    UINT box_width = lock_box.Right - lock_box.Left;
    UINT box_height = lock_box.Bottom - lock_box.Top;
    UINT box_depth = lock_box.Back - lock_box.Front;
    
    pLockedVolume->RowPitch = box_width * bytes_per_pixel;
    pLockedVolume->SlicePitch = pLockedVolume->RowPitch * box_height;
    UINT buffer_size = pLockedVolume->SlicePitch * box_depth;
    
    if (!volumes_[Level].lock_buffer) {
        volumes_[Level].lock_buffer = malloc(buffer_size);
        if (!volumes_[Level].lock_buffer) {
            return E_OUTOFMEMORY;
        }
        memset(volumes_[Level].lock_buffer, 0, buffer_size);
    }
    
    // If locking for read and we have a GL texture, download the data
    if ((Flags & D3DLOCK_READONLY) && gl_texture_) {
        // Note: glGetTexImage for 3D textures requires OpenGL 1.2+
        // We would need to download the entire level and extract the box
        // For now, we'll just provide the buffer
        DX8GL_TRACE("Volume texture read-back not fully implemented");
    }
    
    pLockedVolume->pBits = volumes_[Level].lock_buffer;
    volumes_[Level].locked = true;
    volumes_[Level].lock_flags = Flags;
    volumes_[Level].lock_box = lock_box;
    
    DX8GL_TRACE("Locked volume level %u with flags 0x%08x", Level, Flags);
    return D3D_OK;
}

HRESULT Direct3DVolumeTexture8::UnlockBox(UINT Level) {
    if (Level >= levels_) {
        return D3DERR_INVALIDCALL;
    }
    
    if (!volumes_[Level].locked) {
        DX8GL_ERROR("Volume level %u not locked", Level);
        return D3DERR_INVALIDCALL;
    }
    
    // Upload data to OpenGL if we have a texture and it wasn't locked as read-only
    if (gl_texture_ && volumes_[Level].lock_buffer && 
        !(volumes_[Level].lock_flags & D3DLOCK_READONLY)) {
        
        const D3DBOX& box = volumes_[Level].lock_box;
        UINT box_width = box.Right - box.Left;
        UINT box_height = box.Bottom - box.Top;
        UINT box_depth = box.Back - box.Front;
        
        GLenum internal_format, format, type;
        if (get_gl_format(format_, internal_format, format, type)) {
            glBindTexture(GL_TEXTURE_3D, gl_texture_);
            
            glTexSubImage3D(GL_TEXTURE_3D, Level, 
                          box.Left, box.Top, box.Front,
                          box_width, box_height, box_depth,
                          format, type, volumes_[Level].lock_buffer);
            
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                DX8GL_ERROR("OpenGL error uploading volume level %u: 0x%04x", Level, error);
            }
            
            glBindTexture(GL_TEXTURE_3D, 0);
        }
        
        // Mark as dirty for managed textures
        if (pool_ == D3DPOOL_MANAGED) {
            mark_level_dirty(Level, &box);
        }
    }
    
    volumes_[Level].locked = false;
    volumes_[Level].lock_flags = 0;
    
    DX8GL_TRACE("Unlocked volume level %u", Level);
    return D3D_OK;
}

HRESULT Direct3DVolumeTexture8::AddDirtyBox(const D3DBOX* pDirtyBox) {
    // Track dirty regions for managed textures
    if (pool_ == D3DPOOL_MANAGED) {
        mark_level_dirty(0, pDirtyBox);
    }
    return D3D_OK;
}

// Internal methods
void Direct3DVolumeTexture8::bind(UINT sampler) const {
    if (!gl_texture_) {
        return;
    }
    
    // Upload any dirty regions before binding
    if (has_dirty_regions_ && pool_ == D3DPOOL_MANAGED) {
        const_cast<Direct3DVolumeTexture8*>(this)->upload_dirty_regions();
    }
    
    glActiveTexture(GL_TEXTURE0 + sampler);
    glBindTexture(GL_TEXTURE_3D, gl_texture_);
}

void Direct3DVolumeTexture8::release_gl_resources() {
    DX8GL_DEBUG("Releasing GL resources for volume texture %u (pool=%d)", gl_texture_, pool_);
    
    if (gl_texture_) {
        glDeleteTextures(1, &gl_texture_);
        gl_texture_ = 0;
    }
}

bool Direct3DVolumeTexture8::recreate_gl_resources() {
    DX8GL_DEBUG("Recreating GL resources for volume texture (pool=%d, size=%ux%ux%u, levels=%u)", 
                pool_, width_, height_, depth_, levels_);
    
    // Only D3DPOOL_DEFAULT resources need recreation
    if (pool_ != D3DPOOL_DEFAULT) {
        DX8GL_WARN("Attempted to recreate non-default pool volume texture");
        return true;
    }
    
    // Release old resources first
    release_gl_resources();
    
    // Recreate the volume texture
    return initialize();
}

bool Direct3DVolumeTexture8::get_gl_format(D3DFORMAT format, GLenum& internal_format, 
                                           GLenum& format_out, GLenum& type) {
    return VolumeTextureSupport::get_gl_3d_format(format, internal_format, format_out, type);
}

void Direct3DVolumeTexture8::mark_level_dirty(UINT level, const D3DBOX* dirty_box) {
    if (pool_ != D3DPOOL_MANAGED || level >= levels_) {
        return;
    }
    
    UINT mip_width, mip_height, mip_depth;
    VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, level,
                                            mip_width, mip_height, mip_depth);
    
    D3DBOX box_to_add;
    if (dirty_box) {
        // Clamp the dirty box to level dimensions
        box_to_add.Left = std::max(0u, dirty_box->Left);
        box_to_add.Top = std::max(0u, dirty_box->Top);
        box_to_add.Front = std::max(0u, dirty_box->Front);
        box_to_add.Right = std::min(mip_width, dirty_box->Right);
        box_to_add.Bottom = std::min(mip_height, dirty_box->Bottom);
        box_to_add.Back = std::min(mip_depth, dirty_box->Back);
        
        // Validate the box
        if (box_to_add.Left >= box_to_add.Right || 
            box_to_add.Top >= box_to_add.Bottom ||
            box_to_add.Front >= box_to_add.Back) {
            return;
        }
    } else {
        // Entire level is dirty
        box_to_add.Left = 0;
        box_to_add.Top = 0;
        box_to_add.Front = 0;
        box_to_add.Right = mip_width;
        box_to_add.Bottom = mip_height;
        box_to_add.Back = mip_depth;
        level_fully_dirty_[level] = true;
    }
    
    // If the level is already fully dirty, no need to track individual regions
    if (level_fully_dirty_[level]) {
        has_dirty_regions_ = true;
        return;
    }
    
    // Check if this box makes the entire level dirty
    if (box_to_add.Left == 0 && box_to_add.Top == 0 && box_to_add.Front == 0 &&
        box_to_add.Right == mip_width && box_to_add.Bottom == mip_height && 
        box_to_add.Back == mip_depth) {
        level_fully_dirty_[level] = true;
        // Remove any existing dirty boxes for this level
        dirty_regions_.erase(
            std::remove_if(dirty_regions_.begin(), dirty_regions_.end(),
                          [level](const DirtyBox& db) { return db.level == level; }),
            dirty_regions_.end()
        );
    } else {
        // Merge with existing dirty regions
        merge_dirty_box(level, box_to_add);
    }
    
    has_dirty_regions_ = true;
}

void Direct3DVolumeTexture8::upload_dirty_regions() {
    if (!has_dirty_regions_) {
        return;
    }
    
    glBindTexture(GL_TEXTURE_3D, gl_texture_);
    
    // First, handle fully dirty levels
    for (UINT level = 0; level < levels_; ++level) {
        if (!level_fully_dirty_[level]) {
            continue;
        }
        
        if (!volumes_[level].volume) {
            continue;
        }
        
        Direct3DVolume8* volume = volumes_[level].volume;
        
        // Lock the entire volume
        D3DLOCKED_BOX locked_box;
        HRESULT hr = volume->LockBox(&locked_box, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) {
            DX8GL_ERROR("Failed to lock volume for full level upload");
            continue;
        }
        
        // Get format information
        GLenum internal_format, format, type;
        if (!get_gl_format(format_, internal_format, format, type)) {
            volume->UnlockBox();
            continue;
        }
        
        // Upload the entire level
        UINT mip_width, mip_height, mip_depth;
        VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, level,
                                                mip_width, mip_height, mip_depth);
        
        glTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0,
                       mip_width, mip_height, mip_depth,
                       format, type, locked_box.pBits);
        
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            DX8GL_ERROR("glTexSubImage3D failed for full volume level upload: 0x%04x", error);
        }
        
        volume->UnlockBox();
        
        DX8GL_DEBUG("Uploaded full volume level %u (%ux%ux%u)", level, mip_width, mip_height, mip_depth);
    }
    
    // Process individual dirty regions
    for (const auto& dirty : dirty_regions_) {
        if (!volumes_[dirty.level].volume) {
            continue;
        }
        
        Direct3DVolume8* volume = volumes_[dirty.level].volume;
        
        // Lock the dirty region
        D3DLOCKED_BOX locked_box;
        HRESULT hr = volume->LockBox(&locked_box, &dirty.box, D3DLOCK_READONLY);
        if (FAILED(hr)) {
            DX8GL_ERROR("Failed to lock volume for dirty region upload");
            continue;
        }
        
        // Get format information
        GLenum internal_format, format, type;
        if (!get_gl_format(format_, internal_format, format, type)) {
            volume->UnlockBox();
            continue;
        }
        
        // Calculate region dimensions
        UINT width = dirty.box.Right - dirty.box.Left;
        UINT height = dirty.box.Bottom - dirty.box.Top;
        UINT depth = dirty.box.Back - dirty.box.Front;
        
        // Upload the dirty region
        glTexSubImage3D(GL_TEXTURE_3D, dirty.level,
                       dirty.box.Left, dirty.box.Top, dirty.box.Front,
                       width, height, depth,
                       format, type, locked_box.pBits);
        
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            DX8GL_ERROR("glTexSubImage3D failed for volume dirty region: 0x%04x", error);
        }
        
        volume->UnlockBox();
        
        DX8GL_DEBUG("Uploaded volume dirty region: level=%u, box=(%u,%u,%u,%u,%u,%u)",
                   dirty.level, dirty.box.Left, dirty.box.Top, dirty.box.Front,
                   dirty.box.Right, dirty.box.Bottom, dirty.box.Back);
    }
    
    // Clear dirty regions
    dirty_regions_.clear();
    has_dirty_regions_ = false;
    std::fill(level_fully_dirty_.begin(), level_fully_dirty_.end(), false);
    
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Direct3DVolumeTexture8::merge_dirty_box(UINT level, const D3DBOX& new_box) {
    // For simplicity, just add as a new dirty region
    // A more sophisticated implementation would merge overlapping boxes
    DirtyBox dirty;
    dirty.level = level;
    dirty.box = new_box;
    dirty_regions_.push_back(dirty);
    
    // Optimize if we have too many dirty regions
    if (dirty_regions_.size() > 16) {
        optimize_dirty_regions();
    }
}

void Direct3DVolumeTexture8::optimize_dirty_regions() {
    // For now, just mark entire levels as dirty if there are too many regions
    std::vector<std::vector<D3DBOX>> boxes_by_level(levels_);
    
    for (const auto& dirty : dirty_regions_) {
        if (!level_fully_dirty_[dirty.level]) {
            boxes_by_level[dirty.level].push_back(dirty.box);
        }
    }
    
    dirty_regions_.clear();
    
    for (UINT level = 0; level < levels_; ++level) {
        if (level_fully_dirty_[level]) {
            continue;
        }
        
        auto& boxes = boxes_by_level[level];
        if (boxes.empty()) {
            continue;
        }
        
        // If we have many boxes for this level, just mark it fully dirty
        if (boxes.size() > 4) {
            level_fully_dirty_[level] = true;
        } else {
            // Otherwise, compute the bounding box of all dirty regions
            D3DBOX bounds = boxes[0];
            for (size_t i = 1; i < boxes.size(); ++i) {
                bounds.Left = std::min(bounds.Left, boxes[i].Left);
                bounds.Top = std::min(bounds.Top, boxes[i].Top);
                bounds.Front = std::min(bounds.Front, boxes[i].Front);
                bounds.Right = std::max(bounds.Right, boxes[i].Right);
                bounds.Bottom = std::max(bounds.Bottom, boxes[i].Bottom);
                bounds.Back = std::max(bounds.Back, boxes[i].Back);
            }
            
            DirtyBox dirty;
            dirty.level = level;
            dirty.box = bounds;
            dirty_regions_.push_back(dirty);
        }
    }
}

UINT Direct3DVolumeTexture8::calculate_texture_size(UINT level) {
    UINT mip_width, mip_height, mip_depth;
    VolumeTextureSupport::get_mip_dimensions(width_, height_, depth_, level,
                                            mip_width, mip_height, mip_depth);
    
    UINT bytes_per_pixel = 4;
    switch (format_) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8L8:
            bytes_per_pixel = 2;
            break;
        case D3DFMT_R8G8B8:
            bytes_per_pixel = 3;
            break;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bytes_per_pixel = 4;
            break;
        case D3DFMT_L8:
        case D3DFMT_A8:
            bytes_per_pixel = 1;
            break;
    }
    
    return mip_width * mip_height * mip_depth * bytes_per_pixel;
}

// VolumeTextureSupport implementation
bool VolumeTextureSupport::get_gl_3d_format(D3DFORMAT d3d_format, GLenum& internal_format,
                                           GLenum& format, GLenum& type) {
    switch (d3d_format) {
        case D3DFMT_R8G8B8:
            internal_format = GL_RGB8;
            format = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8R8G8B8:
            internal_format = GL_RGBA8;
            format = GL_BGRA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_X8R8G8B8:
            internal_format = GL_RGB8;
            format = GL_BGRA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_R5G6B5:
            internal_format = GL_RGB;
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            return true;
            
        case D3DFMT_X1R5G5B5:
            internal_format = GL_RGB5;
            format = GL_BGRA;
            type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
            
        case D3DFMT_A1R5G5B5:
            internal_format = GL_RGB5_A1;
            format = GL_BGRA;
            type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
            
        case D3DFMT_A4R4G4B4:
            internal_format = GL_RGBA4;
            format = GL_BGRA;
            type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            return true;
            
        case D3DFMT_L8:
            internal_format = GL_LUMINANCE8;
            format = GL_LUMINANCE;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8:
            internal_format = GL_ALPHA8;
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8L8:
            internal_format = GL_LUMINANCE8_ALPHA8;
            format = GL_LUMINANCE_ALPHA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        default:
            DX8GL_ERROR("Unsupported 3D texture format: 0x%08x", d3d_format);
            return false;
    }
}

void VolumeTextureSupport::get_mip_dimensions(UINT base_width, UINT base_height, UINT base_depth,
                                             UINT level, UINT& out_width, UINT& out_height, UINT& out_depth) {
    out_width = std::max(1u, base_width >> level);
    out_height = std::max(1u, base_height >> level);
    out_depth = std::max(1u, base_depth >> level);
}

std::string VolumeTextureSupport::generate_glsl_3d_declarations(int texture_unit) {
    std::stringstream ss;
    ss << "uniform sampler3D u_volume_texture" << texture_unit << ";\n";
    return ss.str();
}

std::string VolumeTextureSupport::generate_glsl_3d_sampling(const std::string& sampler_name,
                                                           const std::string& coord_expr) {
    return "texture(" + sampler_name + ", " + coord_expr + ")";
}

std::string VolumeTextureSupport::generate_wgsl_3d_declarations(int texture_unit) {
    std::stringstream ss;
    ss << "@group(1) @binding(" << (texture_unit * 2) << ")\n";
    ss << "var volume_texture" << texture_unit << ": texture_3d<f32>;\n";
    ss << "@group(1) @binding(" << (texture_unit * 2 + 1) << ")\n";
    ss << "var volume_sampler" << texture_unit << ": sampler;\n";
    return ss.str();
}

std::string VolumeTextureSupport::generate_wgsl_3d_sampling(const std::string& sampler_name,
                                                           const std::string& coord_expr) {
    return "textureSample(" + sampler_name + "_texture, " + sampler_name + "_sampler, " + coord_expr + ")";
}

std::string VolumeTextureSupport::generate_volumetric_fog_glsl(const std::string& density_texture,
                                                              const std::string& ray_start,
                                                              const std::string& ray_end,
                                                              int num_samples) {
    std::stringstream ss;
    ss << "// Volumetric fog ray marching\n";
    ss << "vec3 ray_dir = " << ray_end << " - " << ray_start << ";\n";
    ss << "float ray_length = length(ray_dir);\n";
    ss << "ray_dir = normalize(ray_dir);\n";
    ss << "float step_size = ray_length / float(" << num_samples << ");\n";
    ss << "vec3 current_pos = " << ray_start << ";\n";
    ss << "float accumulated_fog = 0.0;\n";
    ss << "for (int i = 0; i < " << num_samples << "; i++) {\n";
    ss << "    float density = texture(" << density_texture << ", current_pos).r;\n";
    ss << "    accumulated_fog += density * step_size;\n";
    ss << "    current_pos += ray_dir * step_size;\n";
    ss << "}\n";
    ss << "float fog_factor = 1.0 - exp(-accumulated_fog);\n";
    return ss.str();
}

std::string VolumeTextureSupport::generate_volumetric_fog_wgsl(const std::string& density_texture,
                                                              const std::string& ray_start,
                                                              const std::string& ray_end,
                                                              int num_samples) {
    std::stringstream ss;
    ss << "// Volumetric fog ray marching\n";
    ss << "let ray_dir = " << ray_end << " - " << ray_start << ";\n";
    ss << "let ray_length = length(ray_dir);\n";
    ss << "let ray_dir_norm = normalize(ray_dir);\n";
    ss << "let step_size = ray_length / f32(" << num_samples << ");\n";
    ss << "var current_pos = " << ray_start << ";\n";
    ss << "var accumulated_fog = 0.0;\n";
    ss << "for (var i = 0; i < " << num_samples << "; i++) {\n";
    ss << "    let density = textureSample(" << density_texture << "_texture, " 
       << density_texture << "_sampler, current_pos).r;\n";
    ss << "    accumulated_fog += density * step_size;\n";
    ss << "    current_pos += ray_dir_norm * step_size;\n";
    ss << "}\n";
    ss << "let fog_factor = 1.0 - exp(-accumulated_fog);\n";
    return ss.str();
}

#ifdef DX8GL_HAS_WEBGPU
WGpuTexture VolumeTextureSupport::create_webgpu_3d_texture(WGpuDevice device,
                                                          uint32_t width,
                                                          uint32_t height,
                                                          uint32_t depth,
                                                          uint32_t mip_levels,
                                                          WGpuTextureFormat format) {
    WGpuTextureDescriptor desc = {};
    desc.label = "Volume Texture";
    desc.size.width = width;
    desc.size.height = height;
    desc.size.depthOrArrayLayers = depth;
    desc.mipLevelCount = mip_levels;
    desc.sampleCount = 1;
    desc.dimension = WGPU_TEXTURE_DIMENSION_3D;
    desc.format = format;
    desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
    desc.viewFormatCount = 0;
    desc.viewFormats = nullptr;
    
    return wgpu_device_create_texture(device, &desc);
}

WGpuTextureView VolumeTextureSupport::create_3d_texture_view(WGpuTexture texture) {
    WGpuTextureViewDescriptor desc = {};
    desc.label = "Volume Texture View";
    desc.format = WGPU_TEXTURE_FORMAT_UNDEFINED;  // Use texture's format
    desc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_3D;
    desc.baseMipLevel = 0;
    desc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
    desc.baseArrayLayer = 0;
    desc.arrayLayerCount = 1;
    desc.aspect = WGPU_TEXTURE_ASPECT_ALL;
    
    return wgpu_texture_create_view(texture, &desc);
}
#endif

} // namespace dx8gl