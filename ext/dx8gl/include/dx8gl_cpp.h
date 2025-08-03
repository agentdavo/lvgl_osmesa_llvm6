// dx8gl_cpp.h - Modern C++17 interface for dx8gl
// This provides direct C++ class access without COM overhead

#ifndef DX8GL_CPP_H
#define DX8GL_CPP_H

#include <memory>
#include <cstdint>
#include "d3d8_types.h"

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;
class Direct3DTexture8;
class Direct3DVertexBuffer8;
class Direct3DIndexBuffer8;
class Direct3DSurface8;

// Modern C++17 Direct3D8 interface
class Direct3D8 {
public:
    Direct3D8();
    virtual ~Direct3D8();
    
    // Adapter methods
    virtual UINT GetAdapterCount();
    virtual HRESULT GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
    virtual UINT GetAdapterModeCount(UINT Adapter);
    virtual HRESULT EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
    virtual HRESULT GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode);
    
    // Device capabilities
    virtual HRESULT CheckDeviceType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT DisplayFormat,
                                   D3DFORMAT BackBufferFormat, BOOL Windowed);
    virtual HRESULT CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                     DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    virtual HRESULT GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
    
    // Device creation - returns raw pointer for compatibility, but caller should use unique_ptr
    virtual Direct3DDevice8* CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                         DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Modern C++17 Direct3DDevice8 interface
class Direct3DDevice8 {
public:
    Direct3DDevice8();
    virtual ~Direct3DDevice8();
    
    // Device management
    virtual HRESULT TestCooperativeLevel();
    virtual UINT GetAvailableTextureMem();
    virtual HRESULT GetDeviceCaps(D3DCAPS8* pCaps);
    virtual HRESULT GetDisplayMode(D3DDISPLAYMODE* pMode);
    
    // Rendering
    virtual HRESULT BeginScene();
    virtual HRESULT EndScene();
    virtual HRESULT Present(const RECT* pSourceRect, const RECT* pDestRect, 
                           HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
    virtual HRESULT Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, 
                         D3DCOLOR Color, float Z, DWORD Stencil);
    
    // State management
    virtual HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
    virtual HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue);
    virtual HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    virtual HRESULT GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
    
    // Transforms
    virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
    virtual HRESULT GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
    
    // Resource creation - returns raw pointers for compatibility
    virtual Direct3DTexture8* CreateTexture(UINT Width, UINT Height, UINT Levels, 
                                           DWORD Usage, D3DFORMAT Format, D3DPOOL Pool);
    virtual Direct3DVertexBuffer8* CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool);
    virtual Direct3DIndexBuffer8* CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool);
    
    // Drawing
    virtual HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
    virtual HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, UINT MinIndex, UINT NumVertices,
                                        UINT StartIndex, UINT PrimitiveCount);
    
    // Texture management
    virtual HRESULT SetTexture(DWORD Stage, Direct3DTexture8* pTexture);
    
    // Vertex streams
    virtual HRESULT SetStreamSource(UINT StreamNumber, Direct3DVertexBuffer8* pStreamData, UINT Stride);
    virtual HRESULT SetIndices(Direct3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
    virtual HRESULT SetVertexShader(DWORD Handle);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Factory function - creates Direct3D8 instance
std::unique_ptr<Direct3D8> CreateDirect3D8(UINT SDKVersion);

} // namespace dx8gl

#endif // DX8GL_CPP_H