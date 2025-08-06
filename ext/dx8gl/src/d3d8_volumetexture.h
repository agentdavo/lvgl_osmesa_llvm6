#ifndef DX8GL_D3D8_VOLUMETEXTURE_H
#define DX8GL_D3D8_VOLUMETEXTURE_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <vector>
#include "logger.h"
#include "private_data.h"

// IDirect3DVolumeTexture8 interface definition - must be in global namespace
struct IDirect3DVolumeTexture8 : public IDirect3DBaseTexture8 {
    // IDirect3DVolumeTexture8 methods
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT Level, IDirect3DVolume8** ppVolumeLevel) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockBox(UINT Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX* pDirtyBox) = 0;
};

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DVolume8;

class Direct3DVolumeTexture8 : public IDirect3DVolumeTexture8 {
public:
    Direct3DVolumeTexture8(Direct3DDevice8* device, UINT width, UINT height, UINT depth,
                          UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool);
    virtual ~Direct3DVolumeTexture8();

    // Initialize the volume texture
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

    // IDirect3DVolumeTexture8 methods
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT Level, IDirect3DVolume8** ppVolumeLevel) override;
    HRESULT STDMETHODCALLTYPE LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume,
                                      const D3DBOX* pBox, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockBox(UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX* pDirtyBox) override;

    // Internal methods
    GLuint get_gl_texture() const { return gl_texture_; }
    void bind(UINT sampler) const;
    D3DPOOL get_pool() const { return pool_; }
    void mark_level_dirty(UINT level, const D3DBOX* dirty_box = nullptr);
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
    UINT depth_;
    UINT levels_;
    DWORD usage_;
    D3DFORMAT format_;
    D3DPOOL pool_;
    DWORD priority_;
    DWORD lod_;
    
    // OpenGL resources
    GLuint gl_texture_;
    
    // Volume management
    struct VolumeInfo {
        Direct3DVolume8* volume;
        bool locked;
        void* lock_buffer;
        DWORD lock_flags;
        D3DBOX lock_box;  // Track locked region
    };
    std::vector<VolumeInfo> volumes_;  // One per mip level
    
    // Private data storage
    PrivateDataManager private_data_manager_;
    
    // Dirty region tracking for managed textures
    struct DirtyBox {
        D3DBOX box;
        UINT level;
    };
    std::vector<DirtyBox> dirty_regions_;
    bool has_dirty_regions_;
    
    // Optimized dirty tracking - one per level
    std::vector<bool> level_fully_dirty_;
    
    // Helper methods
    bool get_gl_format(D3DFORMAT format, GLenum& internal_format, GLenum& format_out, GLenum& type);
    void upload_dirty_regions();
    void merge_dirty_box(UINT level, const D3DBOX& new_box);
    void optimize_dirty_regions();
    UINT calculate_texture_size(UINT level);
};

// Helper class for 3D texture support across backends
class VolumeTextureSupport {
public:
    // OpenGL 3D texture format mapping
    static bool get_gl_3d_format(D3DFORMAT d3d_format, GLenum& internal_format, 
                                 GLenum& format, GLenum& type);
    
    // Calculate mipmap dimensions
    static void get_mip_dimensions(UINT base_width, UINT base_height, UINT base_depth,
                                  UINT level, UINT& out_width, UINT& out_height, UINT& out_depth);
    
    // Generate GLSL declarations for 3D textures
    static std::string generate_glsl_3d_declarations(int texture_unit);
    static std::string generate_glsl_3d_sampling(const std::string& sampler_name,
                                                 const std::string& coord_expr);
    
    // Generate WGSL declarations for 3D textures
    static std::string generate_wgsl_3d_declarations(int texture_unit);
    static std::string generate_wgsl_3d_sampling(const std::string& sampler_name,
                                                 const std::string& coord_expr);
    
    // Volumetric effects helpers
    static std::string generate_volumetric_fog_glsl(const std::string& density_texture,
                                                    const std::string& ray_start,
                                                    const std::string& ray_end,
                                                    int num_samples);
    static std::string generate_volumetric_fog_wgsl(const std::string& density_texture,
                                                    const std::string& ray_start,
                                                    const std::string& ray_end,
                                                    int num_samples);
    
#ifdef DX8GL_HAS_WEBGPU
    // WebGPU 3D texture creation
    static WGpuTexture create_webgpu_3d_texture(WGpuDevice device,
                                                uint32_t width,
                                                uint32_t height,
                                                uint32_t depth,
                                                uint32_t mip_levels,
                                                WGpuTextureFormat format);
    
    static WGpuTextureView create_3d_texture_view(WGpuTexture texture);
#endif
};

} // namespace dx8gl

#endif // DX8GL_D3D8_VOLUMETEXTURE_H