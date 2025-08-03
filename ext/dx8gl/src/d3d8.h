#ifndef DX8GL_D3D8_H
#define DX8GL_D3D8_H

/**
 * Direct3D 8 Compatibility Header
 * 
 * This header provides DirectX 8 API compatibility for dx8gl.
 * It supports both C-style COM interfaces (for legacy code compatibility)
 * and C++ interfaces (for modern usage).
 */

// dx8gl uses C++ interfaces internally.
// Game code should include "d3d8_game.h" for COM-style interfaces.
#ifndef DX8GL_BUILDING_DLL
    #warning "Game code should include d3d8_game.h instead of d3d8.h for COM-style interfaces"
#endif

// Pure C++ interfaces (used internally by dx8gl)
#include "d3d8_cpp_interfaces.h"

// Include D3DX utilities
#include "d3dx8.h"

// Main Direct3D creation function
#ifndef DX8GL_USE_CPP_INTERFACES
    // Already declared in d3d8_com_wrapper.h
#else
    extern "C" {
        DX8GL_API IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion);
    }
#endif

#if 0
// Old wrapper class implementation - kept for reference
class IDirect3DDevice8Cpp {
protected:
    IDirect3DDevice8* m_pDevice;
    
public:
    IDirect3DDevice8Cpp(IDirect3DDevice8* pDevice) : m_pDevice(pDevice) {}
    
    // IUnknown methods
    HRESULT QueryInterface(REFIID riid, void** ppvObject) {
        return IDirect3DDevice8_QueryInterface(m_pDevice, riid, ppvObject);
    }
    ULONG AddRef() {
        return IDirect3DDevice8_AddRef(m_pDevice);
    }
    ULONG Release() {
        return IDirect3DDevice8_Release(m_pDevice);
    }
    
