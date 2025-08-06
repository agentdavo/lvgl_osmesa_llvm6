#include "d3d8_device.h"
#include "d3d8_interface.h"
#include "d3d8_vertexbuffer.h"
#include "d3d8_indexbuffer.h"
#include "d3d8_texture.h"
#include "d3d8_cubetexture.h"
#include "d3d8_surface.h"
#include "logger.h"
#include "osmesa_context.h"
#include "render_backend.h"
#include "dx8gl.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <sstream>

// No need for extern declaration - we use get_render_backend() function

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

// Check if a value is a valid FVF (not a vertex shader handle)
#define FVF_IS_VALID_FVF(x) ((x) < 0x100)

// Multithreaded synchronization macros
#define MULTITHREADED_LOCK() \
    std::unique_lock<std::mutex> _mt_lock(is_multithreaded_ ? g_multithreaded_mutex : mutex_, std::defer_lock); \
    if (is_multithreaded_) _mt_lock.lock()

// FPU preservation macros
#ifdef _MSC_VER
#include <float.h>
#define FPU_PRESERVE_DECL unsigned int _fpu_cw
#define FPU_PRESERVE_SAVE() if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) { _fpu_cw = _control87(0, 0); }
#define FPU_PRESERVE_RESTORE() if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) { _control87(_fpu_cw, 0xFFFFFFFF); }
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define FPU_PRESERVE_DECL unsigned short _fpu_cw
#define FPU_PRESERVE_SAVE() if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) { __asm__ __volatile__ ("fnstcw %0" : "=m" (_fpu_cw)); }
#define FPU_PRESERVE_RESTORE() if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) { __asm__ __volatile__ ("fldcw %0" : : "m" (_fpu_cw)); }
#else
// No FPU preservation on other platforms
#define FPU_PRESERVE_DECL
#define FPU_PRESERVE_SAVE()
#define FPU_PRESERVE_RESTORE()
#endif

