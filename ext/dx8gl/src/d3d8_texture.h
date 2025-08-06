#ifndef DX8GL_D3D8_TEXTURE_H
#define DX8GL_D3D8_TEXTURE_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <vector>
#include "logger.h"
#include "private_data.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DSurface8;

class Direct3DTexture8 : public IDirect3DTexture8 {
public:
    Direct3DTexture8(Direct3DDevice8* device, UINT width, UINT height, UINT levels,
                     DWORD usage, D3DFORMAT format, D3DPOOL pool);
    virtual ~Direct3DTexture8();

    // Initialize the texture
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

    // IDirect3DBaseTexture8 methods
    DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override;
    DWORD STDMETHODCALLTYPE GetLOD() override;
    DWORD STDMETHODCALLTYPE GetLevelCount() override;

    // IDirect3DTexture8 methods
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface8** ppSurfaceLevel) override;
    HRESULT STDMETHODCALLTYPE LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect,
                                       const RECT* pRect, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* pDirtyRect) override;

    // Internal methods
    GLuint get_gl_texture() const { return gl_texture_; }
    void bind(UINT sampler) const;
    D3DPOOL get_pool() const { return pool_; }
    void mark_level_dirty(UINT level, const RECT* dirty_rect = nullptr);
    void commit_dirty_regions() { upload_dirty_regions(); }
    
    // Device reset support
    void release_gl_resources();
    bool recreate_gl_resources();

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    
    // Texture properties
    UINT width_;
    UINT height_;
    UINT levels_;
    DWORD usage_;
    D3DFORMAT format_;
    D3DPOOL pool_;
    DWORD priority_;
    DWORD lod_;
    
    // OpenGL resources
    GLuint gl_texture_;
    
    // Surface levels
    std::vector<Direct3DSurface8*> surfaces_;
    
    // Helper methods
    UINT calculate_mip_levels(UINT width, UINT height) const;
    static bool get_gl_format(D3DFORMAT d3d_format, GLenum& internal_format,
                             GLenum& format, GLenum& type);
    void apply_lod_settings();
    void upload_dirty_regions();
    
    // Private data storage
    PrivateDataManager private_data_manager_;
    
    // Dirty region tracking for managed textures
    struct DirtyRect {
        RECT rect;
        UINT level;
    };
    std::vector<DirtyRect> dirty_regions_;
    bool has_dirty_regions_;
    
    // Optimized dirty region tracking
    std::vector<bool> level_fully_dirty_;  // Track if entire level is dirty
    std::vector<RECT> level_dirty_bounds_; // Bounding box of all dirty regions per level
    
    void optimize_dirty_regions();
    void merge_dirty_rect(UINT level, const RECT& new_rect);
};

} // namespace dx8gl

#endif // DX8GL_D3D8_TEXTURE_H