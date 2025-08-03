#include "../src/d3d8.h"
#include "../src/dx8gl.h"
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

IDirect3D8* g_pD3D = nullptr;
IDirect3DDevice8* g_pDevice = nullptr;
bool running = true;
int frame_count = 0;

void main_loop() {
    if (!g_pDevice) return;
    
    if (frame_count < 10) {
        printf("Frame %d: Clearing to blue and presenting...\n", frame_count);
        fflush(stdout);
        
        // Clear to blue
        g_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);
        
        // Present
        g_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
        frame_count++;
    } else if (frame_count == 10) {
        printf("Stopping after 10 frames for debugging\n");
        fflush(stdout);
        frame_count++;
#ifndef __EMSCRIPTEN__
        running = false;
#endif
    }
}

int main(int argc, char* argv[]) {
    printf("Starting minimal dx8gl test...\n");
    
    // Create Direct3D object
    g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_pD3D) {
        printf("ERROR: Direct3DCreate8 failed\n");
        return -1;
    }
    printf("Direct3D8 created\n");
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;
    
    // Create device
    printf("Creating device...\n");
    HRESULT hr = g_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_pDevice
    );
    
    if (FAILED(hr)) {
        printf("ERROR: CreateDevice failed with hr=0x%08X\n", hr);
        g_pD3D->Release();
        return -1;
    }
    printf("Device created successfully\n");
    
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (running) {
        main_loop();
    }
#endif
    
    // Cleanup
    if (g_pDevice) {
        g_pDevice->Release();
    }
    if (g_pD3D) {
        g_pD3D->Release();
    }
    
    return 0;
}