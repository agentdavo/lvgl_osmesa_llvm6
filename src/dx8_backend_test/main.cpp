#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

#include "dx8gl.h"
#include "d3d8.h"
#include "d3dx8.h"

// Test configuration
struct TestConfig {
    const char* backend_name;
    dx8gl_backend_type backend_type;
    const char* output_file;
};

// Simple vertex structure
struct CUSTOMVERTEX {
    float x, y, z;
    DWORD color;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

// Function to save framebuffer as PPM
bool save_framebuffer_ppm(void* data, int width, int height, const char* filename) {
    if (!data || width <= 0 || height <= 0 || !filename) {
        return false;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write PPM header
    file << "P6\n" << width << " " << height << "\n255\n";
    
    // Convert RGBA to RGB and flip Y axis
    uint8_t* pixels = static_cast<uint8_t*>(data);
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            file.put(pixels[idx]);     // R
            file.put(pixels[idx + 1]); // G
            file.put(pixels[idx + 2]); // B
            // Skip alpha
        }
    }
    
    return true;
}

// Function to compare two framebuffers
bool compare_framebuffers(void* fb1, void* fb2, int width, int height, int tolerance = 5) {
    if (!fb1 || !fb2) return false;
    
    uint8_t* pixels1 = static_cast<uint8_t*>(fb1);
    uint8_t* pixels2 = static_cast<uint8_t*>(fb2);
    
    int differences = 0;
    int total_pixels = width * height;
    
    for (int i = 0; i < total_pixels * 4; i += 4) {
        // Compare RGB channels (skip alpha)
        int dr = abs(pixels1[i] - pixels2[i]);
        int dg = abs(pixels1[i + 1] - pixels2[i + 1]);
        int db = abs(pixels1[i + 2] - pixels2[i + 2]);
        
        if (dr > tolerance || dg > tolerance || db > tolerance) {
            differences++;
        }
    }
    
    float diff_percentage = (float)differences / total_pixels * 100.0f;
    std::cout << "  Pixel differences: " << differences << " / " << total_pixels 
              << " (" << diff_percentage << "%)" << std::endl;
    
    // Allow up to 1% difference
    return diff_percentage < 1.0f;
}

// Function to run test with specific backend
bool run_backend_test(const TestConfig& config) {
    std::cout << "\n=== Testing " << config.backend_name << " Backend ===" << std::endl;
    
    // Configure dx8gl with the specified backend
    dx8gl_config dx_config = {};
    dx_config.backend_type = config.backend_type;
    dx_config.enable_logging = true;
    
    if (dx8gl_init(&dx_config) != DX8GL_SUCCESS) {
        std::cerr << "Failed to initialize dx8gl with " << config.backend_name << std::endl;
        return false;
    }
    
    // Create Direct3D8 interface
    IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d) {
        std::cerr << "Failed to create Direct3D8 interface" << std::endl;
        dx8gl_shutdown();
        return false;
    }
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 320;
    pp.BackBufferHeight = 240;
    
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
        std::cerr << "Failed to create Direct3D8 device" << std::endl;
        d3d->Release();
        dx8gl_shutdown();
        return false;
    }
    
    // Clear to a specific color
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                  D3DCOLOR_XRGB(64, 128, 192), 1.0f, 0);
    
    // Set up basic rendering states
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Create a simple triangle
    CUSTOMVERTEX vertices[] = {
        { -0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 0, 0) },  // Red
        {  0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(0, 255, 0) },  // Green
        {  0.0f,  0.5f, 0.5f, D3DCOLOR_XRGB(0, 0, 255) },  // Blue
    };
    
    // Set vertex format
    device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    
    // Begin scene
    device->BeginScene();
    
    // Draw the triangle
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(CUSTOMVERTEX));
    
    // End scene
    device->EndScene();
    
    // Present
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get framebuffer for comparison
    int width, height;
    void* framebuffer = dx8gl_get_framebuffer(device, &width, &height);
    
    bool success = false;
    if (framebuffer) {
        std::cout << "  Framebuffer size: " << width << "x" << height << std::endl;
        
        // Save framebuffer
        if (save_framebuffer_ppm(framebuffer, width, height, config.output_file)) {
            std::cout << "  Saved output to: " << config.output_file << std::endl;
            success = true;
        } else {
            std::cerr << "  Failed to save framebuffer" << std::endl;
        }
        
        // Store framebuffer data for comparison
        if (success) {
            size_t fb_size = width * height * 4;
            void* fb_copy = malloc(fb_size);
            if (fb_copy) {
                memcpy(fb_copy, framebuffer, fb_size);
                // Store for later comparison (in real test, would compare between backends)
                free(fb_copy);
            }
        }
    } else {
        std::cerr << "  Failed to get framebuffer" << std::endl;
    }
    
    // Cleanup
    device->Release();
    d3d->Release();
    dx8gl_shutdown();
    
    return success;
}

int main(int argc, char* argv[]) {
    std::cout << "=== DirectX 8 Backend Regression Test ===" << std::endl;
    
    // Test configurations
    TestConfig configs[] = {
        { "OSMesa", DX8GL_BACKEND_OSMESA, "backend_test_osmesa.ppm" },
        { "EGL", DX8GL_BACKEND_EGL, "backend_test_egl.ppm" }
    };
    
    int passed = 0;
    int failed = 0;
    
    // Run tests
    for (const auto& config : configs) {
        if (run_backend_test(config)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    // Compare outputs if both succeeded
    if (passed == 2) {
        std::cout << "\n=== Comparing Backend Outputs ===" << std::endl;
        
        // Load and compare PPM files
        // In a real test, we would properly load and compare the PPM files
        // For now, just note that comparison would happen here
        std::cout << "  Would compare backend_test_osmesa.ppm vs backend_test_egl.ppm" << std::endl;
        std::cout << "  (Full comparison implementation pending)" << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return failed > 0 ? 1 : 0;
}