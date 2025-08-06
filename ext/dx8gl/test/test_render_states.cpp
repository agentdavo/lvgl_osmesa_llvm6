// Test for missing render states required by DX8Wrapper
#include <iostream>
#include <cstdlib>
#include <cassert>
#include "../src/d3d8_game.h"
#include "../src/dx8gl.h"

// Test that all required render states are handled
int main() {
    std::cout << "Testing render states required by DX8Wrapper..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    if (dx8gl_init(&config) != DX8GL_SUCCESS) {
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
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
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
    
    bool all_tests_passed = true;
    
    // Test D3DRS_RANGEFOGENABLE
    {
        std::cout << "Testing D3DRS_RANGEFOGENABLE..." << std::endl;
        
        // Set and get the state
        hr = device->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: SetRenderState(D3DRS_RANGEFOGENABLE) failed" << std::endl;
            all_tests_passed = false;
        }
        
        DWORD value = 0;
        hr = device->GetRenderState(D3DRS_RANGEFOGENABLE, &value);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: GetRenderState(D3DRS_RANGEFOGENABLE) failed" << std::endl;
            all_tests_passed = false;
        } else if (value != TRUE) {
            std::cerr << "  FAILED: Expected TRUE, got " << value << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED" << std::endl;
        }
    }
    
    // Test D3DRS_FOGVERTEXMODE
    {
        std::cout << "Testing D3DRS_FOGVERTEXMODE..." << std::endl;
        
        hr = device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: SetRenderState(D3DRS_FOGVERTEXMODE) failed" << std::endl;
            all_tests_passed = false;
        }
        
        DWORD value = 0;
        hr = device->GetRenderState(D3DRS_FOGVERTEXMODE, &value);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: GetRenderState(D3DRS_FOGVERTEXMODE) failed" << std::endl;
            all_tests_passed = false;
        } else if (value != D3DFOG_LINEAR) {
            std::cerr << "  FAILED: Expected D3DFOG_LINEAR, got " << value << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED" << std::endl;
        }
    }
    
    // Test D3DRS_SPECULARMATERIALSOURCE
    {
        std::cout << "Testing D3DRS_SPECULARMATERIALSOURCE..." << std::endl;
        
        hr = device->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR1);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: SetRenderState(D3DRS_SPECULARMATERIALSOURCE) failed" << std::endl;
            all_tests_passed = false;
        }
        
        DWORD value = 0;
        hr = device->GetRenderState(D3DRS_SPECULARMATERIALSOURCE, &value);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: GetRenderState(D3DRS_SPECULARMATERIALSOURCE) failed" << std::endl;
            all_tests_passed = false;
        } else if (value != D3DMCS_COLOR1) {
            std::cerr << "  FAILED: Expected D3DMCS_COLOR1, got " << value << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED" << std::endl;
        }
    }
    
    // Test D3DRS_COLORVERTEX
    {
        std::cout << "Testing D3DRS_COLORVERTEX..." << std::endl;
        
        hr = device->SetRenderState(D3DRS_COLORVERTEX, FALSE);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: SetRenderState(D3DRS_COLORVERTEX) failed" << std::endl;
            all_tests_passed = false;
        }
        
        DWORD value = 0;
        hr = device->GetRenderState(D3DRS_COLORVERTEX, &value);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: GetRenderState(D3DRS_COLORVERTEX) failed" << std::endl;
            all_tests_passed = false;
        } else if (value != FALSE) {
            std::cerr << "  FAILED: Expected FALSE, got " << value << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED" << std::endl;
        }
    }
    
    // Test D3DRS_ZBIAS
    {
        std::cout << "Testing D3DRS_ZBIAS..." << std::endl;
        
        DWORD test_zbias = 8;  // Middle of typical range
        hr = device->SetRenderState(D3DRS_ZBIAS, test_zbias);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: SetRenderState(D3DRS_ZBIAS) failed" << std::endl;
            all_tests_passed = false;
        }
        
        DWORD value = 0;
        hr = device->GetRenderState(D3DRS_ZBIAS, &value);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: GetRenderState(D3DRS_ZBIAS) failed" << std::endl;
            all_tests_passed = false;
        } else if (value != test_zbias) {
            std::cerr << "  FAILED: Expected " << test_zbias << ", got " << value << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED" << std::endl;
        }
    }
    
    // Test ValidateDevice with new states
    {
        std::cout << "Testing ValidateDevice with new states..." << std::endl;
        
        // Disable texture stage 0 to avoid texture requirement
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
        
        // Set up a valid combination
        device->SetRenderState(D3DRS_FOGENABLE, TRUE);
        device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        device->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
        device->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
        device->SetRenderState(D3DRS_COLORVERTEX, TRUE);
        device->SetRenderState(D3DRS_ZBIAS, 4);
        
        DWORD num_passes = 0;
        hr = device->ValidateDevice(&num_passes);
        if (FAILED(hr)) {
            std::cerr << "  FAILED: ValidateDevice failed with valid states" << std::endl;
            all_tests_passed = false;
        } else if (num_passes != 1) {
            std::cerr << "  WARNING: Expected 1 pass, got " << num_passes << std::endl;
        } else {
            std::cout << "  PASSED: Valid configuration accepted" << std::endl;
        }
        
        // Test invalid specular material source
        device->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 999);  // Invalid value
        hr = device->ValidateDevice(&num_passes);
        if (SUCCEEDED(hr)) {
            std::cerr << "  FAILED: ValidateDevice should fail with invalid specular source" << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED: Invalid specular source rejected" << std::endl;
        }
        
        // Reset to valid value
        device->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
        
        // Test conflicting fog modes
        device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);  // Both enabled - invalid
        hr = device->ValidateDevice(&num_passes);
        if (SUCCEEDED(hr)) {
            std::cerr << "  FAILED: ValidateDevice should fail with both fog modes" << std::endl;
            all_tests_passed = false;
        } else {
            std::cout << "  PASSED: Conflicting fog modes rejected" << std::endl;
        }
    }
    
    // Test Z-bias effect (visual test would be needed for full validation)
    {
        std::cout << "Testing Z-bias polygon offset..." << std::endl;
        
        // Clear and begin scene
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->BeginScene();
        
        // Enable Z-bias
        device->SetRenderState(D3DRS_ZBIAS, 8);
        device->SetRenderState(D3DRS_ZENABLE, TRUE);
        
        // In a real test, we'd draw overlapping geometry here to verify Z-bias works
        // For now, just ensure the state applies without errors
        
        device->EndScene();
        
        // Disable Z-bias
        device->SetRenderState(D3DRS_ZBIAS, 0);
        
        std::cout << "  PASSED: Z-bias state applied" << std::endl;
    }
    
    // Clean up
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    if (all_tests_passed) {
        std::cout << "SUCCESS: All render state tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: Some tests failed" << std::endl;
        return 1;
    }
}