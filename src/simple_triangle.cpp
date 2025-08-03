#include <iostream>
#include <cstring>
#include "d3d8.h"
#include "dx8gl.h"

struct Vertex {
    float x, y, z;
    DWORD color;
};

#define D3DFVF_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

int main() {
    // Initialize dx8gl
    if (dx8gl_init(nullptr) != 0) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return 1;
    }
    
    // Create Direct3D8
    IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return 1;
    }
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 400;
    pp.BackBufferHeight = 400;
    
    IDirect3DDevice8* device;
    HRESULT hr = d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create device" << std::endl;
        d3d->Release();
        return 1;
    }
    
    // Create a simple triangle
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.5f, 0xFFFF0000 }, // Top, red
        {-0.5f, -0.5f, 0.5f, 0xFF00FF00 }, // Bottom left, green  
        { 0.5f, -0.5f, 0.5f, 0xFF0000FF }, // Bottom right, blue
    };
    
    // Render one frame
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    if (SUCCEEDED(device->BeginScene())) {
        device->SetVertexShader(D3DFVF_VERTEX);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        device->EndScene();
    }
    
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get framebuffer and check if triangle was rendered
    int width, height, frame;
    bool updated;
    void* fb = dx8gl_get_shared_framebuffer(&width, &height, &frame, &updated);
    
    if (fb) {
        std::cout << "Framebuffer: " << width << "x" << height << std::endl;
        
        // Check a few pixels
        uint8_t* pixels = (uint8_t*)fb;
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                int idx = (y * width + x) * 4;
                std::cout << "Pixel(" << x << "," << y << "): " 
                          << (int)pixels[idx] << "," 
                          << (int)pixels[idx+1] << ","
                          << (int)pixels[idx+2] << std::endl;
            }
        }
        
        // Check center pixel (should be part of triangle)
        int cx = 200, cy = 200;
        int idx = (cy * width + cx) * 4;
        std::cout << "Center pixel: " 
                  << (int)pixels[idx] << "," 
                  << (int)pixels[idx+1] << ","
                  << (int)pixels[idx+2] << std::endl;
                  
        // Write the framebuffer to PPM file
        FILE* fp = fopen("dx8gl_test.ppm", "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", width, height);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;
                    uint8_t r = pixels[idx];
                    uint8_t g = pixels[idx+1];
                    uint8_t b = pixels[idx+2];
                    fwrite(&r, 1, 1, fp);
                    fwrite(&g, 1, 1, fp);
                    fwrite(&b, 1, 1, fp);
                }
            }
            fclose(fp);
            std::cout << "Wrote framebuffer to dx8gl_test.ppm" << std::endl;
        }
    }
    
    // Cleanup
    device->Release();
    d3d->Release();
    dx8gl_shutdown();
    
    return 0;
}