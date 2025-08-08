/**
 * Additional COM wrapper implementations for dx8gl
 * 
 * This file contains:
 * 1. Missing IDirect3DDevice8 method implementations
 * 2. Surface wrapping/unwrapping utilities
 * 3. Proper COM reference counting helpers
 */

#include "d3d8_com_wrapper.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include "logger.h"
#include <unordered_map>
#include <mutex>

namespace dx8gl {

// ============================================================================
// Surface Wrapping Utilities
// ============================================================================

// Global map to track COM wrapper <-> C++ object relationships
static std::unordered_map<void*, void*> g_wrapper_to_cpp_map;
static std::unordered_map<void*, void*> g_cpp_to_wrapper_map;
static std::mutex g_wrapper_mutex;

// COM wrapper for IDirect3DSurface8
struct Direct3DSurface8_COM_Wrapper {
    IDirect3DSurface8Vtbl* lpVtbl;
    ULONG ref_count;
    ::IDirect3DSurface8* pCppInterface;  // Points to the C++ implementation
};

// Helper to wrap a C++ surface in a COM wrapper
IDirect3DSurface8* WrapSurface(::IDirect3DSurface8* cpp_surface) {
    if (!cpp_surface) return nullptr;
    
    std::lock_guard<std::mutex> lock(g_wrapper_mutex);
    
    // Check if already wrapped
    auto it = g_cpp_to_wrapper_map.find(cpp_surface);
    if (it != g_cpp_to_wrapper_map.end()) {
        // Already wrapped, increase ref count and return
        IDirect3DSurface8* wrapper = static_cast<IDirect3DSurface8*>(it->second);
        wrapper->lpVtbl->AddRef(wrapper);
        return wrapper;
    }
    
    // Create new wrapper
    Direct3DSurface8_COM_Wrapper* wrapper = new Direct3DSurface8_COM_Wrapper;
    wrapper->lpVtbl = &g_Direct3DSurface8_Vtbl;
    wrapper->ref_count = 1;
    wrapper->pCppInterface = cpp_surface;
    
    // Add to tracking maps
    g_wrapper_to_cpp_map[wrapper] = cpp_surface;
    g_cpp_to_wrapper_map[cpp_surface] = wrapper;
    
    // AddRef the C++ object
    cpp_surface->AddRef();
    
    return reinterpret_cast<IDirect3DSurface8*>(wrapper);
}

// Helper to unwrap a COM surface to get the C++ implementation
::IDirect3DSurface8* UnwrapSurface(IDirect3DSurface8* com_surface) {
    if (!com_surface) return nullptr;
    
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(com_surface);
    return wrapper->pCppInterface;
}

// Clean up wrapper when ref count reaches 0
void ReleaseSurfaceWrapper(IDirect3DSurface8* com_surface) {
    if (!com_surface) return;
    
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(com_surface);
    
    std::lock_guard<std::mutex> lock(g_wrapper_mutex);
    
    // Remove from tracking maps
    g_wrapper_to_cpp_map.erase(wrapper);
    g_cpp_to_wrapper_map.erase(wrapper->pCppInterface);
    
    // Release the C++ object
    wrapper->pCppInterface->Release();
    
    // Delete wrapper
    delete wrapper;
}

// ============================================================================
// Missing IDirect3DDevice8 Methods
// ============================================================================

extern "C" {

// Cursor methods
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCursorPosition(IDirect3DDevice8* This, int X, int Y, DWORD Flags) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetCursorPosition(X, Y, Flags);
}

HRESULT STDMETHODCALLTYPE Direct3DDevice8_ShowCursor(IDirect3DDevice8* This, BOOL bShow) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ShowCursor(bShow);
}

// Memory management
UINT STDMETHODCALLTYPE Direct3DDevice8_GetAvailableTextureMem(IDirect3DDevice8* This) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAvailableTextureMem();
}

// Gamma ramp methods
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetGammaRamp(IDirect3DDevice8* This, DWORD Flags, const D3DGAMMARAMP* pRamp) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetGammaRamp(Flags, pRamp);
}

HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetGammaRamp(IDirect3DDevice8* This, D3DGAMMARAMP* pRamp) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetGammaRamp(pRamp);
}

// Additional missing methods with surface wrapping

