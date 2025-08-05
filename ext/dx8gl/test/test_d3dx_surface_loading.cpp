#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <fstream>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3dx_compat.h"
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

// Create a simple BMP file in memory for testing
static vector<uint8_t> create_test_bmp(uint32_t width, uint32_t height, bool has_alpha = false) {
    uint32_t bits_per_pixel = has_alpha ? 32 : 24;
    uint32_t row_stride = ((width * bits_per_pixel + 31) / 32) * 4;
    uint32_t image_size = row_stride * height;
    uint32_t file_size = 54 + image_size; // BMP header is 54 bytes
    
    vector<uint8_t> bmp_data(file_size);
    
    // BMP file header (14 bytes)
    bmp_data[0] = 'B'; bmp_data[1] = 'M';                    // Signature
    *reinterpret_cast<uint32_t*>(&bmp_data[2]) = file_size; // File size
    *reinterpret_cast<uint32_t*>(&bmp_data[6]) = 0;         // Reserved
    *reinterpret_cast<uint32_t*>(&bmp_data[10]) = 54;       // Data offset
    
    // BMP info header (40 bytes)
    *reinterpret_cast<uint32_t*>(&bmp_data[14]) = 40;                    // Header size
    *reinterpret_cast<int32_t*>(&bmp_data[18]) = static_cast<int32_t>(width);   // Width
    *reinterpret_cast<int32_t*>(&bmp_data[22]) = static_cast<int32_t>(height);  // Height (positive = bottom-up)
    *reinterpret_cast<uint16_t*>(&bmp_data[26]) = 1;                     // Planes
    *reinterpret_cast<uint16_t*>(&bmp_data[28]) = bits_per_pixel;        // Bits per pixel
    *reinterpret_cast<uint32_t*>(&bmp_data[30]) = 0;                     // Compression (none)
    *reinterpret_cast<uint32_t*>(&bmp_data[34]) = image_size;            // Image size
    *reinterpret_cast<uint32_t*>(&bmp_data[38]) = 0;                     // X pixels per meter
    *reinterpret_cast<uint32_t*>(&bmp_data[42]) = 0;                     // Y pixels per meter
    *reinterpret_cast<uint32_t*>(&bmp_data[46]) = 0;                     // Colors used
    *reinterpret_cast<uint32_t*>(&bmp_data[50]) = 0;                     // Important colors
    
    // Fill pixel data with a gradient pattern
    uint8_t* pixel_data = &bmp_data[54];
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t r = static_cast<uint8_t>((x * 255) / width);
            uint8_t g = static_cast<uint8_t>((y * 255) / height);
            uint8_t b = static_cast<uint8_t>(((x + y) * 255) / (width + height));
            uint8_t a = has_alpha ? static_cast<uint8_t>((x * 255) / width) : 0xFF;
            
            uint8_t* pixel = pixel_data + y * row_stride + x * (bits_per_pixel / 8);
            
            if (has_alpha) {
                // BGRA format
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                pixel[3] = a;
            } else {
                // BGR format
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
            }
        }
    }
    
    return bmp_data;
}

// Create a simple TGA file in memory for testing
static vector<uint8_t> create_test_tga(uint32_t width, uint32_t height, bool has_alpha = false) {
    uint32_t bits_per_pixel = has_alpha ? 32 : 24;
    uint32_t image_size = width * height * (bits_per_pixel / 8);
    uint32_t file_size = 18 + image_size; // TGA header is 18 bytes
    
    vector<uint8_t> tga_data(file_size);
    
    // TGA header (18 bytes)
    tga_data[0] = 0;  // ID length
    tga_data[1] = 0;  // Color map type
    tga_data[2] = 2;  // Image type (uncompressed true-color)
    *reinterpret_cast<uint16_t*>(&tga_data[3]) = 0;    // Color map first
    *reinterpret_cast<uint16_t*>(&tga_data[5]) = 0;    // Color map length
    tga_data[7] = 0;  // Color map entry size
    *reinterpret_cast<uint16_t*>(&tga_data[8]) = 0;    // X origin
    *reinterpret_cast<uint16_t*>(&tga_data[10]) = 0;   // Y origin
    *reinterpret_cast<uint16_t*>(&tga_data[12]) = static_cast<uint16_t>(width);   // Width
    *reinterpret_cast<uint16_t*>(&tga_data[14]) = static_cast<uint16_t>(height);  // Height
    tga_data[16] = bits_per_pixel;  // Pixel depth
    tga_data[17] = has_alpha ? 0x08 : 0x00;  // Image descriptor (alpha bits)
    
    // Fill pixel data with a checkerboard pattern
    uint8_t* pixel_data = &tga_data[18];
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            uint8_t r = checker ? 0xFF : 0x00;
            uint8_t g = checker ? 0x00 : 0xFF;
            uint8_t b = checker ? 0x00 : 0x00;
            uint8_t a = has_alpha ? 0xFF : 0xFF;
            
            uint8_t* pixel = pixel_data + (y * width + x) * (bits_per_pixel / 8);
            
            if (has_alpha) {
                // BGRA format
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                pixel[3] = a;
            } else {
                // BGR format
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
            }
        }
    }
    
    return tga_data;
}

