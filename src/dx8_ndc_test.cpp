#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include "../ext/dx8gl/src/d3d8_game.h"
#include "../ext/dx8gl/src/dx8gl.h"

struct Vertex {
    float x, y, z;    // position in NDC space
    DWORD color;      // ARGB color
};

int main() {
    std::cout << "Testing triangle with NDC coordinates..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_init(nullptr);
    
    // Create D3D8 interface
    IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return 1;
    }
    
    // Set up present parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 400;
    pp.BackBufferHeight = 400;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    // Create device
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr) || !device) {
        std::cerr << "Failed to create device: 0x" << std::hex << hr << std::endl;
        d3d->Release();
        return 1;
    }
    
    // Disable depth testing and culling
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Clear to a different color to verify
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF808080, 1.0f, 0);
    
    // Define triangle vertices in NDC space (-1 to 1)
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 0xFFFF0000 },  // Top, red
        {-0.5f, -0.5f, 0.0f, 0xFF00FF00 },  // Bottom left, green
        { 0.5f, -0.5f, 0.0f, 0xFF0000FF }   // Bottom right, blue
    };
    
    // Set vertex shader to position-only + color
    device->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    
    // Draw the triangle
    device->BeginScene();
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
    device->EndScene();
    
    // Present to make framebuffer available
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get the framebuffer
    int width = 0, height = 0;
    int frame_number = 0;
    bool updated = false;
    void* fb = dx8gl_get_shared_framebuffer(&width, &height, &frame_number, &updated);
    if (fb) {
        std::cout << "Got framebuffer: " << width << "x" << height << std::endl;
        
        // Save PPM
        FILE* fp = fopen("dx8_ndc_test.ppm", "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", width, height);
            uint8_t* pixels = (uint8_t*)fb;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;
                    uint8_t rgb[3] = { pixels[idx], pixels[idx+1], pixels[idx+2] };
                    fwrite(rgb, 1, 3, fp);
                }
            }
            fclose(fp);
            std::cout << "Saved dx8_ndc_test.ppm" << std::endl;
        }
        
        // Check pixels
        uint8_t* pixels = (uint8_t*)fb;
        int cx = 200, cy = 200;
        int idx = (cy * width + cx) * 4;
        std::cout << "Center pixel RGB: " << (int)pixels[idx] << ", " 
                  << (int)pixels[idx+1] << ", " << (int)pixels[idx+2] << std::endl;
        
        // Check for non-clear pixels
        int non_clear_count = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 4;
                if (pixels[idx] != 128 || pixels[idx+1] != 128 || pixels[idx+2] != 128) {
                    non_clear_count++;
                    if (non_clear_count <= 5) {
                        std::cout << "Non-clear pixel at (" << x << "," << y << "): RGB=" 
                                  << (int)pixels[idx] << "," << (int)pixels[idx+1] << "," 
                                  << (int)pixels[idx+2] << std::endl;
                    }
                }
            }
        }
        std::cout << "Total non-clear pixels: " << non_clear_count << std::endl;
    }
    
    // Cleanup
    device->Release();
    d3d->Release();
    dx8gl_shutdown();
    
    return 0;
}