// Modified CopyRects to handle surface wrapping
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CopyRects_Wrapped(IDirect3DDevice8* This, 
                                                            IDirect3DSurface8* pSourceSurface,
                                                            const RECT* pSourceRectsArray, 
                                                            UINT cRects,
                                                            IDirect3DSurface8* pDestinationSurface,
                                                            const POINT* pDestPointsArray) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Unwrap surfaces
    ::IDirect3DSurface8* cpp_src = UnwrapSurface(pSourceSurface);
    ::IDirect3DSurface8* cpp_dst = UnwrapSurface(pDestinationSurface);
    
    return wrapper->pCppInterface->CopyRects(cpp_src, pSourceRectsArray, cRects, cpp_dst, pDestPointsArray);
}

// Modified SetRenderTarget to handle surface wrapping
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderTarget_Wrapped(IDirect3DDevice8* This,
                                                                  IDirect3DSurface8* pRenderTarget,
                                                                  IDirect3DSurface8* pNewZStencil) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Unwrap surfaces
    ::IDirect3DSurface8* cpp_rt = UnwrapSurface(pRenderTarget);
    ::IDirect3DSurface8* cpp_ds = UnwrapSurface(pNewZStencil);
    
    return wrapper->pCppInterface->SetRenderTarget(cpp_rt, cpp_ds);
}

// Modified GetRenderTarget to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderTarget_Wrapped(IDirect3DDevice8* This,
                                                                  IDirect3DSurface8** ppRenderTarget) {
    if (!ppRenderTarget) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetRenderTarget(&cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppRenderTarget = WrapSurface(cpp_surface);
        // Release the extra reference from GetRenderTarget since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppRenderTarget = nullptr;
    }
    
    return hr;
}

// Modified GetDepthStencilSurface to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDepthStencilSurface_Wrapped(IDirect3DDevice8* This,
                                                                         IDirect3DSurface8** ppZStencilSurface) {
    if (!ppZStencilSurface) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetDepthStencilSurface(&cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppZStencilSurface = WrapSurface(cpp_surface);
        // Release the extra reference since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppZStencilSurface = nullptr;
    }
    
    return hr;
}

// Modified GetBackBuffer to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetBackBuffer_Wrapped(IDirect3DDevice8* This,
                                                                UINT BackBuffer,
                                                                D3DBACKBUFFER_TYPE Type,
                                                                IDirect3DSurface8** ppBackBuffer) {
    if (!ppBackBuffer) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetBackBuffer(BackBuffer, Type, &cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppBackBuffer = WrapSurface(cpp_surface);
        // Release the extra reference since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppBackBuffer = nullptr;
    }
    
    return hr;
}

// Modified CreateRenderTarget to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateRenderTarget_Wrapped(IDirect3DDevice8* This,
                                                                     UINT Width, UINT Height,
                                                                     D3DFORMAT Format,
                                                                     D3DMULTISAMPLE_TYPE MultiSample,
                                                                     BOOL Lockable,
                                                                     IDirect3DSurface8** ppSurface) {
    if (!ppSurface) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateRenderTarget(Width, Height, Format, MultiSample, Lockable, &cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppSurface = WrapSurface(cpp_surface);
        // Release the extra reference since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppSurface = nullptr;
    }
    
    return hr;
}

// Modified CreateDepthStencilSurface to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateDepthStencilSurface_Wrapped(IDirect3DDevice8* This,
                                                                            UINT Width, UINT Height,
                                                                            D3DFORMAT Format,
                                                                            D3DMULTISAMPLE_TYPE MultiSample,
                                                                            IDirect3DSurface8** ppSurface) {
    if (!ppSurface) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateDepthStencilSurface(Width, Height, Format, MultiSample, &cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppSurface = WrapSurface(cpp_surface);
        // Release the extra reference since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppSurface = nullptr;
    }
    
    return hr;
}

// Modified CreateImageSurface to wrap the returned surface
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateImageSurface_Wrapped(IDirect3DDevice8* This,
                                                                     UINT Width, UINT Height,
                                                                     D3DFORMAT Format,
                                                                     IDirect3DSurface8** ppSurface) {
    if (!ppSurface) return D3DERR_INVALIDCALL;
    
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DSurface8* cpp_surface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateImageSurface(Width, Height, Format, &cpp_surface);
    
    if (SUCCEEDED(hr) && cpp_surface) {
        *ppSurface = WrapSurface(cpp_surface);
        // Release the extra reference since WrapSurface adds one
        cpp_surface->Release();
    } else {
        *ppSurface = nullptr;
    }
    
    return hr;
}

