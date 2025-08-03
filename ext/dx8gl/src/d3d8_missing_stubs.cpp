/**
 * Minimal stub implementations for missing DirectX 8 Device methods
 * These allow the game to link and run, though some features won't work
 */

#include "d3d8_device.h"
#include "logger.h"
#include <cstring>

namespace dx8gl {

// Stub implementations for missing Direct3DDevice8 methods

// CopyRects is already implemented in d3d8_device.cpp

// UpdateTexture is already implemented in d3d8_device.cpp

// GetFrontBuffer is already implemented in d3d8_device.cpp

// MultiplyTransform is already implemented in d3d8_device.cpp

// SetClipPlane is already implemented in d3d8_device.cpp

// GetClipPlane is already implemented in d3d8_device.cpp

// BeginStateBlock is already implemented in d3d8_device.cpp

// EndStateBlock is already implemented in d3d8_device.cpp

// ApplyStateBlock is already implemented in d3d8_device.cpp

// CaptureStateBlock is already implemented in d3d8_device.cpp

// DeleteStateBlock is already implemented in d3d8_device.cpp

// CreateStateBlock is already implemented in d3d8_device.cpp

// SetClipStatus is already implemented in d3d8_device.cpp

// GetClipStatus is already implemented in d3d8_device.cpp

// ValidateDevice is already implemented in d3d8_device.cpp

// GetInfo is already implemented in d3d8_device.cpp

// SetPaletteEntries is already implemented in d3d8_device.cpp

// GetPaletteEntries is already implemented in d3d8_device.cpp

// SetCurrentTexturePalette is already implemented in d3d8_device.cpp

// GetCurrentTexturePalette is already implemented in d3d8_device.cpp

} // namespace dx8gl

// C-style COM wrapper functions for missing methods
extern "C" {

HRESULT Direct3DDevice8_CopyRects(IDirect3DDevice8* This, IDirect3DSurface8* pSourceSurface, 
                                  const RECT* pSourceRects, UINT cRects, 
                                  IDirect3DSurface8* pDestinationSurface, const POINT* pDestPoints) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->CopyRects(pSourceSurface, pSourceRects, cRects, pDestinationSurface, pDestPoints);
}

HRESULT Direct3DDevice8_UpdateTexture(IDirect3DDevice8* This, IDirect3DBaseTexture8* pSourceTexture,
                                      IDirect3DBaseTexture8* pDestinationTexture) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->UpdateTexture(pSourceTexture, pDestinationTexture);
}

HRESULT Direct3DDevice8_GetFrontBuffer(IDirect3DDevice8* This, IDirect3DSurface8* pDestSurface) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetFrontBuffer(pDestSurface);
}

HRESULT Direct3DDevice8_SetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8* pRenderTarget,
                                        IDirect3DSurface8* pNewZStencil) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetRenderTarget(pRenderTarget, pNewZStencil);
}

HRESULT Direct3DDevice8_GetRenderTarget(IDirect3DDevice8* This, IDirect3DSurface8** ppRenderTarget) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetRenderTarget(ppRenderTarget);
}

HRESULT Direct3DDevice8_GetDepthStencilSurface(IDirect3DDevice8* This, IDirect3DSurface8** ppZStencilSurface) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetDepthStencilSurface(ppZStencilSurface);
}

HRESULT Direct3DDevice8_MultiplyTransform(IDirect3DDevice8* This, D3DTRANSFORMSTATETYPE State,
                                          const D3DMATRIX* pMatrix) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->MultiplyTransform(State, pMatrix);
}

HRESULT Direct3DDevice8_SetViewport(IDirect3DDevice8* This, const D3DVIEWPORT8* pViewport) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetViewport(pViewport);
}

HRESULT Direct3DDevice8_GetViewport(IDirect3DDevice8* This, D3DVIEWPORT8* pViewport) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetViewport(pViewport);
}

