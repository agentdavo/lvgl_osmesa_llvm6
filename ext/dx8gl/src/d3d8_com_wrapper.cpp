/**
 * COM-style wrapper implementation for dx8gl
 * 
 * This file implements the COM-style C interface wrappers that bridge
 * between legacy DirectX 8 code and dx8gl's C++ implementation.
 */

#include "d3d8_com_wrapper.h"
#include <cstdio>

// Include the C++ interfaces to wrap
#define DX8GL_USE_CPP_INTERFACES
#include "d3d8_cpp_interfaces.h"
#include "d3d8_interface.h"
#include "d3d8_device.h"
#include "d3d8_cubetexture.h"
#include "d3d8_volumetexture.h"

#include <cstdlib>
#include <cstring>

// Forward declarations for wrapper structures
struct Direct3D8_COM_Wrapper;
struct Direct3DDevice8_COM_Wrapper;
struct Direct3DTexture8_COM_Wrapper;

// Forward declarations for wrapper creation functions
static IDirect3DTexture8* CreateTexture8_COM_Wrapper(::IDirect3DTexture8* pCppTexture);
static IDirect3DDevice8* CreateDevice8_COM_Wrapper(::IDirect3DDevice8* pCppDevice);
static IDirect3DSurface8* CreateSurface8_COM_Wrapper(::IDirect3DSurface8* pCppSurface);
static IDirect3DSwapChain8* CreateSwapChain8_COM_Wrapper(::IDirect3DSwapChain8* pCppSwapChain);
static IDirect3DVolumeTexture8* CreateVolumeTexture8_COM_Wrapper(::IDirect3DVolumeTexture8* pCppVolumeTexture);
static IDirect3DCubeTexture8* CreateCubeTexture8_COM_Wrapper(::IDirect3DCubeTexture8* pCppCubeTexture);
static IDirect3DVertexBuffer8* CreateVertexBuffer8_COM_Wrapper(::IDirect3DVertexBuffer8* pCppVertexBuffer);
static IDirect3DIndexBuffer8* CreateIndexBuffer8_COM_Wrapper(::IDirect3DIndexBuffer8* pCppIndexBuffer);
static IDirect3DVolume8* CreateVolume8_COM_Wrapper(::IDirect3DVolume8* pCppVolume);

// COM wrapper for IDirect3D8
struct Direct3D8_COM_Wrapper {
    IDirect3D8Vtbl* lpVtbl;
    ::IDirect3D8* pCppInterface;  // The C++ interface we're wrapping
    ULONG refCount;
};

// COM wrapper for IDirect3DDevice8  
struct Direct3DDevice8_COM_Wrapper {
    IDirect3DDevice8Vtbl* lpVtbl;
    ::IDirect3DDevice8* pCppInterface;  // The C++ interface we're wrapping
    ULONG refCount;
};

// COM wrapper for IDirect3DTexture8
struct Direct3DTexture8_COM_Wrapper {
    IDirect3DTexture8Vtbl* lpVtbl;
    ::IDirect3DTexture8* pCppInterface;  // The C++ interface we're wrapping
    ULONG refCount;
};

// COM wrapper for IDirect3DSurface8
struct Direct3DSurface8_COM_Wrapper {
    IDirect3DSurface8Vtbl* lpVtbl;
    ::IDirect3DSurface8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;  // For thread safety
};

