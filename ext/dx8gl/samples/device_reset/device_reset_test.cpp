// Device Reset Test for dx8gl
// Demonstrates proper handling of device reset and resource recreation

#include <iostream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <Windows.h>
#include <d3d8.h>
#include <d3dx8.h>
#include "dx8gl.h"

// Window dimensions
static int g_WindowWidth = 800;
static int g_WindowHeight = 600;
static bool g_NeedReset = false;
static bool g_Running = true;

// D3D objects
static IDirect3D8* g_pD3D = nullptr;
static IDirect3DDevice8* g_pDevice = nullptr;
static D3DPRESENT_PARAMETERS g_PresentParams;

// Resources
static IDirect3DTexture8* g_pTextureDefault = nullptr;    // D3DPOOL_DEFAULT - needs recreation
static IDirect3DTexture8* g_pTextureManaged = nullptr;    // D3DPOOL_MANAGED - preserved
static IDirect3DVertexBuffer8* g_pVertexBuffer = nullptr;

struct CUSTOMVERTEX {
    float x, y, z, rhw;
    DWORD color;
    float u, v;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Create a simple texture
bool CreateTexture(IDirect3DTexture8** ppTexture, D3DPOOL pool, DWORD color) {
    HRESULT hr = g_pDevice->CreateTexture(256, 256, 1, 0, D3DFMT_X8R8G8B8, pool, ppTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create texture: " << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // Fill texture with color
    D3DLOCKED_RECT lr;
    (*ppTexture)->LockRect(0, &lr, nullptr, 0);
    for (int y = 0; y < 256; y++) {
        DWORD* row = (DWORD*)((BYTE*)lr.pBits + y * lr.Pitch);
        for (int x = 0; x < 256; x++) {
            // Create a pattern
            if ((x / 32 + y / 32) % 2 == 0) {
                row[x] = color;
            } else {
                row[x] = 0xFF000000;  // Black
            }
        }
    }
    (*ppTexture)->UnlockRect(0);
    
    return true;
}

// Create vertex buffer
bool CreateVertexBuffer() {
    HRESULT hr = g_pDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 
                                               D3DUSAGE_WRITEONLY,
                                               D3DFVF_CUSTOMVERTEX,
                                               D3DPOOL_MANAGED,
                                               &g_pVertexBuffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex buffer: " << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // Fill vertex buffer
    CUSTOMVERTEX* vertices;
    g_pVertexBuffer->Lock(0, 0, (BYTE**)&vertices, 0);
    
    // Left quad (uses DEFAULT texture)
    vertices[0] = { 50.0f, 50.0f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f };
    vertices[1] = { 350.0f, 50.0f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f };
    vertices[2] = { 50.0f, 350.0f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f };
    vertices[3] = { 350.0f, 350.0f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f };
    
    g_pVertexBuffer->Unlock();
    
    return true;
}

// Initialize D3D
bool InitD3D(HWND hWnd) {
    std::cout << "Initializing Direct3D..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_init(nullptr);
    
    // Create D3D object
    g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_pD3D) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return false;
    }
    
    // Setup present parameters
    ZeroMemory(&g_PresentParams, sizeof(g_PresentParams));
    g_PresentParams.Windowed = TRUE;
    g_PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_PresentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
    g_PresentParams.BackBufferWidth = g_WindowWidth;
    g_PresentParams.BackBufferHeight = g_WindowHeight;
    g_PresentParams.EnableAutoDepthStencil = TRUE;
    g_PresentParams.AutoDepthStencilFormat = D3DFMT_D16;
    
    // Create device
    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                      D3DDEVTYPE_HAL,
                                      hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &g_PresentParams,
                                      &g_pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to create device: " << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    std::cout << "Device created successfully" << std::endl;
    
    // Create resources
    if (!CreateTexture(&g_pTextureDefault, D3DPOOL_DEFAULT, 0xFFFF0000)) {  // Red
        std::cerr << "Failed to create DEFAULT texture" << std::endl;
        return false;
    }
    std::cout << "Created texture in D3DPOOL_DEFAULT (will be lost on reset)" << std::endl;
    
    if (!CreateTexture(&g_pTextureManaged, D3DPOOL_MANAGED, 0xFF00FF00)) {  // Green
        std::cerr << "Failed to create MANAGED texture" << std::endl;
        return false;
    }
    std::cout << "Created texture in D3DPOOL_MANAGED (will survive reset)" << std::endl;
    
    if (!CreateVertexBuffer()) {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }
    
    // Set default render states
    g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    return true;
}

