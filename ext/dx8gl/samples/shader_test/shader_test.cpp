// Shader Translation and Cache Test for dx8gl
// Tests ps1.4 and vs1.1 shader features and binary caching

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstring>
#include "d3d8.h"
#include "dx8gl.h"
#include "dx8_shader_translator.h"
#include "shader_binary_cache.h"
#include "logger.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

// Test shader sources
const char* g_vs11_basic = R"(
vs.1.1
dcl_position v0
dcl_normal v1
dcl_texcoord v2
def c40, 1.0, 0.0, 0.0, 1.0
mov r0, v0
m4x4 oPos, r0, c0
mov oT0, v2
mov oD0, c40
)";

const char* g_vs11_address_register = R"(
vs.1.1
dcl_position v0
def c20, 4.0, 0.0, 0.0, 0.0
mov a0.x, c20.x
add r0, v0, c[a0.x + 2]
m4x4 oPos, r0, c0
)";

const char* g_vs11_sincos = R"(
vs.1.1
dcl_position v0
dcl_texcoord v1
mov r0, v0
mov r1.x, v1.x
sincos r2.xy, r1.x
mad r0.xy, r2.xy, c20.xy, r0.xy
m4x4 oPos, r0, c0
)";

const char* g_ps14_basic = R"(
ps.1.4
texld r0, t0
mov r0, r0
)";

const char* g_ps14_bump_mapping = R"(
ps.1.4
texld r0, t0
texld r1, t1
bem r1.xy, r0, r1
phase
texld r2, r1
mul r0, r2, c0
)";

const char* g_ps14_cnd_cmp = R"(
ps.1.4
def c0, 0.5, 0.5, 0.5, 1.0
def c1, 1.0, 0.0, 0.0, 1.0
def c2, 0.0, 1.0, 0.0, 1.0
texld r0, t0
cnd r1, r0.a, c1, c2
cmp r2, r0, c1, c2
add r0, r1, r2
)";

// Test structure
struct ShaderTest {
    const char* name;
    const char* source;
    bool is_vertex_shader;
    std::vector<DWORD> expected_tokens;
};