namespace dx8gl {

// Global device instance for dx8gl_get_shared_framebuffer
static Direct3DDevice8* g_global_device = nullptr;

// Global mutex for multithreaded device protection
static std::mutex g_multithreaded_mutex;

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
    , frame_count_(0)
    , is_multithreaded_(behavior_flags & D3DCREATE_MULTITHREADED) {
    
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
    
    // Get thread pool (kept for potential future use)
    thread_pool_ = &get_global_thread_pool();
    
    // Initialize render thread pointer (will be created in complete_deferred_osmesa_init)
    render_thread_ = nullptr;
    
    // Initialize statistics
    current_stats_.reset();
    last_frame_stats_.reset();
    
    DX8GL_INFO("Direct3DDevice8 created: adapter=%u, type=%d, flags=0x%08x%s", 
               adapter, device_type, behavior_flags,
               is_multithreaded_ ? " (MULTITHREADED)" : "");
}

Direct3DDevice8::~Direct3DDevice8() {
    DX8GL_INFO("Direct3DDevice8 destructor");
    
    // Clear global device instance
    if (g_global_device == this) {
        g_global_device = nullptr;
    }
    
    // Flush any pending commands
    flush_command_buffer();
    
    // Stop and clean up render thread
    if (render_thread_) {
        render_thread_->stop();
        delete render_thread_;
        render_thread_ = nullptr;
    }
    
    // Release resources
    for (auto& tex : textures_) {
        if (tex.second) {
            tex.second->Release();
        }
    }
    textures_.clear();
    
    for (auto& stream : stream_sources_) {
        if (stream.second.vertex_buffer) {
            stream.second.vertex_buffer->Release();
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

void Direct3DDevice8::set_default_global_render_states() {
    DX8GL_INFO("Setting default global render states");
    
    // Fog-related states (matching reference wrapper)
    // Note: We check caps for range fog support
    // FIXME: GetDeviceCaps seems to hang, skip for now
    //D3DCAPS8 caps;
    //DX8GL_INFO("Getting device caps...");
    //parent_d3d_->GetDeviceCaps(adapter_, device_type_, &caps);
    //DX8GL_INFO("Got device caps");
    
    state_manager_->set_render_state(D3DRS_RANGEFOGENABLE, FALSE); // Default to false for now
        //(caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? TRUE : FALSE);
    state_manager_->set_render_state(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    state_manager_->set_render_state(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    
    // Material color source states
    state_manager_->set_render_state(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    state_manager_->set_render_state(D3DRS_COLORVERTEX, TRUE);
    
    // Z-bias (depth bias)
    state_manager_->set_render_state(D3DRS_ZBIAS, 0);
    
    // Bump mapping environment parameters
    // These are texture stage states for bump mapping
    auto F2DW = [](float f) -> DWORD { return *reinterpret_cast<DWORD*>(&f); };
    
    // Bump environment luminance scale and offset for stage 1
    state_manager_->set_texture_stage_state(1, D3DTSS_BUMPENVLSCALE, F2DW(1.0f));
    state_manager_->set_texture_stage_state(1, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f));
    
    // Bump environment matrix for stage 0 (identity matrix)
    state_manager_->set_texture_stage_state(0, D3DTSS_BUMPENVMAT00, F2DW(1.0f));
    state_manager_->set_texture_stage_state(0, D3DTSS_BUMPENVMAT01, F2DW(0.0f));
    state_manager_->set_texture_stage_state(0, D3DTSS_BUMPENVMAT10, F2DW(0.0f));
    state_manager_->set_texture_stage_state(0, D3DTSS_BUMPENVMAT11, F2DW(1.0f));
    
    DX8GL_INFO("Default global render states set");
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
    
    // Use the global render backend instead of creating our own context
    render_backend_ = dx8gl::get_render_backend();
    if (!render_backend_) {
        DX8GL_ERROR("No render backend available. Call dx8gl_init() first.");
        return false;
    }
    
    // Resize backend to match requested dimensions
    if (!render_backend_->resize(width, height)) {
        DX8GL_ERROR("Failed to resize render backend");
        return false;
    }
    
    // Make context current
    if (!render_backend_->make_current()) {
        DX8GL_ERROR("Failed to make render backend context current");
        return false;
    }
    
    DX8GL_INFO("%s backend initialized successfully", 
               render_backend_->get_type() == DX8GL_BACKEND_OSMESA ? "OSMesa" : "EGL");
    
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
    
    // Set default global render states
    set_default_global_render_states();

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
    
    // Create and initialize render thread for sequential command execution
    render_thread_ = new RenderThread();
    if (!render_thread_->initialize(state_manager_.get(),
                                   vertex_shader_manager_.get(),
                                   pixel_shader_manager_.get(),
                                   shader_program_manager_.get(),
                                   render_backend_)) {
        DX8GL_ERROR("Failed to initialize render thread");
        delete render_thread_;
        render_thread_ = nullptr;
        return false;
    }
    
    // Create back buffers
    for (UINT i = 0; i < present_params_.BackBufferCount; ++i) {
        auto surface = new Direct3DSurface8(this, width, height, 
                                          present_params_.BackBufferFormat,
                                          D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT,
                                          present_params_.MultiSampleType);
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
                                      D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT,
                                      present_params_.MultiSampleType);
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
    
    // Use the global render backend
    render_backend_ = dx8gl::get_render_backend();
    if (!render_backend_) {
        DX8GL_ERROR("No render backend available. Call dx8gl_init() first.");
        return false;
    }
    
    // Resize backend to match requested dimensions
    if (!render_backend_->resize(width, height)) {
        DX8GL_ERROR("Failed to resize render backend");
        return false;
    }
    
    if (!render_backend_->make_current()) {
        DX8GL_ERROR("Failed to make render backend context current");
        return false;
    }
    
    // Create state manager
    state_manager_ = std::make_unique<StateManager>();
    if (!state_manager_->initialize()) {
        DX8GL_ERROR("Failed to initialize state manager");
        return false;
    }
    
    // Set default global render states
    set_default_global_render_states();

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
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    FPU_PRESERVE_SAVE();
    
    // Check if we need to complete deferred OSMesa initialization
    if (osmesa_deferred_init_) {
        const char* complete_init = std::getenv("DX8GL_COMPLETE_OSMESA_INIT");
        if (complete_init && std::strcmp(complete_init, "1") == 0) {
            DX8GL_INFO("BeginScene: Completing deferred OSMesa initialization");
            if (!complete_deferred_osmesa_init()) {
                DX8GL_ERROR("Failed to complete deferred OSMesa initialization");
                FPU_PRESERVE_RESTORE();
                return D3DERR_DEVICELOST;
            }
        }
    }
    
    if (in_scene_) {
        FPU_PRESERVE_RESTORE();
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("BeginScene");
    in_scene_ = true;
    
    // Begin statistics collection for this frame
    begin_statistics();
    
    FPU_PRESERVE_RESTORE();
    return D3D_OK;
}

HRESULT Direct3DDevice8::EndScene() {
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_SAVE();
    }
    
    if (!in_scene_) {
        if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
            FPU_PRESERVE_RESTORE();
        }
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("EndScene");
    in_scene_ = false;
    
    // Flush command buffer at end of scene
    flush_command_buffer();
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_RESTORE();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, 
                              D3DCOLOR Color, float Z, DWORD Stencil) {
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_SAVE();
    }
    
    DX8GL_INFO("Clear: count=%u, flags=0x%08x, color=0x%08x, z=%.2f, stencil=%u",
               Count, Flags, Color, Z, Stencil);
    
    // Update statistics
    current_stats_.clear_calls++;
    
    // Check if we need to complete deferred OSMesa initialization
    if (osmesa_deferred_init_) {
        const char* complete_init = std::getenv("DX8GL_COMPLETE_OSMESA_INIT");
        if (complete_init && std::strcmp(complete_init, "1") == 0) {
            DX8GL_INFO("Clear: Completing deferred OSMesa initialization");
            if (!complete_deferred_osmesa_init()) {
                DX8GL_ERROR("Failed to complete deferred OSMesa initialization");
                if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
                    FPU_PRESERVE_RESTORE();
                }
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
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_RESTORE();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::Present(const RECT* pSourceRect, const RECT* pDestRect,
                                HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    FPU_PRESERVE_SAVE();
    
    DX8GL_TRACE("Present: frame=%u", frame_count_.load());
    
    // Update statistics
    current_stats_.present_calls++;
    
    // Flush any pending commands before presenting
    flush_command_buffer();
    
    // Wait for all async command buffers to complete before presenting
    wait_for_pending_commands();
    
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
            FPU_PRESERVE_RESTORE();
            return D3DERR_DRIVERINTERNALERROR;
        }
        
        // Copy the framebuffer from EGL to the back buffer surface
        if (!back_buffers_.empty() && egl_context_->getFramebuffer()) {
            // Get the primary back buffer
            IDirect3DSurface8* back_buffer = back_buffers_[0];
            
            // Lock the back buffer to write the framebuffer data
            D3DLOCKED_RECT locked_rect;
            HRESULT hr = back_buffer->LockRect(&locked_rect, nullptr, 0);
            
            if (SUCCEEDED(hr)) {
                // Get surface description to know the format
                D3DSURFACE_DESC desc;
                back_buffer->GetDesc(&desc);
                
                // EGL renders to RGBA8 format
                unsigned char* src_pixels = static_cast<unsigned char*>(egl_context_->getFramebuffer());
                int fb_width = egl_context_->getWidth();
                int fb_height = egl_context_->getHeight();
                
                for (UINT y = 0; y < desc.Height && y < static_cast<UINT>(fb_height); y++) {
                    BYTE* dst_row = static_cast<BYTE*>(locked_rect.pBits) + y * locked_rect.Pitch;
                    
                    for (UINT x = 0; x < desc.Width && x < static_cast<UINT>(fb_width); x++) {
                        // Read RGBA8 values
                        BYTE r = src_pixels[(y * fb_width + x) * 4 + 0];
                        BYTE g = src_pixels[(y * fb_width + x) * 4 + 1];
                        BYTE b = src_pixels[(y * fb_width + x) * 4 + 2];
                        BYTE a = src_pixels[(y * fb_width + x) * 4 + 3];
                        
                        // Write to destination based on format
                        if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8) {
                            DWORD* dst_pixel = reinterpret_cast<DWORD*>(dst_row) + x;
                            *dst_pixel = (a << 24) | (r << 16) | (g << 8) | b;
                        } else if (desc.Format == D3DFMT_R5G6B5) {
                            WORD* dst_pixel = reinterpret_cast<WORD*>(dst_row) + x;
                            *dst_pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                        }
                    }
                }
                
                back_buffer->UnlockRect();
                
                DX8GL_TRACE("Copied EGL framebuffer to back buffer (%dx%d)", fb_width, fb_height);
            }
        }
        
        // Handle vsync based on presentation interval
        if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE) {
            // No vsync
        } else if (present_params_.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
            // Vsync enabled
        }
    }
#endif
    
    // Backend doesn't have buffer swapping - rendering is done to memory
    if (render_backend_) {
        // Copy the framebuffer from OpenGL to the back buffer surface
        // This allows applications to read back the rendered content
        if (!back_buffers_.empty()) {
            // Get the framebuffer data from the backend
            int fb_width, fb_height, fb_format;
            void* fb_data = render_backend_->get_framebuffer(fb_width, fb_height, fb_format);
            
            if (fb_data) {
                // Get the primary back buffer
                IDirect3DSurface8* back_buffer = back_buffers_[0];
                
                // Lock the back buffer to write the framebuffer data
                D3DLOCKED_RECT locked_rect;
                HRESULT hr = back_buffer->LockRect(&locked_rect, nullptr, 0);
                
                if (SUCCEEDED(hr)) {
                    // Get surface description to know the format
                    D3DSURFACE_DESC desc;
                    back_buffer->GetDesc(&desc);
                    
                    // Copy framebuffer data to the back buffer
                    // OSMesa typically renders to RGBA float format
                    if (fb_format == GL_RGBA || fb_format == 0x1908) {
                        // Convert from RGBA float to the back buffer format
                        float* src_pixels = static_cast<float*>(fb_data);
                        
                        for (UINT y = 0; y < desc.Height && y < static_cast<UINT>(fb_height); y++) {
                            BYTE* dst_row = static_cast<BYTE*>(locked_rect.pBits) + y * locked_rect.Pitch;
                            
                            for (UINT x = 0; x < desc.Width && x < static_cast<UINT>(fb_width); x++) {
                                // Read RGBA float values
                                float r = src_pixels[(y * fb_width + x) * 4 + 0];
                                float g = src_pixels[(y * fb_width + x) * 4 + 1];
                                float b = src_pixels[(y * fb_width + x) * 4 + 2];
                                float a = src_pixels[(y * fb_width + x) * 4 + 3];
                                
                                // Clamp to [0,1] range
                                r = std::max(0.0f, std::min(1.0f, r));
                                g = std::max(0.0f, std::min(1.0f, g));
                                b = std::max(0.0f, std::min(1.0f, b));
                                a = std::max(0.0f, std::min(1.0f, a));
                                
                                // Convert to 8-bit values
                                BYTE r8 = static_cast<BYTE>(r * 255.0f);
                                BYTE g8 = static_cast<BYTE>(g * 255.0f);
                                BYTE b8 = static_cast<BYTE>(b * 255.0f);
                                BYTE a8 = static_cast<BYTE>(a * 255.0f);
                                
                                // Write to destination based on format
                                if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8) {
                                    DWORD* dst_pixel = reinterpret_cast<DWORD*>(dst_row) + x;
                                    *dst_pixel = (a8 << 24) | (r8 << 16) | (g8 << 8) | b8;
                                } else if (desc.Format == D3DFMT_R5G6B5) {
                                    WORD* dst_pixel = reinterpret_cast<WORD*>(dst_row) + x;
                                    *dst_pixel = ((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3);
                                }
                            }
                        }
                    }
                    
                    back_buffer->UnlockRect();
                    
                    DX8GL_TRACE("Copied framebuffer to back buffer (%dx%d)", fb_width, fb_height);
                }
            }
        }
        
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
    if (render_backend_) {
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
    
    FPU_PRESERVE_RESTORE();
    return D3D_OK;
}

// State setting methods
HRESULT Direct3DDevice8::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_SAVE();
    }
    
    DX8GL_TRACE("SetRenderState: state=%d, value=%u", State, Value);
    
    // Update statistics
    current_stats_.render_state_changes++;
    
    // Update state manager immediately
    state_manager_->set_render_state(State, Value);
    
    // Also queue command for batched processing
    auto cmd = current_command_buffer_->allocate_command<SetRenderStateCmd>();
    cmd->state = State;
    cmd->value = Value;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_RESTORE();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
    MULTITHREADED_LOCK();
    if (!pValue) {
        return D3DERR_INVALIDCALL;
    }
    
    *pValue = state_manager_->get_render_state(State);
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) {
    MULTITHREADED_LOCK();
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
    
    // Update statistics
    current_stats_.matrix_changes++;
    
    auto cmd = current_command_buffer_->allocate_command<SetTransformCmd>();
    cmd->state = State;
    cmd->matrix = *pMatrix;
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    MULTITHREADED_LOCK();
    if (!pMatrix) {
        return D3DERR_INVALIDCALL;
    }
    
    state_manager_->get_transform(State, pMatrix);
    return D3D_OK;
}

// Texture management
HRESULT Direct3DDevice8::SetTexture(DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    MULTITHREADED_LOCK();
    if (Stage >= 8) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("SetTexture: stage=%u, texture=%p", Stage, pTexture);
    
    // Update statistics
    current_stats_.texture_changes++;
    
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
    MULTITHREADED_LOCK();
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
    MULTITHREADED_LOCK();
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
    if (it != stream_sources_.end() && it->second.vertex_buffer) {
        it->second.vertex_buffer->Release();
    }
    
    if (pStreamData) {
        pStreamData->AddRef();
        stream_sources_[StreamNumber] = {pStreamData, Stride};
    } else {
        stream_sources_.erase(StreamNumber);
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    MULTITHREADED_LOCK();
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
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_SAVE();
    }
    
    if (!in_scene_) {
        if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
            FPU_PRESERVE_RESTORE();
        }
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_TRACE("DrawPrimitive: type=%d, start=%u, count=%u", 
                PrimitiveType, StartVertex, PrimitiveCount);
    
    // Update statistics
    current_stats_.draw_calls++;
    
    // Calculate vertices and triangles based on primitive type
    uint32_t vertices = 0;
    uint32_t triangles = 0;
    switch (PrimitiveType) {
        case D3DPT_TRIANGLELIST:
            vertices = PrimitiveCount * 3;
            triangles = PrimitiveCount;
            break;
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            vertices = PrimitiveCount + 2;
            triangles = PrimitiveCount;
            break;
        case D3DPT_LINELIST:
            vertices = PrimitiveCount * 2;
            break;
        case D3DPT_LINESTRIP:
            vertices = PrimitiveCount + 1;
            break;
        case D3DPT_POINTLIST:
            vertices = PrimitiveCount;
            break;
    }
    current_stats_.vertices_processed += vertices;
    current_stats_.triangles_drawn += triangles;
    
    auto cmd = current_command_buffer_->allocate_command<DrawPrimitiveCmd>();
    cmd->primitive_type = PrimitiveType;
    cmd->start_vertex = StartVertex;
    cmd->primitive_count = PrimitiveCount;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_RESTORE();
    }
    
    return D3D_OK;
}

HRESULT Direct3DDevice8::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                             UINT MinIndex, UINT NumVertices,
                                             UINT StartIndex, UINT PrimitiveCount) {
    MULTITHREADED_LOCK();
    FPU_PRESERVE_DECL;
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_SAVE();
    }
    
    if (!in_scene_) {
        if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
            FPU_PRESERVE_RESTORE();
        }
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
    
    if (behavior_flags_ & D3DCREATE_FPU_PRESERVE) {
        FPU_PRESERVE_RESTORE();
    }
    
    return D3D_OK;
}

// Fixed function vertex processing
HRESULT Direct3DDevice8::SetVertexShader(DWORD Handle) {
    DX8GL_INFO("SetVertexShader: handle=0x%08x", Handle);
    
    // Update statistics
    current_stats_.shader_changes++;
    
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
    
    // Move current buffer for execution
    auto buffer_to_execute = std::move(current_command_buffer_);
    
    // Get new buffer from pool immediately to allow recording new commands
    current_command_buffer_ = command_buffer_pool_.acquire();
    
    // Submit to render thread for sequential execution
    if (render_thread_) {
        render_thread_->submit(std::move(buffer_to_execute));
    } else {
        // Fallback: execute synchronously if render thread is not available
        DX8GL_WARNING("Render thread not available, executing command buffer synchronously");
        buffer_to_execute->execute(*state_manager_,
                                 vertex_shader_manager_.get(),
                                 pixel_shader_manager_.get(),
                                 shader_program_manager_.get());
        glFlush();
    }
}

void Direct3DDevice8::wait_for_pending_commands() {
    // Wait for render thread to complete all pending commands
    if (render_thread_) {
        DX8GL_INFO("Waiting for render thread to complete all pending commands");
        render_thread_->wait_for_idle();
        DX8GL_INFO("All pending commands completed");
    }
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
    
    // Resize backend with new dimensions
    if (render_backend_) {
        
        if (!render_backend_->resize(width, height)) {
            DX8GL_ERROR("Failed to resize render backend");
            return D3DERR_DEVICELOST;
        }
        
        // Make context current
        if (!render_backend_->make_current()) {
            DX8GL_ERROR("Failed to make render backend context current after reset");
            return D3DERR_DEVICELOST;
        }
    }
    
    // Recreate back buffers
    for (UINT i = 0; i < present_params_.BackBufferCount; ++i) {
        auto surface = new Direct3DSurface8(this, width, height, 
                                          present_params_.BackBufferFormat,
                                          D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT,
                                          present_params_.MultiSampleType);
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
                                      D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT,
                                      present_params_.MultiSampleType);
        if (!ds->initialize()) {
            ds->Release();
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        depth_stencil_ = ds;
    }
    
    // Reset state manager
    state_manager_->reset();
    
    // Invalidate cached render states to force reapplication
    InvalidateCachedRenderStates();
    
    // Set default global render states after reset
    set_default_global_render_states();
    
    // Recreate non-managed resources
    // According to DirectX 8 documentation, resources in D3DPOOL_DEFAULT must be recreated after reset
    std::vector<Direct3DTexture8*> textures_to_recreate;
    
    // Find all textures in D3DPOOL_DEFAULT
    for (auto* texture : all_textures_) {
        if (texture && texture->get_pool() == D3DPOOL_DEFAULT) {
            textures_to_recreate.push_back(texture);
        }
    }
    
    // Notify textures about device reset so they can recreate their GL resources
    for (auto* texture : textures_to_recreate) {
        DX8GL_INFO("Recreating texture %p in D3DPOOL_DEFAULT", texture);
        if (!texture->recreate_gl_resources()) {
            DX8GL_ERROR("Failed to recreate texture %p", texture);
            // Continue with other resources even if one fails
        }
    }
    
    // Vertex buffers in D3DPOOL_DEFAULT also need recreation
    std::vector<Direct3DVertexBuffer8*> vbs_to_recreate;
    for (auto* vb : all_vertex_buffers_) {
        if (vb && vb->get_pool() == D3DPOOL_DEFAULT) {
            vbs_to_recreate.push_back(vb);
        }
    }
    
    // Recreate vertex buffers
    for (auto* vb : vbs_to_recreate) {
        DX8GL_INFO("Recreating vertex buffer %p in D3DPOOL_DEFAULT", vb);
        if (!vb->recreate_gl_resources()) {
            DX8GL_ERROR("Failed to recreate vertex buffer %p", vb);
            // Continue with other resources even if one fails
        }
    }
    
    // Index buffers in D3DPOOL_DEFAULT also need recreation
    std::vector<Direct3DIndexBuffer8*> ibs_to_recreate;
    for (auto* ib : all_index_buffers_) {
        if (ib && ib->get_pool() == D3DPOOL_DEFAULT) {
            ibs_to_recreate.push_back(ib);
        }
    }
    
    // Recreate index buffers
    for (auto* ib : ibs_to_recreate) {
        DX8GL_INFO("Recreating index buffer %p in D3DPOOL_DEFAULT", ib);
        if (!ib->recreate_gl_resources()) {
            DX8GL_ERROR("Failed to recreate index buffer %p", ib);
            // Continue with other resources even if one fails
        }
    }
    
    // Cube textures in D3DPOOL_DEFAULT also need recreation
    std::vector<Direct3DCubeTexture8*> cube_textures_to_recreate;
    for (auto* cube_texture : all_cube_textures_) {
        if (cube_texture && cube_texture->get_pool() == D3DPOOL_DEFAULT) {
            cube_textures_to_recreate.push_back(cube_texture);
        }
    }
    
    // Recreate cube textures
    for (auto* cube_texture : cube_textures_to_recreate) {
        DX8GL_INFO("Recreating cube texture %p in D3DPOOL_DEFAULT", cube_texture);
        if (!cube_texture->recreate_gl_resources()) {
            DX8GL_ERROR("Failed to recreate cube texture %p", cube_texture);
            // Continue with other resources even if one fails
        }
    }
    
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
    
    // Register vertex buffer for device reset tracking
    register_vertex_buffer(vb);
    
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
    
    // Register texture for device reset tracking
    register_texture(texture);
    
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
    
    // Register index buffer for device reset tracking
    register_index_buffer(buffer);
    
    *ppIndexBuffer = buffer;
    return D3D_OK;
}

// Remaining stub methods

HRESULT Direct3DDevice8::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels,
                                           DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                           IDirect3DVolumeTexture8** ppVolumeTexture) {
    // Volume textures not yet fully implemented
    // The UpdateTexture method supports basic volume texture copying
    DX8GL_WARNING("CreateVolumeTexture: Volume textures not fully implemented");
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3DDevice8::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage,
                                         D3DFORMAT Format, D3DPOOL Pool,
                                         IDirect3DCubeTexture8** ppCubeTexture) {
    if (!ppCubeTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("CreateCubeTexture: edge=%u, levels=%u, usage=0x%08x, format=0x%08x, pool=%d",
               EdgeLength, Levels, Usage, Format, Pool);
    
    // Validate edge length (must be power of 2)
    if (EdgeLength == 0 || (EdgeLength & (EdgeLength - 1)) != 0) {
        DX8GL_ERROR("Invalid cube texture edge length: %u (must be power of 2)", EdgeLength);
        return D3DERR_INVALIDCALL;
    }
    
    auto texture = new Direct3DCubeTexture8(this, EdgeLength, Levels, Usage, Format, Pool);
    if (!texture->initialize()) {
        texture->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    // TODO: Register cube texture for device reset tracking
    // register_cube_texture(texture);
    
    *ppCubeTexture = texture;
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format,
                                          D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable,
                                          IDirect3DSurface8** ppSurface) {
    if (!ppSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    auto surface = new Direct3DSurface8(this, Width, Height, Format, D3DUSAGE_RENDERTARGET,
                                      D3DPOOL_DEFAULT, MultiSample);
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
    
    auto surface = new Direct3DSurface8(this, Width, Height, Format, D3DUSAGE_DEPTHSTENCIL,
                                      D3DPOOL_DEFAULT, MultiSample);
    if (!surface->initialize()) {
        surface->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    *ppSurface = surface;
    return D3D_OK;
}

HRESULT Direct3DDevice8::CreateImageSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                          IDirect3DSurface8** ppSurface) {
    if (!ppSurface) {
        DX8GL_ERROR("CreateImageSurface: ppSurface is null");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("CreateImageSurface: %ux%u format=%d", Width, Height, Format);
    
    // Image surfaces are plain surfaces (no special usage flags)
    auto surface = new Direct3DSurface8(this, Width, Height, Format, 0, D3DPOOL_SYSTEMMEM);
    if (!surface->initialize()) {
        DX8GL_ERROR("CreateImageSurface: surface->initialize() failed");
        surface->Release();
        return D3DERR_NOTAVAILABLE;
    }
    
    DX8GL_INFO("CreateImageSurface: success, surface at %p", surface);
    *ppSurface = surface;
    return D3D_OK;
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
            // Cube texture update
            auto* src_cube = static_cast<IDirect3DCubeTexture8*>(pSourceTexture);
            auto* dst_cube = static_cast<IDirect3DCubeTexture8*>(pDestinationTexture);
            
            // Get level counts
            DWORD src_levels = src_cube->GetLevelCount();
            DWORD dst_levels = dst_cube->GetLevelCount();
            DWORD levels_to_copy = std::min(src_levels, dst_levels);
            
            // Copy each face and mip level
            for (int face = 0; face < 6; face++) {
                D3DCUBEMAP_FACES face_type = static_cast<D3DCUBEMAP_FACES>(face);
                
                for (DWORD level = 0; level < levels_to_copy; level++) {
                    IDirect3DSurface8* src_surface = nullptr;
                    IDirect3DSurface8* dst_surface = nullptr;
                    
                    HRESULT hr = src_cube->GetCubeMapSurface(face_type, level, &src_surface);
                    if (FAILED(hr)) {
                        DX8GL_WARNING("Failed to get source cube surface face %d level %u", face, level);
                        continue;
                    }
                    
                    hr = dst_cube->GetCubeMapSurface(face_type, level, &dst_surface);
                    if (FAILED(hr)) {
                        src_surface->Release();
                        DX8GL_WARNING("Failed to get dest cube surface face %d level %u", face, level);
                        continue;
                    }
                    
                    // Copy the surface
                    hr = CopyRects(src_surface, nullptr, 0, dst_surface, nullptr);
                    
                    src_surface->Release();
                    dst_surface->Release();
                    
                    if (FAILED(hr)) {
                        DX8GL_WARNING("Failed to copy cube face %d level %u", face, level);
                        return hr;
                    }
                }
            }
            
            return D3D_OK;
        }
        
        case D3DRTYPE_VOLUMETEXTURE: {
            // Volume textures not yet fully implemented
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
    
    // Update statistics
    current_stats_.viewport_changes++;
    
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
    
    // Update statistics
    current_stats_.material_changes++;
    
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
    
    // Update statistics
    current_stats_.light_changes++;
    
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
    // Update statistics
    current_stats_.light_changes++;
    
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
    // Update statistics
    current_stats_.texture_state_changes++;
    
    state_manager_->set_texture_stage_state(Stage, Type, Value);
    return D3D_OK;
}

HRESULT Direct3DDevice8::ValidateDevice(DWORD* pNumPasses) {
    if (!pNumPasses) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("ValidateDevice called");
    
    // Initialize to 1 pass (we don't support multi-pass rendering yet)
    *pNumPasses = 1;
    
    // Check if we have a valid state manager
    if (!state_manager_) {
        DX8GL_WARN("ValidateDevice: No state manager");
        return D3DERR_INVALIDCALL;
    }
    
    // Validate texture stage states
    for (DWORD stage = 0; stage < 8; stage++) {
        // Check if texture stage is enabled
        DWORD color_op = state_manager_->get_texture_stage_state(stage, D3DTSS_COLOROP);
        if (color_op == D3DTOP_DISABLE) {
            continue; // Stage is disabled, skip validation
        }
        
        // Check if we need a texture for this stage
        bool needs_texture = false;
        DWORD color_arg1 = state_manager_->get_texture_stage_state(stage, D3DTSS_COLORARG1);
        DWORD color_arg2 = state_manager_->get_texture_stage_state(stage, D3DTSS_COLORARG2);
        DWORD alpha_arg1 = state_manager_->get_texture_stage_state(stage, D3DTSS_ALPHAARG1);
        DWORD alpha_arg2 = state_manager_->get_texture_stage_state(stage, D3DTSS_ALPHAARG2);
        
        if ((color_arg1 & D3DTA_SELECTMASK) == D3DTA_TEXTURE ||
            (color_arg2 & D3DTA_SELECTMASK) == D3DTA_TEXTURE ||
            (alpha_arg1 & D3DTA_SELECTMASK) == D3DTA_TEXTURE ||
            (alpha_arg2 & D3DTA_SELECTMASK) == D3DTA_TEXTURE) {
            needs_texture = true;
        }
        
        // Check if texture is bound when needed
        if (needs_texture) {
            auto it = textures_.find(stage);
            if (it == textures_.end() || it->second == nullptr) {
                DX8GL_WARN("ValidateDevice: Texture stage %u requires texture but none is bound", stage);
                return D3DERR_INVALIDCALL; // Texture not set when required
            }
        }
        
        // Validate texture filtering
        DWORD min_filter = state_manager_->get_texture_stage_state(stage, D3DTSS_MINFILTER);
        DWORD mag_filter = state_manager_->get_texture_stage_state(stage, D3DTSS_MAGFILTER);
        DWORD mip_filter = state_manager_->get_texture_stage_state(stage, D3DTSS_MIPFILTER);
        
        // Check for anisotropic filtering without support
        if ((min_filter == D3DTEXF_ANISOTROPIC || mag_filter == D3DTEXF_ANISOTROPIC) &&
            state_manager_->get_texture_stage_state(stage, D3DTSS_MAXANISOTROPY) > 1) {
            // For now, we support anisotropic filtering through OpenGL extensions
            // But we could return an error if the extension is not available
            DX8GL_DEBUG("ValidateDevice: Anisotropic filtering requested on stage %u", stage);
        }
        
        // Check for unsupported filter combinations
        if (mip_filter == D3DTEXF_GAUSSIANCUBIC) { // Unsupported filter type
            DX8GL_WARN("ValidateDevice: Unsupported mipmap filter %u on stage %u", mip_filter, stage);
            return D3DERR_NOTAVAILABLE; // Unsupported texture filter
        }
    }
    
    // Validate render states
    DWORD z_enable = state_manager_->get_render_state(D3DRS_ZENABLE);
    DWORD z_write_enable = state_manager_->get_render_state(D3DRS_ZWRITEENABLE);
    DWORD stencil_enable = state_manager_->get_render_state(D3DRS_STENCILENABLE);
    
    // Check for depth buffer requirement
    if ((z_enable || stencil_enable) && !depth_stencil_) {
        DX8GL_WARN("ValidateDevice: Z-buffer or stencil enabled but no depth buffer");
        return D3DERR_INVALIDCALL; // Z buffer required but not available
    }
    
    // Check alpha blending state
    DWORD alpha_blend_enable = state_manager_->get_render_state(D3DRS_ALPHABLENDENABLE);
    if (alpha_blend_enable) {
        DWORD src_blend = state_manager_->get_render_state(D3DRS_SRCBLEND);
        DWORD dest_blend = state_manager_->get_render_state(D3DRS_DESTBLEND);
        
        // Check for unsupported blend modes
        if (src_blend == D3DBLEND_BOTHSRCALPHA || src_blend == D3DBLEND_BOTHINVSRCALPHA ||
            dest_blend == D3DBLEND_BOTHSRCALPHA || dest_blend == D3DBLEND_BOTHINVSRCALPHA) {
            DX8GL_WARN("ValidateDevice: BOTHSRCALPHA blend modes not supported");
            return D3DERR_INVALIDCALL; // Z buffer required but not available
        }
    }
    
    // Validate vertex shader if one is set
    DWORD vertex_shader = current_fvf_;
    if (vertex_shader != 0 && !FVF_IS_VALID_FVF(vertex_shader)) {
        // It's a vertex shader handle, validate it exists
        if (vertex_shader_manager_) {
            // Check if this is a valid shader handle (simplified check)
            if (vertex_shader > 0xFFFF0000) {
                DX8GL_WARN("ValidateDevice: Invalid vertex shader handle 0x%08X", vertex_shader);
                return D3DERR_INVALIDCALL; // Invalid shader handle
            }
        }
    }
    
    // Note: Pixel shader validation would go here if we had a way to track current pixel shader
    // For now we assume pixel shaders are valid if set
    
    // Check for conflicting fog modes
    DWORD fog_enable = state_manager_->get_render_state(D3DRS_FOGENABLE);
    if (fog_enable) {
        DWORD fog_vertex_mode = state_manager_->get_render_state(D3DRS_FOGVERTEXMODE);
        DWORD fog_table_mode = state_manager_->get_render_state(D3DRS_FOGTABLEMODE);
        
        // Can't have both vertex and table fog
        if (fog_vertex_mode != D3DFOG_NONE && fog_table_mode != D3DFOG_NONE) {
            DX8GL_WARN("ValidateDevice: Both vertex and table fog enabled");
            return D3DERR_INVALIDCALL;
        }
        
        // Validate range fog setting
        DWORD range_fog = state_manager_->get_render_state(D3DRS_RANGEFOGENABLE);
        if (range_fog && fog_table_mode != D3DFOG_NONE) {
            // Range-based fog is typically only supported with vertex fog
            DX8GL_WARN("ValidateDevice: Range fog with table fog may not be supported");
            // This is a warning, not an error - some hardware might support it
        }
    }
    
    // Validate color vertex and material settings
    DWORD color_vertex = state_manager_->get_render_state(D3DRS_COLORVERTEX);
    DWORD lighting = state_manager_->get_render_state(D3DRS_LIGHTING);
    if (color_vertex && !lighting) {
        // Color vertex requires lighting to be enabled for proper material processing
        DX8GL_DEBUG("ValidateDevice: Color vertex enabled without lighting");
    }
    
    // Validate specular material source
    DWORD specular_source = state_manager_->get_render_state(D3DRS_SPECULARMATERIALSOURCE);
    if (specular_source > D3DMCS_COLOR2) {
        DX8GL_WARN("ValidateDevice: Invalid specular material source %u", specular_source);
        return D3DERR_INVALIDCALL;
    }
    
    // Validate Z-bias setting
    DWORD z_bias = state_manager_->get_render_state(D3DRS_ZBIAS);
    if (z_bias > 16) {
        // D3D Z-bias typically ranges from 0-16
        DX8GL_WARN("ValidateDevice: Z-bias value %u exceeds typical range (0-16)", z_bias);
        // This is a warning, not an error - allow it but warn
    }
    
    // Validate point sprite states
    DWORD point_sprite_enable = state_manager_->get_render_state(D3DRS_POINTSPRITEENABLE);
    if (point_sprite_enable) {
        DWORD point_scale_enable = state_manager_->get_render_state(D3DRS_POINTSCALEENABLE);
        if (point_scale_enable) {
            // Point sprites with scaling require vertex shader or specific FVF
            if (vertex_shader == 0 || FVF_IS_VALID_FVF(vertex_shader)) {
                DWORD fvf = FVF_IS_VALID_FVF(vertex_shader) ? vertex_shader : current_fvf_;
                if (!(fvf & D3DFVF_PSIZE)) {
                    DX8GL_WARN("ValidateDevice: Point sprites with scaling require D3DFVF_PSIZE");
                    return D3DERR_INVALIDCALL; // Z buffer required but not available
                }
            }
        }
    }
    
    DX8GL_INFO("ValidateDevice: Pipeline is valid, returning %u passes", *pNumPasses);
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
        *ppStreamData = it->second.vertex_buffer;
        if (*ppStreamData) {
            (*ppStreamData)->AddRef();
        }
        *pStride = it->second.stride;
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
    if (render_backend_) {
        int width, height, format;
        return render_backend_->get_framebuffer(width, height, format);
    }
    return nullptr;
}

void Direct3DDevice8::get_osmesa_dimensions(int* width, int* height) const {
    if (render_backend_) {
        int w, h, format;
        render_backend_->get_framebuffer(w, h, format);
        if (width) *width = w;
        if (height) *height = h;
        return;
    }
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

void* Direct3DDevice8::get_framebuffer(int* width, int* height, int* format) const {
    if (render_backend_) {
        int w, h, fmt;
        void* fb = render_backend_->get_framebuffer(w, h, fmt);
        if (width) *width = w;
        if (height) *height = h;
        if (format) *format = fmt;
        return fb;
    }
    return nullptr;
}

void Direct3DDevice8::InvalidateCachedRenderStates() {
    DX8GL_INFO("InvalidateCachedRenderStates called");
    
    if (!state_manager_) {
        DX8GL_WARN("InvalidateCachedRenderStates: State manager not initialized");
        return;
    }
    
    // Invalidate all cached states in the state manager
    state_manager_->invalidate_cached_render_states();
    
    // Also unbind all textures to ensure clean state
    for (DWORD stage = 0; stage < 8; stage++) {
        auto it = textures_.find(stage);
        if (it != textures_.end() && it->second) {
            // Release the texture reference
            it->second->Release();
            textures_.erase(it);
            
            // Ensure OpenGL texture is unbound
            auto* cmd = current_command_buffer_->allocate_command<SetTextureCmd>();
            cmd->stage = stage;
            cmd->texture = 0;  // nullptr
        }
    }
    
    // Force a command buffer flush to apply texture unbinding
    flush_command_buffer();
    
    DX8GL_INFO("InvalidateCachedRenderStates complete - all states and textures cleared");
}

void Direct3DDevice8::reset_statistics() {
    current_stats_.reset();
    last_frame_stats_.reset();
}

void Direct3DDevice8::begin_statistics() {
    // Save current stats as last frame stats
    last_frame_stats_.matrix_changes = current_stats_.matrix_changes.load();
    last_frame_stats_.render_state_changes = current_stats_.render_state_changes.load();
    last_frame_stats_.texture_state_changes = current_stats_.texture_state_changes.load();
    last_frame_stats_.texture_changes = current_stats_.texture_changes.load();
    last_frame_stats_.draw_calls = current_stats_.draw_calls.load();
    last_frame_stats_.triangles_drawn = current_stats_.triangles_drawn.load();
    last_frame_stats_.vertices_processed = current_stats_.vertices_processed.load();
    last_frame_stats_.state_blocks_created = current_stats_.state_blocks_created.load();
    last_frame_stats_.clear_calls = current_stats_.clear_calls.load();
    last_frame_stats_.present_calls = current_stats_.present_calls.load();
    last_frame_stats_.vertex_buffer_locks = current_stats_.vertex_buffer_locks.load();
    last_frame_stats_.index_buffer_locks = current_stats_.index_buffer_locks.load();
    last_frame_stats_.texture_locks = current_stats_.texture_locks.load();
    last_frame_stats_.shader_changes = current_stats_.shader_changes.load();
    last_frame_stats_.light_changes = current_stats_.light_changes.load();
    last_frame_stats_.material_changes = current_stats_.material_changes.load();
    last_frame_stats_.viewport_changes = current_stats_.viewport_changes.load();
    
    // Reset current stats for new frame
    current_stats_.reset();
}

void Direct3DDevice8::end_statistics() {
    // Statistics collection ends - data is now available in current_stats_
    // This could be used to compute frame deltas or averages
}

std::string Direct3DDevice8::get_statistics_string() const {
    std::stringstream ss;
    ss << "=== dx8gl Device Statistics ===\n";
    ss << "Matrix changes: " << current_stats_.matrix_changes.load() << "\n";
    ss << "Render state changes: " << current_stats_.render_state_changes.load() << "\n";
    ss << "Texture state changes: " << current_stats_.texture_state_changes.load() << "\n";
    ss << "Texture changes: " << current_stats_.texture_changes.load() << "\n";
    ss << "Draw calls: " << current_stats_.draw_calls.load() << "\n";
    ss << "Triangles drawn: " << current_stats_.triangles_drawn.load() << "\n";
    ss << "Vertices processed: " << current_stats_.vertices_processed.load() << "\n";
    ss << "Clear calls: " << current_stats_.clear_calls.load() << "\n";
    ss << "Present calls: " << current_stats_.present_calls.load() << "\n";
    ss << "Shader changes: " << current_stats_.shader_changes.load() << "\n";
    ss << "Light changes: " << current_stats_.light_changes.load() << "\n";
    ss << "Material changes: " << current_stats_.material_changes.load() << "\n";
    ss << "Viewport changes: " << current_stats_.viewport_changes.load() << "\n";
    ss << "Vertex buffer locks: " << current_stats_.vertex_buffer_locks.load() << "\n";
    ss << "Index buffer locks: " << current_stats_.index_buffer_locks.load() << "\n";
    ss << "Texture locks: " << current_stats_.texture_locks.load() << "\n";
    ss << "State blocks created: " << current_stats_.state_blocks_created.load() << "\n";
    ss << "==============================\n";
    return ss.str();
}

void Direct3DDevice8::register_texture(Direct3DTexture8* texture) {
    if (!texture) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    all_textures_.push_back(texture);
    DX8GL_TRACE("Registered texture %p, total textures: %zu", texture, all_textures_.size());
}

void Direct3DDevice8::unregister_texture(Direct3DTexture8* texture) {
    if (!texture) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(all_textures_.begin(), all_textures_.end(), texture);
    if (it != all_textures_.end()) {
        all_textures_.erase(it);
        DX8GL_TRACE("Unregistered texture %p, remaining textures: %zu", texture, all_textures_.size());
    }
}

void Direct3DDevice8::register_vertex_buffer(Direct3DVertexBuffer8* vb) {
    if (!vb) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    all_vertex_buffers_.push_back(vb);
    DX8GL_TRACE("Registered vertex buffer %p, total VBs: %zu", vb, all_vertex_buffers_.size());
}

void Direct3DDevice8::unregister_vertex_buffer(Direct3DVertexBuffer8* vb) {
    if (!vb) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(all_vertex_buffers_.begin(), all_vertex_buffers_.end(), vb);
    if (it != all_vertex_buffers_.end()) {
        all_vertex_buffers_.erase(it);
        DX8GL_TRACE("Unregistered vertex buffer %p, remaining VBs: %zu", vb, all_vertex_buffers_.size());
    }
}

void Direct3DDevice8::register_index_buffer(Direct3DIndexBuffer8* ib) {
    if (!ib) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    all_index_buffers_.push_back(ib);
    DX8GL_TRACE("Registered index buffer %p, total IBs: %zu", ib, all_index_buffers_.size());
}

void Direct3DDevice8::unregister_index_buffer(Direct3DIndexBuffer8* ib) {
    if (!ib) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(all_index_buffers_.begin(), all_index_buffers_.end(), ib);
    if (it != all_index_buffers_.end()) {
        all_index_buffers_.erase(it);
        DX8GL_TRACE("Unregistered index buffer %p, remaining IBs: %zu", ib, all_index_buffers_.size());
    }
}

void Direct3DDevice8::register_cube_texture(Direct3DCubeTexture8* cube_texture) {
    if (!cube_texture) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    all_cube_textures_.push_back(cube_texture);
    DX8GL_TRACE("Registered cube texture %p, total cube textures: %zu", cube_texture, all_cube_textures_.size());
}

void Direct3DDevice8::unregister_cube_texture(Direct3DCubeTexture8* cube_texture) {
    if (!cube_texture) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(all_cube_textures_.begin(), all_cube_textures_.end(), cube_texture);
    if (it != all_cube_textures_.end()) {
        all_cube_textures_.erase(it);
        DX8GL_TRACE("Unregistered cube texture %p, remaining cube textures: %zu", cube_texture, all_cube_textures_.size());
    }
}

} // namespace dx8gl