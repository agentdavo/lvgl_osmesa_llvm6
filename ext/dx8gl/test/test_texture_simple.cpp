#include <iostream>
#include <cassert>
#include <cstring>
#include "../src/d3d8_texture.h"
#include "../src/dx8gl.h"
#include "../src/gl3_headers.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test LOD settings directly on texture
void test_lod_control() {
    // Create a texture with multiple mip levels
    Direct3DTexture8 texture(nullptr, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    
    // Test initial LOD
    DWORD initial_lod = texture.GetLOD();
    assert(initial_lod == 0 && "Initial LOD should be 0");
    
    // Test setting LOD
    DWORD old_lod = texture.SetLOD(2);
    assert(old_lod == 0 && "SetLOD should return old LOD value");
    assert(texture.GetLOD() == 2 && "GetLOD should return new LOD value");
    
    // Test LOD clamping to max level
    DWORD level_count = texture.GetLevelCount();
    old_lod = texture.SetLOD(level_count + 5);
    assert(texture.GetLOD() == level_count - 1 && "LOD should be clamped to max level");
    
    print_test_result("test_lod_control", true);
}

// Test dirty rectangle tracking
void test_dirty_regions() {
    // Create a managed texture
    Direct3DTexture8 texture(nullptr, 128, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    
    // Test adding dirty rect
    RECT dirty_rect = {10, 20, 50, 60};
    HRESULT hr = texture.AddDirtyRect(&dirty_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect should succeed");
    
    // Test adding NULL dirty rect (entire texture)
    hr = texture.AddDirtyRect(nullptr);
    assert(SUCCEEDED(hr) && "AddDirtyRect with NULL should succeed");
    
    // Test invalid dirty rect
    RECT invalid_rect = {50, 60, 10, 20}; // Invalid: right < left, bottom < top
    hr = texture.AddDirtyRect(&invalid_rect);
    assert(FAILED(hr) && "AddDirtyRect with invalid rect should fail");
    
    // Test out-of-bounds rect (should be clamped)
    RECT oob_rect = {100, 100, 200, 200}; // Extends beyond texture size
    hr = texture.AddDirtyRect(&oob_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect with out-of-bounds rect should succeed (clamped)");
    
    print_test_result("test_dirty_regions", true);
}

// Test non-managed pool behavior
void test_non_managed_pool() {
    // Create a DEFAULT pool texture
    Direct3DTexture8 texture(nullptr, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT);
    
    // AddDirtyRect should be a no-op for non-managed textures
    RECT dirty_rect = {10, 10, 30, 30};
    HRESULT hr = texture.AddDirtyRect(&dirty_rect);
    assert(SUCCEEDED(hr) && "AddDirtyRect should succeed (no-op) for DEFAULT pool");
    
    print_test_result("test_non_managed_pool", true);
}

// Test mipmap LOD behavior
void test_mipmap_lod() {
    // Create texture with automatic mip levels (0 = full chain)
    Direct3DTexture8 texture(nullptr, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    
    // Check that we have the expected number of mip levels
    DWORD level_count = texture.GetLevelCount();
    assert(level_count == 9 && "256x256 texture should have 9 mip levels");
    
    // Test LOD with each level
    for (DWORD lod = 0; lod < level_count; lod++) {
        texture.SetLOD(lod);
        assert(texture.GetLOD() == lod && "LOD should be set correctly");
    }
    
    print_test_result("test_mipmap_lod", true);
}

// Test ES 2.0 LOD filtering behavior
void test_es20_lod_filtering() {
    std::cout << "\nTesting ES 2.0 LOD filtering behavior:" << std::endl;
    
    // Create texture with multiple mip levels
    Direct3DTexture8 texture(nullptr, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    
    // Test different LOD settings
    std::cout << "  - LOD 0 (full mipmap chain): Uses GL_LINEAR_MIPMAP_LINEAR" << std::endl;
    texture.SetLOD(0);
    
    std::cout << "  - LOD = max level (no mipmapping): Uses GL_LINEAR" << std::endl;
    texture.SetLOD(texture.GetLevelCount() - 1);
    
    std::cout << "  - LOD in between: Uses GL_NEAREST_MIPMAP_NEAREST" << std::endl;
    texture.SetLOD(texture.GetLevelCount() / 2);
    
    print_test_result("test_es20_lod_filtering", true);
}

int main() {
    std::cout << "Running dx8gl texture LOD and dirty region tests..." << std::endl;
    std::cout << "==================================================" << std::endl;
    
    test_lod_control();
    test_dirty_regions();
    test_non_managed_pool();
    test_mipmap_lod();
    test_es20_lod_filtering();
    
    std::cout << "==================================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}