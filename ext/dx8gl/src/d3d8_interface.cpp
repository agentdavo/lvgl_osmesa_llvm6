#include "d3d8_interface.h"
#include "d3d8_device.h"
#include "logger.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include "gl3_headers.h"
#else
#include <GL/gl.h>
#endif

// Interface ID definitions
const IID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID IID_IDirect3D8 = { 0x1DD9E8DA, 0x1C77, 0x4D40, { 0xB0, 0xCF, 0x98, 0xFE, 0xFD, 0xFF, 0x95, 0x12 } };
const IID IID_IDirect3DDevice8 = { 0x7385E5DF, 0x8FE8, 0x41D5, { 0x86, 0xB6, 0xD7, 0xB4, 0x85, 0x47, 0xB6, 0xCF } };
const IID IID_IDirect3DSurface8 = { 0xB96EEBCA, 0xB326, 0x4EA5, { 0x88, 0x2F, 0x2F, 0xF5, 0xBA, 0xE0, 0x21, 0xDD } };
const IID IID_IDirect3DTexture8 = { 0xE4CDD575, 0x2866, 0x4F01, { 0xB1, 0x2E, 0x7E, 0xEC, 0xE1, 0xEC, 0x93, 0x58 } };
const IID IID_IDirect3DVertexBuffer8 = { 0x8AEEEAC7, 0x05F9, 0x44D4, { 0xB5, 0x91, 0x00, 0x0E, 0x0F, 0xD9, 0xB9, 0xA9 } };
const IID IID_IDirect3DIndexBuffer8 = { 0x0E689C9A, 0x053D, 0x44A0, { 0x9D, 0x92, 0xDB, 0x0E, 0x3D, 0x75, 0x0F, 0x86 } };
const IID IID_IDirect3DSwapChain8 = { 0x928C088B, 0x76B9, 0x4C6B, { 0xA5, 0x36, 0xA5, 0x90, 0x85, 0x38, 0x76, 0xCD } };
const IID IID_IDirect3DResource8 = { 0x1B36BB7B, 0x09B7, 0x410A, { 0xB4, 0x45, 0x7D, 0x14, 0x30, 0xD7, 0xB3, 0x3F } };
const IID IID_IDirect3DBaseTexture8 = { 0xB4211CFA, 0x51B9, 0x4A9F, { 0xAB, 0x78, 0xDB, 0x99, 0xB2, 0xBB, 0x67, 0x8E } };
const IID IID_IDirect3DCubeTexture8 = { 0x3EE5B968, 0x2ACA, 0x4C34, { 0x8B, 0xB5, 0x7E, 0x0C, 0x3D, 0x19, 0xB7, 0x50 } };

namespace dx8gl {

Direct3D8::Direct3D8() 
    : ref_count_(1)
    , initialized_(false) {
    DX8GL_TRACE("Direct3D8 constructor");
}

Direct3D8::~Direct3D8() {
    DX8GL_TRACE("Direct3D8 destructor");
}

bool Direct3D8::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    DX8GL_INFO("Initializing Direct3D8 interface");
    
    DX8GL_INFO("Using OSMesa-only software rendering (no EGL complexity)");
    
    // Enumerate available adapters
    enumerate_adapters();
    
    initialized_ = true;
    return true;
}

// IUnknown methods
HRESULT Direct3D8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3D8)) {
        *ppvObj = static_cast<IDirect3D8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3D8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3D8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3D8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3D8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3D8 methods
HRESULT Direct3D8::RegisterSoftwareDevice(void* pInitializeFunction) {
    DX8GL_WARNING("RegisterSoftwareDevice not implemented");
    return D3DERR_NOTAVAILABLE;
}

UINT Direct3D8::GetAdapterCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS return 1 adapter for OSMesa
    DX8GL_INFO("GetAdapterCount() called - returning 1 (OSMesa software adapter)");
    return 1;
}

HRESULT Direct3D8::GetAdapterIdentifier(UINT Adapter, DWORD Flags, 
                                       D3DADAPTER_IDENTIFIER8* pIdentifier) {
    if (!pIdentifier) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS use OSMesa - only adapter 0 exists
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    // Fill with OSMesa data
    strncpy(pIdentifier->Driver, "OSMesa", MAX_DEVICE_IDENTIFIER_STRING - 1);
    strncpy(pIdentifier->Description, "OSMesa Software Renderer (llvmpipe)", MAX_DEVICE_IDENTIFIER_STRING - 1);
    pIdentifier->Driver[MAX_DEVICE_IDENTIFIER_STRING - 1] = '\0';
    pIdentifier->Description[MAX_DEVICE_IDENTIFIER_STRING - 1] = '\0';
    
    pIdentifier->VendorId = 0x1002;  // ATI fake ID
    pIdentifier->DeviceId = 0x5159;  // Radeon 7500 fake ID
    pIdentifier->SubSysId = 0x00000000;
    pIdentifier->Revision = 0;
    
    // Generate a fake device identifier
    memset(&pIdentifier->DeviceIdentifier, 0, sizeof(GUID));
    pIdentifier->DeviceIdentifier.Data1 = 0x12345678;
    
    // Driver version info
    pIdentifier->DriverVersion.QuadPart = MAKELONG(0, 1);  // Version 1.0
    
    // WHQL info
    if (!(Flags & D3DENUM_NO_WHQL_LEVEL)) {
        pIdentifier->WHQLLevel = 1;  // Pretend to be WHQL certified
    } else {
        pIdentifier->WHQLLevel = 0;
    }
    
    DX8GL_INFO("GetAdapterIdentifier(%u) called:", Adapter);
    DX8GL_INFO("  Driver: %s", pIdentifier->Driver);
    DX8GL_INFO("  Description: %s", pIdentifier->Description);
    DX8GL_INFO("  VendorId: 0x%04X (fake ATI)", pIdentifier->VendorId);
    DX8GL_INFO("  DeviceId: 0x%04X (fake Radeon 7500)", pIdentifier->DeviceId);
    DX8GL_INFO("  DriverVersion: %llu (1.0)", pIdentifier->DriverVersion.QuadPart);
    DX8GL_INFO("  WHQL Level: %d", pIdentifier->WHQLLevel);
    return D3D_OK;
}

