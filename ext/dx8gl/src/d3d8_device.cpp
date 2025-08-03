#include "d3d8_device.h"
#include "d3d8_interface.h"
#include "d3d8_vertexbuffer.h"
#include "d3d8_indexbuffer.h"
#include "d3d8_texture.h"
#include "d3d8_surface.h"
#include "logger.h"
#include "osmesa_context.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>

// Include OpenGL headers - order is important!
#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/osmesa.h>
// MUST include osmesa_gl_loader.h AFTER GL headers to override function definitions
#include "osmesa_gl_loader.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace dx8gl {

// Global device instance for dx8gl_get_shared_framebuffer
static Direct3DDevice8* g_global_device = nullptr;

Direct3DDevice8* get_global_device() {
    return g_global_device;
}

Direct3DDevice8::Direct3DDevice8(Direct3D8* d3d8, UINT adapter, D3DDEVTYPE device_type,
                                 HWND focus_window, DWORD behavior_flags,
                                 D3DPRESENT_PARAMETERS* presentation_params)
    : ref_count_(1)
    , parent_d3d_(d3d8)
    , adapter_(adapter)
    , device_type_(device_type)
    , focus_window_(focus_window)
    , behavior_flags_(behavior_flags)
    , index_buffer_(nullptr)
    , base_vertex_index_(0)
    , render_target_(nullptr)
    , depth_stencil_(nullptr)
    , in_scene_(false)
    , frame_presented_(false)
    , current_fvf_(0)
    , device_lost_(false)
    , can_reset_device_(false)
    , frame_count_(0) {
    
    // Set global device instance for framebuffer access
    g_global_device = this;
    
    // Copy presentation parameters
    memcpy(&present_params_, presentation_params, sizeof(D3DPRESENT_PARAMETERS));
    
    // Set creation parameters
    creation_params_.AdapterOrdinal = adapter;
    creation_params_.DeviceType = device_type;
    creation_params_.hFocusWindow = focus_window;
    creation_params_.BehaviorFlags = behavior_flags;
    
    // Add reference to parent
    parent_d3d_->AddRef();
    
    // Get thread pool
    thread_pool_ = &get_global_thread_pool();
    
    DX8GL_INFO("Direct3DDevice8 created: adapter=%u, type=%d, flags=0x%08x", 
               adapter, device_type, behavior_flags);
}

Direct3DDevice8::~Direct3DDevice8() {
    DX8GL_INFO("Direct3DDevice8 destructor");
    
    // Clear global device instance
    if (g_global_device == this) {
        g_global_device = nullptr;
    }
    
    // Flush any pending commands
    flush_command_buffer();
    
    // Release resources
    for (auto& tex : textures_) {
        if (tex.second) {
            tex.second->Release();
        }
    }
    textures_.clear();
    
    for (auto& stream : stream_sources_) {
        if (stream.second) {
            stream.second->Release();
        }
    }
    stream_sources_.clear();
    
    if (index_buffer_) {
        index_buffer_->Release();
        index_buffer_ = nullptr;
    }
    
    // Release render targets
    for (auto& bb : back_buffers_) {
        if (bb) {
            bb->Release();
        }
    }
    back_buffers_.clear();
    
    if (render_target_) {
        render_target_->Release();
    }
    
    if (depth_stencil_) {
        depth_stencil_->Release();
    }
    
    // Release parent
    if (parent_d3d_) {
        parent_d3d_->Release();
    }
}

bool Direct3DDevice8::initialize() {
    DX8GL_INFO("Initializing Direct3DDevice8");
    
    // Validate presentation parameters
    if (!validate_present_params(&present_params_)) {
        return false;
    }
    
    // Get window dimensions
    int width = present_params_.BackBufferWidth ? present_params_.BackBufferWidth : 800;
    int height = present_params_.BackBufferHeight ? present_params_.BackBufferHeight : 600;
    
    // dx8gl uses OSMesa-only for software rendering (no EGL complexity)
    DX8GL_INFO("Using OSMesa for software rendering");
    
    // Create OSMesa context
    osmesa_context_ = std::make_unique<DX8OSMesaContext>();
    
    if (!osmesa_context_->initialize(width, height)) {
        DX8GL_ERROR("Failed to initialize OSMesa context: %s", osmesa_context_->get_error());
        return false;
    }
    
    if (!osmesa_context_->make_current()) {
        DX8GL_ERROR("Failed to make OSMesa context current");
        return false;
    }
    
    DX8GL_INFO("OSMesa context initialized successfully");
    
#ifdef DX8GL_HAS_OSMESA
    // Clear any OpenGL errors from initialization
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        DX8GL_DEBUG("Cleared initialization GL error: 0x%04x", error);
    }
    
    // Check actual OpenGL version
    const GLubyte* version_str = glGetString(GL_VERSION);
    if (version_str) {
        DX8GL_INFO("Actual OpenGL version: %s", version_str);
        
        // Parse major.minor version
        int major = 0, minor = 0;
        if (sscanf((const char*)version_str, "%d.%d", &major, &minor) == 2) {
            if (major > 3 || (major == 3 && minor >= 2)) {
                // OpenGL 3.2+ Core Profile requires VAO
                DX8GL_INFO("Using OpenGL %d.%d Core Profile - VAO required", major, minor);
                gl_version_major_ = major;
                gl_version_minor_ = minor;
                requires_vao_ = true;
            } else {
                DX8GL_INFO("Using OpenGL %d.%d - VAO not required", major, minor);
                gl_version_major_ = major;
                gl_version_minor_ = minor;
                requires_vao_ = false;
            }
        }
    }
#endif
    
    // Create state manager
    state_manager_ = std::make_unique<StateManager>();
    if (!state_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize state manager");
        return false;
    }

    // Create shader managers
    vertex_shader_manager_ = std::make_unique<VertexShaderManager>();
    if (!vertex_shader_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize vertex shader manager");
        return false;
    }
    
    pixel_shader_manager_ = std::make_unique<PixelShaderManager>();
    if (!pixel_shader_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize pixel shader manager");
        return false;
    }
    
    shader_program_manager_ = std::make_unique<ShaderProgramManager>();
    if (!shader_program_manager_->initialize(vertex_shader_manager_.get(), pixel_shader_manager_.get())) {
        DX8GL_ERROR("Failed to initialize shader program manager");
        return false;
    }
    
    // Create initial command buffer
    current_command_buffer_ = command_buffer_pool_.acquire();
    
    // Create back buffers
    for (UINT i = 0; i < present_params_.BackBufferCount; ++i) {
        auto surface = new Direct3DSurface8(this, width, height, 
                                          present_params_.BackBufferFormat,
                                          D3DUSAGE_RENDERTARGET);
        if (!surface->initialize()) {
            surface->Release();
            return false;
        }
        back_buffers_.push_back(surface);
    }
    
    // Set initial render target
    if (!back_buffers_.empty()) {
        render_target_ = back_buffers_[0];
        render_target_->AddRef();
    }
    
    // Create depth stencil surface if requested
    if (present_params_.EnableAutoDepthStencil) {
        auto ds = new Direct3DSurface8(this, width, height,
                                      present_params_.AutoDepthStencilFormat,
                                      D3DUSAGE_DEPTHSTENCIL);
        if (!ds->initialize()) {
            ds->Release();
            return false;
        }
        depth_stencil_ = ds;
    }
    
    DX8GL_INFO("Direct3DDevice8 initialized successfully");
    return true;
}

