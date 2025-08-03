#ifndef DX8GL_D3D8_CPP_INTERFACES_H
#define DX8GL_D3D8_CPP_INTERFACES_H

// Pure C++ interface definitions for DirectX 8
// These replace the C-style vtable structs with proper C++ abstract classes

#include "d3d8_types.h"
#include "d3d8_constants.h"

// Forward declarations for all interfaces
struct IDirect3D8;
struct IDirect3DDevice8;
struct IDirect3DSurface8;
struct IDirect3DTexture8;
struct IDirect3DBaseTexture8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;
struct IDirect3DSwapChain8;
struct IDirect3DVolumeTexture8;
struct IDirect3DCubeTexture8;
struct IDirect3DVolume8;
struct IDirect3DResource8;

// Pointer typedefs for compatibility
typedef IDirect3D8* LPDIRECT3D8;
typedef IDirect3DDevice8* LPDIRECT3DDEVICE8;
typedef IDirect3DSurface8* LPDIRECT3DSURFACE8;
typedef IDirect3DTexture8* LPDIRECT3DTEXTURE8;
typedef IDirect3DVertexBuffer8* LPDIRECT3DVERTEXBUFFER8;
typedef IDirect3DIndexBuffer8* LPDIRECT3DINDEXBUFFER8;
typedef IDirect3DBaseTexture8* LPDIRECT3DBASETEXTURE8;

// Interface IDs
extern const IID IID_IUnknown;
extern const IID IID_IDirect3D8;
extern const IID IID_IDirect3DDevice8;
extern const IID IID_IDirect3DSurface8;
extern const IID IID_IDirect3DTexture8;
extern const IID IID_IDirect3DVertexBuffer8;
extern const IID IID_IDirect3DIndexBuffer8;
extern const IID IID_IDirect3DSwapChain8;

// DirectX 8 structure definitions

// D3DDISPLAYMODE
struct D3DDISPLAYMODE {
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    D3DFORMAT Format;
};

// D3DPRESENT_PARAMETERS
struct D3DPRESENT_PARAMETERS {
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    D3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT FullScreen_PresentationInterval;
};

// D3DADAPTER_IDENTIFIER8
struct D3DADAPTER_IDENTIFIER8 {
    char Driver[512];
    char Description[512];
    char DeviceName[32];
    LARGE_INTEGER DriverVersion;
    DWORD VendorId;
    DWORD DeviceId;
    DWORD SubSysId;
    DWORD Revision;
    GUID DeviceIdentifier;
    DWORD WHQLLevel;
};

// D3DCAPS8
struct D3DCAPS8 {
    D3DDEVTYPE DeviceType;
    UINT AdapterOrdinal;
    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;
    DWORD PresentationIntervals;
    DWORD CursorCaps;
    DWORD DevCaps;
    DWORD PrimitiveMiscCaps;
    DWORD RasterCaps;
    DWORD ZCmpCaps;
    DWORD SrcBlendCaps;
    DWORD DestBlendCaps;
    DWORD AlphaCmpCaps;
    DWORD ShadeCaps;
    DWORD TextureCaps;
    DWORD TextureFilterCaps;
    DWORD CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps;
    DWORD TextureAddressCaps;
    DWORD VolumeTextureAddressCaps;
    DWORD LineCaps;
    DWORD MaxTextureWidth;
    DWORD MaxTextureHeight;
    DWORD MaxVolumeExtent;
    DWORD MaxTextureRepeat;
    DWORD MaxTextureAspectRatio;
    DWORD MaxAnisotropy;
    float MaxVertexW;
    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;
    float ExtentsAdjust;
    DWORD StencilCaps;
    DWORD FVFCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;
    DWORD VertexProcessingCaps;
    DWORD MaxActiveLights;
    DWORD MaxUserClipPlanes;
    DWORD MaxVertexBlendMatrices;
    DWORD MaxVertexBlendMatrixIndex;
    float MaxPointSize;
    DWORD MaxPrimitiveCount;
    DWORD MaxVertexIndex;
    DWORD MaxStreams;
    DWORD MaxStreamStride;
    DWORD VertexShaderVersion;
    DWORD MaxVertexShaderConst;
    DWORD PixelShaderVersion;
    float MaxPixelShaderValue;
};

