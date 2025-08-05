#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include "../src/fvf_utils.h"
#include "../src/d3d8_constants.h"
#include "../src/gl3_headers.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test vertex structure with multiple texture coordinates
struct MultiTexVertex {
    float x, y, z;           // Position
    float nx, ny, nz;        // Normal
    float u0, v0;            // Texture coordinate set 0
    float u1, v1;            // Texture coordinate set 1
    float u2, v2, w2;        // Texture coordinate set 2 (3D)
    float u3, v3, w3, q3;    // Texture coordinate set 3 (4D)
};

// Test basic FVF parsing with multiple texture coordinates
void test_fvf_multi_texcoords() {
    // Create FVF with position, normal, and 4 texture coordinate sets
    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX4 |
                D3DFVF_TEXCOORDSIZE2(0) |  // 2D for tex0
                D3DFVF_TEXCOORDSIZE2(1) |  // 2D for tex1
                D3DFVF_TEXCOORDSIZE3(2) |  // 3D for tex2
                D3DFVF_TEXCOORDSIZE4(3);   // 4D for tex3
    
    // Get vertex size
    UINT vertex_size = FVFParser::get_vertex_size(fvf);
    UINT expected_size = sizeof(float) * (3 + 3 + 2 + 2 + 3 + 4); // pos + normal + tex0-3
    assert(vertex_size == expected_size && "Vertex size calculation incorrect");
    
    // Check texture coordinate count
    int tex_count = FVFParser::get_texcoord_count(fvf);
    assert(tex_count == 4 && "Should have 4 texture coordinate sets");
    
    // Check individual texture coordinate sizes
    assert(FVFParser::get_texcoord_size(fvf, 0) == 2 && "Tex0 should be 2D");
    assert(FVFParser::get_texcoord_size(fvf, 1) == 2 && "Tex1 should be 2D");
    assert(FVFParser::get_texcoord_size(fvf, 2) == 3 && "Tex2 should be 3D");
    assert(FVFParser::get_texcoord_size(fvf, 3) == 4 && "Tex3 should be 4D");
    
    print_test_result("test_fvf_multi_texcoords", true);
}

// Test vertex attribute parsing with multiple texture coordinates
void test_multi_texcoord_attributes() {
    // Create FVF with multiple texture coordinates
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX3 |
                D3DFVF_TEXCOORDSIZE2(0) |
                D3DFVF_TEXCOORDSIZE3(1) |
                D3DFVF_TEXCOORDSIZE1(2);
    
    // Parse FVF to get attributes
    auto attributes = FVFParser::parse_fvf(fvf);
    
    // Should have: position, color, tex0, tex1, tex2
    assert(attributes.size() == 5 && "Should have 5 attributes");
    
    // Check attribute details
    UINT expected_offset = 0;
    
    // Position
    assert(attributes[0].size == 3 && "Position should be 3 components");
    assert(attributes[0].type == GL_FLOAT && "Position should be float");
    assert(attributes[0].offset == expected_offset && "Position offset incorrect");
    expected_offset += 3 * sizeof(float);
    
    // Diffuse color
    assert(attributes[1].size == 4 && "Color should be 4 components");
    assert(attributes[1].type == GL_UNSIGNED_BYTE && "Color should be unsigned byte");
    assert(attributes[1].normalized == GL_TRUE && "Color should be normalized");
    assert(attributes[1].offset == expected_offset && "Color offset incorrect");
    expected_offset += sizeof(DWORD);
    
    // Texture coordinate 0 (2D)
    assert(attributes[2].size == 2 && "Tex0 should be 2 components");
    assert(attributes[2].type == GL_FLOAT && "Tex0 should be float");
    assert(attributes[2].offset == expected_offset && "Tex0 offset incorrect");
    expected_offset += 2 * sizeof(float);
    
    // Texture coordinate 1 (3D)
    assert(attributes[3].size == 3 && "Tex1 should be 3 components");
    assert(attributes[3].type == GL_FLOAT && "Tex1 should be float");
    assert(attributes[3].offset == expected_offset && "Tex1 offset incorrect");
    expected_offset += 3 * sizeof(float);
    
    // Texture coordinate 2 (1D)
    assert(attributes[4].size == 1 && "Tex2 should be 1 component");
    assert(attributes[4].type == GL_FLOAT && "Tex2 should be float");
    assert(attributes[4].offset == expected_offset && "Tex2 offset incorrect");
    
    print_test_result("test_multi_texcoord_attributes", true);
}