UINT Direct3D8::GetAdapterModeCount(UINT Adapter) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode - return standard display modes
    if (Adapter != 0) {
        DX8GL_WARN("GetAdapterModeCount(%u) - invalid adapter, only 0 is valid", Adapter);
        return 0;
    }
    
    // In DX8, GetAdapterModeCount doesn't take a format parameter
    // So we return total count of all modes for all formats
    // We support multiple resolutions in multiple formats
    const struct {
        UINT width, height;
    } resolutions[] = {
        { 640, 480 },
        { 800, 600 },
        { 1024, 768 },
        { 1280, 720 },
        { 1280, 960 },
        { 1280, 1024 },
        { 1366, 768 },
        { 1600, 900 },
        { 1600, 1200 },
        { 1920, 1080 },
        { 2560, 1440 }
    };
    
    const D3DFORMAT formats[] = {
        D3DFMT_R5G6B5,      // 16-bit
        D3DFMT_X1R5G5B5,    // 16-bit
        D3DFMT_X8R8G8B8,    // 32-bit
        D3DFMT_A8R8G8B8     // 32-bit with alpha
    };
    
    // Total modes = resolutions * formats
    UINT total_modes = (sizeof(resolutions) / sizeof(resolutions[0])) * 
                      (sizeof(formats) / sizeof(formats[0]));
    
    DX8GL_INFO("GetAdapterModeCount(%u) called - returning %u display modes for OSMesa", 
               Adapter, total_modes);
    return total_modes;
}

HRESULT Direct3D8::EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    if (!pMode) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode - return hardcoded display modes
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    // Define the resolutions and formats we support
    const struct {
        UINT width, height;
    } resolutions[] = {
        { 640, 480 },
        { 800, 600 },
        { 1024, 768 },
        { 1280, 720 },
        { 1280, 960 },
        { 1280, 1024 },
        { 1366, 768 },
        { 1600, 900 },
        { 1600, 1200 },
        { 1920, 1080 },
        { 2560, 1440 }
    };
    
    const struct {
        D3DFORMAT format;
        UINT refresh_rate;
    } formats[] = {
        { D3DFMT_R5G6B5, 60 },      // 16-bit
        { D3DFMT_X1R5G5B5, 60 },    // 16-bit
        { D3DFMT_X8R8G8B8, 60 },    // 32-bit
        { D3DFMT_A8R8G8B8, 60 }     // 32-bit with alpha
    };
    
    const UINT num_resolutions = sizeof(resolutions) / sizeof(resolutions[0]);
    const UINT num_formats = sizeof(formats) / sizeof(formats[0]);
    const UINT total_modes = num_resolutions * num_formats;
    
    if (Mode >= total_modes) {
        return D3DERR_INVALIDCALL;
    }
    
    // Calculate which resolution and format this mode index corresponds to
    UINT format_index = Mode / num_resolutions;
    UINT resolution_index = Mode % num_resolutions;
    
    pMode->Width = resolutions[resolution_index].width;
    pMode->Height = resolutions[resolution_index].height;
    pMode->RefreshRate = formats[format_index].refresh_rate;
    pMode->Format = formats[format_index].format;
    
    DX8GL_TRACE("EnumAdapterModes(%u, %u) -> %ux%u@%uHz format=0x%08X (OSMesa)", 
                Adapter, Mode, pMode->Width, pMode->Height, 
                pMode->RefreshRate, pMode->Format);
    return D3D_OK;
}

HRESULT Direct3D8::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) {
    if (!pMode) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode - return default display mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    // Return default 1024x768 32-bit mode for OSMesa
    pMode->Width = 1024;
    pMode->Height = 768;
    pMode->RefreshRate = 60;
    pMode->Format = D3DFMT_A8R8G8B8;
    
    DX8GL_INFO("GetAdapterDisplayMode(%u) called - returning current display mode:", Adapter);
    DX8GL_INFO("  Resolution: %ux%u", pMode->Width, pMode->Height);
    DX8GL_INFO("  Refresh Rate: %u Hz", pMode->RefreshRate);
    DX8GL_INFO("  Format: 0x%08X (A8R8G8B8 - 32-bit ARGB)", pMode->Format);
    return D3D_OK;
}

