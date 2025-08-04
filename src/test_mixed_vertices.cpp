#include <iostream>
#include <cstring>
#include "dx8gl.h"
#include "d3d8.h"

// Test application to isolate crash when mixing regular and XYZRHW vertices

int main() {
    std::cout << "Testing mixed vertex formats (XYZ and XYZRHW)..." << std::endl;
    
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
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
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
    
    // Enable depth testing
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    
    // Define regular vertex structure (XYZ + color)
    struct XYZVertex {
        float x, y, z;
        DWORD color;
    };
    
    // Define XYZRHW vertex structure
    struct XYZRHWVertex {
        float x, y, z, rhw;
        DWORD color;
    };
    
    // Clear the back buffer
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF404040, 1.0f, 0);
    
    // Begin scene
    device->BeginScene();
    
    // First draw a regular 3D triangle
    {
        XYZVertex vertices[3] = {
            { -0.5f,  0.5f, 0.5f, 0xFFFF0000 }, // Top left - red
            {  0.5f,  0.5f, 0.5f, 0xFF00FF00 }, // Top right - green
            {  0.0f, -0.5f, 0.5f, 0xFF0000FF }  // Bottom center - blue
        };
        
        // Set vertex shader for regular vertices
        device->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        
        // Draw the triangle
        std::cout << "Drawing regular XYZ triangle..." << std::endl;
        hr = device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(XYZVertex));
        if (FAILED(hr)) {
            std::cerr << "DrawPrimitiveUP (XYZ) failed: " << std::hex << hr << std::endl;
        }
    }
    
    // Now draw a XYZRHW triangle (HUD)
    {
        XYZRHWVertex vertices[3] = {
            { 128.0f,  50.0f, 0.5f, 1.0f, 0xFFFFFF00 }, // Top center - yellow
            {  50.0f, 200.0f, 0.5f, 1.0f, 0xFFFF00FF }, // Bottom left - magenta
            { 200.0f, 200.0f, 0.5f, 1.0f, 0xFF00FFFF }  // Bottom right - cyan
        };
        
        // Set vertex shader to use XYZRHW format
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        
        // Draw the triangle
        std::cout << "Drawing XYZRHW triangle..." << std::endl;
        hr = device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(XYZRHWVertex));
        if (FAILED(hr)) {
            std::cerr << "DrawPrimitiveUP (XYZRHW) failed: " << std::hex << hr << std::endl;
        }
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