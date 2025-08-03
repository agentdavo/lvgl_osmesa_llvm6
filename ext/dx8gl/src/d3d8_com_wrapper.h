#ifndef DX8GL_D3D8_COM_WRAPPER_H
#define DX8GL_D3D8_COM_WRAPPER_H

/**
 * COM-style C interface wrapper for dx8gl
 * 
 * This header provides C-style COM interfaces that are compatible with
 * legacy DirectX 8 SDK headers that expect lpVtbl vtable pointers.
 * 
 * The game code expects to use macros like:
 * #define IDirect3DDevice8_SetRenderState(p,a,b) (p)->lpVtbl->SetRenderState(p,a,b)
 * 
 * This wrapper provides the necessary vtable structures and C function pointers
 * to bridge between the legacy COM-style interface and dx8gl's C++ implementation.
 */

// First include the C++ interfaces to get all type definitions
#define DX8GL_USE_CPP_INTERFACES
#include "d3d8_cpp_interfaces.h"
#undef DX8GL_USE_CPP_INTERFACES

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
// Forward declarations for C-style interfaces
typedef struct IDirect3D8 IDirect3D8;
typedef struct IDirect3DDevice8 IDirect3DDevice8;
typedef struct IDirect3DSurface8 IDirect3DSurface8;
typedef struct IDirect3DTexture8 IDirect3DTexture8;
typedef struct IDirect3DBaseTexture8 IDirect3DBaseTexture8;
typedef struct IDirect3DVertexBuffer8 IDirect3DVertexBuffer8;
typedef struct IDirect3DIndexBuffer8 IDirect3DIndexBuffer8;
typedef struct IDirect3DSwapChain8 IDirect3DSwapChain8;
typedef struct IDirect3DVolumeTexture8 IDirect3DVolumeTexture8;
typedef struct IDirect3DCubeTexture8 IDirect3DCubeTexture8;
typedef struct IDirect3DVolume8 IDirect3DVolume8;
typedef struct IDirect3DResource8 IDirect3DResource8;
#else
// In C++ mode, these are already defined in d3d8_cpp_interfaces.h
#endif

// Vtable structures - these define the function pointer tables

// IUnknown vtable
typedef struct IUnknownVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUnknown* This, REFIID riid, void** ppvObj);
    ULONG (STDMETHODCALLTYPE *AddRef)(IUnknown* This);
    ULONG (STDMETHODCALLTYPE *Release)(IUnknown* This);
} IUnknownVtbl;