// D3DSURFACE_DESC
struct D3DSURFACE_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    UINT Width;
    UINT Height;
};

// D3DLOCKED_RECT
struct D3DLOCKED_RECT {
    INT Pitch;
    void* pBits;
};

// D3DVERTEXBUFFER_DESC
struct D3DVERTEXBUFFER_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
    DWORD FVF;
};

// D3DINDEXBUFFER_DESC
struct D3DINDEXBUFFER_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
};

// D3DVIEWPORT8
struct D3DVIEWPORT8 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
};

// D3DVECTOR
struct D3DVECTOR {
    float x;
    float y;
    float z;
};

// D3DCOLORVALUE
struct D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};

// D3DMATRIX
struct D3DMATRIX {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
    
    // Provide array access
    float& m(int i, int j) { return (&_11)[i * 4 + j]; }
    const float& m(int i, int j) const { return (&_11)[i * 4 + j]; }
};

// Legacy typedef for _D3DMATRIX
typedef D3DMATRIX _D3DMATRIX;

// D3DMATERIAL8
struct D3DMATERIAL8 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};

// D3DLIGHT8
struct D3DLIGHT8 {
    D3DLIGHTTYPE Type;
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Ambient;
    D3DVECTOR Position;
    D3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
};

// D3DRECT
struct D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
};

// D3DGAMMARAMP
struct D3DGAMMARAMP {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};

// D3DRASTER_STATUS
struct D3DRASTER_STATUS {
    BOOL InVBlank;
    UINT ScanLine;
};

// D3DDEVICE_CREATION_PARAMETERS
struct D3DDEVICE_CREATION_PARAMETERS {
    UINT AdapterOrdinal;
    D3DDEVTYPE DeviceType;
    HWND hFocusWindow;
    DWORD BehaviorFlags;
};

// D3DCLIPSTATUS8
struct D3DCLIPSTATUS8 {
    DWORD ClipUnion;
    DWORD ClipIntersection;
};

// D3DRECTPATCH_INFO
struct D3DRECTPATCH_INFO {
    UINT StartVertexOffsetWidth;
    UINT StartVertexOffsetHeight;
    UINT Width;
    UINT Height;
    UINT Stride;
    D3DBASISTYPE Basis;
    D3DORDERTYPE Order;
};

// D3DTRIPATCH_INFO  
struct D3DTRIPATCH_INFO {
    UINT StartVertexOffset;
    UINT NumVertices;
    D3DBASISTYPE Basis;
    D3DORDERTYPE Order;
};

// Define export macro if not defined
#ifndef DX8GL_API
    #ifdef _WIN32
        #ifdef DX8GL_BUILDING_DLL
            #define DX8GL_API __declspec(dllexport)
        #else
            #define DX8GL_API __declspec(dllimport)
        #endif
    #else
        #define DX8GL_API __attribute__((visibility("default")))
    #endif
#endif

// Define calling conventions for cross-platform
#ifndef STDMETHODCALLTYPE
    #ifdef _WIN32
        #define STDMETHODCALLTYPE __stdcall
    #else
        #define STDMETHODCALLTYPE
    #endif
#endif

#ifndef WINAPI
    #define WINAPI STDMETHODCALLTYPE
#endif

// Base IUnknown interface
struct IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual ~IUnknown() = default;
};

// IDirect3D8 interface
struct IDirect3D8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) = 0;
    virtual UINT STDMETHODCALLTYPE GetAdapterCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags,
                                                           D3DADAPTER_IDENTIFIER8* pIdentifier) = 0;
    virtual UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, UINT Mode,
                                                       D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter,
                                                            D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType,
                                                      D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                                      BOOL Windowed) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType,
                                                        D3DFORMAT AdapterFormat, DWORD Usage,
                                                        D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType,
                                                                 D3DFORMAT SurfaceFormat, BOOL Windowed,
                                                                 D3DMULTISAMPLE_TYPE MultiSampleType) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType,
                                                             D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
                                                             D3DFORMAT DepthStencilFormat) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType,
                                                    D3DCAPS8* pCaps) = 0;
    virtual HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType,
                                                  HWND hFocusWindow, DWORD BehaviorFlags,
                                                  D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  IDirect3DDevice8** ppReturnedDeviceInterface) = 0;
};

