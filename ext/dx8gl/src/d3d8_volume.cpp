#include "d3d8_volume.h"
#include "d3d8_device.h"
#include "d3d8_volumetexture.h"
#include <cstring>
#include <algorithm>

namespace dx8gl {

Direct3DVolume8::Direct3DVolume8(Direct3DDevice8* device, UINT width, UINT height, UINT depth,
                               D3DFORMAT format, DWORD usage, D3DPOOL pool,
                               Direct3DVolumeTexture8* parent_texture)
    : ref_count_(1)
    , device_(device)
    , parent_texture_(parent_texture)
    , width_(width)
    , height_(height)
    , depth_(depth)
    , format_(format)
    , usage_(usage)
    , pool_(pool)
    , data_(nullptr)
    , data_size_(0)
    , locked_(false)
    , lock_flags_(0) {
    
    // Calculate data size
    data_size_ = calculate_slice_pitch() * depth_;
    
    // Add reference to device
    device_->AddRef();
    
    // Add reference to parent texture if exists
    if (parent_texture_) {
        parent_texture_->AddRef();
    }
    
    DX8GL_DEBUG("Direct3DVolume8 created: %ux%ux%u, format=0x%08x, pool=%d",
                width, height, depth, format, pool);
}

Direct3DVolume8::~Direct3DVolume8() {
    DX8GL_DEBUG("Direct3DVolume8 destructor");
    
    // Free data if allocated
    if (data_) {
        free(data_);
        data_ = nullptr;
    }
    
    // Release parent texture reference
    if (parent_texture_) {
        parent_texture_->Release();
    }
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool Direct3DVolume8::initialize() {
    // Allocate memory for volume data
    if (data_size_ > 0) {
        data_ = malloc(data_size_);
        if (!data_) {
            DX8GL_ERROR("Failed to allocate %u bytes for volume data", data_size_);
            return false;
        }
        memset(data_, 0, data_size_);
    }
    
    return true;
}

// IUnknown methods
HRESULT Direct3DVolume8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(riid, IID_IUnknown) || 
        IsEqualGUID(riid, IID_IDirect3DVolume8)) {
        *ppvObj = static_cast<IDirect3DVolume8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DVolume8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DVolume8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DVolume8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DVolume8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DVolume8 methods
HRESULT Direct3DVolume8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppDevice = device_;
    device_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DVolume8::SetPrivateData(REFGUID refguid, const void* pData,
                                        DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DVolume8::GetPrivateData(REFGUID refguid, void* pData,
                                        DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DVolume8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

HRESULT Direct3DVolume8::GetContainer(REFIID riid, void** ppContainer) {
    if (!ppContainer) {
        return D3DERR_INVALIDCALL;
    }
    
    if (!parent_texture_) {
        *ppContainer = nullptr;
        return E_NOINTERFACE;
    }
    
    return parent_texture_->QueryInterface(riid, ppContainer);
}

HRESULT Direct3DVolume8::GetDesc(D3DVOLUME_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    pDesc->Format = format_;
    pDesc->Type = D3DRTYPE_VOLUME;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = data_size_;
    pDesc->Width = width_;
    pDesc->Height = height_;
    pDesc->Depth = depth_;
    
    return D3D_OK;
}

HRESULT Direct3DVolume8::LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) {
    if (!pLockedVolume) {
        return D3DERR_INVALIDCALL;
    }
    
    if (locked_) {
        DX8GL_ERROR("Volume already locked");
        return D3DERR_INVALIDCALL;
    }
    
    // Determine lock region
    if (pBox) {
        lock_box_ = *pBox;
        // Clamp to volume dimensions
        lock_box_.Right = std::min(lock_box_.Right, width_);
        lock_box_.Bottom = std::min(lock_box_.Bottom, height_);
        lock_box_.Back = std::min(lock_box_.Back, depth_);
    } else {
        lock_box_.Left = 0;
        lock_box_.Top = 0;
        lock_box_.Front = 0;
        lock_box_.Right = width_;
        lock_box_.Bottom = height_;
        lock_box_.Back = depth_;
    }
    
    // Calculate pitches
    UINT bytes_per_pixel = get_bytes_per_pixel();
    pLockedVolume->RowPitch = (lock_box_.Right - lock_box_.Left) * bytes_per_pixel;
    pLockedVolume->SlicePitch = pLockedVolume->RowPitch * (lock_box_.Bottom - lock_box_.Top);
    
    // Calculate pointer to locked region
    if (data_) {
        UINT row_pitch = calculate_row_pitch();
        UINT slice_pitch = calculate_slice_pitch();
        
        uint8_t* base = static_cast<uint8_t*>(data_);
        uint8_t* ptr = base + 
                      (lock_box_.Front * slice_pitch) +
                      (lock_box_.Top * row_pitch) +
                      (lock_box_.Left * bytes_per_pixel);
        pLockedVolume->pBits = ptr;
    } else {
        pLockedVolume->pBits = nullptr;
    }
    
    locked_ = true;
    lock_flags_ = Flags;
    
    DX8GL_TRACE("Locked volume with flags 0x%08x", Flags);
    return D3D_OK;
}

HRESULT Direct3DVolume8::UnlockBox() {
    if (!locked_) {
        DX8GL_ERROR("Volume not locked");
        return D3DERR_INVALIDCALL;
    }
    
    locked_ = false;
    lock_flags_ = 0;
    
    DX8GL_TRACE("Unlocked volume");
    return D3D_OK;
}

// Helper methods
UINT Direct3DVolume8::calculate_row_pitch() const {
    return width_ * get_bytes_per_pixel();
}

UINT Direct3DVolume8::calculate_slice_pitch() const {
    return calculate_row_pitch() * height_;
}

UINT Direct3DVolume8::get_bytes_per_pixel() const {
    switch (format_) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8L8:
            return 2;
        case D3DFMT_R8G8B8:
            return 3;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            return 4;
        case D3DFMT_L8:
        case D3DFMT_A8:
            return 1;
        default:
            return 4;  // Default to 4 bytes
    }
}

} // namespace dx8gl