#include <iostream>
#include <cstring>
#include "dx8gl.h"
#include "d3d8.h"

// Test application to isolate XYZRHW crash

int main() {
    std::cout << "Testing XYZRHW vertex rendering..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_init(nullptr);
    
    // Create Direct3D8
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return 1;
    }
    
    // Create device with minimal setup
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 256;
    pp.BackBufferHeight = 256;
    pp.EnableAutoDepthStencil = FALSE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr) || !device) {
        std::cerr << "Failed to create device: " << std::hex << hr << std::endl;
        d3d8->Release();
        return 1;
    }
    
    // Define XYZRHW vertex structure
    struct XYZRHWVertex {
        float x, y, z, rhw;
        DWORD color;
    };
    
    // Create a simple triangle with XYZRHW vertices
    XYZRHWVertex vertices[3] = {
        { 128.0f,  50.0f, 0.5f, 1.0f, 0xFFFF0000 }, // Top center - red
        {  50.0f, 200.0f, 0.5f, 1.0f, 0xFF00FF00 }, // Bottom left - green
        { 200.0f, 200.0f, 0.5f, 1.0f, 0xFF0000FF }  // Bottom right - blue
    };
    
    // Clear the back buffer
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF808080, 1.0f, 0);
    
    // Begin scene
    device->BeginScene();
    
    // Set vertex shader to use XYZRHW format
    device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    
    // Draw the triangle
    std::cout << "Drawing XYZRHW triangle..." << std::endl;
    hr = device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(XYZRHWVertex));
    if (FAILED(hr)) {
        std::cerr << "DrawPrimitiveUP failed: " << std::hex << hr << std::endl;
    }
    
    // End scene
    device->EndScene();
    
    // Present
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    std::cout << "Test completed successfully!" << std::endl;
    
    // Cleanup
    device->Release();
    d3d8->Release();
    
    return 0;
}