// Modified GetFrontBuffer to handle surface wrapping
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetFrontBuffer_Wrapped(IDirect3DDevice8* This,
                                                                 IDirect3DSurface8* pDestSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Unwrap destination surface
    ::IDirect3DSurface8* cpp_dst = UnwrapSurface(pDestSurface);
    
    return wrapper->pCppInterface->GetFrontBuffer(cpp_dst);
}

// ============================================================================
// IDirect3DSurface8 COM Wrapper Implementation
// ============================================================================

// IUnknown methods
HRESULT STDMETHODCALLTYPE Direct3DSurface8_QueryInterface(IDirect3DSurface8* This, REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_POINTER;
    
    // Check for supported interfaces
    if (IsEqualGUID(riid, &IID_IUnknown) || 
        IsEqualGUID(riid, &IID_IDirect3DResource8) ||
        IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        *ppvObj = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Direct3DSurface8_AddRef(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return InterlockedIncrement(&wrapper->ref_count);
}

ULONG STDMETHODCALLTYPE Direct3DSurface8_Release(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    ULONG ref = InterlockedDecrement(&wrapper->ref_count);
    
    if (ref == 0) {
        ReleaseSurfaceWrapper(This);
    }
    
    return ref;
}

// IDirect3DResource8 methods
HRESULT STDMETHODCALLTYPE Direct3DSurface8_GetDevice(IDirect3DSurface8* This, IDirect3DDevice8** ppDevice) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    
    // Need to wrap the device too
    ::IDirect3DDevice8* cpp_device = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetDevice(&cpp_device);
    
    if (SUCCEEDED(hr) && cpp_device) {
        // TODO: Wrap device if needed
        *ppDevice = reinterpret_cast<IDirect3DDevice8*>(cpp_device);
    }
    
    return hr;
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_SetPrivateData(IDirect3DSurface8* This, REFGUID refguid,
                                                          const void* pData, DWORD SizeOfData, DWORD Flags) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_GetPrivateData(IDirect3DSurface8* This, REFGUID refguid,
                                                          void* pData, DWORD* pSizeOfData) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_FreePrivateData(IDirect3DSurface8* This, REFGUID refguid) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->FreePrivateData(refguid);
}

DWORD STDMETHODCALLTYPE Direct3DSurface8_SetPriority(IDirect3DSurface8* This, DWORD PriorityNew) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->SetPriority(PriorityNew);
}

DWORD STDMETHODCALLTYPE Direct3DSurface8_GetPriority(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->GetPriority();
}

void STDMETHODCALLTYPE Direct3DSurface8_PreLoad(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    wrapper->pCppInterface->PreLoad();
}

D3DRESOURCETYPE STDMETHODCALLTYPE Direct3DSurface8_GetType(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->GetType();
}

// IDirect3DSurface8 methods
HRESULT STDMETHODCALLTYPE Direct3DSurface8_GetContainer(IDirect3DSurface8* This, REFIID riid, void** ppContainer) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->GetContainer(riid, ppContainer);
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_GetDesc(IDirect3DSurface8* This, D3DSURFACE_DESC* pDesc) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_LockRect(IDirect3DSurface8* This, D3DLOCKED_RECT* pLockedRect,
                                                    const RECT* pRect, DWORD Flags) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->LockRect(pLockedRect, pRect, Flags);
}

HRESULT STDMETHODCALLTYPE Direct3DSurface8_UnlockRect(IDirect3DSurface8* This) {
    Direct3DSurface8_COM_Wrapper* wrapper = reinterpret_cast<Direct3DSurface8_COM_Wrapper*>(This);
    return wrapper->pCppInterface->UnlockRect();
}

} // extern "C"

// Complete Surface8 vtable
IDirect3DSurface8Vtbl g_Direct3DSurface8_Vtbl_Complete = {
    Direct3DSurface8_QueryInterface,
    Direct3DSurface8_AddRef,
    Direct3DSurface8_Release,
    Direct3DSurface8_GetDevice,
    Direct3DSurface8_SetPrivateData,
    Direct3DSurface8_GetPrivateData,
    Direct3DSurface8_FreePrivateData,
    Direct3DSurface8_SetPriority,
    Direct3DSurface8_GetPriority,
    Direct3DSurface8_PreLoad,
    Direct3DSurface8_GetType,
    Direct3DSurface8_GetContainer,
    Direct3DSurface8_GetDesc,
    Direct3DSurface8_LockRect,
    Direct3DSurface8_UnlockRect
};

// Make the complete vtable available globally
IDirect3DSurface8Vtbl g_Direct3DSurface8_Vtbl = g_Direct3DSurface8_Vtbl_Complete;

} // namespace dx8gl