bool Direct3DDevice8::complete_deferred_osmesa_init() {
#ifdef DX8GL_HAS_OSMESA
    if (!osmesa_deferred_init_) {
        // Already initialized or not using deferred init
        return true;
    }
    
    DX8GL_INFO("Completing deferred OSMesa initialization");
    
    // Get window dimensions
    int width = present_params_.BackBufferWidth ? present_params_.BackBufferWidth : 800;
    int height = present_params_.BackBufferHeight ? present_params_.BackBufferHeight : 600;
    
    // Create OSMesa context
    osmesa_context_ = std::make_unique<DX8OSMesaContext>();
    
    if (!osmesa_context_->initialize(width, height)) {
        DX8GL_ERROR("Failed to initialize OSMesa context: %s", osmesa_context_->get_error());
        return false;
    }
    
    if (!osmesa_context_->make_current()) {
        DX8GL_ERROR("Failed to make OSMesa context current");
        return false;
    }
    
    // Create state manager
    state_manager_ = std::make_unique<StateManager>();
    if (!state_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize state manager");
        return false;
    }

    // Create shader managers
    vertex_shader_manager_ = std::make_unique<VertexShaderManager>();
    if (!vertex_shader_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize vertex shader manager");
        return false;
    }
    
    pixel_shader_manager_ = std::make_unique<PixelShaderManager>();
    if (!pixel_shader_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize pixel shader manager");
        return false;
    }
    
    shader_program_manager_ = std::make_unique<ShaderProgramManager>();
    if (!shader_program_manager_->initialize(vertex_shader_manager_.get(), pixel_shader_manager_.get())) {
        DX8GL_ERROR("Failed to initialize shader program manager");
        return false;
    }
    
    // Mark as no longer deferred
    osmesa_deferred_init_ = false;
    
    DX8GL_INFO("Deferred OSMesa initialization completed successfully");
    return true;
#else
    // Not compiled with OSMesa support
    return true;
#endif
}

// IUnknown methods
HRESULT Direct3DDevice8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DDevice8)) {
        *ppvObj = static_cast<IDirect3DDevice8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DDevice8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DDevice8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DDevice8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DDevice8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// Basic device methods
HRESULT Direct3DDevice8::TestCooperativeLevel() {
    // Check if device is lost
    if (device_lost_) {
        // Check if we can reset the device
        if (can_reset_device_) {
            return D3DERR_DEVICENOTRESET;
        }
        return D3DERR_DEVICELOST;
    }
    
    // Check if window is minimized or lost focus (Windows-specific)
    if (focus_window_) {
        // For now, we'll assume the device is always cooperative
        // In a full implementation, we'd check window state
    }
    
    return D3D_OK;
}

UINT Direct3DDevice8::GetAvailableTextureMem() {
    // Return a reasonable amount (256MB)
    return 256 * 1024 * 1024;
}

HRESULT Direct3DDevice8::ResourceManagerDiscardBytes(DWORD Bytes) {
    DX8GL_TRACE("ResourceManagerDiscardBytes(%u)", Bytes);
    // No-op for now
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetDirect3D(IDirect3D8** ppD3D8) {
    if (!ppD3D8) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppD3D8 = parent_d3d_;
    parent_d3d_->AddRef();
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetDeviceCaps(D3DCAPS8* pCaps) {
    if (!pCaps) {
        return D3DERR_INVALIDCALL;
    }
    
    return parent_d3d_->GetDeviceCaps(adapter_, device_type_, pCaps);
}

HRESULT Direct3DDevice8::GetDisplayMode(D3DDISPLAYMODE* pMode) {
    if (!pMode) {
        return D3DERR_INVALIDCALL;
    }
    
    pMode->Width = present_params_.BackBufferWidth;
    pMode->Height = present_params_.BackBufferHeight;
    pMode->RefreshRate = present_params_.FullScreen_RefreshRateInHz;
    pMode->Format = present_params_.BackBufferFormat;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) {
    if (!pParameters) {
        return D3DERR_INVALIDCALL;
    }
    
    *pParameters = creation_params_;
    return D3D_OK;
}

// Scene management
HRESULT Direct3DDevice8::BeginScene() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we need to complete deferred OSMesa initialization
    if (osmesa_deferred_init_) {
        const char* complete_init = std::getenv("DX8GL_COMPLETE_OSMESA_INIT");
        if (complete_init && std::strcmp(complete_init, "1") == 0) {
            DX8GL_INFO("BeginScene: Completing deferred OSMesa initialization");
            if (!complete_deferred_osmesa_init()) {
                DX8GL_ERROR("Failed to complete deferred OSMesa initialization");
                return D3DERR_DEVICELOST;
            }
        }
    }
    
    if (in_scene_) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("BeginScene");
    in_scene_ = true;
    return D3D_OK;
}

HRESULT Direct3DDevice8::EndScene() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!in_scene_) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("EndScene");
    in_scene_ = false;
    
    // Flush command buffer at end of scene
    flush_command_buffer();
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, 
                              D3DCOLOR Color, float Z, DWORD Stencil) {
    DX8GL_INFO("Clear: count=%u, flags=0x%08x, color=0x%08x, z=%.2f, stencil=%u",
               Count, Flags, Color, Z, Stencil);
    
    // Check if we need to complete deferred OSMesa initialization
    if (osmesa_deferred_init_) {
        const char* complete_init = std::getenv("DX8GL_COMPLETE_OSMESA_INIT");
        if (complete_init && std::strcmp(complete_init, "1") == 0) {
            DX8GL_INFO("Clear: Completing deferred OSMesa initialization");
            if (!complete_deferred_osmesa_init()) {
                DX8GL_ERROR("Failed to complete deferred OSMesa initialization");
                return D3DERR_DEVICELOST;
            }
        }
    }
    
    // Only add clear command to buffer - don't execute immediately
    // This prevents double clearing when the command buffer is flushed
    auto cmd = current_command_buffer_->allocate_command_with_data<ClearCmd>(
        Count * sizeof(D3DRECT));
    cmd->count = Count;
    cmd->flags = Flags;
    cmd->color = Color;
    cmd->z = Z;
    cmd->stencil = Stencil;
    
    if (Count > 0 && pRects) {
        D3DRECT* rect_data = static_cast<D3DRECT*>(
            current_command_buffer_->get_command_data(cmd));
        memcpy(rect_data, pRects, Count * sizeof(D3DRECT));
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::Present(const RECT* pSourceRect, const RECT* pDestRect,
                                HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    DX8GL_TRACE("Present: frame=%u", frame_count_.load());
    
    // Flush any pending commands before presenting
    flush_command_buffer();
    
    // Handle partial present if source/dest rectangles are specified
    if (pSourceRect || pDestRect) {
        DX8GL_DEBUG("Partial present: src=%s, dst=%s", 
                    pSourceRect ? "specified" : "full",
                    pDestRect ? "specified" : "full");
        // For now, we ignore partial present and swap the whole buffer
    }
    
    // Handle dirty region optimization
    if (pDirtyRegion) {
        DX8GL_TRACE("Dirty region specified, ignoring for now");
    }
    
    // Check if we need to present to a different window
    if (hDestWindowOverride && hDestWindowOverride != focus_window_) {
        DX8GL_WARNING("Present to different window not supported");
    }
    
    // Ensure we're not in a scene
    if (in_scene_) {
        DX8GL_WARNING("Present called while in scene - ending scene");
        EndScene();
    }
    
    // Handle EGL surfaceless context
#ifdef DX8GL_HAS_EGL_SURFACELESS
    if (egl_context_ && egl_context_->isInitialized()) {
        // EGL surfaceless doesn't have traditional buffer swapping
        // But we call swapBuffers to read the framebuffer and finish rendering
        if (!egl_context_->swapBuffers()) {
            DX8GL_ERROR("EGL swapBuffers failed");
            return D3DERR_DRIVERINTERNALERROR;
        }
        
        // TODO: Copy EGL framebuffer to display or shared memory
        // The framebuffer data is available via egl_context_->getFramebuffer()
        
        // Handle vsync based on presentation interval
        if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE) {
            // No vsync
        } else if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
            // Vsync enabled
        }
    }
#endif
    
    // OSMesa doesn't have buffer swapping - rendering is done to memory
    if (osmesa_context_) {
        // TODO: Copy OSMesa framebuffer to display or shared memory
        
        // Handle vsync based on presentation interval
        if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE) {
            // No vsync - already handled by SDL swap interval
        } else if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
            // Vsync enabled - default SDL behavior
        }
    }
    
    // Increment frame counter after successful present
    frame_count_++;
    
    // Ensure all OpenGL commands are completed