    // IDirect3DDevice8 methods
    HRESULT TestCooperativeLevel() {
        return IDirect3DDevice8_TestCooperativeLevel(m_pDevice);
    }
    UINT GetAvailableTextureMem() {
        return IDirect3DDevice8_GetAvailableTextureMem(m_pDevice);
    }
    HRESULT ResourceManagerDiscardBytes(DWORD Bytes) {
        return IDirect3DDevice8_ResourceManagerDiscardBytes(m_pDevice, Bytes);
    }
    HRESULT GetDirect3D(IDirect3D8** ppD3D8) {
        return IDirect3DDevice8_GetDirect3D(m_pDevice, ppD3D8);
    }
    HRESULT GetDeviceCaps(D3DCAPS8* pCaps) {
        return IDirect3DDevice8_GetDeviceCaps(m_pDevice, pCaps);
    }
    HRESULT GetDisplayMode(D3DDISPLAYMODE* pMode) {
        return IDirect3DDevice8_GetDisplayMode(m_pDevice, pMode);
    }
    HRESULT GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) {
        return IDirect3DDevice8_GetCreationParameters(m_pDevice, pParameters);
    }
    HRESULT SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
        return IDirect3DDevice8_SetCursorProperties(m_pDevice, XHotSpot, YHotSpot, pCursorBitmap);
    }
    void SetCursorPosition(int X, int Y, DWORD Flags) {
        IDirect3DDevice8_SetCursorPosition(m_pDevice, X, Y, Flags);
    }
    BOOL ShowCursor(BOOL bShow) {
        return IDirect3DDevice8_ShowCursor(m_pDevice, bShow);
    }
    HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain) {
        return IDirect3DDevice8_CreateAdditionalSwapChain(m_pDevice, pPresentationParameters, ppSwapChain);
    }
    HRESULT Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) {
        return IDirect3DDevice8_Reset(m_pDevice, pPresentationParameters);
    }
    HRESULT Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
        return IDirect3DDevice8_Present(m_pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }
    HRESULT GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
        return IDirect3DDevice8_GetBackBuffer(m_pDevice, BackBuffer, Type, ppBackBuffer);
    }
    HRESULT GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) {
        return IDirect3DDevice8_GetRasterStatus(m_pDevice, pRasterStatus);
    }
    void SetGammaRamp(DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
        IDirect3DDevice8_SetGammaRamp(m_pDevice, Flags, pRamp);
    }
    void GetGammaRamp(D3DGAMMARAMP* pRamp) {
        IDirect3DDevice8_GetGammaRamp(m_pDevice, pRamp);
    }
    HRESULT CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) {
        return IDirect3DDevice8_CreateTexture(m_pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture);
    }
    HRESULT CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {
        return IDirect3DDevice8_CreateVolumeTexture(m_pDevice, Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture);
    }
    HRESULT CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {
        return IDirect3DDevice8_CreateCubeTexture(m_pDevice, EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture);
    }
    HRESULT CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
        return IDirect3DDevice8_CreateVertexBuffer(m_pDevice, Length, Usage, FVF, Pool, ppVertexBuffer);
    }
    HRESULT CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
        return IDirect3DDevice8_CreateIndexBuffer(m_pDevice, Length, Usage, Format, Pool, ppIndexBuffer);
    }
    HRESULT CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
        return IDirect3DDevice8_CreateRenderTarget(m_pDevice, Width, Height, Format, MultiSample, Lockable, ppSurface);
    }
    HRESULT CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
        return IDirect3DDevice8_CreateDepthStencilSurface(m_pDevice, Width, Height, Format, MultiSample, ppSurface);
    }
    HRESULT CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
        return IDirect3DDevice8_CreateImageSurface(m_pDevice, Width, Height, Format, ppSurface);
    }
    HRESULT CopyRects(IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRects, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPoints) {
        return IDirect3DDevice8_CopyRects(m_pDevice, pSourceSurface, pSourceRects, cRects, pDestinationSurface, pDestPoints);
    }
    HRESULT UpdateTexture(IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
        return IDirect3DDevice8_UpdateTexture(m_pDevice, pSourceTexture, pDestinationTexture);
    }
    HRESULT GetFrontBuffer(IDirect3DSurface8* pDestSurface) {
        return IDirect3DDevice8_GetFrontBuffer(m_pDevice, pDestSurface);
    }
    HRESULT SetRenderTarget(IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
        return IDirect3DDevice8_SetRenderTarget(m_pDevice, pRenderTarget, pNewZStencil);
    }
    HRESULT GetRenderTarget(IDirect3DSurface8** ppRenderTarget) {
        return IDirect3DDevice8_GetRenderTarget(m_pDevice, ppRenderTarget);
    }
    HRESULT GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface) {
        return IDirect3DDevice8_GetDepthStencilSurface(m_pDevice, ppZStencilSurface);
    }
    HRESULT BeginScene() {
        return IDirect3DDevice8_BeginScene(m_pDevice);
    }
    HRESULT EndScene() {
        return IDirect3DDevice8_EndScene(m_pDevice);
    }
    HRESULT Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
        return IDirect3DDevice8_Clear(m_pDevice, Count, pRects, Flags, Color, Z, Stencil);
    }
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
        return IDirect3DDevice8_SetTransform(m_pDevice, State, pMatrix);
    }
    HRESULT GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
        return IDirect3DDevice8_GetTransform(m_pDevice, State, pMatrix);
    }
    HRESULT MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
        return IDirect3DDevice8_MultiplyTransform(m_pDevice, State, pMatrix);
    }
    HRESULT SetViewport(CONST D3DVIEWPORT8* pViewport) {
        return IDirect3DDevice8_SetViewport(m_pDevice, pViewport);
    }
    HRESULT GetViewport(D3DVIEWPORT8* pViewport) {
        return IDirect3DDevice8_GetViewport(m_pDevice, pViewport);
    }
    HRESULT SetMaterial(CONST D3DMATERIAL8* pMaterial) {
        return IDirect3DDevice8_SetMaterial(m_pDevice, pMaterial);
    }
    HRESULT GetMaterial(D3DMATERIAL8* pMaterial) {
        return IDirect3DDevice8_GetMaterial(m_pDevice, pMaterial);
    }
    HRESULT SetLight(DWORD Index, CONST D3DLIGHT8* pLight) {
        return IDirect3DDevice8_SetLight(m_pDevice, Index, pLight);
    }
    HRESULT GetLight(DWORD Index, D3DLIGHT8* pLight) {
        return IDirect3DDevice8_GetLight(m_pDevice, Index, pLight);
    }
    HRESULT LightEnable(DWORD Index, BOOL Enable) {
        return IDirect3DDevice8_LightEnable(m_pDevice, Index, Enable);
    }
    HRESULT GetLightEnable(DWORD Index, BOOL* pEnable) {
        return IDirect3DDevice8_GetLightEnable(m_pDevice, Index, pEnable);
    }
    HRESULT SetClipPlane(DWORD Index, CONST float* pPlane) {
        return IDirect3DDevice8_SetClipPlane(m_pDevice, Index, pPlane);
    }
    HRESULT GetClipPlane(DWORD Index, float* pPlane) {
        return IDirect3DDevice8_GetClipPlane(m_pDevice, Index, pPlane);
    }
    HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
        return IDirect3DDevice8_SetRenderState(m_pDevice, State, Value);
    }
    HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
        return IDirect3DDevice8_GetRenderState(m_pDevice, State, pValue);
    }
    HRESULT BeginStateBlock() {
        return IDirect3DDevice8_BeginStateBlock(m_pDevice);
    }
    HRESULT EndStateBlock(DWORD* pToken) {
        return IDirect3DDevice8_EndStateBlock(m_pDevice, pToken);
    }
    HRESULT ApplyStateBlock(DWORD Token) {
        return IDirect3DDevice8_ApplyStateBlock(m_pDevice, Token);
    }
    HRESULT CaptureStateBlock(DWORD Token) {
        return IDirect3DDevice8_CaptureStateBlock(m_pDevice, Token);
    }
    HRESULT DeleteStateBlock(DWORD Token) {
        return IDirect3DDevice8_DeleteStateBlock(m_pDevice, Token);
    }
    HRESULT CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
        return IDirect3DDevice8_CreateStateBlock(m_pDevice, Type, pToken);
    }
    HRESULT SetClipStatus(CONST D3DCLIPSTATUS8* pClipStatus) {
        return IDirect3DDevice8_SetClipStatus(m_pDevice, pClipStatus);
    }
    HRESULT GetClipStatus(D3DCLIPSTATUS8* pClipStatus) {
        return IDirect3DDevice8_GetClipStatus(m_pDevice, pClipStatus);
    }
    HRESULT GetTexture(DWORD Stage, IDirect3DBaseTexture8** ppTexture) {
        return IDirect3DDevice8_GetTexture(m_pDevice, Stage, ppTexture);
    }
    HRESULT SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) {
        return IDirect3DDevice8_SetTexture(m_pDevice, Stage, pTexture);
    }
    HRESULT GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
        return IDirect3DDevice8_GetTextureStageState(m_pDevice, Stage, Type, pValue);
    }
    HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
        return IDirect3DDevice8_SetTextureStageState(m_pDevice, Stage, Type, Value);
    }
    HRESULT ValidateDevice(DWORD* pNumPasses) {
        return IDirect3DDevice8_ValidateDevice(m_pDevice, pNumPasses);
    }
    HRESULT GetInfo(DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
        return IDirect3DDevice8_GetInfo(m_pDevice, DevInfoID, pDevInfoStruct, DevInfoStructSize);
    }
    HRESULT SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
        return IDirect3DDevice8_SetPaletteEntries(m_pDevice, PaletteNumber, pEntries);
    }
    HRESULT GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) {
        return IDirect3DDevice8_GetPaletteEntries(m_pDevice, PaletteNumber, pEntries);
    }
    HRESULT SetCurrentTexturePalette(UINT PaletteNumber) {
        return IDirect3DDevice8_SetCurrentTexturePalette(m_pDevice, PaletteNumber);
    }
    HRESULT GetCurrentTexturePalette(UINT* pPaletteNumber) {
        return IDirect3DDevice8_GetCurrentTexturePalette(m_pDevice, pPaletteNumber);
    }
    HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
        return IDirect3DDevice8_DrawPrimitive(m_pDevice, PrimitiveType, StartVertex, PrimitiveCount);
    }
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) {
        return IDirect3DDevice8_DrawIndexedPrimitive(m_pDevice, PrimitiveType, MinIndex, NumVertices, StartIndex, PrimitiveCount);
    }
    HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
        return IDirect3DDevice8_DrawPrimitiveUP(m_pDevice, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
    }
    HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
        return IDirect3DDevice8_DrawIndexedPrimitiveUP(m_pDevice, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
    }
    HRESULT ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) {
        return IDirect3DDevice8_ProcessVertices(m_pDevice, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, Flags);
    }
    HRESULT CreateVertexShader(CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage) {
        return IDirect3DDevice8_CreateVertexShader(m_pDevice, pDeclaration, pFunction, pHandle, Usage);
    }
    HRESULT SetVertexShader(DWORD Handle) {
        return IDirect3DDevice8_SetVertexShader(m_pDevice, Handle);
    }
    HRESULT GetVertexShader(DWORD* pHandle) {
        return IDirect3DDevice8_GetVertexShader(m_pDevice, pHandle);
    }
    HRESULT DeleteVertexShader(DWORD Handle) {
        return IDirect3DDevice8_DeleteVertexShader(m_pDevice, Handle);
    }
    HRESULT SetVertexShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
        return IDirect3DDevice8_SetVertexShaderConstant(m_pDevice, Register, pConstantData, ConstantCount);
    }
    HRESULT GetVertexShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) {
        return IDirect3DDevice8_GetVertexShaderConstant(m_pDevice, Register, pConstantData, ConstantCount);
    }
    HRESULT GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) {
        return IDirect3DDevice8_GetVertexShaderDeclaration(m_pDevice, Handle, pData, pSizeOfData);
    }
    HRESULT GetVertexShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) {
        return IDirect3DDevice8_GetVertexShaderFunction(m_pDevice, Handle, pData, pSizeOfData);
    }
    HRESULT SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) {
        return IDirect3DDevice8_SetStreamSource(m_pDevice, StreamNumber, pStreamData, Stride);
    }
    HRESULT GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) {
        return IDirect3DDevice8_GetStreamSource(m_pDevice, StreamNumber, ppStreamData, pStride);
    }
    HRESULT SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
        return IDirect3DDevice8_SetIndices(m_pDevice, pIndexData, BaseVertexIndex);
    }
    HRESULT GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) {
        return IDirect3DDevice8_GetIndices(m_pDevice, ppIndexData, pBaseVertexIndex);
    }
    HRESULT CreatePixelShader(CONST DWORD* pFunction, DWORD* pHandle) {
        return IDirect3DDevice8_CreatePixelShader(m_pDevice, pFunction, pHandle);
    }
    HRESULT SetPixelShader(DWORD Handle) {
        return IDirect3DDevice8_SetPixelShader(m_pDevice, Handle);
    }
    HRESULT GetPixelShader(DWORD* pHandle) {
        return IDirect3DDevice8_GetPixelShader(m_pDevice, pHandle);
    }
    HRESULT DeletePixelShader(DWORD Handle) {
        return IDirect3DDevice8_DeletePixelShader(m_pDevice, Handle);
    }
    HRESULT SetPixelShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
        return IDirect3DDevice8_SetPixelShaderConstant(m_pDevice, Register, pConstantData, ConstantCount);
    }
    HRESULT GetPixelShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) {
        return IDirect3DDevice8_GetPixelShaderConstant(m_pDevice, Register, pConstantData, ConstantCount);
    }
    HRESULT GetPixelShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) {
        return IDirect3DDevice8_GetPixelShaderFunction(m_pDevice, Handle, pData, pSizeOfData);
    }
    HRESULT DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
        return IDirect3DDevice8_DrawRectPatch(m_pDevice, Handle, pNumSegs, pRectPatchInfo);
    }
    HRESULT DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
        return IDirect3DDevice8_DrawTriPatch(m_pDevice, Handle, pNumSegs, pTriPatchInfo);
    }
    HRESULT DeletePatch(UINT Handle) {
        return IDirect3DDevice8_DeletePatch(m_pDevice, Handle);
    }
    
    // Conversion operator to allow passing to C functions
    operator IDirect3DDevice8*() { return m_pDevice; }
    
    // Get the underlying C interface
    IDirect3DDevice8* GetDevice() { return m_pDevice; }
};

