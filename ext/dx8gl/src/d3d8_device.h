#ifndef DX8GL_D3D8_DEVICE_H
#define DX8GL_D3D8_DEVICE_H

#include "d3d8.h"
#include "state_manager.h"
#include "command_buffer.h"
#include "thread_pool.h"
#include "vertex_shader_manager.h"
#include "pixel_shader_manager.h"
#include "shader_program_manager.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace dx8gl {

// Forward declarations
class Direct3D8;
class Direct3DDevice8;

// Global device accessor for internal use
Direct3DDevice8* get_global_device();
class Direct3DTexture8;
class Direct3DVertexBuffer8;
class Direct3DIndexBuffer8;
class Direct3DSurface8;
class Direct3DSwapChain8;

class Direct3DDevice8 : public IDirect3DDevice8 {
    friend class Direct3DSurface8;
    friend class Direct3DVertexBuffer8;
    friend class Direct3DIndexBuffer8;
    friend class Direct3DTexture8;
    
public:
    Direct3DDevice8(Direct3D8* d3d8, UINT adapter, D3DDEVTYPE device_type,
                    HWND focus_window, DWORD behavior_flags,
                    D3DPRESENT_PARAMETERS* presentation_params);
    virtual ~Direct3DDevice8();

    // Initialize the device
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3DDevice8 methods - Device management
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;
    UINT STDMETHODCALLTYPE GetAvailableTextureMem() override;
    HRESULT STDMETHODCALLTYPE ResourceManagerDiscardBytes(DWORD Bytes) override;
    HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D8** ppD3D8) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS8* pCaps) override;
    HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) override;
    HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) override;
    void STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) override;
    BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow) override;
    HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** ppSwapChain) override;
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;
    HRESULT STDMETHODCALLTYPE Present(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) override;
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) override;
    HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) override;
    void STDMETHODCALLTYPE SetGammaRamp(DWORD Flags, const D3DGAMMARAMP* pRamp) override;
    void STDMETHODCALLTYPE GetGammaRamp(D3DGAMMARAMP* pRamp) override;

    // Resource creation
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) override;
    HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) override;
    HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) override;
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) override;
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) override;
    HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) override;
    HRESULT STDMETHODCALLTYPE CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) override;

    // Resource copying
    HRESULT STDMETHODCALLTYPE CopyRects(IDirect3DSurface8* pSourceSurface, const RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, const POINT* pDestPointsArray) override;
    HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) override;
    HRESULT STDMETHODCALLTYPE GetFrontBuffer(IDirect3DSurface8* pDestSurface) override;

    // Render target management
    HRESULT STDMETHODCALLTYPE SetRenderTarget(IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) override;
    HRESULT STDMETHODCALLTYPE GetRenderTarget(IDirect3DSurface8** ppRenderTarget) override;
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface) override;

    // Scene management
    HRESULT STDMETHODCALLTYPE BeginScene() override;
    HRESULT STDMETHODCALLTYPE EndScene() override;
    HRESULT STDMETHODCALLTYPE Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override;
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT8* pViewport) override;
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT8* pViewport) override;
    HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL8* pMaterial) override;
    HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL8* pMaterial) override;
    HRESULT STDMETHODCALLTYPE SetLight(DWORD Index, const D3DLIGHT8* pLight) override;
    HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT8* pLight) override;
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) override;
    HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) override;
    HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index, const float* pPlane) override;
    HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) override;
    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE BeginStateBlock() override;
    HRESULT STDMETHODCALLTYPE EndStateBlock(DWORD* pToken) override;
    HRESULT STDMETHODCALLTYPE ApplyStateBlock(DWORD Token) override;
    HRESULT STDMETHODCALLTYPE CaptureStateBlock(DWORD Token) override;
    HRESULT STDMETHODCALLTYPE DeleteStateBlock(DWORD Token) override;
    HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken) override;
    HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS8* pClipStatus) override;
    HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS8* pClipStatus) override;
    HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture8** ppTexture) override;
    HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) override;
    HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) override;
    HRESULT STDMETHODCALLTYPE GetInfo(DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) override;
    HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY* pEntries) override;
    HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) override;
    HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) override;
    HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT* PaletteNumber) override;

    // Drawing methods
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) override;
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) override;

    // Vertex and pixel shader methods
    HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* pDeclaration, const DWORD* pFunction, DWORD* pHandle, DWORD Usage) override;
    HRESULT STDMETHODCALLTYPE SetVertexShader(DWORD Handle) override;
    HRESULT STDMETHODCALLTYPE GetVertexShader(DWORD* pHandle) override;
    HRESULT STDMETHODCALLTYPE DeleteVertexShader(DWORD Handle) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstant(DWORD Register, const void* pConstantData, DWORD ConstantCount) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) override;
    HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) override;
    HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) override;
    HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) override;
    HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* pFunction, DWORD* pHandle) override;
    HRESULT STDMETHODCALLTYPE SetPixelShader(DWORD Handle) override;
    HRESULT STDMETHODCALLTYPE GetPixelShader(DWORD* pHandle) override;
    HRESULT STDMETHODCALLTYPE DeletePixelShader(DWORD Handle) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstant(DWORD Register, const void* pConstantData, DWORD ConstantCount) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstant(DWORD Register, void* pConstantData, DWORD ConstantCount) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) override;
    HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) override;
    HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) override;

    // Custom method to complete deferred OSMesa initialization
    bool complete_deferred_osmesa_init();
    
    // OSMesa framebuffer access methods
    void* get_osmesa_framebuffer() const;
    void get_osmesa_dimensions(int* width, int* height) const;
    bool was_frame_presented() const { return frame_presented_; }
    void reset_frame_presented() { frame_presented_ = false; }
    class DX8OSMesaContext* get_osmesa_context() const;