// Test maximum texture coordinate sets
void test_max_texcoords() {
    // Create FVF with maximum 8 texture coordinate sets
    DWORD fvf = D3DFVF_XYZ | D3DFVF_TEX8;
    
    // All texture coordinates default to 2D
    int tex_count = FVFParser::get_texcoord_count(fvf);
    assert(tex_count == 8 && "Should have 8 texture coordinate sets");
    
    // Calculate expected size
    UINT vertex_size = FVFParser::get_vertex_size(fvf);
    UINT expected_size = sizeof(float) * (3 + 2 * 8); // pos + 8 * 2D texcoords
    assert(vertex_size == expected_size && "Vertex size with 8 texcoords incorrect");
    
    // Test mixed sizes for all 8 texture coordinates
    fvf = D3DFVF_XYZ | D3DFVF_TEX8 |
          D3DFVF_TEXCOORDSIZE1(0) |  // 1D
          D3DFVF_TEXCOORDSIZE2(1) |  // 2D (default)
          D3DFVF_TEXCOORDSIZE3(2) |  // 3D
          D3DFVF_TEXCOORDSIZE4(3) |  // 4D
          D3DFVF_TEXCOORDSIZE2(4) |  // 2D
          D3DFVF_TEXCOORDSIZE3(5) |  // 3D
          D3DFVF_TEXCOORDSIZE1(6) |  // 1D
          D3DFVF_TEXCOORDSIZE4(7);   // 4D
    
    vertex_size = FVFParser::get_vertex_size(fvf);
    expected_size = sizeof(float) * (3 + 1 + 2 + 3 + 4 + 2 + 3 + 1 + 4);
    assert(vertex_size == expected_size && "Vertex size with mixed texcoord sizes incorrect");
    
    print_test_result("test_max_texcoords", true);
}

// Test texture coordinate offsets
void test_texcoord_offsets() {
    // Create test vertex data
    MultiTexVertex vertex = {
        1.0f, 2.0f, 3.0f,           // Position
        0.0f, 1.0f, 0.0f,           // Normal
        0.5f, 0.5f,                 // Tex0
        0.25f, 0.75f,               // Tex1
        0.1f, 0.2f, 0.3f,           // Tex2
        0.4f, 0.5f, 0.6f, 0.7f      // Tex3
    };
    
    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX4 |
                D3DFVF_TEXCOORDSIZE2(0) |
                D3DFVF_TEXCOORDSIZE2(1) |
                D3DFVF_TEXCOORDSIZE3(2) |
                D3DFVF_TEXCOORDSIZE4(3);
    
    // Parse attributes
    auto attributes = FVFParser::parse_fvf(fvf);
    
    // Verify we can access the texture coordinate data correctly
    const uint8_t* vertex_data = reinterpret_cast<const uint8_t*>(&vertex);
    
    // Find texture coordinate attributes (should be indices 2, 3, 4, 5)
    for (size_t i = 2; i < 6; i++) {
        const float* tex_data = reinterpret_cast<const float*>(vertex_data + attributes[i].offset);
        
        switch (i - 2) {
            case 0: // Tex0
                assert(tex_data[0] == 0.5f && tex_data[1] == 0.5f && "Tex0 data incorrect");
                break;
            case 1: // Tex1
                assert(tex_data[0] == 0.25f && tex_data[1] == 0.75f && "Tex1 data incorrect");
                break;
            case 2: // Tex2
                assert(tex_data[0] == 0.1f && tex_data[1] == 0.2f && tex_data[2] == 0.3f && 
                       "Tex2 data incorrect");
                break;
            case 3: // Tex3
                assert(tex_data[0] == 0.4f && tex_data[1] == 0.5f && 
                       tex_data[2] == 0.6f && tex_data[3] == 0.7f && "Tex3 data incorrect");
                break;
        }
    }
    
    print_test_result("test_texcoord_offsets", true);
}

// Test FVF with no texture coordinates
void test_no_texcoords() {
    // FVF with just position and normal
    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    
    int tex_count = FVFParser::get_texcoord_count(fvf);
    assert(tex_count == 0 && "Should have 0 texture coordinate sets");
    
    UINT vertex_size = FVFParser::get_vertex_size(fvf);
    UINT expected_size = sizeof(float) * 6; // Just position and normal
    assert(vertex_size == expected_size && "Vertex size without texcoords incorrect");
    
    print_test_result("test_no_texcoords", true);
}

// Test texture coordinate format encoding
void test_texcoord_format_encoding() {
    std::cout << "\nTesting texture coordinate format encoding:" << std::endl;
    
    // Test that format bits are correctly positioned for each stage
    for (int stage = 0; stage < 8; stage++) {
        // Test each format type
        DWORD formats[] = {
            D3DFVF_TEXCOORDSIZE1(stage),
            D3DFVF_TEXCOORDSIZE2(stage),
            D3DFVF_TEXCOORDSIZE3(stage),
            D3DFVF_TEXCOORDSIZE4(stage)
        };
        
        int expected_sizes[] = { 1, 2, 3, 4 };
        
        for (int fmt = 0; fmt < 4; fmt++) {
            DWORD fvf = D3DFVF_XYZ | D3DFVF_TEX1 | formats[fmt];
            int size = FVFParser::get_texcoord_size(fvf, stage);
            
            if (stage == 0) {
                // Only the first texture coordinate set should have the custom size
                assert(size == expected_sizes[fmt] && "Incorrect texture coordinate size");
            } else {
                // Other stages should default to 2D since we only have TEX1
                assert(size == 2 && "Non-existent texture coordinates should default to 2D");
            }
        }
    }
    
    std::cout << "  - Format encoding for all 8 stages verified" << std::endl;
    print_test_result("test_texcoord_format_encoding", true);
}

int main() {
    std::cout << "Running multiple texture coordinate tests..." << std::endl;
    std::cout << "===========================================" << std::endl;
    
    test_fvf_multi_texcoords();
    test_multi_texcoord_attributes();
    test_max_texcoords();
    test_texcoord_offsets();
    test_no_texcoords();
    test_texcoord_format_encoding();
    
    std::cout << "===========================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}