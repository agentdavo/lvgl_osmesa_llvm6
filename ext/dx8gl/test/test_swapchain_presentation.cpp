#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_swapchain.h"
#include "../src/logger.h"

using namespace std;

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            cout << "[PASS] " << message << endl; \
        } else { \
            cout << "[FAIL] " << message << endl; \
            return false; \
        } \
    } while(0)

bool test_swapchain_creation() {
    cout << "\n=== Test: Swap Chain Creation ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Create device with multiple back buffers
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 2;  // Double buffering
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr), "Device creation should succeed");
    TEST_ASSERT(device != nullptr, "Device should not be null");
    
    // The implicit swap chain should be created automatically
    // We can't directly access it, but we can test through device methods
    
    // Get back buffers to verify they exist
    IDirect3DSurface8* backBuffer0 = nullptr;
    IDirect3DSurface8* backBuffer1 = nullptr;
    
    hr = device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer0);
    TEST_ASSERT(SUCCEEDED(hr), "GetBackBuffer(0) should succeed");
    TEST_ASSERT(backBuffer0 != nullptr, "Back buffer 0 should not be null");
    
    hr = device->GetBackBuffer(1, D3DBACKBUFFER_TYPE_MONO, &backBuffer1);
    TEST_ASSERT(SUCCEEDED(hr), "GetBackBuffer(1) should succeed");
    TEST_ASSERT(backBuffer1 != nullptr, "Back buffer 1 should not be null");
    
    // Verify they are different surfaces
    TEST_ASSERT(backBuffer0 != backBuffer1, "Back buffers should be different surfaces");
    
    // Clean up
    backBuffer0->Release();
    backBuffer1->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_swapchain_presentation() {
    cout << "\n=== Test: Swap Chain Presentation ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Create device with multiple back buffers
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 320;
    pp.BackBufferHeight = 240;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 3;  // Triple buffering
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr), "Device creation should succeed");
    
    // Test multiple presentations
    for (int frame = 0; frame < 5; frame++) {
        cout << "  Presenting frame " << (frame + 1) << endl;
        
        // Begin scene
        hr = device->BeginScene();
        TEST_ASSERT(SUCCEEDED(hr), "BeginScene should succeed");
        
        // Clear with different colors for each frame
        DWORD clear_color = 0xFF000000 | (frame * 0x003F3F3F);
        hr = device->Clear(0, nullptr, D3DCLEAR_TARGET, clear_color, 1.0f, 0);
        TEST_ASSERT(SUCCEEDED(hr), "Clear should succeed");
        
        // End scene
        hr = device->EndScene();
        TEST_ASSERT(SUCCEEDED(hr), "EndScene should succeed");
        
        // Present the frame
        hr = device->Present(nullptr, nullptr, nullptr, nullptr);
        TEST_ASSERT(SUCCEEDED(hr), "Present should succeed");
        
        // Small delay to simulate frame timing
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Clean up
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_swapchain_render_target_sync() {
    cout << "\n=== Test: Swap Chain Render Target Synchronization ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 256;
    pp.BackBufferHeight = 256;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount = 2;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr), "Device creation should succeed");
    
    // Get initial render target
    IDirect3DSurface8* initial_rt = nullptr;
    hr = device->GetRenderTarget(&initial_rt);
    TEST_ASSERT(SUCCEEDED(hr), "GetRenderTarget should succeed");
    TEST_ASSERT(initial_rt != nullptr, "Initial render target should not be null");
    
    // Create a custom render target surface
    IDirect3DSurface8* custom_rt = nullptr;
    hr = device->CreateRenderTarget(256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, TRUE, &custom_rt);
    TEST_ASSERT(SUCCEEDED(hr), "CreateRenderTarget should succeed");
    
    // Set custom render target
    hr = device->SetRenderTarget(custom_rt, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "SetRenderTarget should succeed");
    
    // Render to custom target
    hr = device->BeginScene();
    TEST_ASSERT(SUCCEEDED(hr), "BeginScene should succeed");
    
    hr = device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFFFF0000, 1.0f, 0);  // Red
    TEST_ASSERT(SUCCEEDED(hr), "Clear should succeed");
    
    hr = device->EndScene();
    TEST_ASSERT(SUCCEEDED(hr), "EndScene should succeed");
    
    // Present - this should handle synchronization between custom RT and back buffer
    hr = device->Present(nullptr, nullptr, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Present with custom render target should succeed");
    
    // Restore original render target
    hr = device->SetRenderTarget(initial_rt, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Restoring render target should succeed");
    
    // Present again to test buffer flipping
    hr = device->Present(nullptr, nullptr, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Second present should succeed");
    
    // Clean up
    custom_rt->Release();
    initial_rt->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_partial_presentation() {
    cout << "\n=== Test: Partial Presentation with Rectangles ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 400;
    pp.BackBufferHeight = 300;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr), "Device creation should succeed");
    
    // Render something
    hr = device->BeginScene();
    TEST_ASSERT(SUCCEEDED(hr), "BeginScene should succeed");
    
    hr = device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF00FF00, 1.0f, 0);  // Green
    TEST_ASSERT(SUCCEEDED(hr), "Clear should succeed");
    
    hr = device->EndScene();
    TEST_ASSERT(SUCCEEDED(hr), "EndScene should succeed");
    
    // Test partial presentation with source rectangle
    RECT srcRect = { 100, 75, 300, 225 };  // 200x150 region
    hr = device->Present(&srcRect, nullptr, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Present with source rect should succeed");
    
    // Test partial presentation with destination rectangle
    RECT destRect = { 50, 50, 350, 250 };  // Different position
    hr = device->Present(nullptr, &destRect, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Present with dest rect should succeed");
    
    // Test with both source and destination rectangles
    hr = device->Present(&srcRect, &destRect, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Present with both rects should succeed");
    
    // Clean up
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool run_all_tests() {
    cout << "Running Swap Chain Presentation Tests" << endl;
    cout << "=====================================" << endl;
    
    bool all_passed = true;
    
    all_passed &= test_swapchain_creation();
    all_passed &= test_swapchain_presentation();
    all_passed &= test_swapchain_render_target_sync();
    all_passed &= test_partial_presentation();
    
    return all_passed;
}

int main() {
    bool success = run_all_tests();
    
    cout << "\n=====================================" << endl;
    cout << "Test Results: " << tests_passed << "/" << tests_run << " passed" << endl;
    
    if (success && tests_passed == tests_run) {
        cout << "All tests PASSED!" << endl;
        return 0;
    } else {
        cout << "Some tests FAILED!" << endl;
        return 1;
    }
}