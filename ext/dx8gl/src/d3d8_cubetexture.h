#ifndef DX8GL_D3D8_CUBETEXTURE_H
#define DX8GL_D3D8_CUBETEXTURE_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <vector>
#include "logger.h"
#include "private_data.h"

// IDirect3DCubeTexture8 interface definition - must be in global namespace
struct IDirect3DCubeTexture8 : public IDirect3DBaseTexture8 {
    // IDirect3DCubeTexture8 methods
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT* pDirtyRect) = 0;
};

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DSurface8;

class Direct3DCubeTexture8 : public IDirect3DCubeTexture8 {
public:
    Direct3DCubeTexture8(Direct3DDevice8* device, UINT edge_length, UINT levels,
                         DWORD usage, D3DFORMAT format, D3DPOOL pool);
    virtual ~Direct3DCubeTexture8();

    // Initialize the cube texture
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

    // IDirect3DCubeTexture8 methods
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect,
                                       const RECT* pRect, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT* pDirtyRect) override;

    // Internal methods
    GLuint get_gl_texture() const { return gl_texture_; }
    void bind(UINT sampler) const;
    D3DPOOL get_pool() const { return pool_; }
    void mark_face_level_dirty(D3DCUBEMAP_FACES face, UINT level, const RECT* dirty_rect = nullptr);
    void commit_dirty_regions() { upload_dirty_regions(); }
    
    // Device reset support
    void release_gl_resources();
    bool recreate_gl_resources();

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    
    // Texture properties
    UINT edge_length_;
    UINT levels_;
    DWORD usage_;
    D3DFORMAT format_;
    D3DPOOL pool_;
    DWORD priority_;
    DWORD lod_;
    
    // OpenGL resources
    GLuint gl_texture_;
    
    // Surface management
    struct CubeFaceInfo {
        std::vector<Direct3DSurface8*> surfaces;  // One per mip level
        std::vector<bool> locked;
        std::vector<void*> lock_buffers;
        std::vector<DWORD> lock_flags;  // Track lock flags for each level
    };
    CubeFaceInfo faces_[6];  // One for each cube face
    
    // Private data storage
    PrivateDataManager private_data_manager_;
    
    // Dirty region tracking for managed textures
    struct DirtyRect {
        RECT rect;
        D3DCUBEMAP_FACES face;
        UINT level;
    };
    std::vector<DirtyRect> dirty_regions_;
    bool has_dirty_regions_;
    
    // Optimized dirty tracking - one per face per level
    std::vector<std::vector<bool>> face_level_fully_dirty_;  // [face][level]
    
    // Helper methods
    static GLenum get_cube_face_target(D3DCUBEMAP_FACES face);
    bool get_gl_format(D3DFORMAT format, GLenum& internal_format, GLenum& format_out, GLenum& type);
    void upload_dirty_regions();
    void merge_dirty_rect(D3DCUBEMAP_FACES face, UINT level, const RECT& new_rect);
    void optimize_dirty_regions();
};

} // namespace dx8gl

#endif // DX8GL_D3D8_CUBETEXTURE_H