HRESULT Direct3DDevice8_SetMaterial(IDirect3DDevice8* This, const D3DMATERIAL8* pMaterial) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetMaterial(pMaterial);
}

HRESULT Direct3DDevice8_GetMaterial(IDirect3DDevice8* This, D3DMATERIAL8* pMaterial) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetMaterial(pMaterial);
}

HRESULT Direct3DDevice8_SetLight(IDirect3DDevice8* This, DWORD Index, const D3DLIGHT8* pLight) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetLight(Index, pLight);
}

HRESULT Direct3DDevice8_GetLight(IDirect3DDevice8* This, DWORD Index, D3DLIGHT8* pLight) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetLight(Index, pLight);
}

HRESULT Direct3DDevice8_LightEnable(IDirect3DDevice8* This, DWORD LightIndex, BOOL bEnable) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->LightEnable(LightIndex, bEnable);
}

HRESULT Direct3DDevice8_GetLightEnable(IDirect3DDevice8* This, DWORD Index, BOOL* pEnable) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetLightEnable(Index, pEnable);
}

HRESULT Direct3DDevice8_SetClipPlane(IDirect3DDevice8* This, DWORD Index, const float* pPlane) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetClipPlane(Index, pPlane);
}

HRESULT Direct3DDevice8_GetClipPlane(IDirect3DDevice8* This, DWORD Index, float* pPlane) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetClipPlane(Index, pPlane);
}

HRESULT Direct3DDevice8_BeginStateBlock(IDirect3DDevice8* This) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->BeginStateBlock();
}

HRESULT Direct3DDevice8_EndStateBlock(IDirect3DDevice8* This, DWORD* pToken) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->EndStateBlock(pToken);
}

HRESULT Direct3DDevice8_ApplyStateBlock(IDirect3DDevice8* This, DWORD Token) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->ApplyStateBlock(Token);
}

HRESULT Direct3DDevice8_CaptureStateBlock(IDirect3DDevice8* This, DWORD Token) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->CaptureStateBlock(Token);
}

HRESULT Direct3DDevice8_DeleteStateBlock(IDirect3DDevice8* This, DWORD Token) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DeleteStateBlock(Token);
}

HRESULT Direct3DDevice8_CreateStateBlock(IDirect3DDevice8* This, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->CreateStateBlock(Type, pToken);
}

HRESULT Direct3DDevice8_GetTexture(IDirect3DDevice8* This, DWORD Stage, IDirect3DBaseTexture8** ppTexture) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetTexture(Stage, ppTexture);
}

HRESULT Direct3DDevice8_GetTextureStageState(IDirect3DDevice8* This, DWORD Stage, 
                                             D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetTextureStageState(Stage, Type, pValue);
}

HRESULT Direct3DDevice8_SetTextureStageState(IDirect3DDevice8* This, DWORD Stage,
                                             D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetTextureStageState(Stage, Type, Value);
}

HRESULT Direct3DDevice8_GetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber,
                                          PALETTEENTRY* pEntries) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT Direct3DDevice8_SetPaletteEntries(IDirect3DDevice8* This, UINT PaletteNumber,
                                          const PALETTEENTRY* pEntries) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT Direct3DDevice8_GetCurrentTexturePalette(IDirect3DDevice8* This, UINT* pPaletteNumber) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetCurrentTexturePalette(pPaletteNumber);
}

HRESULT Direct3DDevice8_SetCurrentTexturePalette(IDirect3DDevice8* This, UINT PaletteNumber) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetCurrentTexturePalette(PaletteNumber);
}

HRESULT Direct3DDevice8_CreatePixelShader(IDirect3DDevice8* This, const DWORD* pFunction, DWORD* pHandle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->CreatePixelShader(pFunction, pHandle);
}

HRESULT Direct3DDevice8_GetPixelShader(IDirect3DDevice8* This, DWORD* pHandle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetPixelShader(pHandle);
}

