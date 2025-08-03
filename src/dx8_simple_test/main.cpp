#include <iostream>
#include <cstring>
#include "d3d8.h"
#include "dx8gl.h"

#define WIDTH 400
#define HEIGHT 400

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
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = WIDTH;
    pp.BackBufferHeight = HEIGHT;
    pp.EnableAutoDepthStencil = FALSE;
    
    // Create device
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
    
    // Simple triangle in screen space
    Vertex vertices[] = {
        { 200.0f, 100.0f, 0.0f, 0xFFFF0000 },  // Top center (red)
        { 100.0f, 300.0f, 0.0f, 0xFF00FF00 },  // Bottom left (green)
        { 300.0f, 300.0f, 0.0f, 0xFF0000FF }   // Bottom right (blue)
    };
    
    // Set render states
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Use screen space coordinates (no transform)
    D3DMATRIX identity;
    memset(&identity, 0, sizeof(identity));
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    device->SetTransform(D3DTS_WORLD, &identity);
    device->SetTransform(D3DTS_VIEW, &identity);
    
    // Orthographic projection for pixel coordinates
    D3DMATRIX proj;
    memset(&proj, 0, sizeof(proj));
    proj._11 = 2.0f / WIDTH;
    proj._22 = -2.0f / HEIGHT;  // Flip Y
    proj._33 = 1.0f;
    proj._41 = -1.0f;
    proj._42 = 1.0f;
    proj._44 = 1.0f;
    device->SetTransform(D3DTS_PROJECTION, &proj);
    
    // Clear
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    // Draw
    if (SUCCEEDED(device->BeginScene())) {
        device->SetVertexShader(D3DFVF_VERTEX);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        device->EndScene();
    }
    
    // Present
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get framebuffer and save PPM
    int fb_width, fb_height, frame_number;
    bool updated;
    void* framebuffer = dx8gl_get_shared_framebuffer(&fb_width, &fb_height, &frame_number, &updated);
    
    if (framebuffer) {
        std::cout << "Got framebuffer: " << fb_width << "x" << fb_height << std::endl;
        
        FILE* fp = fopen("dx8_simple_test.ppm", "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", fb_width, fb_height);
            uint8_t* src = (uint8_t*)framebuffer;
            
            int non_bg_count = 0;
            for (int y = 0; y < fb_height; y++) {
                for (int x = 0; x < fb_width; x++) {
                    int idx = (y * fb_width + x) * 4;
                    uint8_t r = src[idx + 0];
                    uint8_t g = src[idx + 1];
                    uint8_t b = src[idx + 2];
                    
                    // Count non-background pixels
                    if (r != 64 || g != 64 || b != 128) {
                        non_bg_count++;
                    }
                    
                    fwrite(&r, 1, 1, fp);
                    fwrite(&g, 1, 1, fp);
                    fwrite(&b, 1, 1, fp);
                }
            }
            fclose(fp);
            std::cout << "Saved dx8_simple_test.ppm" << std::endl;
            std::cout << "Non-background pixels: " << non_bg_count << std::endl;
        }
    }
    
    // Cleanup
    device->Release();
    d3d->Release();
    dx8gl_shutdown();
    
    return 0;
}