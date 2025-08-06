// Test HUD font texture loading and rendering
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>
#include <thread>
#include "../src/d3d8.h"
#include "../src/d3dx_compat.h"
#include "../src/dx8gl.h"
#include "../src/hud_system.h"

// Test configuration
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TEST_FRAMES = 300;  // Run for 300 frames

// Vertex structure for test geometry
struct TestVertex {
    float x, y, z;
    D3DCOLOR color;
};

#define D3DFVF_TESTVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

void create_test_scene(IDirect3DDevice8* device) {
    // Create a simple rotating triangle
    TestVertex vertices[] = {
        { -0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(255, 0, 0) },
        {  0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(0, 255, 0) },
        {  0.0f, -0.5f, 0.5f, D3DCOLOR_XRGB(0, 0, 255) }
    };
    
    device->SetVertexShader(D3DFVF_TESTVERTEX);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(TestVertex));
}

int main() {
    std::cout << "Testing HUD font texture loading and rendering..." << std::endl;
    
    // Initialize dx8gl with OSMesa backend
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return 1;
    }
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8 interface" << std::endl;
        dx8gl_shutdown();
        return 1;
    }
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = WINDOW_WIDTH;
    pp.BackBufferHeight = WINDOW_HEIGHT;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    // Create device
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
        std::cerr << "Failed to create Direct3D8 device" << std::endl;
        d3d8->Release();
        dx8gl_shutdown();
        return 1;
    }
    
    // Create and initialize HUD system
    dx8gl::HUD::Create(device);
    dx8gl::HUDSystem* hud = dx8gl::HUD::Get();
    
    if (!hud) {
        std::cerr << "Failed to create HUD system" << std::endl;
        device->Release();
        d3d8->Release();
        dx8gl_shutdown();
        return 1;
    }
    
    // Enable all HUD elements
    hud->SetFlags(dx8gl::HUD_SHOW_ALL);
    
    // Add custom debug information
    hud->SetDebugText("HUD Font Test v1.0");
    hud->AddDebugLine("Testing font texture loading");
    hud->AddDebugLine("Font can be loaded from:");
    hud->AddDebugLine("- assets/fonts/hud_font.tga");
    hud->AddDebugLine("- assets/fonts/hud_font.bmp");
    hud->AddDebugLine("- assets/fonts/hud_font.png");
    
    // Set custom controls
    std::vector<std::string> controls = {
        "ESC - Exit",
        "F1 - Toggle FPS",
        "F2 - Toggle Debug",
        "F3 - Toggle Controls",
        "F4 - Toggle Stats",
        "F5 - Load Custom Font"
    };
    hud->SetControlText(controls);
    
    // Set up basic rendering states
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Set up projection matrix
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 
                               (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 
                               0.1f, 100.0f);
    device->SetTransform(D3DTS_PROJECTION, &matProj);
    
    // Set up view matrix
    D3DXMATRIX matView;
    D3DXVECTOR3 eye(0.0f, 0.0f, -3.0f);
    D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &eye, &at, &up);
    device->SetTransform(D3DTS_VIEW, &matView);
    
    // Main rendering loop
    auto start_time = std::chrono::high_resolution_clock::now();
    float rotation = 0.0f;
    
    for (int frame = 0; frame < TEST_FRAMES; ++frame) {
        // Calculate delta time
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = current_time - start_time;
        float delta_time = 0.016f; // Assume 60 FPS for simplicity
        
        // Update HUD
        hud->Update(delta_time);
        
        // Update stats
        hud->SetStatValue("Frame", std::to_string(frame));
        hud->SetStatValue("Rotation", std::to_string(rotation));
        hud->SetStatValue("Font Loaded", hud->LoadFontTexture("assets/fonts/hud_font.tga") ? "Custom" : "Built-in");
        
        // Clear the screen
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
        
        // Begin scene
        device->BeginScene();
        
        // Set world transform with rotation
        D3DXMATRIX matWorld;
        D3DXMatrixRotationY(&matWorld, rotation);
        device->SetTransform(D3DTS_WORLD, &matWorld);
        rotation += 0.02f;
        
        // Draw test geometry
        create_test_scene(device);
        
        // Render HUD overlay
        hud->Render();
        
        // End scene
        device->EndScene();
        
        // Present
        device->Present(nullptr, nullptr, nullptr, nullptr);
        
        // Small delay to control frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Clean up
    dx8gl::HUD::Destroy();
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    std::cout << "HUD font test completed successfully!" << std::endl;
    return 0;
}