#ifdef DX8GL_HAS_OSMESA
    if (osmesa_context_) {
        glFinish();
    }
#endif
    
    // Mark that a frame was presented for framebuffer access
    frame_presented_ = true;
    
    // Check for device lost conditions
    // In a real implementation, we'd check for:
    // - Window minimized
    // - Display mode changes
    // - GPU reset
    // For now, assume device is always OK
    
    return D3D_OK;
}

// State setting methods
HRESULT Direct3DDevice8::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
    DX8GL_TRACE("SetRenderState: state=%d, value=%u", State, Value);
    
    // Update state manager immediately
    state_manager_->set_render_state(State, Value);
    
    // Also queue command for batched processing
    auto cmd = current_command_buffer_->allocate_command<SetRenderStateCmd>();
    cmd->state = State;
    cmd->value = Value;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
    if (!pValue) {
        return D3DERR_INVALIDCALL;
    }
    
    *pValue = state_manager_->get_render_state(State);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) {
    if (!pMatrix) {
        return D3DERR_INVALIDCALL;
    }
    
    const char* transform_name = "UNKNOWN";
    switch(State) {
        case D3DTS_WORLD: transform_name = "WORLD"; break;
        case D3DTS_VIEW: transform_name = "VIEW"; break;
        case D3DTS_PROJECTION: transform_name = "PROJECTION"; break;
        default: break;
    }
    
    DX8GL_INFO("SetTransform: %s matrix: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
               transform_name,
               pMatrix->_11, pMatrix->_12, pMatrix->_13, pMatrix->_14,
               pMatrix->_21, pMatrix->_22, pMatrix->_23, pMatrix->_24,
               pMatrix->_31, pMatrix->_32, pMatrix->_33, pMatrix->_34,
               pMatrix->_41, pMatrix->_42, pMatrix->_43, pMatrix->_44);
    
    auto cmd = current_command_buffer_->allocate_command<SetTransformCmd>();
    cmd->state = State;
    cmd->matrix = *pMatrix;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    if (!pMatrix) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_transform(State, pMatrix);
    return D3D_OK;
}

// Texture management
HRESULT Direct3DDevice8::SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    if (Stage >= 8) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("SetTexture: stage=%u, texture=%p", Stage, pTexture);
    
    auto cmd = current_command_buffer_->allocate_command<SetTextureCmd>();
    cmd->stage = Stage;
    cmd->texture = reinterpret_cast<uintptr_t>(pTexture);
    
    // Update reference counting
    auto it = textures_.find(Stage);
    if (it != textures_.end() && it->second) {
        it->second->Release();
    }
    
    if (pTexture) {
        pTexture->AddRef();
        textures_[Stage] = pTexture;
    } else {
        textures_.erase(Stage);
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetTexture(DWORD Stage, IDirect3DBaseTexture8** ppTexture) {
    if (!ppTexture || Stage >= 8) {
        return D3DERR_INVALIDCALL;
    }
    
    auto it = textures_.find(Stage);
    if (it != textures_.end()) {
        *ppTexture = it->second;
        if (*ppTexture) {
            (*ppTexture)->AddRef();
        }
    } else {
        *ppTexture = nullptr;
    }
    
    return D3D_OK;
}

// Vertex buffer management
HRESULT Direct3DDevice8::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData,
                                        UINT Stride) {
    if (StreamNumber >= 16) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("SetStreamSource: stream=%u, vb=%p, stride=%u", 
                StreamNumber, pStreamData, Stride);
    
    auto cmd = current_command_buffer_->allocate_command<SetStreamSourceCmd>();
    cmd->stream = StreamNumber;
    cmd->vertex_buffer = reinterpret_cast<uintptr_t>(pStreamData);
    cmd->stride = Stride;
    
    // Update reference counting
    auto it = stream_sources_.find(StreamNumber);
    if (it != stream_sources_.end() && it->second) {
        it->second->Release();
    }
    
    if (pStreamData) {
        pStreamData->AddRef();
        stream_sources_[StreamNumber] = pStreamData;
    } else {
        stream_sources_.erase(StreamNumber);
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    DX8GL_TRACE("SetIndices: ib=%p, base=%u", pIndexData, BaseVertexIndex);
    
    auto cmd = current_command_buffer_->allocate_command<SetIndicesCmd>();
    cmd->index_buffer = reinterpret_cast<uintptr_t>(pIndexData);
    cmd->base_vertex_index = BaseVertexIndex;
    
    // Update reference counting
    if (index_buffer_) {
        index_buffer_->Release();
    }
    
    index_buffer_ = pIndexData;
    base_vertex_index_ = BaseVertexIndex;
    
    if (index_buffer_) {
        index_buffer_->AddRef();
    }
    
    return D3D_OK;
}

// Drawing methods
HRESULT Direct3DDevice8::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, 
                                      UINT StartVertex, UINT PrimitiveCount) {
    if (!in_scene_) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("DrawPrimitive: type=%d, start=%u, count=%u", 
                PrimitiveType, StartVertex, PrimitiveCount);
    
    auto cmd = current_command_buffer_->allocate_command<DrawPrimitiveCmd>();
    cmd->primitive_type = PrimitiveType;
    cmd->start_vertex = StartVertex;
    cmd->primitive_count = PrimitiveCount;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                             UINT MinIndex, UINT NumVertices,
                                             UINT StartIndex, UINT PrimitiveCount) {
    if (!in_scene_) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("DrawIndexedPrimitive: type=%d, min=%u, num=%u, start=%u, count=%u",
                PrimitiveType, MinIndex, NumVertices, StartIndex, PrimitiveCount);
    
    auto cmd = current_command_buffer_->allocate_command<DrawIndexedPrimitiveCmd>();
    cmd->primitive_type = PrimitiveType;
    cmd->min_index = MinIndex;
    cmd->num_vertices = NumVertices;
    cmd->start_index = StartIndex;
    cmd->primitive_count = PrimitiveCount;
    
    return D3D_OK;
}