HRESULT Direct3DDevice8_SetPixelShader(IDirect3DDevice8* This, DWORD Handle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetPixelShader(Handle);
}

HRESULT Direct3DDevice8_DeletePixelShader(IDirect3DDevice8* This, DWORD Handle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DeletePixelShader(Handle);
}

HRESULT Direct3DDevice8_GetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register,
                                               void* pConstantData, DWORD ConstantCount) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8_SetPixelShaderConstant(IDirect3DDevice8* This, DWORD Register,
                                               const void* pConstantData, DWORD ConstantCount) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8_GetPixelShaderFunction(IDirect3DDevice8* This, DWORD Handle,
                                               void* pData, DWORD* pSizeOfData) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetPixelShaderFunction(Handle, pData, pSizeOfData);
}

HRESULT Direct3DDevice8_SetClipStatus(IDirect3DDevice8* This, const D3DCLIPSTATUS8* pClipStatus) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetClipStatus(pClipStatus);
}

HRESULT Direct3DDevice8_GetClipStatus(IDirect3DDevice8* This, D3DCLIPSTATUS8* pClipStatus) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetClipStatus(pClipStatus);
}

HRESULT Direct3DDevice8_ValidateDevice(IDirect3DDevice8* This, DWORD* pNumPasses) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->ValidateDevice(pNumPasses);
}

HRESULT Direct3DDevice8_GetInfo(IDirect3DDevice8* This, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetInfo(DevInfoID, pDevInfoStruct, DevInfoStructSize);
}

// Drawing functions that were missing
HRESULT Direct3DDevice8_DrawPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT Direct3DDevice8_DrawIndexedPrimitiveUP(IDirect3DDevice8* This, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT Direct3DDevice8_ProcessVertices(IDirect3DDevice8* This, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, Flags);
}

// Vertex shader functions
HRESULT Direct3DDevice8_CreateVertexShader(IDirect3DDevice8* This, const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->CreateVertexShader(pDeclaration, pFunction, pHandle, Usage);
}

HRESULT Direct3DDevice8_SetVertexShader(IDirect3DDevice8* This, DWORD Handle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetVertexShader(Handle);
}

HRESULT Direct3DDevice8_GetVertexShader(IDirect3DDevice8* This, DWORD* pHandle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetVertexShader(pHandle);
}

HRESULT Direct3DDevice8_DeleteVertexShader(IDirect3DDevice8* This, DWORD Handle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DeleteVertexShader(Handle);
}

HRESULT Direct3DDevice8_SetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, const void* pConstantData, DWORD ConstantCount) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8_GetVertexShaderConstant(IDirect3DDevice8* This, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8_GetVertexShaderDeclaration(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetVertexShaderDeclaration(Handle, pData, pSizeOfData);
}

HRESULT Direct3DDevice8_GetVertexShaderFunction(IDirect3DDevice8* This, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetVertexShaderFunction(Handle, pData, pSizeOfData);
}

// Stream source and indices
HRESULT Direct3DDevice8_SetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetStreamSource(StreamNumber, pStreamData, Stride);
}

HRESULT Direct3DDevice8_GetStreamSource(IDirect3DDevice8* This, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetStreamSource(StreamNumber, ppStreamData, pStride);
}

HRESULT Direct3DDevice8_SetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->SetIndices(pIndexData, BaseVertexIndex);
}

HRESULT Direct3DDevice8_GetIndices(IDirect3DDevice8* This, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->GetIndices(ppIndexData, pBaseVertexIndex);
}

// Patch functions
HRESULT Direct3DDevice8_DrawRectPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT Direct3DDevice8_DrawTriPatch(IDirect3DDevice8* This, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}

HRESULT Direct3DDevice8_DeletePatch(IDirect3DDevice8* This, UINT Handle) {
    if (!This) return D3DERR_INVALIDCALL;
    auto* device = static_cast<dx8gl::Direct3DDevice8*>(This);
    return device->DeletePatch(Handle);
}

} // extern "C"