HRESULT Direct3D8::CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, 
                                  D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                  BOOL bWindowed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    // We only support HAL device type
    if (DevType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    // Check if formats are compatible
    // For windowed mode, backbuffer format must match adapter format
    if (bWindowed && AdapterFormat != BackBufferFormat) {
        // Allow some compatible format conversions
        if (!((AdapterFormat == D3DFMT_X8R8G8B8 && BackBufferFormat == D3DFMT_A8R8G8B8) ||
              (AdapterFormat == D3DFMT_A8R8G8B8 && BackBufferFormat == D3DFMT_X8R8G8B8))) {
            return D3DERR_NOTAVAILABLE;
        }
    }
    
    // We support these common formats
    switch (BackBufferFormat) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            DX8GL_TRACE("CheckDeviceType(%u, %d, %d, %d, %d) -> OK", 
                        Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed);
            return D3D_OK;
        default:
            return D3DERR_NOTAVAILABLE;
    }
}

HRESULT Direct3D8::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType,
                                    D3DFORMAT AdapterFormat, DWORD Usage,
                                    D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (DeviceType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    // Check texture formats
    if (RType == D3DRTYPE_TEXTURE || RType == D3DRTYPE_CUBETEXTURE) {
        // We support these texture formats
        switch (CheckFormat) {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_A4R4G4B4:
            case D3DFMT_R8G8B8:
            case D3DFMT_A8:
            case D3DFMT_L8:
            case D3DFMT_A8L8:
            case D3DFMT_DXT1:
            case D3DFMT_DXT3:
            case D3DFMT_DXT5:
                return D3D_OK;
            default:
                return D3DERR_NOTAVAILABLE;
        }
    }
    
    // Check depth/stencil formats
    if (Usage & D3DUSAGE_DEPTHSTENCIL) {
        switch (CheckFormat) {
            case D3DFMT_D16:
            case D3DFMT_D24S8:
            case D3DFMT_D24X8:
            case D3DFMT_D32:
                return D3D_OK;
            default:
                return D3DERR_NOTAVAILABLE;
        }
    }
    
    // Check render target formats
    if (Usage & D3DUSAGE_RENDERTARGET) {
        switch (CheckFormat) {
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_A4R4G4B4:
            case D3DFMT_R8G8B8:
            case D3DFMT_X8R8G8B8:
            case D3DFMT_A8R8G8B8:
                return D3D_OK;
            default:
                return D3DERR_NOTAVAILABLE;
        }
    }
    
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3D8::CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType,
                                             D3DFORMAT SurfaceFormat, BOOL Windowed,
                                             D3DMULTISAMPLE_TYPE MultiSampleType) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (DeviceType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    // We only support no multisampling for now
    if (MultiSampleType == D3DMULTISAMPLE_NONE) {
        return D3D_OK;
    }
    
    return D3DERR_NOTAVAILABLE;
}

HRESULT Direct3D8::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType,
                                         D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
                                         D3DFORMAT DepthStencilFormat) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (DeviceType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    // Check if depth/stencil format is valid
    switch (DepthStencilFormat) {
        case D3DFMT_D16:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D32:
            // All depth formats work with all render target formats
            return D3D_OK;
        default:
            return D3DERR_NOTAVAILABLE;
    }
}

HRESULT Direct3D8::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    if (!pCaps) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (DeviceType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    // Initialize with minimal fake caps for OSMesa
    memset(pCaps, 0, sizeof(D3DCAPS8));
    pCaps->DeviceType = D3DDEVTYPE_HAL;
    pCaps->MaxTextureWidth = 1024;
    pCaps->MaxTextureHeight = 1024;
    pCaps->MaxVolumeExtent = 256;
    pCaps->MaxTextureRepeat = 8192;
    pCaps->MaxTextureAspectRatio = 0; // No aspect ratio limit
    pCaps->MaxAnisotropy = 1;
    pCaps->MaxVertexIndex = 65535;
    pCaps->MaxStreams = 1;
    pCaps->MaxStreamStride = 255;
    pCaps->MaxPointSize = 64.0f;
    pCaps->MaxPrimitiveCount = 65535;
    pCaps->MaxVertexShaderConst = 0; // No vertex shader constants
    
    DX8GL_DEBUG("GetDeviceCaps(%u) -> OK (OSMesa)", Adapter);
    return D3D_OK;
}

HMONITOR Direct3D8::GetAdapterMonitor(UINT Adapter) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode - return fake monitor handle
    if (Adapter != 0) {
        return nullptr;
    }
    
    // Return a dummy monitor handle for OSMesa
    return reinterpret_cast<HMONITOR>(0x12345678);
}

