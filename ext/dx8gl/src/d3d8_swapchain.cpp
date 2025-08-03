#include "d3d8_swapchain.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include "logger.h"

namespace dx8gl {

Direct3DSwapChain8::Direct3DSwapChain8(Direct3DDevice8* device, D3DPRESENT_PARAMETERS* params)
    : ref_count_(1)
    , device_(device) {
    
    // Copy presentation parameters
    present_params_ = *params;
    
    // Add reference to device
    device_->AddRef();
    
    DX8GL_DEBUG("Direct3DSwapChain8 created");
}

Direct3DSwapChain8::~Direct3DSwapChain8() {
    DX8GL_DEBUG("Direct3DSwapChain8 destructor");
    
    // Release back buffers
    for (auto& buffer : back_buffers_) {
        if (buffer) {
            buffer->Release();
        }
    }
    back_buffers_.clear();
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool Direct3DSwapChain8::initialize() {
    // Create back buffers
    UINT width = present_params_.BackBufferWidth;
    UINT height = present_params_.BackBufferHeight;
    
    for (UINT i = 0; i < present_params_.BackBufferCount; ++i) {
        auto surface = new Direct3DSurface8(device_, width, height,
                                          present_params_.BackBufferFormat,
                                          D3DUSAGE_RENDERTARGET);
        if (!surface->initialize()) {
            surface->Release();
            return false;
        }
        back_buffers_.push_back(surface);
    }
    
    return true;
}

// IUnknown methods
HRESULT Direct3DSwapChain8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DSwapChain8)) {
        *ppvObj = static_cast<IDirect3DSwapChain8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DSwapChain8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DSwapChain8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DSwapChain8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DSwapChain8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DSwapChain8 methods
HRESULT Direct3DSwapChain8::Present(const RECT* pSourceRect, const RECT* pDestRect,
                                   HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    DX8GL_TRACE("SwapChain Present");
    
    // TODO: Implement swap chain presentation
    // For now, just increment current buffer
    current_buffer_ = (current_buffer_ + 1) % back_buffers_.size();
    
    return D3D_OK;
}

HRESULT Direct3DSwapChain8::GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                         IDirect3DSurface8** ppBackBuffer) {
    if (!ppBackBuffer) {
        return D3DERR_INVALIDCALL;
    }
    
    if (Type != D3DBACKBUFFER_TYPE_MONO || BackBuffer >= back_buffers_.size()) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppBackBuffer = back_buffers_[BackBuffer];
    if (*ppBackBuffer) {
        (*ppBackBuffer)->AddRef();
        return D3D_OK;
    }
    
    return D3DERR_INVALIDCALL;
}

} // namespace dx8gl