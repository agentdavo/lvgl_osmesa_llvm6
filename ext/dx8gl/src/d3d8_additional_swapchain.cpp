#include "d3d8_additional_swapchain.h"
#include "d3d8_device.h"
#include "d3d8_surface.h"
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

namespace dx8gl {

AdditionalSwapChain::AdditionalSwapChain(Direct3DDevice8* device, D3DPRESENT_PARAMETERS* params)
    : ref_count_(1)
    , device_(device)
    , current_buffer_(0) {
    
    if (params) {
        memcpy(&present_params_, params, sizeof(D3DPRESENT_PARAMETERS));
    } else {
        memset(&present_params_, 0, sizeof(D3DPRESENT_PARAMETERS));
    }
    
    // Add reference to device
    if (device_) {
        device_->AddRef();
    }
    
    DX8GL_INFO("AdditionalSwapChain created: %dx%d, format=%d, buffers=%d",
               present_params_.BackBufferWidth, present_params_.BackBufferHeight,
               present_params_.BackBufferFormat, present_params_.BackBufferCount);
}

AdditionalSwapChain::~AdditionalSwapChain() {
    DX8GL_INFO("AdditionalSwapChain destructor");
    
    // Release all resources
    release_resources();
    
    // Release device reference
    if (device_) {
        device_->Release();
    }
}

bool AdditionalSwapChain::initialize() {
    DX8GL_INFO("Initializing AdditionalSwapChain");
    
    // Create framebuffers for back buffers
    if (!create_framebuffers()) {
        DX8GL_ERROR("Failed to create framebuffers");
        return false;
    }
    
    DX8GL_INFO("AdditionalSwapChain initialized successfully with %zu buffers",
               framebuffers_.size());
    return true;
}

bool AdditionalSwapChain::create_framebuffers() {
    // Determine number of back buffers (minimum 1)
    UINT buffer_count = present_params_.BackBufferCount;
    if (buffer_count == 0) {
        buffer_count = 1;  // At least one back buffer
    }
    
    // Convert D3D format to our pixel format
    PixelFormat pixel_format = d3d_format_to_pixel_format(present_params_.BackBufferFormat);
    
    // Create framebuffers
    framebuffers_.clear();
    back_buffer_surfaces_.clear();
    
    for (UINT i = 0; i < buffer_count; i++) {
        // Create offscreen framebuffer
        auto framebuffer = std::make_unique<OffscreenFramebuffer>(
            present_params_.BackBufferWidth,
            present_params_.BackBufferHeight,
            pixel_format,
            true  // CPU accessible for presentation
        );
        
        if (!framebuffer) {
            DX8GL_ERROR("Failed to create framebuffer %d", i);
            destroy_framebuffers();
            return false;
        }
        
        // Create Direct3DSurface8 wrapper for the framebuffer
        Direct3DSurface8* surface = new Direct3DSurface8(
            device_,
            present_params_.BackBufferWidth,
            present_params_.BackBufferHeight,
            present_params_.BackBufferFormat,
            D3DMULTISAMPLE_NONE,
            true,  // Lockable
            D3DPOOL_DEFAULT
        );
        
        if (!surface->initialize()) {
            DX8GL_ERROR("Failed to initialize surface wrapper %d", i);
            delete surface;
            destroy_framebuffers();
            return false;
        }
        
        // Associate the framebuffer with the surface
        // The surface will use the framebuffer's data when locked
        surface->set_external_buffer(framebuffer->get_data());
        
        framebuffers_.push_back(std::move(framebuffer));
        back_buffer_surfaces_.push_back(surface);
        
        DX8GL_INFO("Created back buffer %d: %dx%d, format=%d",
                   i, present_params_.BackBufferWidth, present_params_.BackBufferHeight,
                   present_params_.BackBufferFormat);
    }
    
    return true;
}

void AdditionalSwapChain::destroy_framebuffers() {
    // Release surface wrappers
    for (auto* surface : back_buffer_surfaces_) {
        if (surface) {
            surface->Release();
        }
    }
    back_buffer_surfaces_.clear();
    
    // Framebuffers are automatically destroyed by unique_ptr
    framebuffers_.clear();
}

void AdditionalSwapChain::release_resources() {
    destroy_framebuffers();
}

PixelFormat AdditionalSwapChain::d3d_format_to_pixel_format(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            return PixelFormat::BGRA8;  // D3D uses BGRA order
            
        case D3DFMT_R8G8B8:
            return PixelFormat::BGR8;
            
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            return PixelFormat::RGB565;
            
        default:
            DX8GL_WARN("Unsupported D3D format %d, defaulting to BGRA8", format);
            return PixelFormat::BGRA8;
    }
}