HRESULT Direct3D8::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType,
                               HWND hFocusWindow, DWORD BehaviorFlags,
                               D3DPRESENT_PARAMETERS* pPresentationParameters,
                               IDirect3DDevice8** ppReturnedDeviceInterface) {
    if (!pPresentationParameters || !ppReturnedDeviceInterface) {
        return D3DERR_INVALIDCALL;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ALWAYS OSMesa mode
    if (Adapter != 0) {
        return D3DERR_INVALIDCALL;
    }
    
    if (DeviceType != D3DDEVTYPE_HAL) {
        return D3DERR_NOTAVAILABLE;
    }
    
    DX8GL_INFO("CreateDevice() called with parameters:");
    DX8GL_INFO("  Adapter: %u (OSMesa software adapter)", Adapter);
    DX8GL_INFO("  DeviceType: %s", DeviceType == D3DDEVTYPE_HAL ? "HAL" : "REF/SW");
    DX8GL_INFO("  Focus Window: %p", hFocusWindow);
    DX8GL_INFO("  Behavior Flags: 0x%08X", BehaviorFlags);
    if (BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) {
        DX8GL_INFO("    - SOFTWARE_VERTEXPROCESSING");
    }
    if (BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) {
        DX8GL_INFO("    - HARDWARE_VERTEXPROCESSING");
    }
    if (BehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) {
        DX8GL_INFO("    - MIXED_VERTEXPROCESSING");
    }
    if (BehaviorFlags & D3DCREATE_DISABLE_DRIVER_MANAGEMENT) {
        DX8GL_INFO("    - DISABLE_DRIVER_MANAGEMENT");
    }
    if (BehaviorFlags & 0x00000004) {  // D3DCREATE_MULTITHREADED
        DX8GL_INFO("    - MULTITHREADED");
    }
    DX8GL_INFO("  Present Parameters:");
    DX8GL_INFO("    BackBuffer Size: %ux%u", 
               pPresentationParameters->BackBufferWidth,
               pPresentationParameters->BackBufferHeight);
    DX8GL_INFO("    BackBuffer Format: 0x%08X", pPresentationParameters->BackBufferFormat);
    DX8GL_INFO("    BackBuffer Count: %u", pPresentationParameters->BackBufferCount);
    DX8GL_INFO("    MultiSample Type: %u", pPresentationParameters->MultiSampleType);
    DX8GL_INFO("    SwapEffect: %u", pPresentationParameters->SwapEffect);
    DX8GL_INFO("    Windowed: %s", pPresentationParameters->Windowed ? "Yes" : "No");
    DX8GL_INFO("    EnableAutoDepthStencil: %s", pPresentationParameters->EnableAutoDepthStencil ? "Yes" : "No");
    if (pPresentationParameters->EnableAutoDepthStencil) {
        DX8GL_INFO("    AutoDepthStencilFormat: 0x%08X", pPresentationParameters->AutoDepthStencilFormat);
    }
    DX8GL_INFO("    FullScreen_RefreshRateInHz: %u", pPresentationParameters->FullScreen_RefreshRateInHz);
    DX8GL_INFO("    FullScreen_PresentationInterval: 0x%08X", pPresentationParameters->FullScreen_PresentationInterval);
    
    // Create the device
    DX8GL_INFO("Creating Direct3DDevice8 instance...");
    auto device = new Direct3DDevice8(this, Adapter, DeviceType, hFocusWindow, 
                                     BehaviorFlags, pPresentationParameters);
    DX8GL_INFO("Direct3DDevice8 instance created at %p", device);
    
    DX8GL_INFO("Initializing Direct3DDevice8...");
    if (!device->initialize()) {
        DX8GL_ERROR("Direct3DDevice8 initialization failed!");
        device->Release();
        return D3DERR_NOTAVAILABLE;
    }
    DX8GL_INFO("Direct3DDevice8 initialization successful");
    
    *ppReturnedDeviceInterface = device;
    DX8GL_INFO("CreateDevice() successful - returning IDirect3DDevice8 at %p", device);
    return D3D_OK;
}

// Private methods
void Direct3D8::enumerate_adapters() {
    DX8GL_INFO("Enumerating adapters for OSMesa mode");
    DX8GL_INFO("  - OSMesa provides software rendering via llvmpipe");
    DX8GL_INFO("  - Simulating 1 adapter (no actual hardware enumeration)");
    DX8GL_INFO("  - Adapter 0: OSMesa Software Renderer");
    DX8GL_INFO("  - Vendor: Mesa/llvmpipe (fake ATI VendorId 0x1002)");
    DX8GL_INFO("  - Device: Software Rasterizer (fake Radeon 7500 DeviceId 0x5159)");
    
    // For OSMesa, we don't create any adapter objects to avoid STL/memory issues
    // The GetAdapterCount and GetAdapterIdentifier methods are hardcoded for OSMesa
    // This avoids any memory allocation issues with VS6/C++17 incompatibility
    
    DX8GL_INFO("OSMesa adapter enumeration complete - 1 software adapter available");
}

void Direct3D8::populate_display_modes(AdapterInfo& adapter) {
    // Add common display modes
    const struct {
        UINT width, height, refresh;
        D3DFORMAT format;
    } modes[] = {
        // 4:3 modes
        { 640, 480, 60, D3DFMT_R5G6B5 },
        { 640, 480, 60, D3DFMT_X8R8G8B8 },
        { 800, 600, 60, D3DFMT_R5G6B5 },
        { 800, 600, 60, D3DFMT_X8R8G8B8 },
        { 1024, 768, 60, D3DFMT_R5G6B5 },
        { 1024, 768, 60, D3DFMT_X8R8G8B8 },
        { 1280, 960, 60, D3DFMT_X8R8G8B8 },
        { 1280, 1024, 60, D3DFMT_X8R8G8B8 },
        { 1600, 1200, 60, D3DFMT_X8R8G8B8 },
        
        // 16:9 modes
        { 1280, 720, 60, D3DFMT_X8R8G8B8 },
        { 1366, 768, 60, D3DFMT_X8R8G8B8 },
        { 1920, 1080, 60, D3DFMT_X8R8G8B8 },
        { 2560, 1440, 60, D3DFMT_X8R8G8B8 },
        { 3840, 2160, 60, D3DFMT_X8R8G8B8 },
    };
    
    for (const auto& mode : modes) {
        D3DDISPLAYMODE dm;
        dm.Width = mode.width;
        dm.Height = mode.height;
        dm.RefreshRate = mode.refresh;
        dm.Format = mode.format;
        adapter.display_modes.push_back(dm);
    }
}

void Direct3D8::populate_device_caps(AdapterInfo& adapter) {
    D3DCAPS8& caps = adapter.caps;
    memset(&caps, 0, sizeof(D3DCAPS8));
    
    // Device info
    caps.DeviceType = D3DDEVTYPE_HAL;
    caps.AdapterOrdinal = 0;
    
    // Caps flags
    caps.Caps = D3DCAPS_READ_SCANLINE;
    caps.Caps2 = 0x00080000 /*D3DCAPS2_CANRENDERWINDOWED*/ | D3DCAPS2_FULLSCREENGAMMA;
    caps.Caps3 = D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD;
    
    // Presentation intervals
    caps.PresentationIntervals = D3DPRESENT_INTERVAL_ONE | 
                                D3DPRESENT_INTERVAL_TWO |
                                D3DPRESENT_INTERVAL_THREE |
                                D3DPRESENT_INTERVAL_FOUR |
                                D3DPRESENT_INTERVAL_IMMEDIATE;
    
    // Cursor caps
    caps.CursorCaps = D3DCURSORCAPS_COLOR | D3DCURSORCAPS_LOWRES;
    
    // 3D Device caps
    caps.DevCaps = D3DDEVCAPS_EXECUTESYSTEMMEMORY |
                   D3DDEVCAPS_EXECUTEVIDEOMEMORY |
                   D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
                   D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
                   D3DDEVCAPS_TEXTUREVIDEOMEMORY |
                   D3DDEVCAPS_DRAWPRIMTLVERTEX |
                   D3DDEVCAPS_CANRENDERAFTERFLIP |
                   D3DDEVCAPS_TEXTURENONLOCALVIDMEM |
                   D3DDEVCAPS_HWRASTERIZATION |
                   D3DDEVCAPS_PUREDEVICE |
                   D3DDEVCAPS_QUINTICRTPATCHES |
                   D3DDEVCAPS_RTPATCHES;
    
    // Primitive misc caps
    caps.PrimitiveMiscCaps = D3DPMISCCAPS_MASKZ |
                            D3DPMISCCAPS_CULLNONE |
                            D3DPMISCCAPS_CULLCW |
                            D3DPMISCCAPS_CULLCCW |
                            D3DPMISCCAPS_COLORWRITEENABLE |
                            D3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                            D3DPMISCCAPS_CLIPTLVERTS |
                            D3DPMISCCAPS_TSSARGTEMP |
                            D3DPMISCCAPS_BLENDOP;
    
    // Raster caps
    caps.RasterCaps = D3DPRASTERCAPS_DITHER |
                     D3DPRASTERCAPS_ZTEST |
                     D3DPRASTERCAPS_FOGVERTEX |
                     D3DPRASTERCAPS_FOGTABLE |
                     D3DPRASTERCAPS_MIPMAPLODBIAS |
                     D3DPRASTERCAPS_ZBIAS |
                     D3DPRASTERCAPS_ANISOTROPY |
                     D3DPRASTERCAPS_WFOG |
                     D3DPRASTERCAPS_ZFOG |
                     D3DPRASTERCAPS_COLORPERSPECTIVE |
                     0x01000000 /*D3DPRASTERCAPS_SCISSORTEST*/;
    
    // Z-compare caps
    caps.ZCmpCaps = D3DPCMPCAPS_NEVER |
                   D3DPCMPCAPS_LESS |
                   D3DPCMPCAPS_EQUAL |
                   D3DPCMPCAPS_LESSEQUAL |
                   D3DPCMPCAPS_GREATER |
                   D3DPCMPCAPS_NOTEQUAL |
                   D3DPCMPCAPS_GREATEREQUAL |
                   D3DPCMPCAPS_ALWAYS;
    
    // Source blend caps
    caps.SrcBlendCaps = D3DPBLENDCAPS_ZERO |
                       D3DPBLENDCAPS_ONE |
                       D3DPBLENDCAPS_SRCCOLOR |
                       D3DPBLENDCAPS_INVSRCCOLOR |
                       D3DPBLENDCAPS_SRCALPHA |
                       D3DPBLENDCAPS_INVSRCALPHA |
                       D3DPBLENDCAPS_DESTALPHA |
                       D3DPBLENDCAPS_INVDESTALPHA |
                       D3DPBLENDCAPS_DESTCOLOR |
                       D3DPBLENDCAPS_INVDESTCOLOR |
                       D3DPBLENDCAPS_SRCALPHASAT |
                       D3DPBLENDCAPS_BOTHSRCALPHA |
                       D3DPBLENDCAPS_BOTHINVSRCALPHA;
    
    // Dest blend caps
    caps.DestBlendCaps = caps.SrcBlendCaps;
    
    // Alpha compare caps
    caps.AlphaCmpCaps = caps.ZCmpCaps;
    
    // Shade caps
    caps.ShadeCaps = D3DPSHADECAPS_COLORGOURAUDRGB |
                    D3DPSHADECAPS_SPECULARGOURAUDRGB |
                    D3DPSHADECAPS_ALPHAGOURAUDBLEND |
                    D3DPSHADECAPS_FOGGOURAUD;
    
    // Texture caps
    caps.TextureCaps = D3DPTEXTURECAPS_PERSPECTIVE |
                      D3DPTEXTURECAPS_POW2 |
                      D3DPTEXTURECAPS_ALPHA |
                      D3DPTEXTURECAPS_SQUAREONLY |
                      D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |
                      D3DPTEXTURECAPS_ALPHAPALETTE |
                      D3DPTEXTURECAPS_PROJECTED |
                      D3DPTEXTURECAPS_CUBEMAP |
                      D3DPTEXTURECAPS_VOLUMEMAP |
                      D3DPTEXTURECAPS_MIPMAP |
                      D3DPTEXTURECAPS_MIPVOLUMEMAP |
                      D3DPTEXTURECAPS_MIPCUBEMAP;
    
    // Texture filter caps
    caps.TextureFilterCaps = D3DPTFILTERCAPS_MINFPOINT |
                           D3DPTFILTERCAPS_MINFLINEAR |
                           D3DPTFILTERCAPS_MINFANISOTROPIC |
                           D3DPTFILTERCAPS_MIPFPOINT |
                           D3DPTFILTERCAPS_MIPFLINEAR |
                           D3DPTFILTERCAPS_MAGFPOINT |
                           D3DPTFILTERCAPS_MAGFLINEAR |
                           D3DPTFILTERCAPS_MAGFANISOTROPIC |
                           D3DPTFILTERCAPS_MAGFAFLATCUBIC |
                           D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC;
    
    // Cube texture filter caps
    caps.CubeTextureFilterCaps = caps.TextureFilterCaps;
    
    // Volume texture filter caps
    caps.VolumeTextureFilterCaps = caps.TextureFilterCaps;
    
    // Texture address caps
    caps.TextureAddressCaps = D3DPTADDRESSCAPS_WRAP |
                             D3DPTADDRESSCAPS_MIRROR |
                             D3DPTADDRESSCAPS_CLAMP |
                             D3DPTADDRESSCAPS_BORDER |
                             D3DPTADDRESSCAPS_INDEPENDENTUV |
                             D3DPTADDRESSCAPS_MIRRORONCE;
    
    // Volume texture address caps
    caps.VolumeTextureAddressCaps = caps.TextureAddressCaps;
    
    // Line caps
    caps.LineCaps = D3DLINECAPS_TEXTURE |
                   D3DLINECAPS_ZTEST |
                   D3DLINECAPS_BLEND |
                   D3DLINECAPS_ALPHACMP |
                   D3DLINECAPS_FOG;
    
    // Max texture dimensions
    caps.MaxTextureWidth = 4096;
    caps.MaxTextureHeight = 4096;
    caps.MaxVolumeExtent = 512;
    caps.MaxTextureRepeat = 8192;
    caps.MaxTextureAspectRatio = 8192;
    caps.MaxAnisotropy = 16;
    caps.MaxVertexW = 1e10f;
    
    // Guard band limits
    caps.GuardBandLeft = -1e10f;
    caps.GuardBandTop = -1e10f;
    caps.GuardBandRight = 1e10f;
    caps.GuardBandBottom = 1e10f;
    
    // Fog limits
    caps.ExtentsAdjust = 0;
    
    // Stencil caps
    caps.StencilCaps = D3DSTENCILCAPS_KEEP |
                      D3DSTENCILCAPS_ZERO |
                      D3DSTENCILCAPS_REPLACE |
                      D3DSTENCILCAPS_INCRSAT |
                      D3DSTENCILCAPS_DECRSAT |
                      D3DSTENCILCAPS_INVERT |
                      D3DSTENCILCAPS_INCR |
                      D3DSTENCILCAPS_DECR;
    
    // FVF caps
    caps.FVFCaps = D3DFVFCAPS_TEXCOORDCOUNTMASK |
                  D3DFVFCAPS_DONOTSTRIPELEMENTS |
                  D3DFVFCAPS_PSIZE;
    
    // Texture op caps
    caps.TextureOpCaps = D3DTEXOPCAPS_DISABLE |
                        D3DTEXOPCAPS_SELECTARG1 |
                        D3DTEXOPCAPS_SELECTARG2 |
                        D3DTEXOPCAPS_MODULATE |
                        D3DTEXOPCAPS_MODULATE2X |
                        D3DTEXOPCAPS_MODULATE4X |
                        D3DTEXOPCAPS_ADD |
                        D3DTEXOPCAPS_ADDSIGNED |
                        D3DTEXOPCAPS_ADDSIGNED2X |
                        D3DTEXOPCAPS_SUBTRACT |
                        D3DTEXOPCAPS_ADDSMOOTH |
                        D3DTEXOPCAPS_BLENDDIFFUSEALPHA |
                        D3DTEXOPCAPS_BLENDTEXTUREALPHA |
                        D3DTEXOPCAPS_BLENDFACTORALPHA |
                        D3DTEXOPCAPS_BLENDTEXTUREALPHAPM |
                        D3DTEXOPCAPS_BLENDCURRENTALPHA |
                        D3DTEXOPCAPS_PREMODULATE |
                        D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR |
                        D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
                        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
                        D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
                        D3DTEXOPCAPS_BUMPENVMAP |
                        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE |
                        D3DTEXOPCAPS_DOTPRODUCT3 |
                        D3DTEXOPCAPS_MULTIPLYADD |
                        D3DTEXOPCAPS_LERP;
    
    // Max texture blend stages
    caps.MaxTextureBlendStages = 8;
    caps.MaxSimultaneousTextures = 8;
    
    // Vertex processing caps
    caps.VertexProcessingCaps = D3DVTXPCAPS_TEXGEN |
                               D3DVTXPCAPS_MATERIALSOURCE7 |
                               D3DVTXPCAPS_DIRECTIONALLIGHTS |
                               D3DVTXPCAPS_POSITIONALLIGHTS |
                               D3DVTXPCAPS_LOCALVIEWER |
                               D3DVTXPCAPS_TWEENING;
    
    // Max active lights
    caps.MaxActiveLights = 8;
    caps.MaxUserClipPlanes = 6;
    caps.MaxVertexBlendMatrices = 4;
    caps.MaxVertexBlendMatrixIndex = 255;
    
    // Point parameters
    caps.MaxPointSize = 256.0f;
    caps.MaxPrimitiveCount = 0xFFFFFF;
    caps.MaxVertexIndex = 0xFFFFFF;
    caps.MaxStreams = 16;
    caps.MaxStreamStride = 2048;
    
    // Shader versions
    caps.VertexShaderVersion = D3DVS_VERSION(1, 1);  // Vertex shader 1.1
    caps.MaxVertexShaderConst = 96;
    caps.PixelShaderVersion = D3DPS_VERSION(1, 4);   // Pixel shader 1.4
    caps.MaxPixelShaderValue = 8.0f;
}

D3DFORMAT Direct3D8::get_desktop_format() {
    // Default to 32-bit XRGB
    return D3DFMT_X8R8G8B8;
}

bool Direct3D8::find_color_mode(D3DFORMAT format, UINT width, UINT height, UINT* out_mode) {
    DX8GL_INFO("find_color_mode: format=0x%08X, resolution=%ux%u", format, width, height);
    
    // Get the mode count for this specific format
    UINT mode_count = GetAdapterModeCount(0);
    if (mode_count == 0) {
        DX8GL_WARN("find_color_mode: No modes available");
        return false;
    }
    
    // Look for exact match first
    bool found = false;
    UINT best_mode = 0;
    UINT best_refresh = 0;
    
    for (UINT i = 0; i < mode_count; i++) {
        D3DDISPLAYMODE mode;
        if (SUCCEEDED(EnumAdapterModes(0, i, &mode))) {
            // Check for exact resolution match with highest refresh rate
            if (mode.Width == width && mode.Height == height && mode.Format == format) {
                if (mode.RefreshRate >= best_refresh) {
                    best_refresh = mode.RefreshRate;
                    best_mode = i;
                    found = true;
                    DX8GL_DEBUG("find_color_mode: Found exact match at mode %u: %ux%u@%uHz", 
                               i, mode.Width, mode.Height, mode.RefreshRate);
                }
            }
        }
    }
    
    // If no exact match, find the closest larger resolution
    if (!found) {
        UINT closest_width = UINT_MAX;
        UINT closest_height = UINT_MAX;
        
        for (UINT i = 0; i < mode_count; i++) {
            D3DDISPLAYMODE mode;
            if (SUCCEEDED(EnumAdapterModes(0, i, &mode))) {
                if (mode.Format == format && mode.Width >= width && mode.Height >= height) {
                    // Check if this is closer than our current best
                    if (mode.Width <= closest_width && mode.Height <= closest_height) {
                        if (mode.Width < closest_width || mode.Height < closest_height ||
                            mode.RefreshRate > best_refresh) {
                            closest_width = mode.Width;
                            closest_height = mode.Height;
                            best_refresh = mode.RefreshRate;
                            best_mode = i;
                            found = true;
                            DX8GL_DEBUG("find_color_mode: Found larger match at mode %u: %ux%u@%uHz", 
                                       i, mode.Width, mode.Height, mode.RefreshRate);
                        }
                    }
                }
            }
        }
    }
    
    if (found && out_mode) {
        *out_mode = best_mode;
        DX8GL_INFO("find_color_mode: Selected mode %u", best_mode);
    }
    
    return found;
}

bool Direct3D8::find_z_mode(D3DFORMAT color_format, D3DFORMAT backbuffer_format, D3DFORMAT* out_z_format) {
    DX8GL_INFO("find_z_mode: color_format=0x%08X, backbuffer_format=0x%08X", 
               color_format, backbuffer_format);
    
    // Test depth/stencil formats in order of preference (highest quality first)
    // Prefer formats with stencil buffer
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D24S8)) {
        *out_z_format = D3DFMT_D24S8;
        DX8GL_INFO("find_z_mode: Found D24S8 (24-bit depth, 8-bit stencil)");
        return true;
    }
    
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D32)) {
        *out_z_format = D3DFMT_D32;
        DX8GL_INFO("find_z_mode: Found D32 (32-bit depth, no stencil)");
        return true;
    }
    
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D24X8)) {
        *out_z_format = D3DFMT_D24X8;
        DX8GL_INFO("find_z_mode: Found D24X8 (24-bit depth, no stencil)");
        return true;
    }
    
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D24X4S4)) {
        *out_z_format = D3DFMT_D24X4S4;
        DX8GL_INFO("find_z_mode: Found D24X4S4 (24-bit depth, 4-bit stencil)");
        return true;
    }
    
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D16)) {
        *out_z_format = D3DFMT_D16;
        DX8GL_INFO("find_z_mode: Found D16 (16-bit depth, no stencil)");
        return true;
    }
    
    if (test_z_mode(color_format, backbuffer_format, D3DFMT_D15S1)) {
        *out_z_format = D3DFMT_D15S1;
        DX8GL_INFO("find_z_mode: Found D15S1 (15-bit depth, 1-bit stencil)");
        return true;
    }
    
    DX8GL_WARN("find_z_mode: No compatible depth/stencil format found");
    return false;
}

