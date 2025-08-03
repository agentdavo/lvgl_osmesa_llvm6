#ifndef DX8GL_D3D8_SWAPCHAIN_H
#define DX8GL_D3D8_SWAPCHAIN_H

#include "d3d8.h"
#include <vector>
#include <atomic>
#include "logger.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DSurface8;

class Direct3DSwapChain8 : public IDirect3DSwapChain8 {
public:
    Direct3DSwapChain8(Direct3DDevice8* device, D3DPRESENT_PARAMETERS* params);
    virtual ~Direct3DSwapChain8();

    // Initialize swap chain
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

private:
    std::atomic<LONG> ref_count_;
    Direct3DDevice8* device_;
    D3DPRESENT_PARAMETERS present_params_;
    
    // Back buffers
    std::vector<Direct3DSurface8*> back_buffers_;
    UINT current_buffer_;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_SWAPCHAIN_H