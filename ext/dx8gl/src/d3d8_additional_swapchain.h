#ifndef DX8GL_D3D8_ADDITIONAL_SWAPCHAIN_H
#define DX8GL_D3D8_ADDITIONAL_SWAPCHAIN_H

#include "d3d8.h"
#include "offscreen_framebuffer.h"
#include <vector>
#include <atomic>
#include <memory>
#include "logger.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DSurface8;

/**
 * Additional swap chain implementation using OffscreenFramebuffer
 * 
 * This class provides multi-window rendering support by managing
 * additional swap chains with their own framebuffers. Each swap chain
 * can present to a different window while sharing the same device context.
 */
class AdditionalSwapChain : public IDirect3DSwapChain8 {
public:
    AdditionalSwapChain(Direct3DDevice8* device, D3DPRESENT_PARAMETERS* params);
    virtual ~AdditionalSwapChain();

    // Initialize swap chain with framebuffers
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3DSwapChain8 methods
    HRESULT STDMETHODCALLTYPE Present(const RECT* pSourceRect, const RECT* pDestRect,
                                     HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) override;
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                           IDirect3DSurface8** ppBackBuffer) override;

    // Internal methods for device management
    HRESULT reset(D3DPRESENT_PARAMETERS* params);
    void release_resources();
    
    // Get the current framebuffer for rendering
    OffscreenFramebuffer* get_current_framebuffer();
    
    // Swap buffers (for multiple back buffers)
    void swap_buffers();
    
    // Get presentation parameters
    const D3DPRESENT_PARAMETERS& get_present_params() const { return present_params_; }
    
    // Get window handle
    HWND get_window() const { return present_params_.hDeviceWindow; }

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    D3DPRESENT_PARAMETERS present_params_;
    
    // Offscreen framebuffers for back buffers
    std::vector<std::unique_ptr<OffscreenFramebuffer>> framebuffers_;
    
    // Surface wrappers for DirectX compatibility
    std::vector<Direct3DSurface8*> back_buffer_surfaces_;
    
    // Current back buffer index (for multiple buffering)
    UINT current_buffer_;
    
    // Helper methods
    bool create_framebuffers();
    void destroy_framebuffers();
    PixelFormat d3d_format_to_pixel_format(D3DFORMAT format);
    
    // Platform-specific presentation
    bool present_to_window(const OffscreenFramebuffer* framebuffer, HWND window,
                          const RECT* src_rect, const RECT* dst_rect);
    
#ifdef _WIN32
    // Windows-specific presentation using GDI/OpenGL
    bool present_win32(const OffscreenFramebuffer* framebuffer, HWND window,
                       const RECT* src_rect, const RECT* dst_rect);
#else
    // Linux/X11 presentation
    bool present_x11(const OffscreenFramebuffer* framebuffer, HWND window,
                    const RECT* src_rect, const RECT* dst_rect);
#endif
};

} // namespace dx8gl

#endif // DX8GL_D3D8_ADDITIONAL_SWAPCHAIN_H