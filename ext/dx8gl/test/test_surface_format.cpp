#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include "../src/d3d8_surface.h"
#include "../src/d3d8_types.h"
#include "../src/d3d8_constants.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test format size calculations
void test_format_sizes() {
    struct FormatSizeTest {
        D3DFORMAT format;
        UINT expected_size;
        const char* name;
    };
    
    FormatSizeTest tests[] = {
        { D3DFMT_R8G8B8, 3, "R8G8B8" },
        { D3DFMT_A8R8G8B8, 4, "A8R8G8B8" },
        { D3DFMT_X8R8G8B8, 4, "X8R8G8B8" },
        { D3DFMT_R5G6B5, 2, "R5G6B5" },
        { D3DFMT_X1R5G5B5, 2, "X1R5G5B5" },
        { D3DFMT_A1R5G5B5, 2, "A1R5G5B5" },
        { D3DFMT_A4R4G4B4, 2, "A4R4G4B4" },
        { static_cast<D3DFORMAT>(D3DFMT_X4R4G4B4), 2, "X4R4G4B4" },
        { static_cast<D3DFORMAT>(D3DFMT_L8), 1, "L8" },
        { static_cast<D3DFORMAT>(D3DFMT_A8L8), 2, "A8L8" },
        { D3DFMT_A8, 1, "A8" },
        { D3DFMT_D16, 2, "D16" },
        { D3DFMT_D24S8, 4, "D24S8" },
        { D3DFMT_D24X8, 4, "D24X8" },
        { D3DFMT_D32, 4, "D32" },
    };
    
    bool all_passed = true;
    for (const auto& test : tests) {
        UINT size = Direct3DSurface8::get_format_size(test.format);
        if (size != test.expected_size) {
            std::cout << "  " << test.name << " failed: expected " << test.expected_size 
                      << ", got " << size << std::endl;
            all_passed = false;
        }
    }
    
    print_test_result("test_format_sizes", all_passed);
}

// Test basic format conversions
void test_format_conversions() {
    const int pixel_count = 4;
    
    // Test ARGB to XRGB conversion
    {
        DWORD src_data[] = { 0x80112233, 0x40445566, 0x20778899, 0x00AABBCC };
        DWORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8,
                                                       pixel_count);
        assert(result && "ARGB to XRGB conversion should succeed");
        
        // Check that alpha was set to 255
        assert((dst_data[0] & 0xFF000000) == 0xFF000000 && "Alpha should be 255");
        assert((dst_data[0] & 0x00FFFFFF) == 0x00112233 && "RGB should be preserved");
    }
    
    // Test XRGB to ARGB conversion
    {
        DWORD src_data[] = { 0xFF112233, 0xFF445566, 0xFF778899, 0xFFAABBCC };
        DWORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8,
                                                       pixel_count);
        assert(result && "XRGB to ARGB conversion should succeed");
        assert(dst_data[0] == src_data[0] && "Data should be copied unchanged");
    }
    
    // Test ARGB32 to RGB565 conversion
    {
        DWORD src_data[] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF };
        WORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       D3DFMT_A8R8G8B8, D3DFMT_R5G6B5,
                                                       pixel_count);
        assert(result && "ARGB to RGB565 conversion should succeed");
        
        // Check red pixel (0xFFFF0000 -> 0xF800)
        assert(dst_data[0] == 0xF800 && "Red should convert to 0xF800");
        
        // Check green pixel (0xFF00FF00 -> 0x07E0)
        assert(dst_data[1] == 0x07E0 && "Green should convert to 0x07E0");
        
        // Check blue pixel (0xFF0000FF -> 0x001F)
        assert(dst_data[2] == 0x001F && "Blue should convert to 0x001F");
        
        // Check white pixel (0xFFFFFFFF -> 0xFFFF)
        assert(dst_data[3] == 0xFFFF && "White should convert to 0xFFFF");
    }
    
    // Test RGB565 to ARGB32 conversion
    {
        WORD src_data[] = { 0xF800, 0x07E0, 0x001F, 0xFFFF };
        DWORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       D3DFMT_R5G6B5, D3DFMT_A8R8G8B8,
                                                       pixel_count);
        assert(result && "RGB565 to ARGB conversion should succeed");
        
        // Check that alpha is 255 and colors are expanded correctly
        assert((dst_data[0] & 0xFF000000) == 0xFF000000 && "Alpha should be 255");
        
        // Red: 0xF800 -> 0xFFFF0000 (with bit replication)
        BYTE r = (dst_data[0] >> 16) & 0xFF;
        assert(r >= 0xF8 && "Red should be expanded correctly");
        
        // Green: 0x07E0 -> 0xFF00FC00 (with bit replication)
        BYTE g = (dst_data[1] >> 8) & 0xFF;
        assert(g >= 0xFC && "Green should be expanded correctly");
        
        // Blue: 0x001F -> 0xFF0000FF (with bit replication)
        BYTE b = dst_data[2] & 0xFF;
        assert(b >= 0xF8 && "Blue should be expanded correctly");
    }
    
    // Test L8 to ARGB conversion
    {
        BYTE src_data[] = { 0x00, 0x80, 0xFF, 0x40 };
        DWORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       static_cast<D3DFORMAT>(D3DFMT_L8), D3DFMT_A8R8G8B8,
                                                       pixel_count);
        assert(result && "L8 to ARGB conversion should succeed");
        
        // Check that luminance is replicated to RGB with full alpha
        assert(dst_data[0] == 0xFF000000 && "Black luminance");
        assert(dst_data[1] == 0xFF808080 && "Gray luminance");
        assert(dst_data[2] == 0xFFFFFFFF && "White luminance");
        assert(dst_data[3] == 0xFF404040 && "Dark gray luminance");
    }
    
    // Test A8L8 to ARGB conversion
    {
        BYTE src_data[] = { 0x80, 0x40,  // L=0x80, A=0x40
                           0xFF, 0x80,  // L=0xFF, A=0x80
                           0x00, 0xFF,  // L=0x00, A=0xFF
                           0x40, 0x00 }; // L=0x40, A=0x00
        DWORD dst_data[4] = { 0, 0, 0, 0 };
        
        bool result = Direct3DSurface8::convert_format(src_data, dst_data,
                                                       static_cast<D3DFORMAT>(D3DFMT_A8L8), D3DFMT_A8R8G8B8,
                                                       pixel_count);
        assert(result && "A8L8 to ARGB conversion should succeed");
        
        // Check conversions
        assert(dst_data[0] == 0x40808080 && "A8L8 pixel 0");
        assert(dst_data[1] == 0x80FFFFFF && "A8L8 pixel 1");
        assert(dst_data[2] == 0xFF000000 && "A8L8 pixel 2");
        assert(dst_data[3] == 0x00404040 && "A8L8 pixel 3");
    }
    
    print_test_result("test_format_conversions", true);
}

