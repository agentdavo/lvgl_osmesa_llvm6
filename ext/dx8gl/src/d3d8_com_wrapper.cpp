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

#include <cstdlib>
#include <cstring>

// Forward declarations for wrapper structures
struct Direct3D8_COM_Wrapper;
struct Direct3DDevice8_COM_Wrapper;
struct Direct3DTexture8_COM_Wrapper;

// Forward declarations for wrapper creation functions
static IDirect3DTexture8* CreateTexture8_COM_Wrapper(::IDirect3DTexture8* pCppTexture);

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
static IDirect3D8Vtbl g_Direct3D8_Vtbl = {
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
// These are defined in d3d8_missing_stubs.cpp with extern "C" linkage
extern "C" {
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CopyRects(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, const RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPointsArray);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_UpdateTexture(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetFrontBuffer(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetDepthStencilSurface(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface);
}
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginScene(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndScene(IDirect3DDevice8* This);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Clear(IDirect3DDevice8* This, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
// More functions defined in d3d8_missing_stubs.cpp with extern "C" linkage
extern "C" {
HRESULT STDMETHODCALLTYPE Direct3DDevice8_MultiplyTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetViewport(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetViewport(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetMaterial(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetMaterial(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetLight(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLight(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_LightEnable(IDirect3DDevice8* This, DWORD Index, BOOL Enable);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetLightEnable(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipPlane(IDirect3DDevice8* This, DWORD Index, const float* pPlane);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipPlane(IDirect3DDevice8* This, DWORD Index, float* pPlane);
}
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD Value);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetRenderState(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD* pValue);
// State block and clip status functions defined in d3d8_missing_stubs.cpp with extern "C" linkage
extern "C" {
HRESULT STDMETHODCALLTYPE Direct3DDevice8_BeginStateBlock(IDirect3DDevice8* This);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_EndStateBlock(IDirect3DDevice8* This, DWORD* pToken);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_ApplyStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CaptureStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateStateBlock(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetClipStatus(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetClipStatus(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus);
}
// Texture functions - GetTexture, GetTextureStageState and SetTextureStageState are static implementations below
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8* pTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetTextureStageState(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
// Validation and palette functions defined in d3d8_missing_stubs.cpp with extern "C" linkage
extern "C" {
HRESULT STDMETHODCALLTYPE Direct3DDevice8_ValidateDevice(IDirect3DDevice8* This, DWORD* pNumPasses);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetInfo(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, const PALETTEENTRY* pEntries);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, PALETTEENTRY* pEntries);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetCurrentTexturePalette(IDirect3DDevice8* This, UINT PaletteNumber);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetCurrentTexturePalette(IDirect3DDevice8* This, UINT* PaletteNumber);
}
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
static HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitive(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
// Drawing, shader and patch functions defined in d3d8_missing_stubs.cpp with extern "C" linkage
extern "C" {
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawIndexedPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_ProcessVertices(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexShader(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShader(IDirect3DDevice8* This, DWORD* pHandle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeleteVertexShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderDeclaration(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetVertexShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreatePixelShader(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShader(IDirect3DDevice8* This, DWORD* pHandle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePixelShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_SetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetPixelShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawRectPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DrawTriPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo);
HRESULT STDMETHODCALLTYPE Direct3DDevice8_DeletePatch(IDirect3DDevice8* This, UINT Handle);
}

// Static vtable for IDirect3DDevice8
static IDirect3DDevice8Vtbl g_Direct3DDevice8_Vtbl = {
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
    // TODO: Need to wrap the returned IDirect3D8 interface
    return E_NOTIMPL;
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
    // TODO: Need to unwrap the surface interface
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE Direct3DDevice8_SetCursorPosition(IDirect3DDevice8* This, int X, int Y, DWORD Flags) {
    DEVICE_METHOD_FORWARD_VOID(SetCursorPosition, X, Y, Flags)
}

static BOOL STDMETHODCALLTYPE Direct3DDevice8_ShowCursor(IDirect3DDevice8* This, BOOL bShow) {
    Direct3DDevice8_COM_Wrapper* wrapper = (Direct3DDevice8_COM_Wrapper*)This;
    return wrapper->pCppInterface->ShowCursor(bShow);
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateAdditionalSwapChain(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain) {
    // TODO: Need to wrap the returned swap chain
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Reset(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    DEVICE_METHOD_FORWARD(Reset, pPresentationParameters)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_Present(IDirect3DDevice8* This, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    DEVICE_METHOD_FORWARD(Present, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion)
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetBackBuffer(IDirect3DDevice8* This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    // TODO: Need to wrap the returned surface
    return E_NOTIMPL;
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
    // TODO: Need to wrap the returned texture
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateCubeTexture(IDirect3DDevice8* This, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {
    // TODO: Need to wrap the returned texture
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateVertexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    // TODO: Need to wrap the returned buffer
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateIndexBuffer(IDirect3DDevice8* This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    // TODO: Need to wrap the returned buffer
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateRenderTarget(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    // TODO: Need to wrap the returned surface
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateDepthStencilSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    // TODO: Need to wrap the returned surface
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_CreateImageSurface(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    // TODO: Need to wrap the returned surface
    return E_NOTIMPL;
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
    // TODO: Need to unwrap the texture interface
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Direct3DDevice8_GetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture) {
    // TODO: Need to wrap the returned texture
    return E_NOTIMPL;
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

// External declarations for functions implemented in d3d8_missing_stubs.cpp
extern "C" {
HRESULT Direct3DDevice8_CopyRects(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, const RECT* pSourceRects, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPoints);
HRESULT Direct3DDevice8_UpdateTexture(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
HRESULT Direct3DDevice8_GetFrontBuffer(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface);
HRESULT Direct3DDevice8_SetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
HRESULT Direct3DDevice8_GetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget);
HRESULT Direct3DDevice8_GetDepthStencilSurface(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface);
HRESULT Direct3DDevice8_MultiplyTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
HRESULT Direct3DDevice8_SetViewport(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport);
HRESULT Direct3DDevice8_GetViewport(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport);
HRESULT Direct3DDevice8_SetMaterial(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial);
HRESULT Direct3DDevice8_GetMaterial(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial);
HRESULT Direct3DDevice8_SetLight(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight);
HRESULT Direct3DDevice8_GetLight(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight);
HRESULT Direct3DDevice8_LightEnable(IDirect3DDevice8* This, DWORD LightIndex, BOOL bEnable);
HRESULT Direct3DDevice8_GetLightEnable(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable);
HRESULT Direct3DDevice8_SetClipPlane(IDirect3DDevice8* This, DWORD Index, const float* pPlane);
HRESULT Direct3DDevice8_GetClipPlane(IDirect3DDevice8* This, DWORD Index, float* pPlane);
HRESULT Direct3DDevice8_BeginStateBlock(IDirect3DDevice8* This);
HRESULT Direct3DDevice8_EndStateBlock(IDirect3DDevice8* This, DWORD* pToken);
HRESULT Direct3DDevice8_ApplyStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT Direct3DDevice8_CaptureStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT Direct3DDevice8_DeleteStateBlock(IDirect3DDevice8* This, DWORD Token);
HRESULT Direct3DDevice8_CreateStateBlock(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken);
// GetTexture, GetTextureStageState and SetTextureStageState are implemented as static functions above
HRESULT Direct3DDevice8_SetClipStatus(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus);
HRESULT Direct3DDevice8_GetClipStatus(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus);
HRESULT Direct3DDevice8_ValidateDevice(IDirect3DDevice8* This, DWORD* pNumPasses);
HRESULT Direct3DDevice8_GetInfo(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
HRESULT Direct3DDevice8_SetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, const PALETTEENTRY* pEntries);
HRESULT Direct3DDevice8_GetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber, PALETTEENTRY* pEntries);
HRESULT Direct3DDevice8_SetCurrentTexturePalette(IDirect3DDevice8* This, UINT PaletteNumber);
HRESULT Direct3DDevice8_GetCurrentTexturePalette(IDirect3DDevice8* This, UINT* pPaletteNumber);
HRESULT Direct3DDevice8_DrawPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT Direct3DDevice8_DrawIndexedPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT Direct3DDevice8_ProcessVertices(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);
HRESULT Direct3DDevice8_CreateVertexShader(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage);
HRESULT Direct3DDevice8_SetVertexShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT Direct3DDevice8_GetVertexShader(IDirect3DDevice8* This, DWORD* pHandle);
HRESULT Direct3DDevice8_DeleteVertexShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT Direct3DDevice8_SetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
HRESULT Direct3DDevice8_GetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
HRESULT Direct3DDevice8_GetVertexShaderDeclaration(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT Direct3DDevice8_GetVertexShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT Direct3DDevice8_SetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride);
HRESULT Direct3DDevice8_GetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
HRESULT Direct3DDevice8_SetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
HRESULT Direct3DDevice8_GetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);
HRESULT Direct3DDevice8_CreatePixelShader(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle);
HRESULT Direct3DDevice8_GetPixelShader(IDirect3DDevice8* This, DWORD* pHandle);
HRESULT Direct3DDevice8_SetPixelShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT Direct3DDevice8_DeletePixelShader(IDirect3DDevice8* This, DWORD Handle);
HRESULT Direct3DDevice8_GetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
HRESULT Direct3DDevice8_SetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
HRESULT Direct3DDevice8_GetPixelShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
HRESULT Direct3DDevice8_DrawRectPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo);
HRESULT Direct3DDevice8_DrawTriPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo);
HRESULT Direct3DDevice8_DeletePatch(IDirect3DDevice8* This, UINT Handle);
}


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
static IDirect3DTexture8Vtbl g_Direct3DTexture8_Vtbl = {
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
    // TODO: Need to wrap the returned device
    return E_NOTIMPL;
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
    // TODO: Need to wrap the returned surface
    return E_NOTIMPL;
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