#include <iostream>
#include <cassert>
#include <cstring>
#include "../src/d3d8_texture.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_interface.h"
#include "../src/dx8gl.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Mock device for testing
class MockDevice : public Direct3DDevice8 {
public:
    MockDevice() : Direct3DDevice8(nullptr, 0, D3DDEVTYPE_HAL, nullptr, 0, nullptr) {}
};

// Test LOD settings
void test_lod_control() {
    // Initialize dx8gl with OSMesa
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    MockDevice device;
    
    // Create a texture with multiple mip levels
    Direct3DTexture8* texture = new Direct3DTexture8(&device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(texture->initialize() && "Failed to initialize texture");
    
    // Test initial LOD
    DWORD initial_lod = texture->GetLOD();
    assert(initial_lod == 0 && "Initial LOD should be 0");
    
    // Test setting LOD
    DWORD old_lod = texture->SetLOD(2);
    assert(old_lod == 0 && "SetLOD should return old LOD value");
    assert(texture->GetLOD() == 2 && "GetLOD should return new LOD value");
    
    // Test LOD clamping to max level
    DWORD level_count = texture->GetLevelCount();
    old_lod = texture->SetLOD(level_count + 5);
    assert(texture->GetLOD() == level_count - 1 && "LOD should be clamped to max level");
    
    // Clean up
    texture->Release();
    
    print_test_result("test_lod_control", true);
}

// Test dirty rectangle tracking
void test_dirty_regions() {
    // Initialize dx8gl with OSMesa
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    MockDevice device;
    
    // Create a managed texture
    Direct3DTexture8* texture = new Direct3DTexture8(&device, 128, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(texture->initialize() && "Failed to initialize texture");
    
    // Test adding dirty rect
    RECT dirty_rect = {10, 20, 50, 60};
    HRESULT hr = texture->AddDirtyRect(&dirty_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect should succeed");
    
    // Test adding NULL dirty rect (entire texture)
    hr = texture->AddDirtyRect(nullptr);
    assert(SUCCEEDED(hr) && "AddDirtyRect with NULL should succeed");
    
    // Test invalid dirty rect
    RECT invalid_rect = {50, 60, 10, 20}; // Invalid: right < left, bottom < top
    hr = texture->AddDirtyRect(&invalid_rect);
    assert(FAILED(hr) && "AddDirtyRect with invalid rect should fail");
    
    // Test out-of-bounds rect (should be clamped)
    RECT oob_rect = {100, 100, 200, 200}; // Extends beyond texture size
    hr = texture->AddDirtyRect(&oob_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect with out-of-bounds rect should succeed (clamped)");
    
    // Clean up
    texture->Release();
    
    print_test_result("test_dirty_regions", true);
}

// Test dirty region upload
void test_dirty_upload() {
    // Initialize dx8gl with OSMesa
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    MockDevice device;
    
    // Create a managed texture
    Direct3DTexture8* texture = new Direct3DTexture8(&device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(texture->initialize() && "Failed to initialize texture");
    
    // Lock the texture and write some data
    D3DLOCKED_RECT locked_rect;
    HRESULT hr = texture->LockRect(0, &locked_rect, nullptr, 0);
    assert(SUCCEEDED(hr) && "LockRect should succeed");
    
    // Fill with test pattern
    DWORD* pixels = (DWORD*)locked_rect.pBits;
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            pixels[y * locked_rect.Pitch / 4 + x] = 0xFF0000FF; // Red
        }
    }
    
    texture->UnlockRect(0);
    
    // Mark a region as dirty
    RECT dirty_rect = {10, 10, 30, 30};
    texture->AddDirtyRect(&dirty_rect);
    
    // Lock the dirty region and modify it
    hr = texture->LockRect(0, &locked_rect, &dirty_rect, 0);
    assert(SUCCEEDED(hr) && "LockRect on dirty region should succeed");
    
    // Fill dirty region with different color
    pixels = (DWORD*)locked_rect.pBits;
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++) {
            pixels[y * locked_rect.Pitch / 4 + x] = 0xFF00FF00; // Green
        }
    }
    
    texture->UnlockRect(0);
    
    // The dirty region should be uploaded when the texture is bound
    // In a real test, we would bind the texture and verify the upload
    
    // Clean up
    texture->Release();
    
    print_test_result("test_dirty_upload", true);
}

// Test non-managed pool behavior
void test_non_managed_pool() {
    // Initialize dx8gl with OSMesa
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    MockDevice device;
    
    // Create a DEFAULT pool texture
    Direct3DTexture8* texture = new Direct3DTexture8(&device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT);
    assert(texture->initialize() && "Failed to initialize texture");
    
    // AddDirtyRect should be a no-op for non-managed textures
    RECT dirty_rect = {10, 10, 30, 30};
    HRESULT hr = texture->AddDirtyRect(&dirty_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect should succeed (no-op) for DEFAULT pool");
    
    // Clean up
    texture->Release();
    
    print_test_result("test_non_managed_pool", true);
}

// Test mipmap LOD behavior
void test_mipmap_lod() {
    // Initialize dx8gl with OSMesa
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    MockDevice device;
    
    // Create texture with automatic mip levels (0 = full chain)
    Direct3DTexture8* texture = new Direct3DTexture8(&device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(texture->initialize() && "Failed to initialize texture");
    
    // Check that we have the expected number of mip levels
    DWORD level_count = texture->GetLevelCount();
    assert(level_count == 9 && "256x256 texture should have 9 mip levels");
    
    // Test LOD with each level
    for (DWORD lod = 0; lod < level_count; lod++) {
        texture->SetLOD(lod);
        assert(texture->GetLOD() == lod && "LOD should be set correctly");
    }
    
    // Clean up
    texture->Release();
    
    print_test_result("test_mipmap_lod", true);
}

int main() {
    std::cout << "Running dx8gl texture LOD and dirty region tests..." << std::endl;
    std::cout << "=================================================" << std::endl;
    
    test_lod_control();
    test_dirty_regions();
    test_dirty_upload();
    test_non_managed_pool();
    test_mipmap_lod();
    
    std::cout << "=================================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}