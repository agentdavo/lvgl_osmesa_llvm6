#include "d3d8_swapchain.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include "logger.h"
#include "gl3_headers.h"

namespace dx8gl {

Direct3DSwapChain8::Direct3DSwapChain8(Direct3DDevice8* device, D3DPRESENT_PARAMETERS* params)
    : ref_count_(1)
    , device_(device)
    , current_buffer_(0) {
    
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
    DX8GL_INFO("SwapChain Present: current_buffer=%u, buffer_count=%zu", 
               current_buffer_, back_buffers_.size());
    
    if (back_buffers_.empty()) {
        DX8GL_ERROR("No back buffers available for presentation");
        return D3DERR_INVALIDCALL;
    }
    
    // Get the current back buffer that we're presenting
    Direct3DSurface8* current_back_buffer = back_buffers_[current_buffer_];
    if (!current_back_buffer) {
        DX8GL_ERROR("Current back buffer is null");
        return D3DERR_INVALIDCALL;
    }
    
    // Ensure OpenGL commands are finished before presentation
    glFinish();
    
    // For software rendering (OSMesa), we need to synchronize the current render target
    // with the back buffer being presented
    IDirect3DSurface8* current_render_target = nullptr;
    HRESULT hr = device_->GetRenderTarget(&current_render_target);
    if (SUCCEEDED(hr) && current_render_target) {
        // If the current render target is not the back buffer we're presenting,
        // we need to copy it to ensure the presentation shows the latest rendering
        if (current_render_target != current_back_buffer) {
            DX8GL_DEBUG("Render target differs from current back buffer, copying content");
            
            // Copy from render target to back buffer
            RECT full_rect = { 0, 0, 
                              static_cast<LONG>(present_params_.BackBufferWidth),
                              static_cast<LONG>(present_params_.BackBufferHeight) };
            
            POINT dest_point = { 0, 0 };
            Direct3DSurface8* rt_surface = static_cast<Direct3DSurface8*>(current_render_target);
            current_back_buffer->copy_from(rt_surface, &full_rect, &dest_point);
        }
        current_render_target->Release();
    }
    
    // Call the device's presentation logic to handle the actual buffer flip/copy
    hr = device_->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    if (FAILED(hr)) {
        DX8GL_ERROR("Device Present failed: 0x%08x", hr);
        return hr;
    }
    
    // Flip to the next back buffer for multi-buffering
    if (back_buffers_.size() > 1) {
        UINT next_buffer = (current_buffer_ + 1) % back_buffers_.size();
        DX8GL_DEBUG("Flipping from buffer %u to buffer %u", current_buffer_, next_buffer);
        current_buffer_ = next_buffer;
        
        // Update the device's render target to the new back buffer
        // This ensures subsequent rendering goes to the correct buffer
        hr = device_->SetRenderTarget(back_buffers_[current_buffer_], nullptr);
        if (FAILED(hr)) {
            DX8GL_WARNING("Failed to set render target to new back buffer: 0x%08x", hr);
            // Don't fail the Present call for this, just continue
        }
    }
    
    DX8GL_DEBUG("SwapChain Present completed successfully");
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