private:
    // Helper methods
    void flush_command_buffer();
    void execute_command_buffer_async();
    bool validate_present_params(D3DPRESENT_PARAMETERS* params);
    
    // Internal access for friend classes
    
    // Member variables
    std::atomic<LONG> ref_count_;
    Direct3D8* parent_d3d_;
    
    // Device properties
    UINT adapter_;
    D3DDEVTYPE device_type_;
    HWND focus_window_;
    DWORD behavior_flags_;
    D3DPRESENT_PARAMETERS present_params_;
    D3DDEVICE_CREATION_PARAMETERS creation_params_;
    
    // Software rendering contexts
#ifdef DX8GL_HAS_EGL_SURFACELESS
    std::unique_ptr<class EGLSurfacelessContext> egl_context_;
#endif
#ifdef DX8GL_HAS_OSMESA
    std::unique_ptr<class DX8OSMesaContext> osmesa_context_;
    bool osmesa_deferred_init_ = false;  // Flag to defer OSMesa initialization
    int gl_version_major_ = 0;
    int gl_version_minor_ = 0;
    bool requires_vao_ = false;
#endif
    
    // State management
    std::unique_ptr<StateManager> state_manager_;
    
    // Shader management
    std::unique_ptr<VertexShaderManager> vertex_shader_manager_;
    std::unique_ptr<PixelShaderManager> pixel_shader_manager_;
    std::unique_ptr<ShaderProgramManager> shader_program_manager_;
    
    // Command buffer for batching
    std::unique_ptr<CommandBuffer> current_command_buffer_;
    CommandBufferPool command_buffer_pool_;
    
    // Thread pool for parallel command execution
    ThreadPool* thread_pool_;
    
    // Resource tracking
    std::unordered_map<DWORD, IDirect3DBaseTexture8*> textures_;
    std::unordered_map<UINT, IDirect3DVertexBuffer8*> stream_sources_;
    IDirect3DIndexBuffer8* index_buffer_;
    UINT base_vertex_index_;
    
    // Render targets
    IDirect3DSurface8* render_target_;
    IDirect3DSurface8* depth_stencil_;
    std::vector<IDirect3DSurface8*> back_buffers_;
    
    // Scene state
    bool in_scene_;
    bool frame_presented_;
    
    // Vertex processing state
    DWORD current_fvf_;
    
    // Device state for cooperative level
    std::atomic<bool> device_lost_;
    std::atomic<bool> can_reset_device_;
    
    // Synchronization
    mutable std::mutex mutex_;
    std::atomic<uint32_t> frame_count_;
    
    // Helper methods
    HRESULT copy_rect_internal(IDirect3DSurface8* src, const RECT* src_rect,
                              IDirect3DSurface8* dst, const POINT* dst_point);
};

} // namespace dx8gl

#endif // DX8GL_D3D8_DEVICE_H