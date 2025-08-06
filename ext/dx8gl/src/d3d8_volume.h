#ifndef DX8GL_D3D8_VOLUME_H
#define DX8GL_D3D8_VOLUME_H

#include "d3d8.h"
#include <atomic>
#include "logger.h"
#include "private_data.h"

// IDirect3DVolume8 interface definition
struct IDirect3DVolume8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockBox() = 0;
};

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DVolumeTexture8;

class Direct3DVolume8 : public IDirect3DVolume8 {
public:
    Direct3DVolume8(Direct3DDevice8* device, UINT width, UINT height, UINT depth,
                   D3DFORMAT format, DWORD usage, D3DPOOL pool,
                   Direct3DVolumeTexture8* parent_texture = nullptr);
    virtual ~Direct3DVolume8();

    // Initialize the volume
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3DVolume8 methods
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData,
                                             DWORD SizeOfData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData,
                                             DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override;
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockBox() override;

    // Internal methods
    void* get_data() const { return data_; }
    UINT get_width() const { return width_; }
    UINT get_height() const { return height_; }
    UINT get_depth() const { return depth_; }
    D3DFORMAT get_format() const { return format_; }

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    Direct3DVolumeTexture8* parent_texture_;
    
    // Volume properties
    UINT width_;
    UINT height_;
    UINT depth_;
    D3DFORMAT format_;
    DWORD usage_;
    D3DPOOL pool_;
    
    // Volume data
    void* data_;
    UINT data_size_;
    bool locked_;
    D3DBOX lock_box_;
    DWORD lock_flags_;
    
    // Private data storage
    PrivateDataManager private_data_manager_;
    
    // Helper methods
    UINT calculate_row_pitch() const;
    UINT calculate_slice_pitch() const;
    UINT get_bytes_per_pixel() const;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_VOLUME_H