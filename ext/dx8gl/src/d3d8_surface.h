#ifndef DX8GL_D3D8_SURFACE_H
#define DX8GL_D3D8_SURFACE_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <memory>
#include <atomic>
#include <mutex>
#include "logger.h"
#include "private_data.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DTexture8;

class Direct3DSurface8 : public IDirect3DSurface8 {
public:
    // Constructor for standalone surface
    Direct3DSurface8(Direct3DDevice8* device, UINT width, UINT height,
                     D3DFORMAT format, DWORD usage);
    
    // Constructor for texture surface
    Direct3DSurface8(Direct3DTexture8* texture, UINT level, UINT width, UINT height,
                     D3DFORMAT format, DWORD usage);
    
    virtual ~Direct3DSurface8();

    // Initialize the surface
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3DSurface8 methods
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData,
                                             DWORD SizeOfData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData,
                                             DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override;
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect,
                                       DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect() override;

    // Internal methods
    GLuint get_texture() const { return texture_; }
    GLuint get_gl_texture() const { return texture_; }  // Alias for compatibility
    GLuint get_renderbuffer() const { return renderbuffer_; }
    GLuint get_fbo() const { return framebuffer_; }
    UINT get_width() const { return width_; }
    UINT get_height() const { return height_; }
    D3DFORMAT get_format() const { return format_; }
    
    // Check if this is a render target
    bool is_render_target() const { return (usage_ & D3DUSAGE_RENDERTARGET) != 0; }
    bool is_depth_stencil() const { return (usage_ & D3DUSAGE_DEPTHSTENCIL) != 0; }
    
    // Copy surface data
    bool copy_from(Direct3DSurface8* source, const RECT* src_rect, const POINT* dest_point);

    // Convert D3D format to GL format
    static bool get_gl_format(D3DFORMAT d3d_format, GLenum& internal_format,
                             GLenum& format, GLenum& type);
    
    // Convert between pixel formats
    static bool convert_format(const void* src, void* dst, D3DFORMAT src_format,
                              D3DFORMAT dst_format, UINT pixel_count);
    
    // Calculate bytes per pixel
    static UINT get_format_size(D3DFORMAT format);

private:

    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    Direct3DTexture8* parent_texture_;
    
    // Surface properties
    UINT width_;
    UINT height_;
    D3DFORMAT format_;
    DWORD usage_;
    UINT level_;
    
    // OpenGL resources
    GLuint texture_;         // For color surfaces
    GLuint renderbuffer_;    // For depth/stencil surfaces
    GLuint framebuffer_;     // For render targets
    
    // Lock state
    mutable std::mutex lock_mutex_;
    bool locked_;
    void* lock_buffer_;
    RECT lock_rect_;
    DWORD lock_flags_;
    UINT pitch_;
    
    // Private data storage
    PrivateDataManager private_data_manager_;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_SURFACE_H