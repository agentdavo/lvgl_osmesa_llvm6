#include <iostream>
#include <cassert>
#include <cstring>
#include <cmath>
#include <vector>
#include "../src/dx8gl.h"
#include "../src/d3d8.h"
#include "../src/d3dx_compat.h"
#include "../src/render_backend.h"
#include "../src/offscreen_framebuffer.h"
#include "../src/logger.h"
#include <fstream>

// OpenGL constants
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif

// Backend access function
namespace dx8gl {
    extern DX8RenderBackend* get_render_backend();
}

using namespace dx8gl;

// Test utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERTION FAILED: " << message << std::endl; \
            std::cerr << "  at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASSED" << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            failed++; \
        } \
        total++; \
    } while(0)

// Helper function to compare colors with tolerance
bool colors_equal(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
                 uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2,
                 uint8_t tolerance = 1) {
    return std::abs(r1 - r2) <= tolerance &&
           std::abs(g1 - g2) <= tolerance &&
           std::abs(b1 - b2) <= tolerance &&
           std::abs(a1 - a2) <= tolerance;
}

// Test 1: OffscreenFramebuffer creation and basic operations
bool test_framebuffer_creation() {
    // Test RGBA8 framebuffer
    {
        OffscreenFramebuffer fb(100, 100, PixelFormat::RGBA8, true);
        
        TEST_ASSERT(fb.get_width() == 100, "Wrong width");
        TEST_ASSERT(fb.get_height() == 100, "Wrong height");
        TEST_ASSERT(fb.get_format() == PixelFormat::RGBA8, "Wrong format");
        TEST_ASSERT(fb.get_bytes_per_pixel() == 4, "Wrong bytes per pixel for RGBA8");
        TEST_ASSERT(fb.get_size_bytes() == 100 * 100 * 4, "Wrong buffer size");
        TEST_ASSERT(fb.get_data() != nullptr, "Buffer is null");
    }
    
    // Test RGB565 framebuffer
    {
        OffscreenFramebuffer fb(64, 64, PixelFormat::RGB565, true);
        
        TEST_ASSERT(fb.get_bytes_per_pixel() == 2, "Wrong bytes per pixel for RGB565");
        TEST_ASSERT(fb.get_size_bytes() == 64 * 64 * 2, "Wrong buffer size for RGB565");
    }
    
    // Test FLOAT_RGBA framebuffer
    {
        OffscreenFramebuffer fb(32, 32, PixelFormat::FLOAT_RGBA, true);
        
        TEST_ASSERT(fb.get_bytes_per_pixel() == 16, "Wrong bytes per pixel for FLOAT_RGBA");
        TEST_ASSERT(fb.get_size_bytes() == 32 * 32 * 16, "Wrong buffer size for FLOAT_RGBA");
    }
    
    return true;
}

// Test 2: Framebuffer clear operation
bool test_framebuffer_clear() {
    OffscreenFramebuffer fb(10, 10, PixelFormat::RGBA8, true);
    
    // Clear to red
    fb.clear(1.0f, 0.0f, 0.0f, 1.0f);
    
    uint8_t* pixels = fb.get_data_as<uint8_t>();
    TEST_ASSERT(pixels != nullptr, "Pixels are null");
    
    // Check first pixel
    TEST_ASSERT(colors_equal(pixels[0], pixels[1], pixels[2], pixels[3],
                            255, 0, 0, 255),
               "First pixel not red after clear");
    
    // Check last pixel
    int last_idx = (10 * 10 - 1) * 4;
    TEST_ASSERT(colors_equal(pixels[last_idx], pixels[last_idx+1], pixels[last_idx+2], pixels[last_idx+3],
                            255, 0, 0, 255),
               "Last pixel not red after clear");
    
    // Clear to semi-transparent green
    fb.clear(0.0f, 0.5f, 0.0f, 0.5f);
    
    // Check a pixel
    TEST_ASSERT(colors_equal(pixels[0], pixels[1], pixels[2], pixels[3],
                            0, 127, 0, 127, 2),
               "Pixel not semi-transparent green after clear");
    
    return true;
}

