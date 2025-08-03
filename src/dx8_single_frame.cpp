#include <iostream>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "d3d8.h"
#include "dx8gl.h"

// Simple triangle vertices
struct Vertex {
    float x, y, z;
    DWORD color;
};

#define MY_D3DFVF_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

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
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
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
    
    // Set render states
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Set up transforms using GLM
    // World matrix - identity
    glm::mat4 world = glm::mat4(1.0f);
    D3DMATRIX d3dWorld;
    const float* worldPtr = glm::value_ptr(world);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            (&d3dWorld._11)[i*4+j] = worldPtr[j*4+i]; // Transpose from column-major to row-major
        }
    }
    device->SetTransform(D3DTS_WORLD, &d3dWorld);
    
    // View matrix - camera looking at origin
    glm::vec3 eye(0.0f, 0.0f, -3.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, center, up);
    D3DMATRIX d3dView;
    const float* viewPtr = glm::value_ptr(view);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            (&d3dView._11)[i*4+j] = viewPtr[j*4+i]; // Transpose
        }
    }
    device->SetTransform(D3DTS_VIEW, &d3dView);
    
    // Projection matrix
    float fov = glm::radians(45.0f);
    float aspect = 1.0f; // 400x400
    float znear = 0.1f;
    float zfar = 100.0f;
    glm::mat4 proj = glm::perspective(fov, aspect, znear, zfar);
    D3DMATRIX d3dProj;
    const float* projPtr = glm::value_ptr(proj);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            (&d3dProj._11)[i*4+j] = projPtr[j*4+i]; // Transpose
        }
    }
    device->SetTransform(D3DTS_PROJECTION, &d3dProj);
    
    // Set viewport
    D3DVIEWPORT8 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = 400;
    viewport.Height = 400;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    device->SetViewport(&viewport);
    
    // Create a triangle
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 0xFFFF0000 }, // Top, red
        {-0.5f, -0.5f, 0.0f, 0xFF00FF00 }, // Bottom left, green  
        { 0.5f, -0.5f, 0.0f, 0xFF0000FF }, // Bottom right, blue
    };
    
    std::cout << "Clearing to dark blue..." << std::endl;
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    std::cout << "Beginning scene..." << std::endl;
    if (SUCCEEDED(device->BeginScene())) {
        std::cout << "Setting vertex shader..." << std::endl;
        device->SetVertexShader(MY_D3DFVF_VERTEX);
        
        std::cout << "Drawing triangle..." << std::endl;
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        
        std::cout << "Ending scene..." << std::endl;
        device->EndScene();
    }
    
    std::cout << "Presenting..." << std::endl;
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get framebuffer and save it
    int width, height, frame;
    bool updated;
    void* fb = dx8gl_get_shared_framebuffer(&width, &height, &frame, &updated);
    
    if (fb) {
        std::cout << "Framebuffer: " << width << "x" << height << ", updated=" << updated << std::endl;
        
        // Save as PPM
        FILE* fp = fopen("dx8_single_frame.ppm", "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", width, height);
            uint8_t* pixels = (uint8_t*)fb;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;
                    uint8_t r = pixels[idx];
                    uint8_t g = pixels[idx + 1];
                    uint8_t b = pixels[idx + 2];
                    fwrite(&r, 1, 1, fp);
                    fwrite(&g, 1, 1, fp);
                    fwrite(&b, 1, 1, fp);
                }
            }
            fclose(fp);
            std::cout << "Saved dx8_single_frame.ppm" << std::endl;
        }
        
        // Check several pixels
        uint8_t* pixels = (uint8_t*)fb;
        for (int y = 0; y < height; y += 50) {
            for (int x = 0; x < width; x += 50) {
                int idx = (y * width + x) * 4;
                uint8_t r = pixels[idx];
                uint8_t g = pixels[idx+1];
                uint8_t b = pixels[idx+2];
                if (r != 64 || g != 64 || b != 128) {
                    std::cout << "Non-clear pixel at (" << x << "," << y << "): RGB=" 
                              << (int)r << "," << (int)g << "," << (int)b << std::endl;
                }
            }
        }
        
        // Also check center pixel and surrounding area
        int cx = 200, cy = 200;
        int idx = (cy * width + cx) * 4;
        std::cout << "Center pixel RGB: " << (int)pixels[idx] << ", " 
                  << (int)pixels[idx+1] << ", " << (int)pixels[idx+2] << std::endl;
        
        // Check pixels where triangle should be
        std::cout << "Checking triangle area pixels:" << std::endl;
        // Top vertex area (200, 100)
        idx = (100 * width + 200) * 4;
        std::cout << "  Top (200,100): RGB=" << (int)pixels[idx] << "," 
                  << (int)pixels[idx+1] << "," << (int)pixels[idx+2] << std::endl;
        // Bottom left area (100, 300)
        idx = (300 * width + 100) * 4;
        std::cout << "  Bottom-left (100,300): RGB=" << (int)pixels[idx] << "," 
                  << (int)pixels[idx+1] << "," << (int)pixels[idx+2] << std::endl;
        // Bottom right area (300, 300)
        idx = (300 * width + 300) * 4;
        std::cout << "  Bottom-right (300,300): RGB=" << (int)pixels[idx] << "," 
                  << (int)pixels[idx+1] << "," << (int)pixels[idx+2] << std::endl;
    }
    
    // Cleanup
    device->Release();
    d3d->Release();
    dx8gl_shutdown();
    
    return 0;
}