// IDirect3D8 vtable
typedef struct IDirect3D8Vtbl {
    // IUnknown methods
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3D8* This, REFIID riid, void** ppvObj);
    ULONG (STDMETHODCALLTYPE *AddRef)(IDirect3D8* This);
    ULONG (STDMETHODCALLTYPE *Release)(IDirect3D8* This);
    
    // IDirect3D8 methods
    HRESULT (STDMETHODCALLTYPE *RegisterSoftwareDevice)(IDirect3D8* This, void* pInitializeFunction);
    UINT (STDMETHODCALLTYPE *GetAdapterCount)(IDirect3D8* This);
    HRESULT (STDMETHODCALLTYPE *GetAdapterIdentifier)(IDirect3D8* This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
    UINT (STDMETHODCALLTYPE *GetAdapterModeCount)(IDirect3D8* This, UINT Adapter);
    HRESULT (STDMETHODCALLTYPE *EnumAdapterModes)(IDirect3D8* This, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
    HRESULT (STDMETHODCALLTYPE *GetAdapterDisplayMode)(IDirect3D8* This, UINT Adapter, D3DDISPLAYMODE* pMode);
    HRESULT (STDMETHODCALLTYPE *CheckDeviceType)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
    HRESULT (STDMETHODCALLTYPE *CheckDeviceFormat)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    HRESULT (STDMETHODCALLTYPE *CheckDeviceMultiSampleType)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType);
    HRESULT (STDMETHODCALLTYPE *CheckDepthStencilMatch)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    HRESULT (STDMETHODCALLTYPE *GetDeviceCaps)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
    HMONITOR (STDMETHODCALLTYPE *GetAdapterMonitor)(IDirect3D8* This, UINT Adapter);
    HRESULT (STDMETHODCALLTYPE *CreateDevice)(IDirect3D8* This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
} IDirect3D8Vtbl;

// IDirect3DDevice8 vtable
typedef struct IDirect3DDevice8Vtbl {
    // IUnknown methods
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice8* This, REFIID riid, void** ppvObj);
    ULONG (STDMETHODCALLTYPE *AddRef)(IDirect3DDevice8* This);
    ULONG (STDMETHODCALLTYPE *Release)(IDirect3DDevice8* This);
    
    // IDirect3DDevice8 methods
    HRESULT (STDMETHODCALLTYPE *TestCooperativeLevel)(IDirect3DDevice8* This);
    UINT (STDMETHODCALLTYPE *GetAvailableTextureMem)(IDirect3DDevice8* This);
    HRESULT (STDMETHODCALLTYPE *ResourceManagerDiscardBytes)(IDirect3DDevice8* This, DWORD Bytes);
    HRESULT (STDMETHODCALLTYPE *GetDirect3D)(IDirect3DDevice8* This, IDirect3D8** ppD3D8);
    HRESULT (STDMETHODCALLTYPE *GetDeviceCaps)(IDirect3DDevice8* This, D3DCAPS8* pCaps);
    HRESULT (STDMETHODCALLTYPE *GetDisplayMode)(IDirect3DDevice8* This, D3DDISPLAYMODE* pMode);
    HRESULT (STDMETHODCALLTYPE *GetCreationParameters)(IDirect3DDevice8* This, D3DDEVICE_CREATION_PARAMETERS* pParameters);
    HRESULT (STDMETHODCALLTYPE *SetCursorProperties)(IDirect3DDevice8* This, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap);
    void (STDMETHODCALLTYPE *SetCursorPosition)(IDirect3DDevice8* This, int X, int Y, DWORD Flags);
    BOOL (STDMETHODCALLTYPE *ShowCursor)(IDirect3DDevice8* This, BOOL bShow);
    HRESULT (STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain);
    HRESULT (STDMETHODCALLTYPE *Reset)(IDirect3DDevice8* This, D3DPRESENT_PARAMETERS* pPresentationParameters);
    HRESULT (STDMETHODCALLTYPE *Present)(IDirect3DDevice8* This, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
    HRESULT (STDMETHODCALLTYPE *GetBackBuffer)(IDirect3DDevice8* This, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer);
    HRESULT (STDMETHODCALLTYPE *GetRasterStatus)(IDirect3DDevice8* This, D3DRASTER_STATUS* pRasterStatus);
    void (STDMETHODCALLTYPE *SetGammaRamp)(IDirect3DDevice8* This, DWORD Flags, const D3DGAMMARAMP* pRamp);
    void (STDMETHODCALLTYPE *GetGammaRamp)(IDirect3DDevice8* This, D3DGAMMARAMP* pRamp);
    HRESULT (STDMETHODCALLTYPE *CreateTexture)(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture);
    HRESULT (STDMETHODCALLTYPE *CreateVolumeTexture)(IDirect3DDevice8* This, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture);
    HRESULT (STDMETHODCALLTYPE *CreateCubeTexture)(IDirect3DDevice8* This, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture);
    HRESULT (STDMETHODCALLTYPE *CreateVertexBuffer)(IDirect3DDevice8* This, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer);
    HRESULT (STDMETHODCALLTYPE *CreateIndexBuffer)(IDirect3DDevice8* This, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer);
    HRESULT (STDMETHODCALLTYPE *CreateRenderTarget)(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface);
    HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface)(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface);
    HRESULT (STDMETHODCALLTYPE *CreateImageSurface)(IDirect3DDevice8* This, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface);
    HRESULT (STDMETHODCALLTYPE *CopyRects)(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, const RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPointsArray);
    HRESULT (STDMETHODCALLTYPE *UpdateTexture)(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
    HRESULT (STDMETHODCALLTYPE *GetFrontBuffer)(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface);
    HRESULT (STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
    HRESULT (STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget);
    HRESULT (STDMETHODCALLTYPE *GetDepthStencilSurface)(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface);
    HRESULT (STDMETHODCALLTYPE *BeginScene)(IDirect3DDevice8* This);
    HRESULT (STDMETHODCALLTYPE *EndScene)(IDirect3DDevice8* This);
    HRESULT (STDMETHODCALLTYPE *Clear)(IDirect3DDevice8* This, DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
    HRESULT (STDMETHODCALLTYPE *SetTransform)(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
    HRESULT (STDMETHODCALLTYPE *GetTransform)(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
    HRESULT (STDMETHODCALLTYPE *MultiplyTransform)(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
    HRESULT (STDMETHODCALLTYPE *SetViewport)(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport);
    HRESULT (STDMETHODCALLTYPE *GetViewport)(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport);
    HRESULT (STDMETHODCALLTYPE *SetMaterial)(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial);
    HRESULT (STDMETHODCALLTYPE *GetMaterial)(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial);
    HRESULT (STDMETHODCALLTYPE *SetLight)(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight);
    HRESULT (STDMETHODCALLTYPE *GetLight)(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight);
    HRESULT (STDMETHODCALLTYPE *LightEnable)(IDirect3DDevice8* This, DWORD Index, BOOL Enable);
    HRESULT (STDMETHODCALLTYPE *GetLightEnable)(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable);
    HRESULT (STDMETHODCALLTYPE *SetClipPlane)(IDirect3DDevice8* This, DWORD Index, const float* pPlane);
    HRESULT (STDMETHODCALLTYPE *GetClipPlane)(IDirect3DDevice8* This, DWORD Index, float* pPlane);
    HRESULT (STDMETHODCALLTYPE *SetRenderState)(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD Value);
    HRESULT (STDMETHODCALLTYPE *GetRenderState)(IDirect3DDevice8* This, D3DRENDERSTATETYPE State, DWORD* pValue);
    HRESULT (STDMETHODCALLTYPE *BeginStateBlock)(IDirect3DDevice8* This);
    HRESULT (STDMETHODCALLTYPE *EndStateBlock)(IDirect3DDevice8* This, DWORD* pToken);
    HRESULT (STDMETHODCALLTYPE *ApplyStateBlock)(IDirect3DDevice8* This, DWORD Token);
    HRESULT (STDMETHODCALLTYPE *CaptureStateBlock)(IDirect3DDevice8* This, DWORD Token);
    HRESULT (STDMETHODCALLTYPE *DeleteStateBlock)(IDirect3DDevice8* This, DWORD Token);
    HRESULT (STDMETHODCALLTYPE *CreateStateBlock)(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken);
    HRESULT (STDMETHODCALLTYPE *SetClipStatus)(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus);
    HRESULT (STDMETHODCALLTYPE *GetClipStatus)(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus);
    HRESULT (STDMETHODCALLTYPE *GetTexture)(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture);
    HRESULT (STDMETHODCALLTYPE *SetTexture)(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8* pTexture);
    HRESULT (STDMETHODCALLTYPE *GetTextureStageState)(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
    HRESULT (STDMETHODCALLTYPE *SetTextureStageState)(IDirect3DDevice8* This, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    HRESULT (STDMETHODCALLTYPE *ValidateDevice)(IDirect3DDevice8* This, DWORD* pNumPasses);
    HRESULT (STDMETHODCALLTYPE *GetInfo)(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
    HRESULT (STDMETHODCALLTYPE *SetPaletteEntries)(IDirect3DDevice8* This, UINT PaletteNumber, const PALETTEENTRY* pEntries);
    HRESULT (STDMETHODCALLTYPE *GetPaletteEntries)(IDirect3DDevice8* This, UINT PaletteNumber, PALETTEENTRY* pEntries);
    HRESULT (STDMETHODCALLTYPE *SetCurrentTexturePalette)(IDirect3DDevice8* This, UINT PaletteNumber);
    HRESULT (STDMETHODCALLTYPE *GetCurrentTexturePalette)(IDirect3DDevice8* This, UINT* PaletteNumber);
    HRESULT (STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
    HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
    HRESULT (STDMETHODCALLTYPE *DrawPrimitiveUP)(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveUP)(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    HRESULT (STDMETHODCALLTYPE *ProcessVertices)(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);
    HRESULT (STDMETHODCALLTYPE *CreateVertexShader)(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage);
    HRESULT (STDMETHODCALLTYPE *SetVertexShader)(IDirect3DDevice8* This, DWORD Handle);
    HRESULT (STDMETHODCALLTYPE *GetVertexShader)(IDirect3DDevice8* This, DWORD* pHandle);
    HRESULT (STDMETHODCALLTYPE *DeleteVertexShader)(IDirect3DDevice8* This, DWORD Handle);
    HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstant)(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
    HRESULT (STDMETHODCALLTYPE *GetVertexShaderConstant)(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
    HRESULT (STDMETHODCALLTYPE *GetVertexShaderDeclaration)(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
    HRESULT (STDMETHODCALLTYPE *GetVertexShaderFunction)(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
    HRESULT (STDMETHODCALLTYPE *SetStreamSource)(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride);
    HRESULT (STDMETHODCALLTYPE *GetStreamSource)(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
    HRESULT (STDMETHODCALLTYPE *SetIndices)(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
    HRESULT (STDMETHODCALLTYPE *GetIndices)(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);
    HRESULT (STDMETHODCALLTYPE *CreatePixelShader)(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle);
    HRESULT (STDMETHODCALLTYPE *SetPixelShader)(IDirect3DDevice8* This, DWORD Handle);
    HRESULT (STDMETHODCALLTYPE *GetPixelShader)(IDirect3DDevice8* This, DWORD* pHandle);
    HRESULT (STDMETHODCALLTYPE *DeletePixelShader)(IDirect3DDevice8* This, DWORD Handle);
    HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstant)(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount);
    HRESULT (STDMETHODCALLTYPE *GetPixelShaderConstant)(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount);
    HRESULT (STDMETHODCALLTYPE *GetPixelShaderFunction)(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData);
    HRESULT (STDMETHODCALLTYPE *DrawRectPatch)(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo);
    HRESULT (STDMETHODCALLTYPE *DrawTriPatch)(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo);
    HRESULT (STDMETHODCALLTYPE *DeletePatch)(IDirect3DDevice8* This, UINT Handle);
} IDirect3DDevice8Vtbl;

// IDirect3DTexture8 vtable
typedef struct IDirect3DTexture8Vtbl {
    // IUnknown methods
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DTexture8* This, REFIID riid, void** ppvObj);
    ULONG (STDMETHODCALLTYPE *AddRef)(IDirect3DTexture8* This);
    ULONG (STDMETHODCALLTYPE *Release)(IDirect3DTexture8* This);
    
    // IDirect3DResource8 methods
    HRESULT (STDMETHODCALLTYPE *GetDevice)(IDirect3DTexture8* This, IDirect3DDevice8** ppDevice);
    HRESULT (STDMETHODCALLTYPE *SetPrivateData)(IDirect3DTexture8* This, REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);
    HRESULT (STDMETHODCALLTYPE *GetPrivateData)(IDirect3DTexture8* This, REFGUID refguid, void* pData, DWORD* pSizeOfData);
    HRESULT (STDMETHODCALLTYPE *FreePrivateData)(IDirect3DTexture8* This, REFGUID refguid);
    DWORD (STDMETHODCALLTYPE *SetPriority)(IDirect3DTexture8* This, DWORD PriorityNew);
    DWORD (STDMETHODCALLTYPE *GetPriority)(IDirect3DTexture8* This);
    void (STDMETHODCALLTYPE *PreLoad)(IDirect3DTexture8* This);
    D3DRESOURCETYPE (STDMETHODCALLTYPE *GetType)(IDirect3DTexture8* This);
    
    // IDirect3DBaseTexture8 methods
    DWORD (STDMETHODCALLTYPE *SetLOD)(IDirect3DTexture8* This, DWORD LODNew);
    DWORD (STDMETHODCALLTYPE *GetLOD)(IDirect3DTexture8* This);
    DWORD (STDMETHODCALLTYPE *GetLevelCount)(IDirect3DTexture8* This);
    
    // IDirect3DTexture8 methods
    HRESULT (STDMETHODCALLTYPE *GetLevelDesc)(IDirect3DTexture8* This, UINT Level, D3DSURFACE_DESC* pDesc);
    HRESULT (STDMETHODCALLTYPE *GetSurfaceLevel)(IDirect3DTexture8* This, UINT Level, IDirect3DSurface8** ppSurfaceLevel);
    HRESULT (STDMETHODCALLTYPE *LockRect)(IDirect3DTexture8* This, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
    HRESULT (STDMETHODCALLTYPE *UnlockRect)(IDirect3DTexture8* This, UINT Level);
    HRESULT (STDMETHODCALLTYPE *AddDirtyRect)(IDirect3DTexture8* This, const RECT* pDirtyRect);
} IDirect3DTexture8Vtbl;

// C-style interface structures with lpVtbl pointers
// These need unique names to avoid conflicts with C++ interfaces

#ifndef __cplusplus
// In C code, use the standard names
struct IDirect3D8 {
    IDirect3D8Vtbl* lpVtbl;
};

struct IDirect3DDevice8 {
    IDirect3DDevice8Vtbl* lpVtbl;
};

struct IDirect3DTexture8 {
    IDirect3DTexture8Vtbl* lpVtbl;
};
#else
// In C++ code, these are already defined as C++ classes
// The COM wrapper will cast between the two
#endif

// Macro definitions for C-style interface usage
// These match the DirectX 8 SDK header patterns

// IDirect3D8 methods
#define IDirect3D8_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3D8_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDirect3D8_Release(p) (p)->lpVtbl->Release(p)
#define IDirect3D8_RegisterSoftwareDevice(p,a) (p)->lpVtbl->RegisterSoftwareDevice(p,a)
#define IDirect3D8_GetAdapterCount(p) (p)->lpVtbl->GetAdapterCount(p)
#define IDirect3D8_GetAdapterIdentifier(p,a,b,c) (p)->lpVtbl->GetAdapterIdentifier(p,a,b,c)
#define IDirect3D8_GetAdapterModeCount(p,a) (p)->lpVtbl->GetAdapterModeCount(p,a)
#define IDirect3D8_EnumAdapterModes(p,a,b,c) (p)->lpVtbl->EnumAdapterModes(p,a,b,c)
#define IDirect3D8_GetAdapterDisplayMode(p,a,b) (p)->lpVtbl->GetAdapterDisplayMode(p,a,b)
#define IDirect3D8_CheckDeviceType(p,a,b,c,d,e) (p)->lpVtbl->CheckDeviceType(p,a,b,c,d,e)
#define IDirect3D8_CheckDeviceFormat(p,a,b,c,d,e,f) (p)->lpVtbl->CheckDeviceFormat(p,a,b,c,d,e,f)
#define IDirect3D8_CheckDeviceMultiSampleType(p,a,b,c,d,e) (p)->lpVtbl->CheckDeviceMultiSampleType(p,a,b,c,d,e)
#define IDirect3D8_CheckDepthStencilMatch(p,a,b,c,d,e) (p)->lpVtbl->CheckDepthStencilMatch(p,a,b,c,d,e)
#define IDirect3D8_GetDeviceCaps(p,a,b,c) (p)->lpVtbl->GetDeviceCaps(p,a,b,c)
#define IDirect3D8_GetAdapterMonitor(p,a) (p)->lpVtbl->GetAdapterMonitor(p,a)
#define IDirect3D8_CreateDevice(p,a,b,c,d,e,f) (p)->lpVtbl->CreateDevice(p,a,b,c,d,e,f)

// IDirect3DDevice8 methods (partial list - add more as needed)
#define IDirect3DDevice8_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DDevice8_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDirect3DDevice8_Release(p) (p)->lpVtbl->Release(p)
#define IDirect3DDevice8_TestCooperativeLevel(p) (p)->lpVtbl->TestCooperativeLevel(p)
#define IDirect3DDevice8_GetAvailableTextureMem(p) (p)->lpVtbl->GetAvailableTextureMem(p)
#define IDirect3DDevice8_ResourceManagerDiscardBytes(p,a) (p)->lpVtbl->ResourceManagerDiscardBytes(p,a)
#define IDirect3DDevice8_GetDirect3D(p,a) (p)->lpVtbl->GetDirect3D(p,a)
#define IDirect3DDevice8_GetDeviceCaps(p,a) (p)->lpVtbl->GetDeviceCaps(p,a)
#define IDirect3DDevice8_GetDisplayMode(p,a) (p)->lpVtbl->GetDisplayMode(p,a)
#define IDirect3DDevice8_GetCreationParameters(p,a) (p)->lpVtbl->GetCreationParameters(p,a)
#define IDirect3DDevice8_SetCursorProperties(p,a,b,c) (p)->lpVtbl->SetCursorProperties(p,a,b,c)
#define IDirect3DDevice8_SetCursorPosition(p,a,b,c) (p)->lpVtbl->SetCursorPosition(p,a,b,c)
#define IDirect3DDevice8_ShowCursor(p,a) (p)->lpVtbl->ShowCursor(p,a)
#define IDirect3DDevice8_CreateAdditionalSwapChain(p,a,b) (p)->lpVtbl->CreateAdditionalSwapChain(p,a,b)
#define IDirect3DDevice8_Reset(p,a) (p)->lpVtbl->Reset(p,a)
#define IDirect3DDevice8_Present(p,a,b,c,d) (p)->lpVtbl->Present(p,a,b,c,d)
#define IDirect3DDevice8_GetBackBuffer(p,a,b,c) (p)->lpVtbl->GetBackBuffer(p,a,b,c)
#define IDirect3DDevice8_GetRasterStatus(p,a) (p)->lpVtbl->GetRasterStatus(p,a)
#define IDirect3DDevice8_SetGammaRamp(p,a,b) (p)->lpVtbl->SetGammaRamp(p,a,b)
#define IDirect3DDevice8_GetGammaRamp(p,a) (p)->lpVtbl->GetGammaRamp(p,a)
#define IDirect3DDevice8_CreateTexture(p,a,b,c,d,e,f,g) (p)->lpVtbl->CreateTexture(p,a,b,c,d,e,f,g)
#define IDirect3DDevice8_CreateVertexBuffer(p,a,b,c,d,e) (p)->lpVtbl->CreateVertexBuffer(p,a,b,c,d,e)
#define IDirect3DDevice8_CreateIndexBuffer(p,a,b,c,d,e) (p)->lpVtbl->CreateIndexBuffer(p,a,b,c,d,e)
#define IDirect3DDevice8_BeginScene(p) (p)->lpVtbl->BeginScene(p)
#define IDirect3DDevice8_EndScene(p) (p)->lpVtbl->EndScene(p)
#define IDirect3DDevice8_Clear(p,a,b,c,d,e,f) (p)->lpVtbl->Clear(p,a,b,c,d,e,f)
#define IDirect3DDevice8_SetTransform(p,a,b) (p)->lpVtbl->SetTransform(p,a,b)
#define IDirect3DDevice8_GetTransform(p,a,b) (p)->lpVtbl->GetTransform(p,a,b)
#define IDirect3DDevice8_SetViewport(p,a) (p)->lpVtbl->SetViewport(p,a)
#define IDirect3DDevice8_GetViewport(p,a) (p)->lpVtbl->GetViewport(p,a)
#define IDirect3DDevice8_SetMaterial(p,a) (p)->lpVtbl->SetMaterial(p,a)
#define IDirect3DDevice8_GetMaterial(p,a) (p)->lpVtbl->GetMaterial(p,a)
#define IDirect3DDevice8_SetLight(p,a,b) (p)->lpVtbl->SetLight(p,a,b)
#define IDirect3DDevice8_GetLight(p,a,b) (p)->lpVtbl->GetLight(p,a,b)
#define IDirect3DDevice8_LightEnable(p,a,b) (p)->lpVtbl->LightEnable(p,a,b)
#define IDirect3DDevice8_GetLightEnable(p,a,b) (p)->lpVtbl->GetLightEnable(p,a,b)
#define IDirect3DDevice8_SetRenderState(p,a,b) (p)->lpVtbl->SetRenderState(p,a,b)
#define IDirect3DDevice8_GetRenderState(p,a,b) (p)->lpVtbl->GetRenderState(p,a,b)
#define IDirect3DDevice8_SetTexture(p,a,b) (p)->lpVtbl->SetTexture(p,a,b)
#define IDirect3DDevice8_GetTexture(p,a,b) (p)->lpVtbl->GetTexture(p,a,b)
#define IDirect3DDevice8_SetTextureStageState(p,a,b,c) (p)->lpVtbl->SetTextureStageState(p,a,b,c)
#define IDirect3DDevice8_GetTextureStageState(p,a,b,c) (p)->lpVtbl->GetTextureStageState(p,a,b,c)
#define IDirect3DDevice8_DrawPrimitive(p,a,b,c) (p)->lpVtbl->DrawPrimitive(p,a,b,c)
#define IDirect3DDevice8_DrawIndexedPrimitive(p,a,b,c,d,e) (p)->lpVtbl->DrawIndexedPrimitive(p,a,b,c,d,e)
#define IDirect3DDevice8_DrawPrimitiveUP(p,a,b,c,d) (p)->lpVtbl->DrawPrimitiveUP(p,a,b,c,d)
#define IDirect3DDevice8_DrawIndexedPrimitiveUP(p,a,b,c,d,e,f,g,h) (p)->lpVtbl->DrawIndexedPrimitiveUP(p,a,b,c,d,e,f,g,h)
#define IDirect3DDevice8_SetVertexShader(p,a) (p)->lpVtbl->SetVertexShader(p,a)
#define IDirect3DDevice8_GetVertexShader(p,a) (p)->lpVtbl->GetVertexShader(p,a)
#define IDirect3DDevice8_SetStreamSource(p,a,b,c) (p)->lpVtbl->SetStreamSource(p,a,b,c)
#define IDirect3DDevice8_GetStreamSource(p,a,b,c) (p)->lpVtbl->GetStreamSource(p,a,b,c)
#define IDirect3DDevice8_SetIndices(p,a,b) (p)->lpVtbl->SetIndices(p,a,b)
#define IDirect3DDevice8_GetIndices(p,a,b) (p)->lpVtbl->GetIndices(p,a,b)

// IDirect3DTexture8 methods
#define IDirect3DTexture8_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DTexture8_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IDirect3DTexture8_Release(p) (p)->lpVtbl->Release(p)
#define IDirect3DTexture8_GetDevice(p,a) (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DTexture8_SetPrivateData(p,a,b,c,d) (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
#define IDirect3DTexture8_GetPrivateData(p,a,b,c) (p)->lpVtbl->GetPrivateData(p,a,b,c)
#define IDirect3DTexture8_FreePrivateData(p,a) (p)->lpVtbl->FreePrivateData(p,a)
#define IDirect3DTexture8_SetPriority(p,a) (p)->lpVtbl->SetPriority(p,a)
#define IDirect3DTexture8_GetPriority(p) (p)->lpVtbl->GetPriority(p)
#define IDirect3DTexture8_PreLoad(p) (p)->lpVtbl->PreLoad(p)
#define IDirect3DTexture8_GetType(p) (p)->lpVtbl->GetType(p)
#define IDirect3DTexture8_SetLOD(p,a) (p)->lpVtbl->SetLOD(p,a)
#define IDirect3DTexture8_GetLOD(p) (p)->lpVtbl->GetLOD(p)
#define IDirect3DTexture8_GetLevelCount(p) (p)->lpVtbl->GetLevelCount(p)
#define IDirect3DTexture8_GetLevelDesc(p,a,b) (p)->lpVtbl->GetLevelDesc(p,a,b)
#define IDirect3DTexture8_GetSurfaceLevel(p,a,b) (p)->lpVtbl->GetSurfaceLevel(p,a,b)
#define IDirect3DTexture8_LockRect(p,a,b,c,d) (p)->lpVtbl->LockRect(p,a,b,c,d)
#define IDirect3DTexture8_UnlockRect(p,a) (p)->lpVtbl->UnlockRect(p,a)
#define IDirect3DTexture8_AddDirtyRect(p,a) (p)->lpVtbl->AddDirtyRect(p,a)

// Export the main Direct3D creation function
DX8GL_API IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion);

#ifdef __cplusplus
}
#endif

#endif // DX8GL_D3D8_COM_WRAPPER_H