bool Direct3D8::test_z_mode(D3DFORMAT color_format, D3DFORMAT backbuffer_format, D3DFORMAT z_format) {
    // First check if the depth format is supported
    HRESULT hr = CheckDeviceFormat(0, D3DDEVTYPE_HAL, color_format, 
                                   D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, z_format);
    if (FAILED(hr)) {
        DX8GL_DEBUG("test_z_mode: CheckDeviceFormat failed for z_format=0x%08X", z_format);
        return false;
    }
    
    // Then check if it's compatible with the render target format
    hr = CheckDepthStencilMatch(0, D3DDEVTYPE_HAL, color_format, 
                                backbuffer_format, z_format);
    if (FAILED(hr)) {
        DX8GL_DEBUG("test_z_mode: CheckDepthStencilMatch failed for z_format=0x%08X", z_format);
        return false;
    }
    
    DX8GL_DEBUG("test_z_mode: z_format=0x%08X is compatible", z_format);
    return true;
}

bool Direct3D8::FindColorAndZMode(UINT width, UINT height, UINT bit_depth,
                                 D3DFORMAT* out_color_format, D3DFORMAT* out_backbuffer_format,
                                 D3DFORMAT* out_z_format) {
    DX8GL_INFO("FindColorAndZMode: %ux%u %u-bit", width, height, bit_depth);
    
    // Format tables based on bit depth
    static const D3DFORMAT formats16[] = {
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A1R5G5B5
    };
    
    static const D3DFORMAT formats32[] = {
        D3DFMT_X8R8G8B8,
        D3DFMT_A8R8G8B8
    };
    
    const D3DFORMAT* format_table;
    int format_count;
    
    if (bit_depth == 16) {
        format_table = formats16;
        format_count = sizeof(formats16) / sizeof(formats16[0]);
    } else if (bit_depth == 32) {
        format_table = formats32;
        format_count = sizeof(formats32) / sizeof(formats32[0]);
    } else {
        DX8GL_WARN("FindColorAndZMode: Unsupported bit depth %u", bit_depth);
        return false;
    }
    
    // Try to find a valid color mode
    bool found = false;
    UINT mode = 0;
    int format_index;
    
    for (format_index = 0; format_index < format_count; format_index++) {
        if (find_color_mode(format_table[format_index], width, height, &mode)) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        DX8GL_WARN("FindColorAndZMode: No suitable color mode found");
        return false;
    }
    
    // Set the color and backbuffer formats
    if (out_color_format) {
        *out_color_format = format_table[format_index];
    }
    if (out_backbuffer_format) {
        *out_backbuffer_format = format_table[format_index];
    }
    
    // Promote 32-bit X8R8G8B8 to A8R8G8B8 if supported
    if (bit_depth == 32 && format_table[format_index] == D3DFMT_X8R8G8B8) {
        HRESULT hr = CheckDeviceType(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 
                                    D3DFMT_A8R8G8B8, TRUE);
        if (SUCCEEDED(hr) && out_backbuffer_format) {
            *out_backbuffer_format = D3DFMT_A8R8G8B8;
            DX8GL_INFO("FindColorAndZMode: Promoted X8R8G8B8 to A8R8G8B8");
        }
    }
    
    // Find a compatible Z buffer format
    D3DFORMAT color_fmt = out_color_format ? *out_color_format : format_table[format_index];
    D3DFORMAT backbuffer_fmt = out_backbuffer_format ? *out_backbuffer_format : format_table[format_index];
    
    bool z_found = find_z_mode(color_fmt, backbuffer_fmt, out_z_format);
    if (!z_found) {
        DX8GL_WARN("FindColorAndZMode: No compatible Z buffer format found");
        // Still return true since we found color formats
    }
    
    DX8GL_INFO("FindColorAndZMode: Success - color=0x%08X, backbuffer=0x%08X, z=0x%08X",
               color_fmt, backbuffer_fmt, out_z_format ? *out_z_format : 0);
    
    return true;
}

} // namespace dx8gl