// IDirect3DResource8 interface
struct IDirect3DResource8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData,
                                                     DWORD SizeOfData, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData,
                                                     DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) = 0;
    virtual DWORD STDMETHODCALLTYPE GetPriority() = 0;
    virtual void STDMETHODCALLTYPE PreLoad() = 0;
    virtual D3DRESOURCETYPE STDMETHODCALLTYPE GetType() = 0;
};

// IDirect3DBaseTexture8 interface
struct IDirect3DBaseTexture8 : public IDirect3DResource8 {
    virtual DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) = 0;
    virtual DWORD STDMETHODCALLTYPE GetLOD() = 0;
    virtual DWORD STDMETHODCALLTYPE GetLevelCount() = 0;
};

// IDirect3DTexture8 interface
struct IDirect3DTexture8 : public IDirect3DBaseTexture8 {
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface8** ppSurfaceLevel) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect,
                                               const RECT* pRect, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* pDirtyRect) = 0;
};

// IDirect3DSurface8 interface
struct IDirect3DSurface8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData,
                                                     DWORD SizeOfData, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData,
                                                     DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect,
                                               DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect() = 0;
};

// IDirect3DVertexBuffer8 interface
struct IDirect3DVertexBuffer8 : public IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock,
                                           BYTE** ppbData, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* pDesc) = 0;
};

// IDirect3DIndexBuffer8 interface
struct IDirect3DIndexBuffer8 : public IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock,
                                           BYTE** ppbData, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* pDesc) = 0;
};

// IDirect3DSwapChain8 interface
struct IDirect3DSwapChain8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Present(const RECT* pSourceRect, const RECT* pDestRect,
                                             HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                                   IDirect3DSurface8** ppBackBuffer) = 0;
};

// IDirect3DDevice8 interface
struct IDirect3DDevice8 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel() = 0;
    virtual UINT STDMETHODCALLTYPE GetAvailableTextureMem() = 0;
    virtual HRESULT STDMETHODCALLTYPE ResourceManagerDiscardBytes(DWORD Bytes) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D8** ppD3D8) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS8* pCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) = 0;
    virtual void STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) = 0;
    virtual BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) = 0;
    virtual void STDMETHODCALLTYPE SetGammaRamp(DWORD Flags, const D3DGAMMARAMP* pRamp) = 0;
    virtual void STDMETHODCALLTYPE GetGammaRamp(D3DGAMMARAMP* pRamp) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyRects(IDirect3DSurface8* pSourceSurface, const RECT* pSourceRects, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPoints) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrontBuffer(IDirect3DSurface8* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderTarget(IDirect3DSurface8** ppRenderTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT8* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT8* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL8* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL8* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLight(DWORD Index, const D3DLIGHT8* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT8* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index, const float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginStateBlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndStateBlock(DWORD* pToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE ApplyStateBlock(DWORD Token) = 0;
    virtual HRESULT STDMETHODCALLTYPE CaptureStateBlock(DWORD Token) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteStateBlock(DWORD Token) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS8* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS8* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetInfo(DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT* pPaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShader(DWORD Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShader(DWORD* pHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteVertexShader(DWORD Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstant(DWORD Register, const void* pConstantData, DWORD ConstantCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* pFunction, DWORD* pHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShader(DWORD Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShader(DWORD* pHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePixelShader(DWORD Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstant(DWORD Register, const void* pConstantData, DWORD ConstantCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) = 0;
};

// Factory function for creating Direct3D8 interface
extern "C" IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion);

#endif // DX8GL_D3D8_CPP_INTERFACES_H