// Fixed function vertex processing
HRESULT Direct3DDevice8::SetVertexShader(DWORD Handle) {
    DX8GL_INFO("SetVertexShader: handle=0x%08x", Handle);
    
    // Check if this is an FVF or a vertex shader handle
    // Try to set it as a vertex shader first
    if (vertex_shader_manager_ && Handle > 0) {
        HRESULT hr = vertex_shader_manager_->set_vertex_shader(Handle);
        if (SUCCEEDED(hr)) {
            current_fvf_ = 0; // Clear FVF when using vertex shader
            if (shader_program_manager_) {
                shader_program_manager_->invalidate_current_program();
            }
            return hr;
        }
        // If setting vertex shader failed, fall through to treat as FVF
    }
    
    // Handle == 0 (disabled) or not a valid vertex shader handle - use fixed function pipeline
    if (vertex_shader_manager_) {
        vertex_shader_manager_->set_vertex_shader(0); // Disable vertex shader
    }
    current_fvf_ = Handle;
    
    // Update state manager with current FVF
    if (state_manager_) {
        state_manager_->set_current_fvf(Handle);
    }
    
    if (shader_program_manager_) {
        shader_program_manager_->invalidate_current_program();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetVertexShader(DWORD* pHandle) {
    if (!pHandle) {
        return D3DERR_INVALIDCALL;
    }
    
    *pHandle = current_fvf_;
    return D3D_OK;
}

// Helper methods
void Direct3DDevice8::flush_command_buffer() {
    if (!current_command_buffer_ || current_command_buffer_->empty()) {
        return;
    }
    
    DX8GL_INFO("Flushing command buffer: %zu commands, %zu bytes",
               current_command_buffer_->get_command_count(),
               current_command_buffer_->size());
    
    // Execute commands synchronously for now
    // TODO: Make this asynchronous with thread pool
    
    // Ensure OSMesa context is current before executing OpenGL commands
#ifdef DX8GL_HAS_OSMESA
    if (osmesa_context_ && !osmesa_context_->make_current()) {
        DX8GL_ERROR("Failed to make OSMesa context current for command buffer execution");
        return;
    }
#endif
    
    current_command_buffer_->execute(*state_manager_,
                                   vertex_shader_manager_.get(),
                                   pixel_shader_manager_.get(),
                                   shader_program_manager_.get());
    
    // Get new buffer from pool
    current_command_buffer_ = command_buffer_pool_.acquire();
}

bool Direct3DDevice8::validate_present_params(D3DPRESENT_PARAMETERS* params) {
    // Ensure we have valid dimensions
    if (params->BackBufferWidth == 0 || params->BackBufferHeight == 0) {
        if (params->Windowed) {
            // Use default size for windowed mode
            params->BackBufferWidth = 800;
            params->BackBufferHeight = 600;
        } else {
            DX8GL_ERROR("Invalid backbuffer dimensions");
            return false;
        }
    }
    
    // Validate format
    switch (params->BackBufferFormat) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            break;
        default:
            DX8GL_ERROR("Unsupported backbuffer format: %d", params->BackBufferFormat);
            return false;
    }
    
    // Ensure we have at least one back buffer
    if (params->BackBufferCount == 0) {
        params->BackBufferCount = 1;
    }
    
    return true;
}

// Stub implementations for remaining methods
// These will be implemented as needed

HRESULT Direct3DDevice8::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, 
                                            IDirect3DSurface8* pCursorBitmap) {
    return D3DERR_NOTAVAILABLE;
}

void Direct3DDevice8::SetCursorPosition(int X, int Y, DWORD Flags) {
    // No-op in headless mode
}

BOOL Direct3DDevice8::ShowCursor(BOOL bShow) {
    return FALSE;
}

