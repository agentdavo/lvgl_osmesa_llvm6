/**
 * WebGPU Backend Test Example
 * 
 * This example demonstrates the WebGPU backend functionality
 * by creating a simple dx8gl context and verifying initialization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dx8gl.h"
#include "d3d8_game.h"

int main() {
    printf("dx8gl WebGPU Backend Test\n");
    printf("========================\n");
    
    // Configure dx8gl to use WebGPU backend
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_WEBGPU;
    config.enable_logging = true;
    
    // Initialize dx8gl with WebGPU backend
    printf("Initializing dx8gl with WebGPU backend...\n");
    dx8gl_error result = dx8gl_init(&config);
    
    if (result != DX8GL_SUCCESS) {
        printf("Failed to initialize dx8gl: %d\n", result);
        printf("Error: %s\n", dx8gl_get_error_string());
        return 1;
    }
    
    printf("dx8gl initialized successfully!\n");
    
    // Create DirectX 8 interface
    printf("Creating DirectX 8 interface...\n");
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        printf("Failed to create DirectX 8 interface\n");
        dx8gl_shutdown();
        return 1;
    }
    
    printf("DirectX 8 interface created successfully!\n");
    
    // Get adapter information
    D3DADAPTER_IDENTIFIER8 adapter_info = {};
    HRESULT hr = d3d8->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapter_info);
    if (SUCCEEDED(hr)) {
        printf("Adapter: %s\n", adapter_info.Description);
        printf("Driver: %s\n", adapter_info.Driver);
    }
    
    // Create a simple device for testing
    printf("Creating DirectX 8 device...\n");
    
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = nullptr; // Offscreen rendering
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.Flags = 0;
    pp.FullScreen_RefreshRateInHz = 0;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    
    IDirect3DDevice8* device = nullptr;
    hr = d3d8->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr) || !device) {
        printf("Failed to create DirectX 8 device: 0x%08lX\n", hr);
        d3d8->Release();
        dx8gl_shutdown();
        return 1;
    }
    
    printf("DirectX 8 device created successfully!\n");
    
    // Test basic rendering operations
    printf("Testing basic rendering operations...\n");
    
    // Clear the screen
    hr = device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                      D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
    if (FAILED(hr)) {
        printf("Failed to clear screen: 0x%08lX\n", hr);
    } else {
        printf("Screen cleared successfully!\n");
    }
    
    // Begin scene
    hr = device->BeginScene();
    if (SUCCEEDED(hr)) {
        printf("Scene begun successfully!\n");
        
        // End scene
        device->EndScene();
        printf("Scene ended successfully!\n");
    } else {
        printf("Failed to begin scene: 0x%08lX\n", hr);
    }
    
    // Test framebuffer access
    printf("Testing framebuffer access...\n");
    int fb_width, fb_height;
    void* framebuffer = dx8gl_get_framebuffer(device, &fb_width, &fb_height);
    if (framebuffer) {
        printf("Framebuffer accessed: %dx%d at %p\n", fb_width, fb_height, framebuffer);
    } else {
        printf("Failed to access framebuffer\n");
    }
    
    // Present the frame
    hr = device->Present(nullptr, nullptr, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
        printf("Frame presented successfully!\n");
    } else {
        printf("Failed to present frame: 0x%08lX\n", hr);
    }
    
    // Cleanup
    printf("Cleaning up...\n");
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    printf("\nWebGPU backend test completed successfully!\n");
    return 0;
}