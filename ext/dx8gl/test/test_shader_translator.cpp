#include <iostream>
#include <cassert>
#include "../src/dx8_shader_translator.h"

using namespace dx8gl;

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test swizzle encoding
void test_swizzle_encoding() {
    DX8ShaderTranslator translator;
    
    // Test vertex shader with custom swizzles
    std::string vs_source = R"(
vs.1.1
dcl_position v0
dcl_texcoord v1
mov r0, v0.xyzw
mov r1.xy, v1.xy
dp3 r2.x, v0.xyz, c0.xyz
mov oPos, r0
mov oT0.xy, r1.xy
)";
    
    std::string error;
    bool parsed = translator.parse_shader(vs_source, error);
    assert(parsed && "Failed to parse vertex shader with swizzles");
    
    // Check bytecode contains proper swizzle encoding
    auto bytecode = translator.get_bytecode();
    assert(bytecode.size() > 0 && "Bytecode should not be empty");
    
    // Generate GLSL and verify swizzles are preserved
    std::string glsl = translator.generate_glsl();
    assert(glsl.find(".xyz") != std::string::npos && "Swizzle .xyz should be in GLSL");
    assert(glsl.find(".xy") != std::string::npos && "Swizzle .xy should be in GLSL");
    
    print_test_result("test_swizzle_encoding", true);
}

// Test varying usage tracking
void test_varying_tracking() {
    DX8ShaderTranslator vs_translator;
    
    // Vertex shader that writes to specific varyings
    std::string vs_source = R"(
vs.1.1
dcl_position v0
dcl_normal v1
dcl_texcoord v2
mov r0, v0
m4x4 oPos, r0, c0
mov oD0, c4
mov oT0.xy, v2.xy
mov oT2.xy, v2.xy
)";
    
    std::string error;
    bool parsed = vs_translator.parse_shader(vs_source, error);
    assert(parsed && "Failed to parse vertex shader");
    
    std::string vs_glsl = vs_translator.generate_glsl();
    
    // Check that only used varyings are declared
    assert(vs_glsl.find("varying vec4 v_color0") != std::string::npos && "v_color0 should be declared");
    assert(vs_glsl.find("varying vec4 v_color1") == std::string::npos && "v_color1 should not be declared");
    assert(vs_glsl.find("varying vec4 v_texcoord0") != std::string::npos && "v_texcoord0 should be declared");
    assert(vs_glsl.find("varying vec2 v_texcoord2") != std::string::npos && "v_texcoord2 should be declared");
    assert(vs_glsl.find("varying vec2 v_texcoord1") == std::string::npos && "v_texcoord1 should not be declared");
    
    // Test pixel shader varying usage
    DX8ShaderTranslator ps_translator;
    std::string ps_source = R"(
ps.1.4
texld r0, t0
mov r1, v0
mad r0, r0, r1, c0
mov r0, r0
)";
    
    parsed = ps_translator.parse_shader(ps_source, error);
    assert(parsed && "Failed to parse pixel shader");
    
    std::string ps_glsl = ps_translator.generate_glsl();
    
    // Check that only used varyings are declared
    assert(ps_glsl.find("varying vec4 v_color0") != std::string::npos && "v_color0 should be declared in PS");
    assert(ps_glsl.find("varying vec4 v_texcoord0") != std::string::npos && "v_texcoord0 should be declared in PS");
    
    print_test_result("test_varying_tracking", true);
}

// Test complex swizzle patterns
void test_complex_swizzles() {
    DX8ShaderTranslator translator;
    
    std::string source = R"(
vs.1.1
dcl_position v0
dcl_normal v1
mov r0.x, v0.w
mov r0.yzw, v1.xxx
dp3 r1.w, v0.xyx, v1.zyx
mov oPos, r0.wzyx
)";
    
    std::string error;
    bool parsed = translator.parse_shader(source, error);
    assert(parsed && "Failed to parse shader with complex swizzles");
    
    std::string glsl = translator.generate_glsl();
    
    // Verify various swizzle patterns are handled
    assert(glsl.find(".w") != std::string::npos && "Single component swizzle should work");
    assert(glsl.find(".xxx") != std::string::npos && "Replicated swizzle should work");
    assert(glsl.find(".xyx") != std::string::npos && "Mixed swizzle should work");
    assert(glsl.find(".zyx") != std::string::npos && "Reverse swizzle should work");
    assert(glsl.find(".wzyx") != std::string::npos && "Full reverse swizzle should work");
    
    print_test_result("test_complex_swizzles", true);
}

// Test minimal varying usage (no varyings used)
void test_minimal_varyings() {
    DX8ShaderTranslator translator;
    
    // Vertex shader that only outputs position
    std::string source = R"(
vs.1.1
dcl_position v0
m4x4 oPos, v0, c0
)";
    
    std::string error;
    bool parsed = translator.parse_shader(source, error);
    assert(parsed && "Failed to parse minimal vertex shader");
    
    std::string glsl = translator.generate_glsl();
    
    // Check that no color varyings are declared if not used
    size_t color_count = 0;
    size_t pos = 0;
    while ((pos = glsl.find("varying vec4 v_color", pos)) != std::string::npos) {
        color_count++;
        pos += 20;
    }
    assert(color_count == 0 && "No color varyings should be declared when not used");
    
    print_test_result("test_minimal_varyings", true);
}

// Test register encoding with different swizzles through bytecode
void test_register_encoding() {
    DX8ShaderTranslator translator;
    
    // Test swizzle encoding through bytecode generation
    std::string source = R"(
vs.1.1
mov r0.xyzw, c0.xyzw
mov r1.wzyx, c1.wzyx
mov r2.xxxx, c2.xxxx
)";
    
    std::string error;
    bool parsed = translator.parse_shader(source, error);
    assert(parsed && "Failed to parse shader for register encoding test");
    
    // Get bytecode and verify it contains proper encoding
    auto bytecode = translator.get_bytecode();
    assert(bytecode.size() > 0 && "Bytecode should not be empty");
    
    // The bytecode should contain the swizzle information
    // We can't directly test private encode_register, but we can verify
    // that different swizzles produce different bytecode
    
    // Parse another shader with different swizzles
    DX8ShaderTranslator translator2;
    std::string source2 = R"(
vs.1.1
mov r0.xyzw, c0.yyyy
mov r1.wzyx, c1.xyzw
mov r2.xxxx, c2.zzzz
)";
    
    parsed = translator2.parse_shader(source2, error);
    assert(parsed && "Failed to parse second shader");
    
    auto bytecode2 = translator2.get_bytecode();
    
    // The bytecodes should be different due to different swizzles
    assert(bytecode != bytecode2 && "Different swizzles should produce different bytecode");
    
    print_test_result("test_register_encoding", true);
}

int main() {
    std::cout << "Running dx8gl shader translator tests..." << std::endl;
    std::cout << "=======================================" << std::endl;
    
    test_swizzle_encoding();
    test_varying_tracking();
    test_complex_swizzles();
    test_minimal_varyings();
    test_register_encoding();
    
    std::cout << "=======================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}