bool test_surface_loading_basic() {
    cout << "\n=== Test: Basic Surface Loading ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    TEST_ASSERT(SUCCEEDED(hr), "Device creation should succeed");
    TEST_ASSERT(device != nullptr, "Device should not be null");
    
    // Create a surface for testing
    IDirect3DSurface8* surface = nullptr;
    cout << "Creating surface with format D3DFMT_A8R8G8B8 (value=" << D3DFMT_A8R8G8B8 << ")" << endl;
    hr = device->CreateImageSurface(256, 256, D3DFMT_A8R8G8B8, &surface);
    cout << "CreateImageSurface returned HRESULT: 0x" << hex << hr << dec << endl;
    TEST_ASSERT(SUCCEEDED(hr), "Surface creation should succeed");
    TEST_ASSERT(surface != nullptr, "Surface should not be null");
    
    // Test loading from invalid file
    hr = D3DXLoadSurfaceFromFile(surface, nullptr, nullptr, "nonexistent.bmp", 
                                nullptr, D3DX_DEFAULT, 0, nullptr);
    TEST_ASSERT(hr == D3DERR_NOTFOUND, "Loading nonexistent file should fail");
    
    // Create test BMP data
    vector<uint8_t> bmp_data = create_test_bmp(64, 64, false);
    
    // Save to temporary file
    const char* temp_file = "test_temp.bmp";
    ofstream file(temp_file, ios::binary);
    file.write(reinterpret_cast<const char*>(bmp_data.data()), bmp_data.size());
    file.close();
    
    // Test loading from file
    hr = D3DXLoadSurfaceFromFile(surface, nullptr, nullptr, temp_file,
                                nullptr, D3DX_DEFAULT, 0, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Loading BMP file should succeed");
    
    // Clean up
    remove(temp_file);
    surface->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_surface_loading_rectangles() {
    cout << "\n=== Test: Surface Loading with Rectangles ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface and device
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    
    // Create surfaces for testing
    IDirect3DSurface8* surface = nullptr;
    device->CreateImageSurface(128, 128, D3DFMT_A8R8G8B8, &surface);
    
    // Create test TGA data
    vector<uint8_t> tga_data = create_test_tga(64, 64, true);
    
    // Save to temporary file
    const char* temp_file = "test_temp.tga";
    ofstream file(temp_file, ios::binary);
    file.write(reinterpret_cast<const char*>(tga_data.data()), tga_data.size());
    file.close();
    
    // Test loading with destination rectangle
    RECT dest_rect = { 32, 32, 96, 96 };
    HRESULT hr = D3DXLoadSurfaceFromFile(surface, nullptr, &dest_rect, temp_file,
                                        nullptr, D3DX_DEFAULT, 0, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Loading TGA with dest rect should succeed");
    
    // Test loading with source rectangle
    RECT src_rect = { 16, 16, 48, 48 };
    hr = D3DXLoadSurfaceFromFile(surface, nullptr, nullptr, temp_file,
                                &src_rect, D3DX_DEFAULT, 0, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Loading TGA with src rect should succeed");
    
    // Clean up
    remove(temp_file);
    surface->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_surface_loading_color_key() {
    cout << "\n=== Test: Surface Loading with Color Key ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface and device
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    
    // Create surface for testing
    IDirect3DSurface8* surface = nullptr;
    device->CreateImageSurface(64, 64, D3DFMT_A8R8G8B8, &surface);
    
    // Create test BMP data with specific colors
    vector<uint8_t> bmp_data = create_test_bmp(32, 32, false);
    
    // Save to temporary file
    const char* temp_file = "test_temp_colorkey.bmp";
    ofstream file(temp_file, ios::binary);
    file.write(reinterpret_cast<const char*>(bmp_data.data()), bmp_data.size());
    file.close();
    
    // Test loading with color key (magenta = 0xFF00FF)
    D3DCOLOR color_key = 0x00FF00FF; // Magenta
    HRESULT hr = D3DXLoadSurfaceFromFile(surface, nullptr, nullptr, temp_file,
                                        nullptr, D3DX_DEFAULT, color_key, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Loading BMP with color key should succeed");
    
    // Clean up
    remove(temp_file);
    surface->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_surface_loading_format_conversion() {
    cout << "\n=== Test: Surface Loading with Format Conversion ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface and device
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    
    // Create surface with different format (16-bit)
    IDirect3DSurface8* surface = nullptr;
    HRESULT hr = device->CreateImageSurface(64, 64, D3DFMT_R5G6B5, &surface);
    if (FAILED(hr)) {
        // Fall back to 32-bit format if 16-bit is not supported
        hr = device->CreateImageSurface(64, 64, D3DFMT_A8R8G8B8, &surface);
    }
    TEST_ASSERT(SUCCEEDED(hr), "Surface creation should succeed");
    
    // Create test data
    vector<uint8_t> bmp_data = create_test_bmp(32, 32, true);
    
    // Save to temporary file
    const char* temp_file = "test_temp_format.bmp";
    ofstream file(temp_file, ios::binary);
    file.write(reinterpret_cast<const char*>(bmp_data.data()), bmp_data.size());
    file.close();
    
    // Test loading with format conversion
    hr = D3DXLoadSurfaceFromFile(surface, nullptr, nullptr, temp_file,
                                nullptr, D3DX_DEFAULT, 0, nullptr);
    TEST_ASSERT(SUCCEEDED(hr), "Loading with format conversion should succeed");
    
    // Clean up
    remove(temp_file);
    surface->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool test_surface_loading_from_memory() {
    cout << "\n=== Test: Surface Loading from Memory ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Create Direct3D8 interface and device
    IDirect3D8* d3d8 = Direct3DCreate8(120);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DDevice8* device = nullptr;
    d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);
    
    // Create surface for testing
    IDirect3DSurface8* surface = nullptr;
    device->CreateImageSurface(32, 32, D3DFMT_A8R8G8B8, &surface);
    
    // Create test pixel data (32x32 ARGB)
    vector<uint32_t> pixel_data(32 * 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t r = static_cast<uint8_t>((x * 255) / 31);
            uint8_t g = static_cast<uint8_t>((y * 255) / 31);
            uint8_t b = 128;
            uint8_t a = 255;
            pixel_data[y * 32 + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    // Test loading from memory
    HRESULT hr = D3DXLoadSurfaceFromMemory(surface, nullptr, nullptr,
                                          pixel_data.data(), D3DFMT_A8R8G8B8, 32 * 4,
                                          nullptr, nullptr, D3DX_DEFAULT, 0);
    TEST_ASSERT(SUCCEEDED(hr), "Loading from memory should succeed");
    
    // Clean up
    surface->Release();
    device->Release();
    delete static_cast<dx8gl::Direct3D8*>(d3d8);
    dx8gl_shutdown();
    
    return true;
}

bool run_all_tests() {
    cout << "Running D3DX Surface Loading Tests" << endl;
    cout << "===================================" << endl;
    
    bool all_passed = true;
    
    all_passed &= test_surface_loading_basic();
    all_passed &= test_surface_loading_rectangles();
    all_passed &= test_surface_loading_color_key();
    all_passed &= test_surface_loading_format_conversion();
    all_passed &= test_surface_loading_from_memory();
    
    return all_passed;
}

int main() {
    bool success = run_all_tests();
    
    cout << "\n===================================" << endl;
    cout << "Test Results: " << tests_passed << "/" << tests_run << " passed" << endl;
    
    if (success && tests_passed == tests_run) {
        cout << "All tests PASSED!" << endl;
        return 0;
    } else {
        cout << "Some tests FAILED!" << endl;
        return 1;
    }
}