// COM wrapper for IDirect3DSwapChain8
struct Direct3DSwapChain8_COM_Wrapper {
    IDirect3DSwapChain8Vtbl* lpVtbl;
    ::IDirect3DSwapChain8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// COM wrapper for IDirect3DVolumeTexture8
struct Direct3DVolumeTexture8_COM_Wrapper {
    IDirect3DVolumeTexture8Vtbl* lpVtbl;
    ::IDirect3DVolumeTexture8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// COM wrapper for IDirect3DCubeTexture8
struct Direct3DCubeTexture8_COM_Wrapper {
    IDirect3DCubeTexture8Vtbl* lpVtbl;
    ::IDirect3DCubeTexture8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// COM wrapper for IDirect3DVertexBuffer8
struct Direct3DVertexBuffer8_COM_Wrapper {
    IDirect3DVertexBuffer8Vtbl* lpVtbl;
    ::IDirect3DVertexBuffer8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// COM wrapper for IDirect3DIndexBuffer8
struct Direct3DIndexBuffer8_COM_Wrapper {
    IDirect3DIndexBuffer8Vtbl* lpVtbl;
    ::IDirect3DIndexBuffer8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// COM wrapper for IDirect3DVolume8
struct Direct3DVolume8_COM_Wrapper {
    IDirect3DVolume8Vtbl* lpVtbl;
    ::IDirect3DVolume8* pCppInterface;
    ULONG refCount;
    std::mutex mutex;
};

// Forward declaration of vtables
extern IDirect3D8Vtbl g_Direct3D8_Vtbl;
extern IDirect3DDevice8Vtbl g_Direct3DDevice8_Vtbl;
extern IDirect3DTexture8Vtbl g_Direct3DTexture8_Vtbl;
extern IDirect3DSurface8Vtbl g_Direct3DSurface8_Vtbl;
extern IDirect3DSwapChain8Vtbl g_Direct3DSwapChain8_Vtbl;
extern IDirect3DVolumeTexture8Vtbl g_Direct3DVolumeTexture8_Vtbl;
extern IDirect3DCubeTexture8Vtbl g_Direct3DCubeTexture8_Vtbl;
extern IDirect3DVertexBuffer8Vtbl g_Direct3DVertexBuffer8_Vtbl;
extern IDirect3DIndexBuffer8Vtbl g_Direct3DIndexBuffer8_Vtbl;
extern IDirect3DVolume8Vtbl g_Direct3DVolume8_Vtbl;

// Forward declarations for vtable functions
static HRESULT STDMETHODCALLTYPE Direct3D8_QueryInterface(IDirect3D8* This, REFIID riid, void** ppvObj);
static ULONG STDMETHODCALLTYPE Direct3D8_AddRef(IDirect3D8* This);
static ULONG STDMETHODCALLTYPE Direct3D8_Release(IDirect3D8* This);
static HRESULT STDMETHODCALLTYPE Direct3D8_RegisterSoftwareDevice(IDirect3D8* This, void* pInitializeFunction);
static UINT STDMETHODCALLTYPE Direct3D8_GetAdapterCount(IDirect3D8* This);
static HRESULT STDMETHODCALLTYPE Direct3D8_GetAdapterIdentifier(IDirect3D8* This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
static UINT STDMETHODCALLTYPE Direct3D8_GetAdapterModeCount(IDirect3D8* This, UINT Adapter);
static HRESULT STDMETHODCALLTYPE Direct3D8_EnumAdapterModes(IDirect3D8* This, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
static HRESULT STDMETHODCALLTYPE Direct3D8_GetAdapterDisplayMode(IDirect3D8* This, UINT Adapter, D3DDISPLAYMODE* pMode);
static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceType(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceFormat(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceMultiSampleType(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType);
static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDepthStencilMatch(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
static HRESULT STDMETHODCALLTYPE Direct3D8_GetDeviceCaps(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
static HMONITOR STDMETHODCALLTYPE Direct3D8_GetAdapterMonitor(IDirect3D8* This, UINT Adapter);
static HRESULT STDMETHODCALLTYPE Direct3D8_CreateDevice(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);

// Static vtable for IDirect3D8
IDirect3D8Vtbl g_Direct3D8_Vtbl = {
    Direct3D8_QueryInterface,
    Direct3D8_AddRef,
    Direct3D8_Release,
    Direct3D8_RegisterSoftwareDevice,
    Direct3D8_GetAdapterCount,
    Direct3D8_GetAdapterIdentifier,
    Direct3D8_GetAdapterModeCount,
    Direct3D8_EnumAdapterModes,
    Direct3D8_GetAdapterDisplayMode,
    Direct3D8_CheckDeviceType,
    Direct3D8_CheckDeviceFormat,
    Direct3D8_CheckDeviceMultiSampleType,
    Direct3D8_CheckDepthStencilMatch,
    Direct3D8_GetDeviceCaps,
    Direct3D8_GetAdapterMonitor,
    Direct3D8_CreateDevice
};

// Implementation of IDirect3D8 COM wrapper methods

static HRESULT STDMETHODCALLTYPE Direct3D8_QueryInterface(IDirect3D8* This, REFIID riid, void** ppvObj) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->QueryInterface(riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE Direct3D8_AddRef(IDirect3D8* This) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    wrapper->refCount++;
    return wrapper->refCount;
}

static ULONG STDMETHODCALLTYPE Direct3D8_Release(IDirect3D8* This) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    wrapper->refCount--;
    if (wrapper->refCount == 0) {
        wrapper->pCppInterface->Release();
        free(wrapper);
        return 0;
    }
    return wrapper->refCount;
}

static HRESULT STDMETHODCALLTYPE Direct3D8_RegisterSoftwareDevice(IDirect3D8* This, void* pInitializeFunction) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->RegisterSoftwareDevice(pInitializeFunction);
}

static UINT STDMETHODCALLTYPE Direct3D8_GetAdapterCount(IDirect3D8* This) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAdapterCount();
}

static HRESULT STDMETHODCALLTYPE Direct3D8_GetAdapterIdentifier(IDirect3D8* This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
}

static UINT STDMETHODCALLTYPE Direct3D8_GetAdapterModeCount(IDirect3D8* This, UINT Adapter) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAdapterModeCount(Adapter);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_EnumAdapterModes(IDirect3D8* This, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->EnumAdapterModes(Adapter, Mode, pMode);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_GetAdapterDisplayMode(IDirect3D8* This, UINT Adapter, D3DDISPLAYMODE* pMode) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAdapterDisplayMode(Adapter, pMode);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceType(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CheckDeviceType(Adapter, DevType, AdapterFormat, BackBufferFormat, Windowed);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceFormat(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDeviceMultiSampleType(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_CheckDepthStencilMatch(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
}

static HRESULT STDMETHODCALLTYPE Direct3D8_GetDeviceCaps(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetDeviceCaps(Adapter, DeviceType, pCaps);
}

static HMONITOR STDMETHODCALLTYPE Direct3D8_GetAdapterMonitor(IDirect3D8* This, UINT Adapter) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAdapterMonitor(Adapter);
}

// Forward declaration for device wrapper creation
static IDirect3DDevice8* CreateDevice8_COM_Wrapper(::IDirect3DDevice8* pCppDevice);

static HRESULT STDMETHODCALLTYPE Direct3D8_CreateDevice(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, 
                                                        HWND hFocusWindow, DWORD BehaviorFlags, 
                                                        D3DPRESENT_PARAMETERS* pPresentationParameters, 
                                                        IDirect3DDevice8** ppReturnedDeviceInterface) {
    Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)This;
    
    printf("[DX8GL] CreateDevice called: Adapter=%u, DeviceType=%d, hFocusWindow=%p, BehaviorFlags=0x%08X\n", 
           Adapter, DeviceType, hFocusWindow, BehaviorFlags);
    
    if (pPresentationParameters) {
        printf("[DX8GL] Present params: %ux%u, Windowed=%d, BackBufferFormat=%d\n",
               pPresentationParameters->BackBufferWidth,
               pPresentationParameters->BackBufferHeight,
               pPresentationParameters->Windowed,
               pPresentationParameters->BackBufferFormat);
    }
    
    // Call the C++ implementation to create a device
    ::IDirect3DDevice8* pCppDevice = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, 
                                                      pPresentationParameters, &pCppDevice);
    
    printf("[DX8GL] C++ CreateDevice returned hr=0x%08X, pCppDevice=%p\n", hr, pCppDevice);
    
    if (SUCCEEDED(hr) && pCppDevice) {
        // Wrap the C++ device in a COM wrapper
        *ppReturnedDeviceInterface = CreateDevice8_COM_Wrapper(pCppDevice);
        if (!*ppReturnedDeviceInterface) {
            printf("[DX8GL] Failed to create COM wrapper for device\n");
            pCppDevice->Release();
            return E_OUTOFMEMORY;
        }
        printf("[DX8GL] Created device COM wrapper at %p\n", *ppReturnedDeviceInterface);
    } else {
        printf("[DX8GL] CreateDevice failed with hr=0x%08X\n", hr);
    }
    
    return hr;
}

// IDirect3DDevice8 COM wrapper implementation
// TODO: Implement all device methods similar to the Direct3D8 methods above
// For now, let's implement a minimal set to get things compiling

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_QueryInterface(IDirect3DDevice8* This, REFIID riid, void** ppvObj);
static ULONG STDMETHODCALLTYPE Direct3DDevice8_AddRef(IDirect3DDevice8* This);
static ULONG STDMETHODCALLTYPE Direct3DDevice8_Release(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_TestCooperativeLevel(IDirect3DDevice8* This);
static UINT STDMETHODCALLTYPE Direct3DDevice8_GetAvailableTextureMem(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ResourceManagerDiscardBytes(IDirect3DDevice8* This, DWORD Bytes);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDirect3D(IDirect3DDevice8* This, IDirect3D8** ppD3D8);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDeviceCaps(IDirect3DDevice8* This, D3DCAPS8* pCaps);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDisplayMode(IDirect3DDevice8* This, D3DDISPLAYMODE* pMode);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetCreationParameters(IDirect3DDevice8* This, D3DDEVICE_CREATION_PARAMETERS* pParameters);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCursorProperties(IDirect3DDevice8* This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap);
static void STDMETHODCALLTYPE Direct3DDevice8_SetCursorPosition(IDirect3DDevice8* This, int X, int Y, DWORD Flags);
static BOOL STDMETHODCALLTYPE Direct3DDevice8_ShowCursor(IDirect3DDevice8* This, BOOL bShow);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateAdditionalSwapChain(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Reset(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Present(IDirect3DDevice8* This, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetBackBuffer(IDirect3DDevice8* This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRasterStatus(IDirect3DDevice8* This, D3DRASTER_STATUS* pRasterStatus);
static void STDMETHODCALLTYPE Direct3DDevice8_SetGammaRamp(IDirect3DDevice8* This, DWORD Flags, const D3DGAMMARAMP* pRamp);
static void STDMETHODCALLTYPE Direct3DDevice8_GetGammaRamp(IDirect3DDevice8* This, D3DGAMMARAMP* pRamp);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateTexture(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVolumeTexture(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateCubeTexture(IDirect3DDevice8* This, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateIndexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateRenderTarget(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateDepthStencilSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateImageSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface);
// Forward declarations for methods that forward to C++ implementation
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CopyRects(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, const RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPointsArray);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_UpdateTexture(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetFrontBuffer(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDepthStencilSurface(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginScene(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndScene(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Clear(IDirect3DDevice8* This, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
// More method forward declarations
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_MultiplyTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetViewport(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetViewport(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetMaterial(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetMaterial(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetLight(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLight(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_LightEnable(IDirect3DDevice8* This, DWORD Index, BOOL Enable);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLightEnable(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipPlane(IDirect3DDevice8* This, DWORD Index, const float* pPlane);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipPlane(IDirect3DDevice8* This, DWORD Index, float* pPlane);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD Value);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD* pValue);
// State block and clip status functions
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginStateBlock(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndStateBlock(IDirect3DDevice8* This, DWORD* pToken);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ApplyStateBlock(IDirect3DDevice8* This, DWORD Token);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CaptureStateBlock(IDirect3DDevice8* This, DWORD Token);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteStateBlock(IDirect3DDevice8* This, DWORD Token);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateStateBlock(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipStatus(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipStatus(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus);
// Texture functions - GetTexture, GetTextureStageState and SetTextureStageState are static implementations below
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8* pTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
// Validation and palette functions
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ValidateDevice(IDirect3DDevice8* This, DWORD* pNumPasses);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetInfo(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, const PALETTEENTRY* pEntries);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, PALETTEENTRY* pEntries);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCurrentTexturePalette(IDirect3DDevice8* This, UINT PaletteNumber);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetCurrentTexturePalette(IDirect3DDevice8* This, UINT* PaletteNumber);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
// Drawing, shader and patch functions
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ProcessVertices(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexShader(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShader(IDirect3DDevice8* This, DWORD Handle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShader(IDirect3DDevice8* This, DWORD* pHandle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteVertexShader(IDirect3DDevice8* This, DWORD Handle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderDeclaration(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreatePixelShader(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShader(IDirect3DDevice8* This, DWORD Handle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShader(IDirect3DDevice8* This, DWORD* pHandle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePixelShader(IDirect3DDevice8* This, DWORD Handle);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawRectPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawTriPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePatch(IDirect3DDevice8* This, UINT Handle);

// Static vtable for IDirect3DDevice8
IDirect3DDevice8Vtbl g_Direct3DDevice8_Vtbl = {
    Direct3DDevice8_QueryInterface,
    Direct3DDevice8_AddRef,
    Direct3DDevice8_Release,
    Direct3DDevice8_TestCooperativeLevel,
    Direct3DDevice8_GetAvailableTextureMem,
    Direct3DDevice8_ResourceManagerDiscardBytes,
    Direct3DDevice8_GetDirect3D,
    Direct3DDevice8_GetDeviceCaps,
    Direct3DDevice8_GetDisplayMode,
    Direct3DDevice8_GetCreationParameters,
    Direct3DDevice8_SetCursorProperties,
    Direct3DDevice8_SetCursorPosition,
    Direct3DDevice8_ShowCursor,
    Direct3DDevice8_CreateAdditionalSwapChain,
    Direct3DDevice8_Reset,
    Direct3DDevice8_Present,
    Direct3DDevice8_GetBackBuffer,
    Direct3DDevice8_GetRasterStatus,
    Direct3DDevice8_SetGammaRamp,
    Direct3DDevice8_GetGammaRamp,
    Direct3DDevice8_CreateTexture,
    Direct3DDevice8_CreateVolumeTexture,
    Direct3DDevice8_CreateCubeTexture,
    Direct3DDevice8_CreateVertexBuffer,
    Direct3DDevice8_CreateIndexBuffer,
    Direct3DDevice8_CreateRenderTarget,
    Direct3DDevice8_CreateDepthStencilSurface,
    Direct3DDevice8_CreateImageSurface,
    Direct3DDevice8_CopyRects,
    Direct3DDevice8_UpdateTexture,
    Direct3DDevice8_GetFrontBuffer,
    Direct3DDevice8_SetRenderTarget,
    Direct3DDevice8_GetRenderTarget,
    Direct3DDevice8_GetDepthStencilSurface,
    Direct3DDevice8_BeginScene,
    Direct3DDevice8_EndScene,
    Direct3DDevice8_Clear,
    Direct3DDevice8_SetTransform,
    Direct3DDevice8_GetTransform,
    Direct3DDevice8_MultiplyTransform,
    Direct3DDevice8_SetViewport,
    Direct3DDevice8_GetViewport,
    Direct3DDevice8_SetMaterial,
    Direct3DDevice8_GetMaterial,
    Direct3DDevice8_SetLight,
    Direct3DDevice8_GetLight,
    Direct3DDevice8_LightEnable,
    Direct3DDevice8_GetLightEnable,
    Direct3DDevice8_SetClipPlane,
    Direct3DDevice8_GetClipPlane,
    Direct3DDevice8_SetRenderState,
    Direct3DDevice8_GetRenderState,
    Direct3DDevice8_BeginStateBlock,
    Direct3DDevice8_EndStateBlock,
    Direct3DDevice8_ApplyStateBlock,
    Direct3DDevice8_CaptureStateBlock,
    Direct3DDevice8_DeleteStateBlock,
    Direct3DDevice8_CreateStateBlock,
    Direct3DDevice8_SetClipStatus,
    Direct3DDevice8_GetClipStatus,
    Direct3DDevice8_GetTexture,
    Direct3DDevice8_SetTexture,
    Direct3DDevice8_GetTextureStageState,
    Direct3DDevice8_SetTextureStageState,
    Direct3DDevice8_ValidateDevice,
    Direct3DDevice8_GetInfo,
    Direct3DDevice8_SetPaletteEntries,
    Direct3DDevice8_GetPaletteEntries,
    Direct3DDevice8_SetCurrentTexturePalette,
    Direct3DDevice8_GetCurrentTexturePalette,
    Direct3DDevice8_DrawPrimitive,
    Direct3DDevice8_DrawIndexedPrimitive,
    Direct3DDevice8_DrawPrimitiveUP,
    Direct3DDevice8_DrawIndexedPrimitiveUP,
    Direct3DDevice8_ProcessVertices,
    Direct3DDevice8_CreateVertexShader,
    Direct3DDevice8_SetVertexShader,
    Direct3DDevice8_GetVertexShader,
    Direct3DDevice8_DeleteVertexShader,
    Direct3DDevice8_SetVertexShaderConstant,
    Direct3DDevice8_GetVertexShaderConstant,
    Direct3DDevice8_GetVertexShaderDeclaration,
    Direct3DDevice8_GetVertexShaderFunction,
    Direct3DDevice8_SetStreamSource,
    Direct3DDevice8_GetStreamSource,
    Direct3DDevice8_SetIndices,
    Direct3DDevice8_GetIndices,
    Direct3DDevice8_CreatePixelShader,
    Direct3DDevice8_SetPixelShader,
    Direct3DDevice8_GetPixelShader,
    Direct3DDevice8_DeletePixelShader,
    Direct3DDevice8_SetPixelShaderConstant,
    Direct3DDevice8_GetPixelShaderConstant,
    Direct3DDevice8_GetPixelShaderFunction,
    Direct3DDevice8_DrawRectPatch,
    Direct3DDevice8_DrawTriPatch,
    Direct3DDevice8_DeletePatch
};

// Minimal implementation of device methods - just forward to C++ interface
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_QueryInterface(IDirect3DDevice8* This, REFIID riid, void** ppvObj) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->QueryInterface(riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE Direct3DDevice8_AddRef(IDirect3DDevice8* This) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    wrapper->refCount++;
    return wrapper->refCount;
}

static ULONG STDMETHODCALLTYPE Direct3DDevice8_Release(IDirect3DDevice8* This) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    wrapper->refCount--;
    if (wrapper->refCount == 0) {
        wrapper->pCppInterface->Release();
        free(wrapper);
        return 0;
    }
    return wrapper->refCount;
}

// Implement all remaining device methods by forwarding to C++ interface
#define DEVICE_METHOD_FORWARD(method, ...) \
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This; \
    return wrapper->pCppInterface->method(__VA_ARGS__);

#define DEVICE_METHOD_FORWARD_VOID(method, ...) \
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This; \
    wrapper->pCppInterface->method(__VA_ARGS__);

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_TestCooperativeLevel(IDirect3DDevice8* This) {
    DEVICE_METHOD_FORWARD(TestCooperativeLevel)
}

static UINT STDMETHODCALLTYPE Direct3DDevice8_GetAvailableTextureMem(IDirect3DDevice8* This) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetAvailableTextureMem();
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ResourceManagerDiscardBytes(IDirect3DDevice8* This, DWORD Bytes) {
    DEVICE_METHOD_FORWARD(ResourceManagerDiscardBytes, Bytes)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDirect3D(IDirect3DDevice8* This, IDirect3D8** ppD3D8) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3D8* pCppD3D8 = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetDirect3D(&pCppD3D8);
    
    if (SUCCEEDED(hr) && pCppD3D8) {
        // Create a COM wrapper for the returned Direct3D8 interface
        Direct3D8_COM_Wrapper* d3d8_wrapper = (Direct3D8_COM_Wrapper*)malloc(sizeof(Direct3D8_COM_Wrapper));
        if (!d3d8_wrapper) {
            pCppD3D8->Release();
            return E_OUTOFMEMORY;
        }
        
        d3d8_wrapper->lpVtbl = &g_Direct3D8_Vtbl;
        d3d8_wrapper->pCppInterface = pCppD3D8;
        d3d8_wrapper->refCount = 1;
        
        *ppD3D8 = (IDirect3D8*)d3d8_wrapper;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDeviceCaps(IDirect3DDevice8* This, D3DCAPS8* pCaps) {
    DEVICE_METHOD_FORWARD(GetDeviceCaps, pCaps)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDisplayMode(IDirect3DDevice8* This, D3DDISPLAYMODE* pMode) {
    DEVICE_METHOD_FORWARD(GetDisplayMode, pMode)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetCreationParameters(IDirect3DDevice8* This, D3DDEVICE_CREATION_PARAMETERS* pParameters) {
    DEVICE_METHOD_FORWARD(GetCreationParameters, pParameters)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCursorProperties(IDirect3DDevice8* This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // TODO: Implement proper surface unwrapping when surface wrappers are implemented
    // For now, assume pCursorBitmap is already a C++ interface
    return wrapper->pCppInterface->SetCursorProperties(XHotSpot, YHotSpot, (::IDirect3DSurface8*)pCursorBitmap);
}

static void STDMETHODCALLTYPE Direct3DDevice8_SetCursorPosition(IDirect3DDevice8* This, int X, int Y, DWORD Flags) {
    DEVICE_METHOD_FORWARD_VOID(SetCursorPosition, X, Y, Flags)
}

static BOOL STDMETHODCALLTYPE Direct3DDevice8_ShowCursor(IDirect3DDevice8* This, BOOL bShow) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ShowCursor(bShow);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateAdditionalSwapChain(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the additional swap chain
    ::IDirect3DSwapChain8* pCppSwapChain = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateAdditionalSwapChain(pPresentationParameters, &pCppSwapChain);
    if (FAILED(hr) || !pCppSwapChain) {
        return hr;
    }
    
    // Wrap the swap chain in COM wrapper
    *ppSwapChain = CreateSwapChain8_COM_Wrapper(pCppSwapChain);
    if (!*ppSwapChain) {
        pCppSwapChain->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Reset(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    DEVICE_METHOD_FORWARD(Reset, pPresentationParameters)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Present(IDirect3DDevice8* This, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    DEVICE_METHOD_FORWARD(Present, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetBackBuffer(IDirect3DDevice8* This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to get the back buffer
    ::IDirect3DSurface8* pCppSurface = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetBackBuffer(BackBuffer, Type, &pCppSurface);
    if (FAILED(hr) || !pCppSurface) {
        return hr;
    }
    
    // Wrap the surface in COM wrapper
    *ppBackBuffer = CreateSurface8_COM_Wrapper(pCppSurface);
    if (!*ppBackBuffer) {
        pCppSurface->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRasterStatus(IDirect3DDevice8* This, D3DRASTER_STATUS* pRasterStatus) {
    DEVICE_METHOD_FORWARD(GetRasterStatus, pRasterStatus)
}

static void STDMETHODCALLTYPE Direct3DDevice8_SetGammaRamp(IDirect3DDevice8* This, DWORD Flags, const D3DGAMMARAMP* pRamp) {
    DEVICE_METHOD_FORWARD_VOID(SetGammaRamp, Flags, pRamp)
}

static void STDMETHODCALLTYPE Direct3DDevice8_GetGammaRamp(IDirect3DDevice8* This, D3DGAMMARAMP* pRamp) {
    DEVICE_METHOD_FORWARD_VOID(GetGammaRamp, pRamp)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateTexture(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call the C++ implementation to create a texture
    ::IDirect3DTexture8* pCppTexture = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pCppTexture);
    
    if (SUCCEEDED(hr) && pCppTexture) {
        // Wrap the C++ texture in a COM wrapper
        *ppTexture = CreateTexture8_COM_Wrapper(pCppTexture);
        if (!*ppTexture) {
            pCppTexture->Release();
            return E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVolumeTexture(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the volume texture
    ::IDirect3DVolumeTexture8* pCppVolumeTexture = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, &pCppVolumeTexture);
    if (FAILED(hr) || !pCppVolumeTexture) {
        return hr;
    }
    
    // Wrap the volume texture in COM wrapper
    *ppVolumeTexture = CreateVolumeTexture8_COM_Wrapper(pCppVolumeTexture);
    if (!*ppVolumeTexture) {
        pCppVolumeTexture->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateCubeTexture(IDirect3DDevice8* This, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the cube texture
    ::IDirect3DCubeTexture8* pCppCubeTexture = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, &pCppCubeTexture);
    if (FAILED(hr) || !pCppCubeTexture) {
        return hr;
    }
    
    // Wrap the cube texture in COM wrapper
    *ppCubeTexture = CreateCubeTexture8_COM_Wrapper(pCppCubeTexture);
    if (!*ppCubeTexture) {
        pCppCubeTexture->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the vertex buffer
    ::IDirect3DVertexBuffer8* pCppVertexBuffer = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateVertexBuffer(Length, Usage, FVF, Pool, &pCppVertexBuffer);
    if (FAILED(hr) || !pCppVertexBuffer) {
        return hr;
    }
    
    // Wrap the vertex buffer in COM wrapper
    *ppVertexBuffer = CreateVertexBuffer8_COM_Wrapper(pCppVertexBuffer);
    if (!*ppVertexBuffer) {
        pCppVertexBuffer->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateIndexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the index buffer
    ::IDirect3DIndexBuffer8* pCppIndexBuffer = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateIndexBuffer(Length, Usage, Format, Pool, &pCppIndexBuffer);
    if (FAILED(hr) || !pCppIndexBuffer) {
        return hr;
    }
    
    // Wrap the index buffer in COM wrapper
    *ppIndexBuffer = CreateIndexBuffer8_COM_Wrapper(pCppIndexBuffer);
    if (!*ppIndexBuffer) {
        pCppIndexBuffer->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateRenderTarget(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the render target
    ::IDirect3DSurface8* pCppSurface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateRenderTarget(Width, Height, Format, MultiSample, Lockable, &pCppSurface);
    if (FAILED(hr) || !pCppSurface) {
        return hr;
    }
    
    // Wrap the surface in COM wrapper
    *ppSurface = CreateSurface8_COM_Wrapper(pCppSurface);
    if (!*ppSurface) {
        pCppSurface->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateDepthStencilSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the depth stencil surface
    ::IDirect3DSurface8* pCppSurface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateDepthStencilSurface(Width, Height, Format, MultiSample, &pCppSurface);
    if (FAILED(hr) || !pCppSurface) {
        return hr;
    }
    
    // Wrap the surface in COM wrapper
    *ppSurface = CreateSurface8_COM_Wrapper(pCppSurface);
    if (!*ppSurface) {
        pCppSurface->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateImageSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // Call C++ implementation to create the image surface
    ::IDirect3DSurface8* pCppSurface = nullptr;
    HRESULT hr = wrapper->pCppInterface->CreateImageSurface(Width, Height, Format, &pCppSurface);
    if (FAILED(hr) || !pCppSurface) {
        return hr;
    }
    
    // Wrap the surface in COM wrapper
    *ppSurface = CreateSurface8_COM_Wrapper(pCppSurface);
    if (!*ppSurface) {
        pCppSurface->Release();
        return E_OUTOFMEMORY;
    }
    
    return hr;
}

// Continue with minimal implementations for the rest...
// For brevity, I'll implement the key rendering methods

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Clear(IDirect3DDevice8* This, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    DEVICE_METHOD_FORWARD(Clear, Count, pRects, Flags, Color, Z, Stencil)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginScene(IDirect3DDevice8* This) {
    DEVICE_METHOD_FORWARD(BeginScene)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndScene(IDirect3DDevice8* This) {
    DEVICE_METHOD_FORWARD(EndScene)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) {
    DEVICE_METHOD_FORWARD(SetTransform, State, pMatrix)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    DEVICE_METHOD_FORWARD(GetTransform, State, pMatrix)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD Value) {
    DEVICE_METHOD_FORWARD(SetRenderState, State, Value)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD* pValue) {
    DEVICE_METHOD_FORWARD(GetRenderState, State, pValue)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    // If pTexture is a COM wrapper, unwrap it to get the C++ interface
    ::IDirect3DBaseTexture8* pCppTexture = nullptr;
    if (pTexture) {
        // Check if it's a texture wrapper by trying to cast to our wrapper type
        Direct3DTexture8_COM_Wrapper* textureWrapper = (Direct3DTexture8_COM_Wrapper*)pTexture;
        if (textureWrapper && textureWrapper->lpVtbl == &g_Direct3DTexture8_Vtbl) {
            pCppTexture = (::IDirect3DBaseTexture8*)textureWrapper->pCppInterface;
        } else {
            // Assume it's already a C++ interface
            pCppTexture = (::IDirect3DBaseTexture8*)pTexture;
        }
    }
    
    return wrapper->pCppInterface->SetTexture(Stage, pCppTexture);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    
    ::IDirect3DBaseTexture8* pCppTexture = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetTexture(Stage, &pCppTexture);
    
    if (SUCCEEDED(hr) && pCppTexture) {
        // Check the type of texture and create appropriate wrapper
        D3DRESOURCETYPE type = pCppTexture->GetType();
        
        if (type == D3DRTYPE_TEXTURE) {
            // Create texture wrapper
            *ppTexture = (IDirect3DBaseTexture8*)CreateTexture8_COM_Wrapper((::IDirect3DTexture8*)pCppTexture);
        } else {
            // For now, just return the C++ interface for other texture types
            *ppTexture = (IDirect3DBaseTexture8*)pCppTexture;
        }
        
        if (!*ppTexture) {
            pCppTexture->Release();
            return E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    DEVICE_METHOD_FORWARD(SetTextureStageState, Stage, Type, Value)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    DEVICE_METHOD_FORWARD(GetTextureStageState, Stage, Type, pValue)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    DEVICE_METHOD_FORWARD(DrawPrimitive, PrimitiveType, StartVertex, PrimitiveCount)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) {
    DEVICE_METHOD_FORWARD(DrawIndexedPrimitive, PrimitiveType, MinIndex, NumVertices, StartIndex, PrimitiveCount)
}

// These methods are now implemented in d3d8_missing_stubs.cpp with extern "C" linkage
// The implementations handle the COM wrapper forwarding to the C++ class methods
// Removed duplicate STUB_METHOD calls to avoid multiple definition errors

// Implement remaining methods that forward to C++ implementation

// Surface and texture management methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CopyRects(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, const RECT* pSourceRects, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPoints) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CopyRects((::IDirect3DSurface8*)pSourceSurface, pSourceRects, cRects, (::IDirect3DSurface8*)pDestinationSurface, pDestPoints);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_UpdateTexture(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->UpdateTexture((::IDirect3DBaseTexture8*)pSourceTexture, (::IDirect3DBaseTexture8*)pDestinationTexture);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetFrontBuffer(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetFrontBuffer((::IDirect3DSurface8*)pDestSurface);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetRenderTarget((::IDirect3DSurface8*)pRenderTarget, (::IDirect3DSurface8*)pNewZStencil);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetRenderTarget((::IDirect3DSurface8**)ppRenderTarget);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDepthStencilSurface(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetDepthStencilSurface((::IDirect3DSurface8**)ppZStencilSurface);
}

// Transform and viewport methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_MultiplyTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->MultiplyTransform(State, pMatrix);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetViewport(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetViewport(pViewport);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetViewport(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetViewport(pViewport);
}

// Material and lighting methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetMaterial(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetMaterial(pMaterial);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetMaterial(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetMaterial(pMaterial);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetLight(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetLight(Index, pLight);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLight(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetLight(Index, pLight);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_LightEnable(IDirect3DDevice8* This, DWORD Index, BOOL Enable) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->LightEnable(Index, Enable);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLightEnable(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetLightEnable(Index, pEnable);
}

// Clipping plane methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipPlane(IDirect3DDevice8* This, DWORD Index, const float* pPlane) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetClipPlane(Index, pPlane);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipPlane(IDirect3DDevice8* This, DWORD Index, float* pPlane) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetClipPlane(Index, pPlane);
}

// State block methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginStateBlock(IDirect3DDevice8* This) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->BeginStateBlock();
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndStateBlock(IDirect3DDevice8* This, DWORD* pToken) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->EndStateBlock(pToken);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ApplyStateBlock(IDirect3DDevice8* This, DWORD Token) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ApplyStateBlock(Token);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CaptureStateBlock(IDirect3DDevice8* This, DWORD Token) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CaptureStateBlock(Token);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteStateBlock(IDirect3DDevice8* This, DWORD Token) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DeleteStateBlock(Token);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateStateBlock(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CreateStateBlock(Type, pToken);
}

// Clip status methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipStatus(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetClipStatus(pClipStatus);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipStatus(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetClipStatus(pClipStatus);
}

// Validation and info methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ValidateDevice(IDirect3DDevice8* This, DWORD* pNumPasses) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ValidateDevice(pNumPasses);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetInfo(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetInfo(DevInfoID, pDevInfoStruct, DevInfoStructSize);
}

// Palette methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, const PALETTEENTRY* pEntries) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetPaletteEntries(PaletteNumber, pEntries);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPaletteEntries(PaletteNumber, pEntries);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCurrentTexturePalette(IDirect3DDevice8* This, UINT PaletteNumber) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetCurrentTexturePalette(PaletteNumber);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetCurrentTexturePalette(IDirect3DDevice8* This, UINT* PaletteNumber) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetCurrentTexturePalette(PaletteNumber);
}

// Drawing methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_ProcessVertices(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, (::IDirect3DVertexBuffer8*)pDestBuffer, Flags);
}

// Vertex shader methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexShader(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CreateVertexShader(pDeclaration, pFunction, pHandle, Usage);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShader(IDirect3DDevice8* This, DWORD Handle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetVertexShader(Handle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShader(IDirect3DDevice8* This, DWORD* pHandle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetVertexShader(pHandle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteVertexShader(IDirect3DDevice8* This, DWORD Handle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DeleteVertexShader(Handle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderDeclaration(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetVertexShaderDeclaration(Handle, pData, pSizeOfData);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetVertexShaderFunction(Handle, pData, pSizeOfData);
}

// Stream source and indices methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetStreamSource(StreamNumber, (::IDirect3DVertexBuffer8*)pStreamData, Stride);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetStreamSource(StreamNumber, (::IDirect3DVertexBuffer8**)ppStreamData, pStride);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetIndices((::IDirect3DIndexBuffer8*)pIndexData, BaseVertexIndex);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetIndices((::IDirect3DIndexBuffer8**)ppIndexData, pBaseVertexIndex);
}

// Pixel shader methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreatePixelShader(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->CreatePixelShader(pFunction, pHandle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShader(IDirect3DDevice8* This, DWORD Handle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetPixelShader(Handle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShader(IDirect3DDevice8* This, DWORD* pHandle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPixelShader(pHandle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePixelShader(IDirect3DDevice8* This, DWORD Handle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DeletePixelShader(Handle);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPixelShaderFunction(Handle, pData, pSizeOfData);
}

// Patch methods
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawRectPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawTriPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePatch(IDirect3DDevice8* This, UINT Handle) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->DeletePatch(Handle);
}

// The following declarations are no longer needed as we've implemented all methods above
// Commenting out to avoid duplicate definitions
/*
extern "C" {
*/


// Forward declarations for texture vtable functions
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_QueryInterface(IDirect3DTexture8* This, REFIID riid, void** ppvObj);
static ULONG STDMETHODCALLTYPE Direct3DTexture8_AddRef(IDirect3DTexture8* This);
static ULONG STDMETHODCALLTYPE Direct3DTexture8_Release(IDirect3DTexture8* This);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetDevice(IDirect3DTexture8* This, IDirect3DDevice8** ppDevice);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_SetPrivateData(IDirect3DTexture8* This, REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetPrivateData(IDirect3DTexture8* This, REFGUID refguid, void* pData, DWORD* pSizeOfData);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_FreePrivateData(IDirect3DTexture8* This, REFGUID refguid);
static DWORD STDMETHODCALLTYPE Direct3DTexture8_SetPriority(IDirect3DTexture8* This, DWORD PriorityNew);
static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetPriority(IDirect3DTexture8* This);
static void STDMETHODCALLTYPE Direct3DTexture8_PreLoad(IDirect3DTexture8* This);
static D3DRESOURCETYPE STDMETHODCALLTYPE Direct3DTexture8_GetType(IDirect3DTexture8* This);
static DWORD STDMETHODCALLTYPE Direct3DTexture8_SetLOD(IDirect3DTexture8* This, DWORD LODNew);
static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetLOD(IDirect3DTexture8* This);
static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetLevelCount(IDirect3DTexture8* This);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetLevelDesc(IDirect3DTexture8* This, UINT Level, D3DSURFACE_DESC* pDesc);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetSurfaceLevel(IDirect3DTexture8* This, UINT Level, IDirect3DSurface8** ppSurfaceLevel);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_LockRect(IDirect3DTexture8* This, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_UnlockRect(IDirect3DTexture8* This, UINT Level);
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_AddDirtyRect(IDirect3DTexture8* This, const RECT* pDirtyRect);

// Static vtable for IDirect3DTexture8
IDirect3DTexture8Vtbl g_Direct3DTexture8_Vtbl = {
    // IUnknown methods
    Direct3DTexture8_QueryInterface,
    Direct3DTexture8_AddRef,
    Direct3DTexture8_Release,
    // IDirect3DResource8 methods
    Direct3DTexture8_GetDevice,
    Direct3DTexture8_SetPrivateData,
    Direct3DTexture8_GetPrivateData,
    Direct3DTexture8_FreePrivateData,
    Direct3DTexture8_SetPriority,
    Direct3DTexture8_GetPriority,
    Direct3DTexture8_PreLoad,
    Direct3DTexture8_GetType,
    // IDirect3DBaseTexture8 methods
    Direct3DTexture8_SetLOD,
    Direct3DTexture8_GetLOD,
    Direct3DTexture8_GetLevelCount,
    // IDirect3DTexture8 methods
    Direct3DTexture8_GetLevelDesc,
    Direct3DTexture8_GetSurfaceLevel,
    Direct3DTexture8_LockRect,
    Direct3DTexture8_UnlockRect,
    Direct3DTexture8_AddDirtyRect
};

// Implementation of IDirect3DTexture8 COM wrapper methods
static HRESULT STDMETHODCALLTYPE Direct3DTexture8_QueryInterface(IDirect3DTexture8* This, REFIID riid, void** ppvObj) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->QueryInterface(riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE Direct3DTexture8_AddRef(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    wrapper->refCount++;
    return wrapper->refCount;
}

static ULONG STDMETHODCALLTYPE Direct3DTexture8_Release(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    wrapper->refCount--;
    if (wrapper->refCount == 0) {
        wrapper->pCppInterface->Release();
        delete wrapper;
        return 0;
    }
    return wrapper->refCount;
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetDevice(IDirect3DTexture8* This, IDirect3DDevice8** ppDevice) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    
    ::IDirect3DDevice8* pCppDevice = nullptr;
    HRESULT hr = wrapper->pCppInterface->GetDevice(&pCppDevice);
    
    if (SUCCEEDED(hr) && pCppDevice) {
        // Create a device wrapper
        *ppDevice = CreateDevice8_COM_Wrapper(pCppDevice);
        if (!*ppDevice) {
            pCppDevice->Release();
            return E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_SetPrivateData(IDirect3DTexture8* This, REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetPrivateData(refguid, pData, SizeOfData, Flags);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetPrivateData(IDirect3DTexture8* This, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPrivateData(refguid, pData, pSizeOfData);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_FreePrivateData(IDirect3DTexture8* This, REFGUID refguid) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->FreePrivateData(refguid);
}

static DWORD STDMETHODCALLTYPE Direct3DTexture8_SetPriority(IDirect3DTexture8* This, DWORD PriorityNew) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetPriority(PriorityNew);
}

static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetPriority(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetPriority();
}

static void STDMETHODCALLTYPE Direct3DTexture8_PreLoad(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    wrapper->pCppInterface->PreLoad();
}

static D3DRESOURCETYPE STDMETHODCALLTYPE Direct3DTexture8_GetType(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetType();
}

static DWORD STDMETHODCALLTYPE Direct3DTexture8_SetLOD(IDirect3DTexture8* This, DWORD LODNew) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->SetLOD(LODNew);
}

static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetLOD(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetLOD();
}

static DWORD STDMETHODCALLTYPE Direct3DTexture8_GetLevelCount(IDirect3DTexture8* This) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetLevelCount();
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetLevelDesc(IDirect3DTexture8* This, UINT Level, D3DSURFACE_DESC* pDesc) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->GetLevelDesc(Level, pDesc);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_GetSurfaceLevel(IDirect3DTexture8* This, UINT Level, IDirect3DSurface8** ppSurfaceLevel) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    
    // TODO: Implement proper surface wrapping when surface wrappers are implemented
    // For now, pass through to C++ implementation
    return wrapper->pCppInterface->GetSurfaceLevel(Level, (::IDirect3DSurface8**)ppSurfaceLevel);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_LockRect(IDirect3DTexture8* This, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->LockRect(Level, pLockedRect, pRect, Flags);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_UnlockRect(IDirect3DTexture8* This, UINT Level) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->UnlockRect(Level);
}

static HRESULT STDMETHODCALLTYPE Direct3DTexture8_AddDirtyRect(IDirect3DTexture8* This, const RECT* pDirtyRect) {
    Direct3DTexture8_COM_Wrapper* wrapper = (Direct3DTexture8_COM_Wrapper*)This;
    return wrapper->pCppInterface->AddDirtyRect(pDirtyRect);
}

// Helper function to create a texture wrapper
static IDirect3DTexture8* CreateTexture8_COM_Wrapper(::IDirect3DTexture8* pCppTexture) {
    Direct3DTexture8_COM_Wrapper* wrapper = new Direct3DTexture8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DTexture8_Vtbl;
    wrapper->pCppInterface = pCppTexture;
    wrapper->refCount = 1;
    return (IDirect3DTexture8*)wrapper;
}

// Helper function to create a device wrapper
static IDirect3DDevice8* CreateDevice8_COM_Wrapper(::IDirect3DDevice8* pCppDevice) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)malloc(sizeof(Direct3DDevice8_COM_Wrapper));
    wrapper->lpVtbl = &g_Direct3DDevice8_Vtbl;
    wrapper->pCppInterface = pCppDevice;
    wrapper->refCount = 1;
    return (IDirect3DDevice8*)wrapper;
}

// Helper function to create a surface wrapper
static IDirect3DSurface8* CreateSurface8_COM_Wrapper(::IDirect3DSurface8* pCppSurface) {
    if (!pCppSurface) return nullptr;
    Direct3DSurface8_COM_Wrapper* wrapper = new Direct3DSurface8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DSurface8_Vtbl;
    wrapper->pCppInterface = pCppSurface;
    wrapper->refCount = 1;
    return (IDirect3DSurface8*)wrapper;
}

// Helper function to create a swap chain wrapper
static IDirect3DSwapChain8* CreateSwapChain8_COM_Wrapper(::IDirect3DSwapChain8* pCppSwapChain) {
    if (!pCppSwapChain) return nullptr;
    Direct3DSwapChain8_COM_Wrapper* wrapper = new Direct3DSwapChain8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DSwapChain8_Vtbl;
    wrapper->pCppInterface = pCppSwapChain;
    wrapper->refCount = 1;
    return (IDirect3DSwapChain8*)wrapper;
}

// Helper function to create a volume texture wrapper
static IDirect3DVolumeTexture8* CreateVolumeTexture8_COM_Wrapper(::IDirect3DVolumeTexture8* pCppVolumeTexture) {
    if (!pCppVolumeTexture) return nullptr;
    Direct3DVolumeTexture8_COM_Wrapper* wrapper = new Direct3DVolumeTexture8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DVolumeTexture8_Vtbl;
    wrapper->pCppInterface = pCppVolumeTexture;
    wrapper->refCount = 1;
    return (IDirect3DVolumeTexture8*)wrapper;
}

// Helper function to create a cube texture wrapper
static IDirect3DCubeTexture8* CreateCubeTexture8_COM_Wrapper(::IDirect3DCubeTexture8* pCppCubeTexture) {
    if (!pCppCubeTexture) return nullptr;
    Direct3DCubeTexture8_COM_Wrapper* wrapper = new Direct3DCubeTexture8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DCubeTexture8_Vtbl;
    wrapper->pCppInterface = pCppCubeTexture;
    wrapper->refCount = 1;
    return (IDirect3DCubeTexture8*)wrapper;
}

// Helper function to create a vertex buffer wrapper
static IDirect3DVertexBuffer8* CreateVertexBuffer8_COM_Wrapper(::IDirect3DVertexBuffer8* pCppVertexBuffer) {
    if (!pCppVertexBuffer) return nullptr;
    Direct3DVertexBuffer8_COM_Wrapper* wrapper = new Direct3DVertexBuffer8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DVertexBuffer8_Vtbl;
    wrapper->pCppInterface = pCppVertexBuffer;
    wrapper->refCount = 1;
    return (IDirect3DVertexBuffer8*)wrapper;
}

// Helper function to create an index buffer wrapper
static IDirect3DIndexBuffer8* CreateIndexBuffer8_COM_Wrapper(::IDirect3DIndexBuffer8* pCppIndexBuffer) {
    if (!pCppIndexBuffer) return nullptr;
    Direct3DIndexBuffer8_COM_Wrapper* wrapper = new Direct3DIndexBuffer8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DIndexBuffer8_Vtbl;
    wrapper->pCppInterface = pCppIndexBuffer;
    wrapper->refCount = 1;
    return (IDirect3DIndexBuffer8*)wrapper;
}

// Helper function to create a volume wrapper
static IDirect3DVolume8* CreateVolume8_COM_Wrapper(::IDirect3DVolume8* pCppVolume) {
    if (!pCppVolume) return nullptr;
    Direct3DVolume8_COM_Wrapper* wrapper = new Direct3DVolume8_COM_Wrapper();
    wrapper->lpVtbl = &g_Direct3DVolume8_Vtbl;
    wrapper->pCppInterface = pCppVolume;
    wrapper->refCount = 1;
    return (IDirect3DVolume8*)wrapper;
}

// Forward declaration for the C++ Direct3DCreate8 function
namespace dx8gl {
    extern "C" ::IDirect3D8* Direct3DCreate8_CPP(UINT SDKVersion);
}

// Main entry point - Returns C++ interface directly for game compatibility
extern "C" IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion) {
    printf("[DX8GL] Direct3DCreate8 called with SDK version 0x%08X (expected 0x%08X)\n", SDKVersion, D3D_SDK_VERSION);
    
    // Call the C++ implementation from dx8gl.cpp
    ::IDirect3D8* pCppInterface = dx8gl::Direct3DCreate8_CPP(SDKVersion);
    if (!pCppInterface) {
        printf("[DX8GL] Direct3DCreate8_CPP returned nullptr - initialization failed\n");
        return nullptr;
    }
    
    printf("[DX8GL] Direct3DCreate8_CPP returned C++ interface at %p\n", pCppInterface);
    
    // For C++17 compatibility with the game code that includes d3d8_game.h,
    // we need to return the C++ interface directly
    #ifdef DX8GL_USE_CPP_INTERFACES
        printf("[DX8GL] Returning C++ interface directly (compiled with DX8GL_USE_CPP_INTERFACES)\n");
        return pCppInterface;
    #else
        // Only create COM wrapper if explicitly needed
        const char* usecom = std::getenv("DX8GL_USE_COM");
        if (usecom && std::strcmp(usecom, "1") == 0) {
            // Create a COM wrapper for the C++ interface
            Direct3D8_COM_Wrapper* wrapper = (Direct3D8_COM_Wrapper*)malloc(sizeof(Direct3D8_COM_Wrapper));
            if (!wrapper) {
                printf("[DX8GL] Failed to allocate COM wrapper\n");
                pCppInterface->Release();
                return nullptr;
            }
            
            wrapper->lpVtbl = &g_Direct3D8_Vtbl;
            wrapper->pCppInterface = pCppInterface;
            wrapper->refCount = 1;
            
            printf("[DX8GL] Created COM wrapper at %p wrapping C++ interface at %p\n", wrapper, pCppInterface);
            return (IDirect3D8*)wrapper;
        } else {
            // Default: return C++ interface for game compatibility
            printf("[DX8GL] Returning C++ interface directly (default for game compatibility)\n");
            return pCppInterface;
        }
    #endif
}

// Alternative entry point that always returns the C++ interface
extern "C" IDirect3D8* WINAPI Direct3DCreate8_NoCOM(UINT SDKVersion) {
    return dx8gl::Direct3DCreate8_CPP(SDKVersion);
}

// Define stub vtables for the missing COM interfaces
// These are needed for linking but won't be used unless the wrappers are created

// Stub Surface8 vtable
IDirect3DSurface8Vtbl g_Direct3DSurface8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // GetContainer
    nullptr, // GetDesc
    nullptr, // LockRect
    nullptr  // UnlockRect
};

// Stub SwapChain8 vtable
IDirect3DSwapChain8Vtbl g_Direct3DSwapChain8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // Present
    nullptr  // GetBackBuffer
};

// Stub VolumeTexture8 vtable
IDirect3DVolumeTexture8Vtbl g_Direct3DVolumeTexture8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // SetPriority
    nullptr, // GetPriority
    nullptr, // PreLoad
    nullptr, // GetType
    nullptr, // SetLOD
    nullptr, // GetLOD
    nullptr, // GetLevelCount
    nullptr, // GetLevelDesc
    nullptr, // GetVolumeLevel
    nullptr, // LockBox
    nullptr, // UnlockBox
    nullptr  // AddDirtyBox
};

// Stub CubeTexture8 vtable
IDirect3DCubeTexture8Vtbl g_Direct3DCubeTexture8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // SetPriority
    nullptr, // GetPriority
    nullptr, // PreLoad
    nullptr, // GetType
    nullptr, // SetLOD
    nullptr, // GetLOD
    nullptr, // GetLevelCount
    nullptr, // GetLevelDesc
    nullptr, // GetCubeMapSurface
    nullptr, // LockRect
    nullptr, // UnlockRect
    nullptr  // AddDirtyRect
};

// Stub VertexBuffer8 vtable
IDirect3DVertexBuffer8Vtbl g_Direct3DVertexBuffer8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // SetPriority
    nullptr, // GetPriority
    nullptr, // PreLoad
    nullptr, // GetType
    nullptr, // Lock
    nullptr, // Unlock
    nullptr  // GetDesc
};

// Stub IndexBuffer8 vtable
IDirect3DIndexBuffer8Vtbl g_Direct3DIndexBuffer8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // SetPriority
    nullptr, // GetPriority
    nullptr, // PreLoad
    nullptr, // GetType
    nullptr, // Lock
    nullptr, // Unlock
    nullptr  // GetDesc
};

// Stub Volume8 vtable
IDirect3DVolume8Vtbl g_Direct3DVolume8_Vtbl = {
    nullptr, // QueryInterface
    nullptr, // AddRef
    nullptr, // Release
    nullptr, // GetDevice
    nullptr, // SetPrivateData
    nullptr, // GetPrivateData
    nullptr, // FreePrivateData
    nullptr, // GetContainer
    nullptr, // GetDesc
    nullptr, // LockBox
    nullptr  // UnlockBox
};