// C++ wrapper for IDirect3DBaseTexture8
class IDirect3DBaseTexture8Cpp {
protected:
    IDirect3DBaseTexture8* m_pTexture;
    
public:
    IDirect3DBaseTexture8Cpp(IDirect3DBaseTexture8* pTexture) : m_pTexture(pTexture) {}
    
    // IUnknown methods
    HRESULT QueryInterface(REFIID riid, void** ppvObject) {
        return IDirect3DBaseTexture8_QueryInterface(m_pTexture, (void*)riid, ppvObject);
    }
    ULONG AddRef() {
        return IDirect3DBaseTexture8_AddRef(m_pTexture);
    }
    ULONG Release() {
        return IDirect3DBaseTexture8_Release(m_pTexture);
    }
    
    // IDirect3DResource8 methods
    HRESULT GetDevice(IDirect3DDevice8** ppDevice) {
        return IDirect3DBaseTexture8_GetDevice(m_pTexture, ppDevice);
    }
    HRESULT SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
        return IDirect3DBaseTexture8_SetPrivateData(m_pTexture, refguid, pData, SizeOfData, Flags);
    }
    HRESULT GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) {
        return IDirect3DBaseTexture8_GetPrivateData(m_pTexture, refguid, pData, pSizeOfData);
    }
    HRESULT FreePrivateData(REFGUID refguid) {
        return IDirect3DBaseTexture8_FreePrivateData(m_pTexture, refguid);
    }
    DWORD SetPriority(DWORD PriorityNew) {
        return IDirect3DBaseTexture8_SetPriority(m_pTexture, PriorityNew);
    }
    DWORD GetPriority() {
        return IDirect3DBaseTexture8_GetPriority(m_pTexture);
    }
    void PreLoad() {
        IDirect3DBaseTexture8_PreLoad(m_pTexture);
    }
    D3DRESOURCETYPE GetType() {
        return IDirect3DBaseTexture8_GetType(m_pTexture);
    }
    
    // IDirect3DBaseTexture8 methods
    DWORD SetLOD(DWORD LODNew) {
        return IDirect3DBaseTexture8_SetLOD(m_pTexture, LODNew);
    }
    DWORD GetLOD() {
        return IDirect3DBaseTexture8_GetLOD(m_pTexture);
    }
    DWORD GetLevelCount() {
        return IDirect3DBaseTexture8_GetLevelCount(m_pTexture);
    }
    
    // Conversion operator to allow passing to C functions
    operator IDirect3DBaseTexture8*() { return m_pTexture; }
    
    // Get the underlying C interface
    IDirect3DBaseTexture8* GetTexture() { return m_pTexture; }
};