HRESULT Direct3DDevice8::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  IDirect3DSwapChain8** ppSwapChain) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (!pPresentationParameters) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("Reset device with new presentation parameters");
    
    // Validate new presentation parameters
    if (!validate_present_params(pPresentationParameters)) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Flush any pending commands
    flush_command_buffer();
    
    // Release current render targets and back buffers
    if (render_target_) {
        render_target_->Release();
        render_target_ = nullptr;
    }
    
    if (depth_stencil_) {
        depth_stencil_->Release();
        depth_stencil_ = nullptr;
    }
    
    for (auto& bb : back_buffers_) {
        if (bb) {
            bb->Release();
        }
    }
    back_buffers_.clear();
    
    // Update presentation parameters
    memcpy(&present_params_, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));
    
    // Get dimensions
    int width = present_params_.BackBufferWidth ? present_params_.BackBufferWidth : 800;
    int height = present_params_.BackBufferHeight ? present_params_.BackBufferHeight : 600;
    
    // Resize OSMesa context with new dimensions
    if (osmesa_context_) {
        
        if (!osmesa_context_->resize(width, height)) {
            DX8GL_ERROR("Failed to resize OSMesa context");
            return D3DERR_DEVICELOST;
        }
        
        // Make context current
        if (!osmesa_context_->make_current()) {
            DX8GL_ERROR("Failed to make OSMesa context current after reset");
            return D3DERR_DEVICELOST;
        }
    }
    
    // Recreate back buffers
    for (UINT i = 0; i < present_params_.BackBufferCount; ++i) {
        auto surface = new Direct3DSurface8(this, width, height, 
                                          present_params_.BackBufferFormat,
                                          D3DUSAGE_RENDERTARGET);
        if (!surface->initialize()) {
            surface->Release();
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        back_buffers_.push_back(surface);
    }
    
    // Set initial render target
    if (!back_buffers_.empty()) {
        render_target_ = back_buffers_[0];
        render_target_->AddRef();
    }
    
    // Recreate depth stencil surface if requested
    if (present_params_.EnableAutoDepthStencil) {
        auto ds = new Direct3DSurface8(this, width, height,
                                      present_params_.AutoDepthStencilFormat,
                                      D3DUSAGE_DEPTHSTENCIL);
        if (!ds->initialize()) {
            ds->Release();
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        depth_stencil_ = ds;
    }
    
    // Reset state manager
    state_manager_->reset();
    
    // Reset viewport to full window
    D3DVIEWPORT8 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    SetViewport(&viewport);
    
    // Clear device lost state
    device_lost_ = false;
    can_reset_device_ = false;
    
    DX8GL_INFO("Device reset complete: %dx%d", width, height);
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                      IDirect3DSurface8** ppBackBuffer) {
    if (!ppBackBuffer) {
        return D3DERR_INVALIDCALL;
    }
    
    if (BackBuffer >= back_buffers_.size()) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppBackBuffer = back_buffers_[BackBuffer];
    (*ppBackBuffer)->AddRef();
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) {
    if (!pRasterStatus) {
        return D3DERR_INVALIDCALL;
    }
    
    pRasterStatus->InVBlank = FALSE;
    pRasterStatus->ScanLine = 0;
    
    return D3D_OK;
}

void Direct3DDevice8::SetGammaRamp(DWORD Flags, const D3DGAMMARAMP* pRamp) {
    // No-op in headless mode
}

void Direct3DDevice8::GetGammaRamp(D3DGAMMARAMP* pRamp) {
    if (pRamp) {
        // Return linear ramp
        for (int i = 0; i < 256; i++) {
            WORD value = (i * 65535) / 255;
            pRamp->red[i] = value;
            pRamp->green[i] = value;
            pRamp->blue[i] = value;
        }
    }
}

// Create vertex buffer
HRESULT Direct3DDevice8::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF,
                                           D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    if (!ppVertexBuffer) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("CreateVertexBuffer: length=%u, usage=0x%08x, fvf=0x%08x, pool=%d",
               Length, Usage, FVF, Pool);
    
    auto vb = new Direct3DVertexBuffer8(this, Length, Usage, FVF, Pool);
    if (!vb->initialize()) {
        vb->Release();
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    
    *ppVertexBuffer = vb;
    return D3D_OK;
}

// Remaining method stubs...
HRESULT Direct3DDevice8::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                      D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) {
    if (!ppTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    auto texture = new Direct3DTexture8(this, Width, Height, Levels, Usage, Format, Pool);
    if (!texture->initialize()) {
        texture->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    *ppTexture = texture;
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format,
                                          D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    if (!ppIndexBuffer) {
        return D3DERR_INVALIDCALL;
    }
    
    auto buffer = new Direct3DIndexBuffer8(this, Length, Usage, Format, Pool);
    if (!buffer->initialize()) {
        buffer->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    *ppIndexBuffer = buffer;
    return D3D_OK;
}

// Remaining stub methods

HRESULT Direct3DDevice8::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels,
                                           DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                           IDirect3DVolumeTexture8** ppVolumeTexture) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage,
                                         D3DFORMAT Format, D3DPOOL Pool,
                                         IDirect3DCubeTexture8** ppCubeTexture) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format,
                                          D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable,
                                          IDirect3DSurface8** ppSurface) {
    if (!ppSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    auto surface = new Direct3DSurface8(this, Width, Height, Format, D3DUSAGE_RENDERTARGET);
    if (!surface->initialize()) {
        surface->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    *ppSurface = surface;
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                                  D3DMULTISAMPLE_TYPE MultiSample,
                                                  IDirect3DSurface8** ppSurface) {
    if (!ppSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    auto surface = new Direct3DSurface8(this, Width, Height, Format, D3DUSAGE_DEPTHSTENCIL);
    if (!surface->initialize()) {
        surface->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    *ppSurface = surface;
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                          IDirect3DSurface8** ppSurface) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CopyRects(IDirect3DSurface8* pSourceSurface, const RECT* pSourceRectsArray,
                                 UINT cRects, IDirect3DSurface8* pDestinationSurface,
                                 const POINT* pDestPointsArray) {
    if (!pSourceSurface || !pDestinationSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("CopyRects: %u rectangles", cRects);
    
    // Get surface descriptions
    D3DSURFACE_DESC src_desc, dst_desc;
    HRESULT hr = pSourceSurface->GetDesc(&src_desc);
    if (FAILED(hr)) return hr;
    
    hr = pDestinationSurface->GetDesc(&dst_desc);
    if (FAILED(hr)) return hr;
    
    // If no rectangles specified, copy entire surface
    if (cRects == 0 || pSourceRectsArray == nullptr) {
        RECT src_rect = { 0, 0, static_cast<LONG>(src_desc.Width), static_cast<LONG>(src_desc.Height) };
        POINT dst_point = { 0, 0 };
        
        return copy_rect_internal(pSourceSurface, &src_rect, pDestinationSurface, &dst_point);
    }
    
    // Copy each rectangle
    for (UINT i = 0; i < cRects; i++) {
        const RECT* src_rect = &pSourceRectsArray[i];
        POINT dst_point = { 0, 0 };
        
        if (pDestPointsArray) {
            dst_point = pDestPointsArray[i];
        }
        
        hr = copy_rect_internal(pSourceSurface, src_rect, pDestinationSurface, &dst_point);
        if (FAILED(hr)) return hr;
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::UpdateTexture(IDirect3DBaseTexture8* pSourceTexture,
                                     IDirect3DBaseTexture8* pDestinationTexture) {
    if (!pSourceTexture || !pDestinationTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    // Get resource types
    D3DRESOURCETYPE src_type = pSourceTexture->GetType();
    D3DRESOURCETYPE dst_type = pDestinationTexture->GetType();
    
    if (src_type != dst_type) {
        DX8GL_WARNING("UpdateTexture: Source and destination must be same type");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("UpdateTexture: type=%d", src_type);
    
    switch (src_type) {
        case D3DRTYPE_TEXTURE: {
            // Standard 2D texture
            auto* src_tex = static_cast<IDirect3DTexture8*>(pSourceTexture);
            auto* dst_tex = static_cast<IDirect3DTexture8*>(pDestinationTexture);
            
            // Get level counts
            DWORD src_levels = src_tex->GetLevelCount();
            DWORD dst_levels = dst_tex->GetLevelCount();
            DWORD levels_to_copy = std::min(src_levels, dst_levels);
            
            // Copy each mip level
            for (DWORD level = 0; level < levels_to_copy; level++) {
                IDirect3DSurface8* src_surface = nullptr;
                IDirect3DSurface8* dst_surface = nullptr;
                
                HRESULT hr = src_tex->GetSurfaceLevel(level, &src_surface);
                if (FAILED(hr)) {
                    DX8GL_WARNING("Failed to get source surface level %u", level);
                    return hr;
                }
                
                hr = dst_tex->GetSurfaceLevel(level, &dst_surface);
                if (FAILED(hr)) {
                    src_surface->Release();
                    DX8GL_WARNING("Failed to get destination surface level %u", level);
                    return hr;
                }
                
                // Use CopyRects to copy the entire surface
                hr = CopyRects(src_surface, nullptr, 0, dst_surface, nullptr);
                
                src_surface->Release();
                dst_surface->Release();
                
                if (FAILED(hr)) {
                    DX8GL_WARNING("Failed to copy texture level %u", level);
                    return hr;
                }
            }
            
            return D3D_OK;
        }
        
        case D3DRTYPE_CUBETEXTURE: {
            // Cube texture support not implemented yet
            // TODO: Implement IDirect3DCubeTexture8 interface
            DX8GL_WARNING("UpdateTexture: Cube textures not implemented");
            return D3DERR_NOTAVAILABLE;
        }
        
        case D3DRTYPE_VOLUMETEXTURE: {
            // Volume textures not yet supported
            DX8GL_WARNING("UpdateTexture: Volume textures not implemented");
            return D3DERR_NOTAVAILABLE;
        }
        
        default:
            return D3DERR_INVALIDCALL;
    }
}

HRESULT Direct3DDevice8::GetFrontBuffer(IDirect3DSurface8* pDestSurface) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::SetRenderTarget(IDirect3DSurface8* pRenderTarget,
                                       IDirect3DSurface8* pNewZStencil) {
    DX8GL_TRACE("SetRenderTarget: rt=%p, ds=%p", pRenderTarget, pNewZStencil);
    
    // Validate render target
    if (pRenderTarget) {
        // Make sure it's a render target surface
        Direct3DSurface8* surface = static_cast<Direct3DSurface8*>(pRenderTarget);
        D3DSURFACE_DESC desc;
        surface->GetDesc(&desc);
        
        if (!(desc.Usage & D3DUSAGE_RENDERTARGET)) {
            DX8GL_ERROR("Surface is not a render target");
            return D3DERR_INVALIDCALL;
        }
        
        // Check if depth stencil format is compatible with render target
        if (pNewZStencil) {
            Direct3DSurface8* ds_surface = static_cast<Direct3DSurface8*>(pNewZStencil);
            D3DSURFACE_DESC ds_desc;
            ds_surface->GetDesc(&ds_desc);
            
            if (!(ds_desc.Usage & D3DUSAGE_DEPTHSTENCIL)) {
                DX8GL_ERROR("Surface is not a depth stencil");
                return D3DERR_INVALIDCALL;
            }
            
            // Check dimensions match
            if (desc.Width != ds_desc.Width || desc.Height != ds_desc.Height) {
                DX8GL_ERROR("Render target and depth stencil dimensions don't match");
                return D3DERR_INVALIDCALL;
            }
        }
    }
    
    // Release old render target
    if (render_target_) {
        render_target_->Release();
    }
    
    // Set new render target
    render_target_ = static_cast<Direct3DSurface8*>(pRenderTarget);
    if (render_target_) {
        render_target_->AddRef();
        
        // Bind the render target's FBO
        Direct3DSurface8* rt_surface = static_cast<Direct3DSurface8*>(render_target_);
        GLuint fbo = rt_surface->get_fbo();
        
        if (fbo != 0) {
            // Bind custom FBO
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            
            // Check framebuffer completeness
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                DX8GL_ERROR("Framebuffer incomplete: 0x%x", status);
                render_target_->Release();
                render_target_ = nullptr;
                return D3DERR_INVALIDCALL;
            }
        } else {
            // Back buffer - bind default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        // Update viewport to match render target dimensions
        D3DSURFACE_DESC desc;
        rt_surface->GetDesc(&desc);
        DX8GL_INFO("Setting viewport: %dx%d", desc.Width, desc.Height);
        glViewport(0, 0, desc.Width, desc.Height);
    } else {
        // No render target - bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // Update depth stencil
    if (depth_stencil_) {
        depth_stencil_->Release();
    }
    
    depth_stencil_ = static_cast<Direct3DSurface8*>(pNewZStencil);
    if (depth_stencil_) {
        depth_stencil_->AddRef();
        
        // If we have a custom render target FBO, attach the depth stencil
        if (render_target_) {
            Direct3DSurface8* rt_surface = static_cast<Direct3DSurface8*>(render_target_);
            GLuint fbo = rt_surface->get_fbo();
            
            if (fbo != 0) {
                Direct3DSurface8* ds_surface = static_cast<Direct3DSurface8*>(depth_stencil_);
                GLuint ds_texture = ds_surface->get_gl_texture();
                
                // Determine attachment point based on format
                D3DSURFACE_DESC ds_desc;
                ds_surface->GetDesc(&ds_desc);
                
                if (ds_desc.Format == D3DFMT_D24S8 || ds_desc.Format == D3DFMT_D24X8) {
                    // Depth + Stencil - attach to both points in ES 2.0
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                                         GL_TEXTURE_2D, ds_texture, 0);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 
                                         GL_TEXTURE_2D, ds_texture, 0);
                } else {
                    // Depth only
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                         GL_TEXTURE_2D, ds_texture, 0);
                }
            }
        }
    }
    
    DX8GL_INFO("Render target set: %dx%d", 
               render_target_ ? static_cast<Direct3DSurface8*>(render_target_)->get_width() : 0,
               render_target_ ? static_cast<Direct3DSurface8*>(render_target_)->get_height() : 0);
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetRenderTarget(IDirect3DSurface8** ppRenderTarget) {
    if (!ppRenderTarget) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppRenderTarget = render_target_;
    if (render_target_) {
        render_target_->AddRef();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetDepthStencilSurface(IDirect3DSurface8** ppZStencilSurface) {
    if (!ppZStencilSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppZStencilSurface = depth_stencil_;
    if (depth_stencil_) {
        depth_stencil_->AddRef();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::SetViewport(const D3DVIEWPORT8* pViewport) {
    if (!pViewport) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->set_viewport(pViewport);
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetViewport(D3DVIEWPORT8* pViewport) {
    if (!pViewport) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_viewport(pViewport);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetMaterial(const D3DMATERIAL8* pMaterial) {
    if (!pMaterial) {
        return D3DERR_INVALIDCALL;
    }
    
    // Update state manager immediately
    state_manager_->set_material(pMaterial);
    
    // Also queue command for batched processing
    auto cmd = current_command_buffer_->allocate_command<SetMaterialCmd>();
    cmd->material = *pMaterial;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetMaterial(D3DMATERIAL8* pMaterial) {
    if (!pMaterial) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_material(pMaterial);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetLight(DWORD Index, const D3DLIGHT8* pLight) {
    if (!pLight) {
        return D3DERR_INVALIDCALL;
    }
    
    // Update state manager immediately
    state_manager_->set_light(Index, pLight);
    
    // Also queue command for batched processing
    auto cmd = current_command_buffer_->allocate_command<SetLightCmd>();
    cmd->index = Index;
    cmd->light = *pLight;
    cmd->enable = state_manager_->is_light_enabled(Index);
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetLight(DWORD Index, D3DLIGHT8* pLight) {
    if (!pLight) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_light(Index, pLight);
    return D3D_OK;
}

HRESULT Direct3DDevice8::LightEnable(DWORD Index, BOOL Enable) {
    // Update state manager immediately
    state_manager_->light_enable(Index, Enable);
    
    // Also queue command for batched processing
    // Get the current light properties to include in the command
    D3DLIGHT8 light;
    state_manager_->get_light(Index, &light);
    
    auto cmd = current_command_buffer_->allocate_command<SetLightCmd>();
    cmd->index = Index;
    cmd->light = light;
    cmd->enable = Enable;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetLightEnable(DWORD Index, BOOL* pEnable) {
    if (!pEnable) {
        return D3DERR_INVALIDCALL;
    }
    
    *pEnable = state_manager_->is_light_enabled(Index);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetClipPlane(DWORD Index, const float* pPlane) {
    if (!pPlane) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->set_clip_plane(Index, pPlane);
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetClipPlane(DWORD Index, float* pPlane) {
    if (!pPlane) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_clip_plane(Index, pPlane);
    return D3D_OK;
}

HRESULT Direct3DDevice8::BeginStateBlock() {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::EndStateBlock(DWORD* pToken) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::ApplyStateBlock(DWORD Token) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CaptureStateBlock(DWORD Token) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::DeleteStateBlock(DWORD Token) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::SetClipStatus(const D3DCLIPSTATUS8* pClipStatus) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::GetClipStatus(D3DCLIPSTATUS8* pClipStatus) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type,
                                             DWORD* pValue) {
    if (!pValue) {
        return D3DERR_INVALIDCALL;
    }
    
    *pValue = state_manager_->get_texture_stage_state(Stage, Type);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type,
                                             DWORD Value) {
    state_manager_->set_texture_stage_state(Stage, Type, Value);
    return D3D_OK;
}

HRESULT Direct3DDevice8::ValidateDevice(DWORD* pNumPasses) {
    if (!pNumPasses) {
        return D3DERR_INVALIDCALL;
    }
    
    *pNumPasses = 1;
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetInfo(DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY* pEntries) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::SetCurrentTexturePalette(UINT PaletteNumber) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::GetCurrentTexturePalette(UINT* PaletteNumber) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                                        const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    if (!pVertexStreamZeroData || PrimitiveCount == 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (!in_scene_) {
        DX8GL_WARNING("DrawPrimitiveUP called outside of scene");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("DrawPrimitiveUP: type=%d, count=%u, stride=%u", 
               PrimitiveType, PrimitiveCount, VertexStreamZeroStride);
    
    // Calculate vertex count from primitive count
    UINT vertex_count = 0;
    switch (PrimitiveType) {
        case D3DPT_POINTLIST:
            vertex_count = PrimitiveCount;
            break;
        case D3DPT_LINELIST:
            vertex_count = PrimitiveCount * 2;
            break;
        case D3DPT_LINESTRIP:
            vertex_count = PrimitiveCount + 1;
            break;
        case D3DPT_TRIANGLELIST:
            vertex_count = PrimitiveCount * 3;
            break;
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            vertex_count = PrimitiveCount + 2;
            break;
        default:
            return D3DERR_INVALIDCALL;
    }
    
    // Calculate data size
    size_t data_size = vertex_count * VertexStreamZeroStride;
    
    // Record immediate mode draw command
    current_command_buffer_->draw_primitive_up(
        PrimitiveType,
        0,  // start vertex
        PrimitiveCount,
        pVertexStreamZeroData,
        data_size,
        VertexStreamZeroStride,
        current_fvf_
    );
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex,
                                               UINT NumVertexIndices, UINT PrimitiveCount,
                                               const void* pIndexData, D3DFORMAT IndexDataFormat,
                                               const void* pVertexStreamZeroData,
                                               UINT VertexStreamZeroStride) {
    if (!pIndexData || !pVertexStreamZeroData || PrimitiveCount == 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (!in_scene_) {
        DX8GL_WARNING("DrawIndexedPrimitiveUP called outside of scene");
        return D3DERR_INVALIDCALL;
    }
    
    // Validate index format
    if (IndexDataFormat != D3DFMT_INDEX16 && IndexDataFormat != D3DFMT_INDEX32) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("DrawIndexedPrimitiveUP: type=%d, minIdx=%u, numVerts=%u, count=%u, idxFmt=%d, stride=%u",
                PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount, 
                IndexDataFormat, VertexStreamZeroStride);
    
    // Calculate index count from primitive count
    UINT index_count = 0;
    switch (PrimitiveType) {
        case D3DPT_POINTLIST:
            index_count = PrimitiveCount;
            break;
        case D3DPT_LINELIST:
            index_count = PrimitiveCount * 2;
            break;
        case D3DPT_LINESTRIP:
            index_count = PrimitiveCount + 1;
            break;
        case D3DPT_TRIANGLELIST:
            index_count = PrimitiveCount * 3;
            break;
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            index_count = PrimitiveCount + 2;
            break;
        default:
            return D3DERR_INVALIDCALL;
    }
    
    // Calculate data sizes
    size_t vertex_data_size = NumVertexIndices * VertexStreamZeroStride;
    size_t index_size = (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
    size_t index_data_size = index_count * index_size;
    
    // Record immediate mode indexed draw command
    current_command_buffer_->draw_indexed_primitive_up(
        PrimitiveType,
        MinVertexIndex,
        NumVertexIndices,
        PrimitiveCount,
        pIndexData,
        index_data_size,
        IndexDataFormat,
        pVertexStreamZeroData,
        vertex_data_size,
        VertexStreamZeroStride,
        current_fvf_
    );
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
                                       IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CreateVertexShader(const DWORD* pDeclaration, const DWORD* pFunction,
                                           DWORD* pHandle, DWORD Usage) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->create_vertex_shader(pDeclaration, pFunction, pHandle, Usage);
}

HRESULT Direct3DDevice8::DeleteVertexShader(DWORD Handle) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->delete_vertex_shader(Handle);
}

HRESULT Direct3DDevice8::SetVertexShaderConstant(DWORD Register, const void* pConstantData,
                                                DWORD ConstantCount) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->set_vertex_shader_constant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8::GetVertexShaderConstant(DWORD Register, void* pConstantData,
                                                DWORD ConstantCount) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->get_vertex_shader_constant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8::GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->get_vertex_shader_declaration(Handle, pData, pSizeOfData);
}

HRESULT Direct3DDevice8::GetVertexShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) {
    if (!vertex_shader_manager_) {
        return D3DERR_NOTAVAILABLE;
    }
    
    return vertex_shader_manager_->get_vertex_shader_function(Handle, pData, pSizeOfData);
}

HRESULT Direct3DDevice8::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData,
                                       UINT* pStride) {
    if (!ppStreamData || !pStride) {
        return D3DERR_INVALIDCALL;
    }
    
    auto it = stream_sources_.find(StreamNumber);
    if (it != stream_sources_.end()) {
        *ppStreamData = it->second;
        if (*ppStreamData) {
            (*ppStreamData)->AddRef();
        }
        // TODO: Store and retrieve stride
        *pStride = 0;
    } else {
        *ppStreamData = nullptr;
        *pStride = 0;
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) {
    if (!ppIndexData || !pBaseVertexIndex) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppIndexData = index_buffer_;
    *pBaseVertexIndex = base_vertex_index_;
    
    if (index_buffer_) {
        index_buffer_->AddRef();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreatePixelShader(const DWORD* pFunction, DWORD* pHandle) {
    return pixel_shader_manager_->create_pixel_shader(pFunction, pHandle);
}

HRESULT Direct3DDevice8::SetPixelShader(DWORD Handle) {
    return pixel_shader_manager_->set_pixel_shader(Handle);
}

HRESULT Direct3DDevice8::GetPixelShader(DWORD* pHandle) {
    if (!pHandle) {
        return D3DERR_INVALIDCALL;
    }
    
    *pHandle = pixel_shader_manager_->get_current_shader_handle();
    return D3D_OK;
}

HRESULT Direct3DDevice8::DeletePixelShader(DWORD Handle) {
    return pixel_shader_manager_->delete_pixel_shader(Handle);
}

HRESULT Direct3DDevice8::SetPixelShaderConstant(DWORD Register, const void* pConstantData,
                                               DWORD ConstantCount) {
    return pixel_shader_manager_->set_pixel_shader_constant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8::GetPixelShaderConstant(DWORD Register, void* pConstantData,
                                               DWORD ConstantCount) {
    return pixel_shader_manager_->get_pixel_shader_constant(Register, pConstantData, ConstantCount);
}

HRESULT Direct3DDevice8::GetPixelShaderFunction(DWORD Handle, void* pData, DWORD* pSizeOfData) {
    return pixel_shader_manager_->get_pixel_shader_function(Handle, pData, pSizeOfData);
}

HRESULT Direct3DDevice8::DrawRectPatch(UINT Handle, const float* pNumSegs,
                                     const D3DRECTPATCH_INFO* pRectPatchInfo) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::DrawTriPatch(UINT Handle, const float* pNumSegs,
                                    const D3DTRIPATCH_INFO* pTriPatchInfo) {
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::DeletePatch(UINT Handle) {
    return D3DERR_NOTAVAILABLE;
}

// Helper method implementation
HRESULT Direct3DDevice8::copy_rect_internal(IDirect3DSurface8* src, const RECT* src_rect,
                                          IDirect3DSurface8* dst, const POINT* dst_point) {
    // Get surface descriptions
    D3DSURFACE_DESC src_desc, dst_desc;
    HRESULT hr = src->GetDesc(&src_desc);
    if (FAILED(hr)) return hr;
    
    hr = dst->GetDesc(&dst_desc);
    if (FAILED(hr)) return hr;
    
    // Calculate copy dimensions
    LONG src_x = src_rect ? src_rect->left : 0;
    LONG src_y = src_rect ? src_rect->top : 0;
    LONG src_width = src_rect ? (src_rect->right - src_rect->left) : src_desc.Width;
    LONG src_height = src_rect ? (src_rect->bottom - src_rect->top) : src_desc.Height;
    
    LONG dst_x = dst_point ? dst_point->x : 0;
    LONG dst_y = dst_point ? dst_point->y : 0;
    
    // Clip to destination bounds
    if (dst_x + src_width > static_cast<LONG>(dst_desc.Width)) {
        src_width = dst_desc.Width - dst_x;
    }
    if (dst_y + src_height > static_cast<LONG>(dst_desc.Height)) {
        src_height = dst_desc.Height - dst_y;
    }
    
    // Check if formats are compatible
    if (src_desc.Format != dst_desc.Format) {
        DX8GL_WARNING("CopyRects: Format conversion not supported");
        return D3DERR_INVALIDCALL;
    }
    
    // Lock both surfaces
    D3DLOCKED_RECT src_locked, dst_locked;
    RECT lock_src_rect = { src_x, src_y, src_x + src_width, src_y + src_height };
    RECT lock_dst_rect = { dst_x, dst_y, dst_x + src_width, dst_y + src_height };
    
    hr = src->LockRect(&src_locked, &lock_src_rect, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;
    
    hr = dst->LockRect(&dst_locked, &lock_dst_rect, 0);
    if (FAILED(hr)) {
        src->UnlockRect();
        return hr;
    }
    
    // Copy data
    UINT bytes_per_pixel = 0;
    switch (src_desc.Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bytes_per_pixel = 4;
            break;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
            bytes_per_pixel = 2;
            break;
        case D3DFMT_R8G8B8:
            bytes_per_pixel = 3;
            break;
        case D3DFMT_A8:
        case D3DFMT_L8:
            bytes_per_pixel = 1;
            break;
        case D3DFMT_A8L8:
            bytes_per_pixel = 2;
            break;
        default:
            DX8GL_WARNING("CopyRects: Unsupported format %d", src_desc.Format);
            src->UnlockRect();
            dst->UnlockRect();
            return D3DERR_INVALIDCALL;
    }
    
    // Perform the copy
    const uint8_t* src_bits = static_cast<const uint8_t*>(src_locked.pBits);
    uint8_t* dst_bits = static_cast<uint8_t*>(dst_locked.pBits);
    
    for (LONG y = 0; y < src_height; y++) {
        memcpy(dst_bits + y * dst_locked.Pitch,
               src_bits + y * src_locked.Pitch,
               src_width * bytes_per_pixel);
    }
    
    // Unlock surfaces
    dst->UnlockRect();
    src->UnlockRect();
    
    return D3D_OK;
}

void* Direct3DDevice8::get_osmesa_framebuffer() const {
#ifdef DX8GL_HAS_OSMESA
    if (osmesa_context_) {
        return osmesa_context_->get_framebuffer();
    }
#endif
    return nullptr;
}

void Direct3DDevice8::get_osmesa_dimensions(int* width, int* height) const {
#ifdef DX8GL_HAS_OSMESA
    if (osmesa_context_) {
        if (width) *width = osmesa_context_->get_width();
        if (height) *height = osmesa_context_->get_height();
        return;
    }
#endif
    if (width) *width = 0;
    if (height) *height = 0;
}

DX8OSMesaContext* Direct3DDevice8::get_osmesa_context() const {
#ifdef DX8GL_HAS_OSMESA
    return osmesa_context_.get();
#else
    return nullptr;
#endif
}

} // namespace dx8gl