// Clean up D3D
void CleanupD3D() {
    if (g_pVertexBuffer) {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
    if (g_pTextureDefault) {
        g_pTextureDefault->Release();
        g_pTextureDefault = nullptr;
    }
    if (g_pTextureManaged) {
        g_pTextureManaged->Release();
        g_pTextureManaged = nullptr;
    }
    if (g_pDevice) {
        g_pDevice->Release();
        g_pDevice = nullptr;
    }
    if (g_pD3D) {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
}

// Reset device
bool ResetDevice() {
    std::cout << "\n--- DEVICE RESET ---" << std::endl;
    std::cout << "Releasing resources in D3DPOOL_DEFAULT..." << std::endl;
    
    // Release resources in D3DPOOL_DEFAULT
    if (g_pTextureDefault) {
        g_pTextureDefault->Release();
        g_pTextureDefault = nullptr;
    }
    
    // Update present parameters with new size
    g_PresentParams.BackBufferWidth = g_WindowWidth;
    g_PresentParams.BackBufferHeight = g_WindowHeight;
    
    std::cout << "Calling Device->Reset() with new size: " << g_WindowWidth << "x" << g_WindowHeight << std::endl;
    
    // Reset the device
    HRESULT hr = g_pDevice->Reset(&g_PresentParams);
    if (FAILED(hr)) {
        std::cerr << "Failed to reset device: " << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    std::cout << "Device reset successful!" << std::endl;
    
    // Recreate resources in D3DPOOL_DEFAULT
    std::cout << "Recreating resources in D3DPOOL_DEFAULT..." << std::endl;
    if (!CreateTexture(&g_pTextureDefault, D3DPOOL_DEFAULT, 0xFF0000FF)) {  // Blue (different color)
        std::cerr << "Failed to recreate DEFAULT texture" << std::endl;
        return false;
    }
    std::cout << "Recreated texture with new color (blue)" << std::endl;
    
    // Restore render states (they were reset to defaults)
    std::cout << "Restoring render states..." << std::endl;
    g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    std::cout << "--- RESET COMPLETE ---\n" << std::endl;
    
    return true;
}

// Render frame
void Render() {
    // Test cooperative level
    HRESULT hr = g_pDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICELOST) {
        // Device is lost, can't render
        return;
    } else if (hr == D3DERR_DEVICENOTRESET) {
        // Device is ready to be reset
        if (!ResetDevice()) {
            g_Running = false;
            return;
        }
    }
    
    // Clear
    g_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF404040, 1.0f, 0);
    
    // Begin scene
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        // Set vertex format
        g_pDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
        g_pDevice->SetStreamSource(0, g_pVertexBuffer, sizeof(CUSTOMVERTEX));
        
        // Draw left quad with DEFAULT texture
        g_pDevice->SetTexture(0, g_pTextureDefault);
        g_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        
        // Draw right quad with MANAGED texture
        CUSTOMVERTEX* vertices;
        g_pVertexBuffer->Lock(0, 0, (BYTE**)&vertices, 0);
        vertices[0].x = 450.0f;
        vertices[1].x = 750.0f;
        vertices[2].x = 450.0f;
        vertices[3].x = 750.0f;
        g_pVertexBuffer->Unlock();
        
        g_pDevice->SetTexture(0, g_pTextureManaged);
        g_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        
        g_pDevice->EndScene();
    }
    
    // Present
    g_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                g_WindowWidth = LOWORD(lParam);
                g_WindowHeight = HIWORD(lParam);
                g_NeedReset = true;
            }
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_SPACE) {
                // Manual reset on spacebar
                std::cout << "Manual reset requested (spacebar)" << std::endl;
                g_NeedReset = true;
            } else if (wParam == VK_ESCAPE) {
                g_Running = false;
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main() {
    std::cout << "Device Reset Test for dx8gl" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Resize window to trigger device reset" << std::endl;
    std::cout << "  - Press SPACE to manually trigger reset" << std::endl;
    std::cout << "  - Press ESC to exit" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "DeviceResetTest";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);
    
    // Create window
    HWND hWnd = CreateWindow("DeviceResetTest", "Device Reset Test - dx8gl",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           g_WindowWidth, g_WindowHeight,
                           nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    // Initialize D3D
    if (!InitD3D(hWnd)) {
        std::cerr << "Failed to initialize Direct3D" << std::endl;
        return 1;
    }
    
    // Main loop
    MSG msg = {};
    while (g_Running && msg.message != WM_QUIT) {
        // Handle messages
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Check if reset is needed
            if (g_NeedReset) {
                g_NeedReset = false;
                ResetDevice();
            }
            
            // Render
            Render();
            
            // Small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    // Cleanup
    CleanupD3D();
    
    return 0;
}