// C++ wrapper for IDirect3DTexture8
class IDirect3DTexture8Cpp : public IDirect3DBaseTexture8Cpp {
public:
    IDirect3DTexture8Cpp(IDirect3DTexture8* pTexture) : IDirect3DBaseTexture8Cpp((IDirect3DBaseTexture8*)pTexture) {}
    
    HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) {
        return IDirect3DTexture8_GetLevelDesc((IDirect3DTexture8*)m_pTexture, Level, pDesc);
    }
    HRESULT GetSurfaceLevel(UINT Level, IDirect3DSurface8** ppSurfaceLevel) {
        return IDirect3DTexture8_GetSurfaceLevel((IDirect3DTexture8*)m_pTexture, Level, ppSurfaceLevel);
    }
    HRESULT LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
        return IDirect3DTexture8_LockRect((IDirect3DTexture8*)m_pTexture, Level, pLockedRect, pRect, Flags);
    }
    HRESULT UnlockRect(UINT Level) {
        return IDirect3DTexture8_UnlockRect((IDirect3DTexture8*)m_pTexture, Level);
    }
    HRESULT AddDirtyRect(CONST RECT* pDirtyRect) {
        return IDirect3DTexture8_AddDirtyRect((IDirect3DTexture8*)m_pTexture, pDirtyRect);
    }
};

#endif // 0

#endif // DX8GL_D3D8_H