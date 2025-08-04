#ifndef DX8GL_D3D8_INTERFACE_H
#define DX8GL_D3D8_INTERFACE_H

#include "d3d8.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace dx8gl {

// Forward declarations
class Direct3DDevice8;

// Adapter information
struct AdapterInfo {
    std::string description;
    std::string driver;
    UINT vendor_id;
    UINT device_id;
    UINT subsys_id;
    UINT revision;
    GUID device_identifier;
    std::vector<D3DDISPLAYMODE> display_modes;
    D3DCAPS8 caps;
};

class Direct3D8 final : public IDirect3D8 {
public:
    Direct3D8();
    virtual ~Direct3D8();

    // Initialize the Direct3D8 object
    bool initialize();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDirect3D8 methods
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) override;
    UINT STDMETHODCALLTYPE GetAdapterCount() override;
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, 
                                                   D3DADAPTER_IDENTIFIER8* pIdentifier) override;
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter) override;
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, UINT Mode, 
                                               D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, 
                                                    D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, 
                                             D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                             BOOL bWindowed) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType,
                                               D3DFORMAT AdapterFormat, DWORD Usage,
                                               D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType,
                                                        D3DFORMAT SurfaceFormat, BOOL Windowed,
                                                        D3DMULTISAMPLE_TYPE MultiSampleType) override;
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType,
                                                     D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
                                                     D3DFORMAT DepthStencilFormat) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType,
                                           D3DCAPS8* pCaps) override;
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) override;
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType,
                                          HWND hFocusWindow, DWORD BehaviorFlags,
                                          D3DPRESENT_PARAMETERS* pPresentationParameters,
                                          IDirect3DDevice8** ppReturnedDeviceInterface) override;

    // Public helper methods for display configuration
    bool FindColorAndZMode(UINT width, UINT height, UINT bit_depth,
                          D3DFORMAT* out_color_format, D3DFORMAT* out_backbuffer_format,
                          D3DFORMAT* out_z_format);

private:
    void enumerate_adapters();
    void populate_display_modes(AdapterInfo& adapter);
    void populate_device_caps(AdapterInfo& adapter);
    D3DFORMAT get_desktop_format();
    
    // Helper functions for mode enumeration
    bool find_color_mode(D3DFORMAT format, UINT width, UINT height, UINT* out_mode);
    bool find_z_mode(D3DFORMAT color_format, D3DFORMAT backbuffer_format, D3DFORMAT* out_z_format);
    bool test_z_mode(D3DFORMAT color_format, D3DFORMAT backbuffer_format, D3DFORMAT z_format);

    std::atomic<LONG> ref_count_;
    std::vector<AdapterInfo> adapters_;  // Not used in OSMesa mode
    mutable std::mutex mutex_;
    bool initialized_;
};

} // namespace dx8gl

#endif // DX8GL_D3D8_INTERFACE_H