// Test unsupported conversions
void test_unsupported_conversions() {
    DWORD src_data = 0xFFFFFFFF;
    DWORD dst_data = 0;
    
    // Test conversion from ARGB to L8 (not implemented)
    bool result = Direct3DSurface8::convert_format(&src_data, &dst_data,
                                                   D3DFMT_A8R8G8B8, static_cast<D3DFORMAT>(D3DFMT_L8), 1);
    assert(!result && "ARGB to L8 conversion should fail (not implemented)");
    
    // Test same format "conversion"
    result = Direct3DSurface8::convert_format(&src_data, &dst_data,
                                             D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, 1);
    assert(!result && "Same format conversion should return false");
    
    // Test null pointers
    result = Direct3DSurface8::convert_format(nullptr, &dst_data,
                                             D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, 1);
    assert(!result && "Null source should fail");
    
    result = Direct3DSurface8::convert_format(&src_data, nullptr,
                                             D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, 1);
    assert(!result && "Null destination should fail");
    
    print_test_result("test_unsupported_conversions", true);
}

// Test GL format mapping
void test_gl_format_mapping() {
    struct GLFormatTest {
        D3DFORMAT d3d_format;
        GLenum expected_internal;
        GLenum expected_format;
        GLenum expected_type;
        const char* name;
    };
    
    GLFormatTest tests[] = {
        { D3DFMT_R8G8B8, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, "R8G8B8" },
#ifdef __EMSCRIPTEN__
        { D3DFMT_A8R8G8B8, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, "A8R8G8B8 (WebGL)" },
#else
        { D3DFMT_A8R8G8B8, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, "A8R8G8B8 (Desktop)" },
#endif
        { D3DFMT_R5G6B5, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, "R5G6B5" },
        { D3DFMT_A4R4G4B4, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, "A4R4G4B4" },
        { D3DFMT_A1R5G5B5, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, "A1R5G5B5" },
        { static_cast<D3DFORMAT>(D3DFMT_L8), GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, "L8" },
        { static_cast<D3DFORMAT>(D3DFMT_A8L8), GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, "A8L8" },
        { D3DFMT_A8, GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, "A8" },
        { D3DFMT_D16, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, "D16" },
    };
    
    bool all_passed = true;
    for (const auto& test : tests) {
        GLenum internal_format, format, type;
        bool result = Direct3DSurface8::get_gl_format(test.d3d_format, internal_format, format, type);
        
        if (!result) {
            std::cout << "  " << test.name << " failed: format not supported" << std::endl;
            all_passed = false;
            continue;
        }
        
        if (internal_format != test.expected_internal ||
            format != test.expected_format ||
            type != test.expected_type) {
            std::cout << "  " << test.name << " failed: got ("
                      << std::hex << internal_format << ", " << format << ", " << type
                      << "), expected ("
                      << test.expected_internal << ", " << test.expected_format << ", " 
                      << test.expected_type << ")" << std::dec << std::endl;
            all_passed = false;
        }
    }
    
    print_test_result("test_gl_format_mapping", all_passed);
}

int main() {
    std::cout << "Running surface format tests..." << std::endl;
    std::cout << "===============================" << std::endl;
    
    test_format_sizes();
    test_format_conversions();
    test_unsupported_conversions();
    test_gl_format_mapping();
    
    std::cout << "===============================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}