// Test 3: Framebuffer resize
bool test_framebuffer_resize() {
    OffscreenFramebuffer fb(50, 50, PixelFormat::RGBA8, true);
    
    // Initial clear to blue
    fb.clear(0.0f, 0.0f, 1.0f, 1.0f);
    
    // Resize
    TEST_ASSERT(fb.resize(100, 100), "Resize failed");
    TEST_ASSERT(fb.get_width() == 100, "Wrong width after resize");
    TEST_ASSERT(fb.get_height() == 100, "Wrong height after resize");
    TEST_ASSERT(fb.get_size_bytes() == 100 * 100 * 4, "Wrong size after resize");
    
    // After resize, buffer should be reallocated (contents undefined)
    // Clear to verify it works
    fb.clear(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    uint8_t* pixels = fb.get_data_as<uint8_t>();
    TEST_ASSERT(colors_equal(pixels[0], pixels[1], pixels[2], pixels[3],
                            255, 255, 0, 255),
               "Pixel not yellow after resize and clear");
    
    return true;
}

// Test 4: Format conversion - RGBA8 to RGB565
bool test_format_conversion_rgba8_to_rgb565() {
    OffscreenFramebuffer fb(2, 2, PixelFormat::RGBA8, true);
    
    // Set specific pixel values
    uint8_t* pixels = fb.get_data_as<uint8_t>();
    // Pixel 0: Red
    pixels[0] = 255; pixels[1] = 0; pixels[2] = 0; pixels[3] = 255;
    // Pixel 1: Green
    pixels[4] = 0; pixels[5] = 255; pixels[6] = 0; pixels[7] = 255;
    // Pixel 2: Blue
    pixels[8] = 0; pixels[9] = 0; pixels[10] = 255; pixels[11] = 255;
    // Pixel 3: White
    pixels[12] = 255; pixels[13] = 255; pixels[14] = 255; pixels[15] = 255;
    
    // Convert to RGB565
    uint16_t output[4];
    TEST_ASSERT(fb.convert_to(PixelFormat::RGB565, output), "Conversion failed");
    
    // Check converted values
    // Red: R=31, G=0, B=0 -> 0xF800
    TEST_ASSERT((output[0] & 0xF800) == 0xF800, "Red channel incorrect");
    TEST_ASSERT((output[0] & 0x07E0) == 0x0000, "Green channel not zero for red pixel");
    
    // Green: R=0, G=63, B=0 -> 0x07E0
    TEST_ASSERT((output[1] & 0xF800) == 0x0000, "Red channel not zero for green pixel");
    TEST_ASSERT((output[1] & 0x07E0) == 0x07E0, "Green channel incorrect");
    
    // Blue: R=0, G=0, B=31 -> 0x001F
    TEST_ASSERT((output[2] & 0x001F) == 0x001F, "Blue channel incorrect");
    
    // White: R=31, G=63, B=31 -> 0xFFFF
    TEST_ASSERT(output[3] == 0xFFFF, "White pixel incorrect");
    
    return true;
}

// Test 5: Format conversion - RGB565 to RGBA8
bool test_format_conversion_rgb565_to_rgba8() {
    OffscreenFramebuffer fb(2, 1, PixelFormat::RGB565, true);
    
    uint16_t* pixels = fb.get_data_as<uint16_t>();
    // Pixel 0: Pure red in RGB565 (R=31, G=0, B=0)
    pixels[0] = 0xF800;
    // Pixel 1: Pure green in RGB565 (R=0, G=63, B=0)
    pixels[1] = 0x07E0;
    
    // Convert to RGBA8
    uint8_t output[8];
    TEST_ASSERT(fb.convert_to(PixelFormat::RGBA8, output), "Conversion failed");
    
    // Check converted values
    // Red pixel should be ~(248, 0, 0, 255) due to 5-bit to 8-bit conversion
    TEST_ASSERT(colors_equal(output[0], output[1], output[2], output[3],
                            248, 0, 0, 255, 8),
               "Red pixel conversion incorrect");
    
    // Green pixel should be ~(0, 252, 0, 255) due to 6-bit to 8-bit conversion
    TEST_ASSERT(colors_equal(output[4], output[5], output[6], output[7],
                            0, 252, 0, 255, 8),
               "Green pixel conversion incorrect");
    
    return true;
}

// Test 6: Backend framebuffer integration
bool test_backend_framebuffer_integration() {
    // Initialize with OSMesa backend
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize");
    
    // Get the backend
    auto* backend = dx8gl::get_render_backend();
    TEST_ASSERT(backend != nullptr, "Backend is null");
    
    // Get framebuffer
    int width, height, format;
    void* fb_data = backend->get_framebuffer(width, height, format);
    
    TEST_ASSERT(fb_data != nullptr, "Framebuffer data is null");
    TEST_ASSERT(width == 64, "Wrong framebuffer width");
    TEST_ASSERT(height == 64, "Wrong framebuffer height");
    TEST_ASSERT(format == GL_RGBA || format == 0x1908, "Unexpected framebuffer format");
    
    // Test resize
    TEST_ASSERT(backend->resize(128, 128), "Backend resize failed");
    
    fb_data = backend->get_framebuffer(width, height, format);
    TEST_ASSERT(width == 128, "Wrong width after resize");
    TEST_ASSERT(height == 128, "Wrong height after resize");
    
    dx8gl_shutdown();
    return true;
}

// Test 7: Float to RGBA8 conversion
bool test_float_rgba_conversion() {
    OffscreenFramebuffer fb(2, 2, PixelFormat::FLOAT_RGBA, true);
    
    float* pixels = fb.get_data_as<float>();
    // Pixel 0: Half intensity red
    pixels[0] = 0.5f; pixels[1] = 0.0f; pixels[2] = 0.0f; pixels[3] = 1.0f;
    // Pixel 1: Overrange green (should clamp)
    pixels[4] = 0.0f; pixels[5] = 1.5f; pixels[6] = 0.0f; pixels[7] = 1.0f;
    // Pixel 2: Negative blue (should clamp to 0)
    pixels[8] = 0.0f; pixels[9] = 0.0f; pixels[10] = -0.5f; pixels[11] = 1.0f;
    // Pixel 3: Normal white
    pixels[12] = 1.0f; pixels[13] = 1.0f; pixels[14] = 1.0f; pixels[15] = 1.0f;
    
    // Convert to RGBA8
    uint8_t output[16];
    TEST_ASSERT(fb.convert_to(PixelFormat::RGBA8, output), "Conversion failed");
    
    // Check conversions
    TEST_ASSERT(colors_equal(output[0], output[1], output[2], output[3],
                            127, 0, 0, 255, 2),
               "Half intensity red incorrect");
    
    TEST_ASSERT(colors_equal(output[4], output[5], output[6], output[7],
                            0, 255, 0, 255),
               "Clamped green incorrect");
    
    TEST_ASSERT(colors_equal(output[8], output[9], output[10], output[11],
                            0, 0, 0, 255),
               "Clamped negative blue incorrect");
    
    TEST_ASSERT(colors_equal(output[12], output[13], output[14], output[15],
                            255, 255, 255, 255),
               "White pixel incorrect");
    
    return true;
}

// Test 8: Multiple format conversions
bool test_multiple_conversions() {
    // Start with RGBA8
    OffscreenFramebuffer fb(1, 1, PixelFormat::RGBA8, true);
    fb.clear(0.5f, 0.25f, 0.75f, 1.0f);
    
    // Convert to RGB565
    auto fb_565 = fb.convert_to(PixelFormat::RGB565);
    TEST_ASSERT(fb_565 != nullptr, "Conversion to RGB565 failed");
    TEST_ASSERT(fb_565->get_format() == PixelFormat::RGB565, "Wrong format after conversion");
    
    // Convert back to RGBA8
    auto fb_rgba = fb_565->convert_to(PixelFormat::RGBA8);
    TEST_ASSERT(fb_rgba != nullptr, "Conversion back to RGBA8 failed");
    TEST_ASSERT(fb_rgba->get_format() == PixelFormat::RGBA8, "Wrong format after back conversion");
    
    // Due to precision loss in RGB565, values won't match exactly
    uint8_t* original = fb.get_data_as<uint8_t>();
    uint8_t* converted = fb_rgba->get_data_as<uint8_t>();
    
    // Allow larger tolerance due to 565 conversion
    TEST_ASSERT(colors_equal(original[0], original[1], original[2], original[3],
                            converted[0], converted[1], converted[2], converted[3],
                            16),
               "Color significantly different after round-trip conversion");
    
    return true;
}

// Test 9: D3DXSaveSurfaceToFile functionality
bool test_save_surface_to_file() {
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize dx8gl");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    TEST_ASSERT(d3d8 != nullptr, "Failed to create Direct3D8");
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 32;
    pp.BackBufferHeight = 32;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr) && device != nullptr, "Failed to create device");
    
    // Create a surface
    IDirect3DSurface8* surface = nullptr;
    hr = device->CreateRenderTarget(32, 32, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, FALSE, &surface);
    TEST_ASSERT(SUCCEEDED(hr) && surface != nullptr, "Failed to create render target surface");
    
    // Lock and fill the surface with test pattern
    D3DLOCKED_RECT locked_rect;
    hr = surface->LockRect(&locked_rect, nullptr, 0);
    TEST_ASSERT(SUCCEEDED(hr), "Failed to lock surface");
    
    uint32_t* pixels = static_cast<uint32_t*>(locked_rect.pBits);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            // Create a simple gradient pattern
            uint8_t r = (x * 255) / 31;
            uint8_t g = (y * 255) / 31;
            uint8_t b = 128;
            uint8_t a = 255;
            pixels[y * (locked_rect.Pitch / 4) + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    surface->UnlockRect();
    
    // Test saving as BMP
    const char* bmp_filename = "test_surface.bmp";
    hr = D3DXSaveSurfaceToFile(bmp_filename, D3DXIFF_BMP, surface, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Failed to save surface as BMP");
    
    // Verify BMP file exists
    {
        std::ifstream file(bmp_filename, std::ios::binary);
        TEST_ASSERT(file.is_open(), "BMP file was not created");
        
        // Check BMP header
        char header[2];
        file.read(header, 2);
        TEST_ASSERT(header[0] == 'B' && header[1] == 'M', "Invalid BMP file header");
        file.close();
        
        // Clean up
        std::remove(bmp_filename);
    }
    
    // Test saving as TGA
    const char* tga_filename = "test_surface.tga";
    hr = D3DXSaveSurfaceToFile(tga_filename, D3DXIFF_TGA, surface, nullptr, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Failed to save surface as TGA");
    
    // Verify TGA file exists
    {
        std::ifstream file(tga_filename, std::ios::binary);
        TEST_ASSERT(file.is_open(), "TGA file was not created");
        file.close();
        
        // Clean up
        std::remove(tga_filename);
    }
    
    // Test saving with a source rectangle
    RECT src_rect = { 8, 8, 24, 24 }; // 16x16 region
    hr = D3DXSaveSurfaceToFile("test_surface_rect.bmp", D3DXIFF_BMP, surface, nullptr, &src_rect);
    TEST_ASSERT(SUCCEEDED(hr), "Failed to save surface region as BMP");
    std::remove("test_surface_rect.bmp");
    
    // Test error cases
    hr = D3DXSaveSurfaceToFile(nullptr, D3DXIFF_BMP, surface, nullptr, nullptr);
    TEST_ASSERT(FAILED(hr), "Should fail with null filename");
    
    hr = D3DXSaveSurfaceToFile("test.png", D3DXIFF_PNG, surface, nullptr, nullptr);
    TEST_ASSERT(FAILED(hr), "Should fail with unsupported format");
    
    // Clean up
    surface->Release();
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    return true;
}

int main() {
    std::cout << "=== dx8gl Framebuffer Correctness Tests ===" << std::endl;
    
    int total = 0, passed = 0, failed = 0;
    
    // Run all tests
    RUN_TEST(test_framebuffer_creation);
    RUN_TEST(test_framebuffer_clear);
    RUN_TEST(test_framebuffer_resize);
    RUN_TEST(test_format_conversion_rgba8_to_rgb565);
    RUN_TEST(test_format_conversion_rgb565_to_rgba8);
    RUN_TEST(test_backend_framebuffer_integration);
    RUN_TEST(test_float_rgba_conversion);
    RUN_TEST(test_multiple_conversions);
    RUN_TEST(test_save_surface_to_file);
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return failed == 0 ? 0 : 1;
}