HRESULT AdditionalSwapChain::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (riid == IID_IUnknown || riid == IID_IDirect3DSwapChain8) {
        *ppvObj = static_cast<IDirect3DSwapChain8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG AdditionalSwapChain::AddRef() {
    return ++ref_count_;
}

ULONG AdditionalSwapChain::Release() {
    ULONG ref = --ref_count_;
    if (ref == 0) {
        delete this;
    }
    return ref;
}

HRESULT AdditionalSwapChain::Present(const RECT* pSourceRect, const RECT* pDestRect,
                                    HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    if (framebuffers_.empty()) {
        DX8GL_ERROR("No framebuffers available for presentation");
        return D3DERR_INVALIDCALL;
    }
    
    // Use override window if provided, otherwise use swap chain's window
    HWND target_window = hDestWindowOverride ? hDestWindowOverride : present_params_.hDeviceWindow;
    if (!target_window) {
        DX8GL_WARN("No target window for presentation");
        return D3D_OK;  // Not an error, just nothing to present to
    }
    
    // Get current back buffer framebuffer
    OffscreenFramebuffer* framebuffer = framebuffers_[current_buffer_].get();
    if (!framebuffer) {
        DX8GL_ERROR("Current framebuffer is null");
        return D3DERR_INVALIDCALL;
    }
    
    // Present the framebuffer to the window
    if (!present_to_window(framebuffer, target_window, pSourceRect, pDestRect)) {
        DX8GL_ERROR("Failed to present framebuffer to window");
        return D3DERR_INVALIDCALL;
    }
    
    // Swap buffers if multiple back buffers
    if (framebuffers_.size() > 1) {
        swap_buffers();
    }
    
    return D3D_OK;
}

HRESULT AdditionalSwapChain::GetBackBuffer(UINT BackBuffer, D3DBACKBUFFER_TYPE Type,
                                          IDirect3DSurface8** ppBackBuffer) {
    if (!ppBackBuffer) {
        return D3DERR_INVALIDCALL;
    }
    
    *ppBackBuffer = nullptr;
    
    // Only support back buffer type
    if (Type != D3DBACKBUFFER_TYPE_MONO) {
        DX8GL_ERROR("Unsupported back buffer type: %d", Type);
        return D3DERR_INVALIDCALL;
    }
    
    // Validate buffer index
    if (BackBuffer >= back_buffer_surfaces_.size()) {
        DX8GL_ERROR("Invalid back buffer index: %d (have %zu buffers)",
                   BackBuffer, back_buffer_surfaces_.size());
        return D3DERR_INVALIDCALL;
    }
    
    // Return the requested back buffer surface
    Direct3DSurface8* surface = back_buffer_surfaces_[BackBuffer];
    if (surface) {
        surface->AddRef();
        *ppBackBuffer = surface;
        DX8GL_INFO("Returning back buffer %d", BackBuffer);
        return D3D_OK;
    }
    
    return D3DERR_INVALIDCALL;
}

void AdditionalSwapChain::swap_buffers() {
    if (framebuffers_.size() > 1) {
        current_buffer_ = (current_buffer_ + 1) % framebuffers_.size();
        DX8GL_INFO("Swapped to buffer %d", current_buffer_);
    }
}

OffscreenFramebuffer* AdditionalSwapChain::get_current_framebuffer() {
    if (current_buffer_ < framebuffers_.size()) {
        return framebuffers_[current_buffer_].get();
    }
    return nullptr;
}

HRESULT AdditionalSwapChain::reset(D3DPRESENT_PARAMETERS* params) {
    if (!params) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("Resetting additional swap chain: %dx%d -> %dx%d",
               present_params_.BackBufferWidth, present_params_.BackBufferHeight,
               params->BackBufferWidth, params->BackBufferHeight);
    
    // Release current resources
    release_resources();
    
    // Update parameters
    memcpy(&present_params_, params, sizeof(D3DPRESENT_PARAMETERS));
    
    // Recreate framebuffers with new parameters
    if (!create_framebuffers()) {
        DX8GL_ERROR("Failed to recreate framebuffers after reset");
        return D3DERR_INVALIDCALL;
    }
    
    current_buffer_ = 0;
    
    DX8GL_INFO("Additional swap chain reset successfully");
    return D3D_OK;
}

bool AdditionalSwapChain::present_to_window(const OffscreenFramebuffer* framebuffer, HWND window,
                                           const RECT* src_rect, const RECT* dst_rect) {
#ifdef _WIN32
    return present_win32(framebuffer, window, src_rect, dst_rect);
#else
    return present_x11(framebuffer, window, src_rect, dst_rect);
#endif
}

#ifdef _WIN32
bool AdditionalSwapChain::present_win32(const OffscreenFramebuffer* framebuffer, HWND window,
                                        const RECT* src_rect, const RECT* dst_rect) {
    // Get window DC
    HDC hdc = GetDC(window);
    if (!hdc) {
        DX8GL_ERROR("Failed to get window DC");
        return false;
    }
    
    // Get window dimensions
    RECT window_rect;
    GetClientRect(window, &window_rect);
    
    // Determine source rectangle
    RECT src = {0, 0, framebuffer->get_width(), framebuffer->get_height()};
    if (src_rect) {
        src = *src_rect;
    }
    
    // Determine destination rectangle
    RECT dst = window_rect;
    if (dst_rect) {
        dst = *dst_rect;
    }
    
    // Create DIB for blitting
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = framebuffer->get_width();
    bmi.bmiHeader.biHeight = -framebuffer->get_height();  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // Convert framebuffer to BGRA if needed
    const void* pixels = framebuffer->get_data();
    std::vector<uint8_t> converted_buffer;
    
    if (framebuffer->get_format() != PixelFormat::BGRA8) {
        // Need to convert to BGRA for Windows
        converted_buffer.resize(framebuffer->get_width() * framebuffer->get_height() * 4);
        if (!const_cast<OffscreenFramebuffer*>(framebuffer)->convert_to(PixelFormat::BGRA8, converted_buffer.data())) {
            DX8GL_ERROR("Failed to convert framebuffer to BGRA");
            ReleaseDC(window, hdc);
            return false;
        }
        pixels = converted_buffer.data();
    }
    
    // Blit to window
    int result = StretchDIBits(
        hdc,
        dst.left, dst.top,
        dst.right - dst.left, dst.bottom - dst.top,
        src.left, src.top,
        src.right - src.left, src.bottom - src.top,
        pixels,
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
    
    ReleaseDC(window, hdc);
    
    if (result == 0) {
        DX8GL_ERROR("StretchDIBits failed");
        return false;
    }
    
    return true;
}
#else
bool AdditionalSwapChain::present_x11(const OffscreenFramebuffer* framebuffer, HWND window,
                                      const RECT* src_rect, const RECT* dst_rect) {
    // X11 implementation for Linux
    // This is a simplified version - full implementation would need X11 context management
    
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        DX8GL_ERROR("Failed to open X11 display");
        return false;
    }
    
    Window x_window = reinterpret_cast<Window>(window);
    
    // Get window attributes
    XWindowAttributes attrs;
    XGetWindowAttributes(display, x_window, &attrs);
    
    // Create XImage from framebuffer
    // Note: This is simplified - proper implementation would handle format conversion
    XImage* image = XCreateImage(
        display,
        DefaultVisual(display, DefaultScreen(display)),
        24,  // depth
        ZPixmap,
        0,
        const_cast<char*>(reinterpret_cast<const char*>(framebuffer->get_data())),
        framebuffer->get_width(),
        framebuffer->get_height(),
        32,  // bitmap_pad
        0    // bytes_per_line (0 = auto calculate)
    );
    
    if (!image) {
        DX8GL_ERROR("Failed to create XImage");
        XCloseDisplay(display);
        return false;
    }
    
    // Get graphics context
    GC gc = DefaultGC(display, DefaultScreen(display));
    
    // Put image to window
    XPutImage(display, x_window, gc, image, 
             0, 0,  // src x,y
             0, 0,  // dst x,y
             framebuffer->get_width(), framebuffer->get_height());
    
    XFlush(display);
    
    // Cleanup (don't destroy data as it's owned by framebuffer)
    image->data = nullptr;
    XDestroyImage(image);
    XCloseDisplay(display);
    
    return true;
}
#endif

} // namespace dx8gl