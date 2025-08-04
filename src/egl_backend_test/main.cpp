#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>
#include "dx8gl.h"
#include "d3d8.h"

// Simple EGL backend test that uses dx8gl with EGL backend
int main(int argc, char* argv[]) {
    std::cout << "=== dx8gl EGL Backend Test ===" << std::endl;
    
    // Configure dx8gl to use EGL backend
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_EGL;
    config.enable_logging = true;
    
    // Initialize dx8gl with EGL backend
    dx8gl_error result = dx8gl_init(&config);
    if (result != DX8GL_SUCCESS) {
        std::cerr << "Failed to initialize dx8gl with EGL backend: " << result << std::endl;
        std::cerr << "Error: " << dx8gl_get_error_string() << std::endl;
        return -1;
    }
    
    std::cout << "dx8gl initialized with EGL backend" << std::endl;
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8 interface" << std::endl;
        dx8gl_shutdown();
        return -1;
    }
    
    std::cout << "Direct3D8 interface created" << std::endl;
    
    // Setup presentation parameters (no window needed for EGL surfaceless)
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    // Create device
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,  // No window handle needed for EGL surfaceless
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr) || !device) {
        std::cerr << "Failed to create Direct3D8 device: 0x" << std::hex << hr << std::endl;
        d3d8->Release();
        dx8gl_shutdown();
        return -1;
    }
    
    std::cout << "Direct3D8 device created with EGL backend" << std::endl;
    
    // Clear the screen to blue and render a simple triangle
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                  D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
    
    if (SUCCEEDED(device->BeginScene())) {
        // Simple triangle vertices
        struct Vertex {
            float x, y, z, rhw;
            DWORD color;
        };
        
        Vertex vertices[] = {
            { 400.0f, 100.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) },   // Top (red)
            { 200.0f, 400.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 255, 0) },   // Bottom left (green)
            { 600.0f, 400.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 0, 255) },   // Bottom right (blue)
        };
        
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        
        device->EndScene();
    }
    
    // Present the frame
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get framebuffer data to verify rendering
    int width, height;
    void* framebuffer = dx8gl_get_framebuffer(device, &width, &height);
    
    if (framebuffer) {
        std::cout << "Framebuffer retrieved: " << width << "x" << height << std::endl;
        
        // Save as PPM to verify rendering worked
        const char* filename = "egl_test_output.ppm";
        FILE* fp = fopen(filename, "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", width, height);
            
            // Convert RGBA to RGB and flip Y axis
            const uint8_t* pixels = static_cast<const uint8_t*>(framebuffer);
            for (int y = height - 1; y >= 0; y--) {
                for (int x = 0; x < width; x++) {
                    const uint8_t* pixel = &pixels[(y * width + x) * 4];
                    fputc(pixel[0], fp);  // R
                    fputc(pixel[1], fp);  // G
                    fputc(pixel[2], fp);  // B
                }
            }
            fclose(fp);
            std::cout << "Output saved to " << filename << std::endl;
        }
    } else {
        std::cerr << "Failed to get framebuffer" << std::endl;
    }
    
    // Cleanup
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    std::cout << "EGL backend test completed successfully" << std::endl;
    return 0;
}