// Helper to run a test
bool run_shader_test(const ShaderTest& test) {
    std::cout << "\nTesting: " << test.name << std::endl;
    
    // Create translator
    dx8gl::DX8ShaderTranslator translator;
    
    // Parse shader
    std::string error;
    if (!translator.parse_shader(test.source, error)) {
        std::cerr << "  FAILED: Parse error: " << error << std::endl;
        return false;
    }
    
    // Get bytecode
    auto bytecode = translator.get_bytecode();
    std::cout << "  Bytecode size: " << bytecode.size() << " DWORDs" << std::endl;
    
    // Verify shader type
    dx8gl::ShaderType expected_type = test.is_vertex_shader ? 
        dx8gl::SHADER_TYPE_VERTEX : dx8gl::SHADER_TYPE_PIXEL;
    if (translator.get_shader_type() != expected_type) {
        std::cerr << "  FAILED: Wrong shader type detected" << std::endl;
        return false;
    }
    
    // Generate GLSL
    std::string glsl = translator.generate_glsl();
    if (glsl.empty()) {
        std::cerr << "  FAILED: Empty GLSL generated" << std::endl;
        return false;
    }
    
    std::cout << "  GLSL generated (" << glsl.length() << " chars)" << std::endl;
    
    // Check for required features in GLSL
    if (test.is_vertex_shader) {
        // Vertex shader checks
        if (glsl.find("#version 450 core") == std::string::npos &&
            glsl.find("#version 300 es") == std::string::npos) {
            std::cerr << "  FAILED: Missing proper version directive" << std::endl;
            return false;
        }
        
        if (test.name == std::string("VS 1.1 Address Register")) {
            if (glsl.find("ivec4 a0") == std::string::npos) {
                std::cerr << "  FAILED: Missing address register declaration" << std::endl;
                return false;
            }
            if (glsl.find("c[int(a0.x)") == std::string::npos) {
                std::cerr << "  FAILED: Missing relative addressing" << std::endl;
                return false;
            }
        }
        
        if (test.name == std::string("VS 1.1 SINCOS")) {
            if (glsl.find("cos(") == std::string::npos || 
                glsl.find("sin(") == std::string::npos) {
                std::cerr << "  FAILED: Missing SINCOS implementation" << std::endl;
                return false;
            }
        }
    } else {
        // Pixel shader checks
        if (test.name == std::string("PS 1.4 Bump Mapping")) {
            if (glsl.find("u_bumpEnvMat") == std::string::npos) {
                std::cerr << "  FAILED: Missing bump environment matrix" << std::endl;
                return false;
            }
            if (glsl.find("PHASE") == std::string::npos) {
                std::cerr << "  FAILED: Missing phase marker" << std::endl;
                return false;
            }
        }
        
        if (test.name == std::string("PS 1.4 CND/CMP")) {
            if (glsl.find("mix(") == std::string::npos) {
                std::cerr << "  FAILED: Missing CND/CMP implementation" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "  PASSED: Shader translation successful" << std::endl;
    return true;
}

// Test binary caching
bool test_shader_caching() {
    std::cout << "\nTesting shader binary caching..." << std::endl;
    
    // Initialize cache
    if (!dx8gl::g_shader_binary_cache) {
        dx8gl::ShaderBinaryCacheConfig config;
        config.disk_cache_directory = "./shader_cache_test";
        if (!dx8gl::initialize_shader_binary_cache(config)) {
            std::cerr << "  FAILED: Could not initialize shader cache" << std::endl;
            return false;
        }
    }
    
    // Create test bytecode
    std::vector<DWORD> vs_bytecode = {0xFFFE0101, 0x00000001, 0x800F0000, 0x90E40000, 0x0000FFFF};
    std::vector<DWORD> ps_bytecode = {0xFFFF0104, 0x00000001, 0x800F0000, 0xB0E40000, 0x0000FFFF};
    
    // Compute hash
    std::string hash1 = dx8gl::ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_bytecode);
    std::cout << "  Hash 1: " << hash1 << std::endl;
    
    // Verify hash format
    if (hash1.substr(0, 4) != "dx8_") {
        std::cerr << "  FAILED: Hash doesn't start with dx8_ prefix" << std::endl;
        return false;
    }
    
    // Test different bytecode produces different hash
    ps_bytecode[1] = 0x00000002;
    std::string hash2 = dx8gl::ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_bytecode);
    std::cout << "  Hash 2: " << hash2 << std::endl;
    
    if (hash1 == hash2) {
        std::cerr << "  FAILED: Different bytecode produced same hash" << std::endl;
        return false;
    }
    
    // Test cache statistics
    auto stats = dx8gl::g_shader_binary_cache->get_statistics();
    std::cout << "  Cache stats - Memory hits: " << stats.memory_cache_hits 
              << ", Memory misses: " << stats.memory_cache_misses << std::endl;
    
    std::cout << "  PASSED: Shader caching test successful" << std::endl;
    return true;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::cout << "=== DirectX 8 Shader Translation and Cache Tests ===" << std::endl;
    
    // Initialize dx8gl
    dx8gl_init(nullptr);
    
    // Set up logging
    dx8gl::set_log_level(dx8gl::LOG_LEVEL_INFO);
    
    // Define tests
    std::vector<ShaderTest> tests = {
        {"VS 1.1 Basic", g_vs11_basic, true, {}},
        {"VS 1.1 Address Register", g_vs11_address_register, true, {}},
        {"VS 1.1 SINCOS", g_vs11_sincos, true, {}},
        {"PS 1.4 Basic", g_ps14_basic, false, {}},
        {"PS 1.4 Bump Mapping", g_ps14_bump_mapping, false, {}},
        {"PS 1.4 CND/CMP", g_ps14_cnd_cmp, false, {}}
    };
    
    int passed = 0;
    int failed = 0;
    
    // Run shader translation tests
    for (const auto& test : tests) {
        if (run_shader_test(test)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    // Run caching test
    if (test_shader_caching()) {
        passed++;
    } else {
        failed++;
    }
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << (passed + failed) << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    // Cleanup
    dx8gl::shutdown_shader_binary_cache();
    
    return failed > 0 ? 1 : 0;
}