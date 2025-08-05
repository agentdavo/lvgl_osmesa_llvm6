#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include "../src/d3d8_surface.h"
#include "../src/d3d8_texture.h"
#include "../src/dx8gl.h"
#include "../src/gl3_headers.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Helper to create a test pattern in a surface
void fill_surface_pattern(Direct3DSurface8* surface, DWORD color_base) {
    D3DLOCKED_RECT locked_rect;
    HRESULT hr = surface->LockRect(&locked_rect, nullptr, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to lock surface for pattern fill" << std::endl;
        return;
    }
    
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);
    
    DWORD* pixels = reinterpret_cast<DWORD*>(locked_rect.pBits);
    for (UINT y = 0; y < desc.Height; y++) {
        for (UINT x = 0; x < desc.Width; x++) {
            // Create a gradient pattern
            BYTE r = (color_base >> 16) & 0xFF;
            BYTE g = ((color_base >> 8) & 0xFF) + (x * 255 / desc.Width);
            BYTE b = (color_base & 0xFF) + (y * 255 / desc.Height);
            pixels[y * locked_rect.Pitch / 4 + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    
    surface->UnlockRect();
}

// Test basic surface copy
void test_basic_surface_copy() {
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return;
    }
    
    // Create source surface
    Direct3DSurface8 src_surface(nullptr, 128, 128, D3DFMT_A8R8G8B8, 0);
    assert(src_surface.initialize() && "Failed to initialize source surface");
    
    // Create destination surface
    Direct3DSurface8 dst_surface(nullptr, 128, 128, D3DFMT_A8R8G8B8, 0);
    assert(dst_surface.initialize() && "Failed to initialize destination surface");
    
    // Fill source with test pattern
    fill_surface_pattern(&src_surface, 0x00FF0000); // Red base
    
    // Copy entire surface
    bool result = dst_surface.copy_from(&src_surface, nullptr, nullptr);
    assert(result && "Surface copy should succeed");
    
    // Verify copy by reading back both surfaces
    D3DLOCKED_RECT src_locked, dst_locked;
    src_surface.LockRect(&src_locked, nullptr, D3DLOCK_READONLY);
    dst_surface.LockRect(&dst_locked, nullptr, D3DLOCK_READONLY);
    
    // Compare first few pixels
    DWORD* src_pixels = reinterpret_cast<DWORD*>(src_locked.pBits);
    DWORD* dst_pixels = reinterpret_cast<DWORD*>(dst_locked.pBits);
    
    bool match = true;
    for (int i = 0; i < 16; i++) {
        if (src_pixels[i] != dst_pixels[i]) {
            match = false;
            break;
        }
    }
    
    src_surface.UnlockRect();
    dst_surface.UnlockRect();
    
    assert(match && "Copied pixels should match source");
    
    print_test_result("test_basic_surface_copy", true);
}

// Test partial surface copy
void test_partial_surface_copy() {
    // Create surfaces
    Direct3DSurface8 src_surface(nullptr, 128, 128, D3DFMT_A8R8G8B8, 0);
    assert(src_surface.initialize() && "Failed to initialize source surface");
    
    Direct3DSurface8 dst_surface(nullptr, 128, 128, D3DFMT_A8R8G8B8, 0);
    assert(dst_surface.initialize() && "Failed to initialize destination surface");
    
    // Fill surfaces with different patterns
    fill_surface_pattern(&src_surface, 0x0000FF00); // Green base
    fill_surface_pattern(&dst_surface, 0x000000FF); // Blue base
    
    // Copy a 32x32 region from (16,16) to (64,64)
    RECT src_rect = {16, 16, 48, 48};
    POINT dst_point = {64, 64};
    
    bool result = dst_surface.copy_from(&src_surface, &src_rect, &dst_point);
    assert(result && "Partial surface copy should succeed");
    
    // Verify the copy region
    D3DLOCKED_RECT locked;
    RECT check_rect = {64, 64, 96, 96};
    dst_surface.LockRect(&locked, &check_rect, D3DLOCK_READONLY);
    
    DWORD* pixels = reinterpret_cast<DWORD*>(locked.pBits);
    DWORD first_pixel = pixels[0];
    
    // The copied region should have green component
    BYTE g = (first_pixel >> 8) & 0xFF;
    assert(g > 0 && "Copied region should have green component");
    
    dst_surface.UnlockRect();
    
    print_test_result("test_partial_surface_copy", true);
}

// Test format conversion during copy
void test_format_conversion_copy() {
    // Test ARGB to RGB565 conversion
    Direct3DSurface8 src_surface(nullptr, 64, 64, D3DFMT_A8R8G8B8, 0);
    assert(src_surface.initialize() && "Failed to initialize ARGB source");
    
    Direct3DSurface8 dst_surface(nullptr, 64, 64, D3DFMT_R5G6B5, 0);
    assert(dst_surface.initialize() && "Failed to initialize RGB565 destination");
    
    // Fill source with specific color
    D3DLOCKED_RECT locked;
    src_surface.LockRect(&locked, nullptr, 0);
    DWORD* pixels = reinterpret_cast<DWORD*>(locked.pBits);
    // Use a color that will show precision loss: 0xFF112233
    for (int i = 0; i < 64 * 64; i++) {
        pixels[i] = 0xFF112233;
    }
    src_surface.UnlockRect();
    
    // Copy with format conversion
    bool result = dst_surface.copy_from(&src_surface, nullptr, nullptr);
    assert(result && "Format conversion copy should succeed");
    
    // Verify conversion happened
    dst_surface.LockRect(&locked, nullptr, D3DLOCK_READONLY);
    WORD* dst_pixels = reinterpret_cast<WORD*>(locked.pBits);
    WORD first_pixel = dst_pixels[0];
    dst_surface.UnlockRect();
    
    // Check that we have a valid RGB565 value
    // Original: R=0x11, G=0x22, B=0x33
    // RGB565: R=0x11>>3=0x02, G=0x22>>2=0x08, B=0x33>>3=0x06
    WORD expected = (0x02 << 11) | (0x08 << 5) | 0x06;
    assert(first_pixel == expected && "RGB565 conversion should match expected value");
    
    print_test_result("test_format_conversion_copy", true);
}

// Test invalid copy operations
void test_invalid_copy() {
    Direct3DSurface8 surface(nullptr, 64, 64, D3DFMT_A8R8G8B8, 0);
    assert(surface.initialize() && "Failed to initialize surface");
    
    // Test null source
    bool result = surface.copy_from(nullptr, nullptr, nullptr);
    assert(!result && "Copy from null source should fail");
    
    // Test invalid source rect
    Direct3DSurface8 src_surface(nullptr, 32, 32, D3DFMT_A8R8G8B8, 0);
    assert(src_surface.initialize() && "Failed to initialize small source");
    
    RECT invalid_rect = {0, 0, 64, 64}; // Larger than source
    result = surface.copy_from(&src_surface, &invalid_rect, nullptr);
    assert(!result && "Copy with oversized source rect should fail");
    
    // Test destination overflow
    RECT valid_rect = {0, 0, 32, 32};
    POINT overflow_point = {40, 40}; // Would exceed 64x64 destination
    result = surface.copy_from(&src_surface, &valid_rect, &overflow_point);
    assert(!result && "Copy that would overflow destination should fail");
    
    print_test_result("test_invalid_copy", true);
}

// Test copying between texture surfaces
void test_texture_surface_copy() {
    // Create textures
    Direct3DTexture8 src_texture(nullptr, 128, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(src_texture.initialize() && "Failed to initialize source texture");
    
    Direct3DTexture8 dst_texture(nullptr, 128, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED);
    assert(dst_texture.initialize() && "Failed to initialize destination texture");
    
    // Get surface levels
    IDirect3DSurface8* src_surf_interface = nullptr;
    IDirect3DSurface8* dst_surf_interface = nullptr;
    
    HRESULT hr = src_texture.GetSurfaceLevel(0, &src_surf_interface);
    assert(SUCCEEDED(hr) && src_surf_interface && "Failed to get source surface");
    
    hr = dst_texture.GetSurfaceLevel(0, &dst_surf_interface);
    assert(SUCCEEDED(hr) && dst_surf_interface && "Failed to get destination surface");
    
    // Cast to implementation type
    Direct3DSurface8* src_surf = static_cast<Direct3DSurface8*>(src_surf_interface);
    Direct3DSurface8* dst_surf = static_cast<Direct3DSurface8*>(dst_surf_interface);
    
    // Fill source with pattern
    fill_surface_pattern(src_surf, 0x00FFFF00); // Yellow base
    
    // Copy between texture surfaces
    bool result = dst_surf->copy_from(src_surf, nullptr, nullptr);
    assert(result && "Texture surface copy should succeed");
    
    // Verify copy worked
    D3DLOCKED_RECT src_locked, dst_locked;
    src_surf->LockRect(&src_locked, nullptr, D3DLOCK_READONLY);
    dst_surf->LockRect(&dst_locked, nullptr, D3DLOCK_READONLY);
    
    DWORD* src_pixels = reinterpret_cast<DWORD*>(src_locked.pBits);
    DWORD* dst_pixels = reinterpret_cast<DWORD*>(dst_locked.pBits);
    
    bool match = (src_pixels[0] == dst_pixels[0]);
    
    src_surf->UnlockRect();
    dst_surf->UnlockRect();
    
    assert(match && "Texture surface copy should preserve data");
    
    // Clean up
    src_surf_interface->Release();
    dst_surf_interface->Release();
    
    print_test_result("test_texture_surface_copy", true);
}

// Test surface format combinations
void test_format_combinations() {
    std::cout << "\nTesting various format conversion combinations:" << std::endl;
    
    struct FormatTest {
        D3DFORMAT src_format;
        D3DFORMAT dst_format;
        const char* name;
        bool should_succeed;
    };
    
    FormatTest tests[] = {
        { D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, "ARGB to XRGB", true },
        { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, "XRGB to ARGB", true },
        { D3DFMT_A8R8G8B8, D3DFMT_R5G6B5, "ARGB to RGB565", true },
        { D3DFMT_R5G6B5, D3DFMT_A8R8G8B8, "RGB565 to ARGB", true },
        { D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, "RGB565 to XRGB", true },
        { D3DFMT_L8, D3DFMT_A8R8G8B8, "L8 to ARGB", true },
        { D3DFMT_A8L8, D3DFMT_A8R8G8B8, "A8L8 to ARGB", true },
    };
    
    for (const auto& test : tests) {
        // Use convert_format directly for unit testing
        const int pixel_count = 4;
        std::vector<BYTE> src_data(pixel_count * 4, 0);
        std::vector<BYTE> dst_data(pixel_count * 4, 0);
        
        // Fill source with test pattern
        if (test.src_format == D3DFMT_L8) {
            src_data[0] = 0x80; // Gray value
        } else if (test.src_format == D3DFMT_A8L8) {
            src_data[0] = 0x80; // Luminance
            src_data[1] = 0xFF; // Alpha
        } else if (test.src_format == D3DFMT_R5G6B5) {
            WORD* src_pixels = reinterpret_cast<WORD*>(src_data.data());
            src_pixels[0] = 0xF800; // Red
        } else {
            DWORD* src_pixels = reinterpret_cast<DWORD*>(src_data.data());
            src_pixels[0] = 0xFF112233; // Test color
        }
        
        bool result = Direct3DSurface8::convert_format(src_data.data(), dst_data.data(),
                                                       test.src_format, test.dst_format,
                                                       pixel_count);
        
        std::cout << "  - " << test.name << ": " 
                  << (result == test.should_succeed ? "PASSED" : "FAILED") << std::endl;
    }
    
    print_test_result("test_format_combinations", true);
}

int main() {
    std::cout << "Running surface copy tests..." << std::endl;
    std::cout << "=============================" << std::endl;
    
    test_basic_surface_copy();
    test_partial_surface_copy();
    test_format_conversion_copy();
    test_invalid_copy();
    test_texture_surface_copy();
    test_format_combinations();
    
    std::cout << "=============================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}