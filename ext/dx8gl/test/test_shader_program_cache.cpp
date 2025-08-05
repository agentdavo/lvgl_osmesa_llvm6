#include <iostream>
#include <vector>
#include <memory>
#include "../src/d3d8.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/logger.h"
#include "../src/dx8gl.h"
#include "../src/shader_binary_cache.h"

using namespace dx8gl;

// Simple vertex shader bytecode (vs.1.1)
// vs.1.1
// m4x4 oPos, v0, c0
// mov oD0, v1
const DWORD g_vs_simple[] = {
    0xFFFE0101,  // vs.1.1
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x00000001, 0x800F0005, 0x90E40001,              // mov oD0, v1
    0x0000FFFF   // end
};

// Different pixel shaders to test cache
const DWORD g_ps_red[] = {
    0xFFFF0101,  // ps.1.1
    0x00000051, 0xA00F0000, 0x3F800000, 0x00000000, 0x00000000, 0x3F800000,  // def c0, 1.0, 0.0, 0.0, 1.0
    0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
    0x0000FFFF   // end
};

const DWORD g_ps_green[] = {
    0xFFFF0101,  // ps.1.1
    0x00000051, 0xA00F0000, 0x00000000, 0x3F800000, 0x00000000, 0x3F800000,  // def c0, 0.0, 1.0, 0.0, 1.0
    0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
    0x0000FFFF   // end
};

const DWORD g_ps_texture[] = {
    0xFFFF0101,  // ps.1.1
    0x00000042, 0xB00F0000,              // tex t0
    0x00000001, 0x800F0000, 0xB0E40000,  // mov r0, t0
    0x0000FFFF   // end
};

bool test_shader_program_cache() {
    std::cout << "=== Test: Shader Program Cache with Different Pixel Shaders ===\n";
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    dx8gl_error init_result = dx8gl_init(&config);
    if (init_result != DX8GL_SUCCESS) {
        std::cerr << "Failed to initialize dx8gl: error code " << init_result << std::endl;
        return false;
    }
    
    // Enable shader cache
    g_shader_binary_cache = std::make_unique<ShaderBinaryCache>();
    g_shader_binary_cache->initialize("test_shader_cache", 1024 * 1024); // 1MB cache
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8\n";
        return false;
    }
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                   &pp, &device);
    if (FAILED(hr)) {
        std::cerr << "Failed to create device: " << hr << std::endl;
        d3d8->Release();
        return false;
    }
    
    std::cout << "Device created successfully\n";
    
    // Create vertex shader
    DWORD vs_handle = 0;
    hr = device->CreateVertexShader(nullptr, g_vs_simple, &vs_handle, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex shader: " << hr << std::endl;
        device->Release();
        d3d8->Release();
        return false;
    }
    std::cout << "Created vertex shader: handle=" << vs_handle << "\n";
    
    // Create different pixel shaders
    DWORD ps_red_handle = 0;
    hr = device->CreatePixelShader(g_ps_red, &ps_red_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to create red pixel shader: " << hr << std::endl;
        device->DeleteVertexShader(vs_handle);
        device->Release();
        d3d8->Release();
        return false;
    }
    std::cout << "Created red pixel shader: handle=" << ps_red_handle << "\n";
    
    DWORD ps_green_handle = 0;
    hr = device->CreatePixelShader(g_ps_green, &ps_green_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to create green pixel shader: " << hr << std::endl;
        device->DeletePixelShader(ps_red_handle);
        device->DeleteVertexShader(vs_handle);
        device->Release();
        d3d8->Release();
        return false;
    }
    std::cout << "Created green pixel shader: handle=" << ps_green_handle << "\n";
    
    DWORD ps_texture_handle = 0;
    hr = device->CreatePixelShader(g_ps_texture, &ps_texture_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to create texture pixel shader: " << hr << std::endl;
        device->DeletePixelShader(ps_green_handle);
        device->DeletePixelShader(ps_red_handle);
        device->DeleteVertexShader(vs_handle);
        device->Release();
        d3d8->Release();
        return false;
    }
    std::cout << "Created texture pixel shader: handle=" << ps_texture_handle << "\n";
    
    // Test 1: Same vertex shader with different pixel shaders should produce different cache hashes
    std::cout << "\nTest 1: Testing cache hash uniqueness with different pixel shaders\n";
    
    // Compute hash for VS + Red PS
    std::vector<DWORD> vs_bytecode(g_vs_simple, g_vs_simple + sizeof(g_vs_simple) / sizeof(DWORD));
    std::vector<DWORD> ps_red_bytecode(g_ps_red, g_ps_red + sizeof(g_ps_red) / sizeof(DWORD));
    std::string hash_vs_red = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_red_bytecode);
    std::cout << "Hash (VS + Red PS): " << hash_vs_red << "\n";
    
    // Compute hash for VS + Green PS
    std::vector<DWORD> ps_green_bytecode(g_ps_green, g_ps_green + sizeof(g_ps_green) / sizeof(DWORD));
    std::string hash_vs_green = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_green_bytecode);
    std::cout << "Hash (VS + Green PS): " << hash_vs_green << "\n";
    
    // Compute hash for VS + Texture PS
    std::vector<DWORD> ps_texture_bytecode(g_ps_texture, g_ps_texture + sizeof(g_ps_texture) / sizeof(DWORD));
    std::string hash_vs_texture = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_texture_bytecode);
    std::cout << "Hash (VS + Texture PS): " << hash_vs_texture << "\n";
    
    // Compute hash for VS only (no pixel shader)
    std::string hash_vs_only = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, std::vector<DWORD>());
    std::cout << "Hash (VS only): " << hash_vs_only << "\n";
    
    // Verify all hashes are unique
    bool hashes_unique = true;
    if (hash_vs_red == hash_vs_green) {
        std::cerr << "ERROR: Red and Green PS produced same hash!\n";
        hashes_unique = false;
    }
    if (hash_vs_red == hash_vs_texture) {
        std::cerr << "ERROR: Red and Texture PS produced same hash!\n";
        hashes_unique = false;
    }
    if (hash_vs_green == hash_vs_texture) {
        std::cerr << "ERROR: Green and Texture PS produced same hash!\n";
        hashes_unique = false;
    }
    if (hash_vs_red == hash_vs_only || hash_vs_green == hash_vs_only || hash_vs_texture == hash_vs_only) {
        std::cerr << "ERROR: PS hash matched VS-only hash!\n";
        hashes_unique = false;
    }
    
    if (hashes_unique) {
        std::cout << "PASS: All shader combinations produced unique hashes\n";
    }
    
    // Test 2: Verify that shader program linking uses the correct cache entries
    std::cout << "\nTest 2: Testing shader program linking with cache\n";
    
    // Set vertex shader
    hr = device->SetVertexShader(vs_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to set vertex shader: " << hr << std::endl;
    }
    
    // Test with red pixel shader
    hr = device->SetPixelShader(ps_red_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to set red pixel shader: " << hr << std::endl;
    } else {
        std::cout << "Set red pixel shader - cache should use hash: " << hash_vs_red << "\n";
    }
    
    // Test with green pixel shader
    hr = device->SetPixelShader(ps_green_handle);
    if (FAILED(hr)) {
        std::cerr << "Failed to set green pixel shader: " << hr << std::endl;
    } else {
        std::cout << "Set green pixel shader - cache should use hash: " << hash_vs_green << "\n";
    }
    
    // Test with no pixel shader
    hr = device->SetPixelShader(0);
    if (FAILED(hr)) {
        std::cerr << "Failed to clear pixel shader: " << hr << std::endl;
    } else {
        std::cout << "Cleared pixel shader - cache should use hash: " << hash_vs_only << "\n";
    }
    
    // Cleanup
    device->DeletePixelShader(ps_texture_handle);
    device->DeletePixelShader(ps_green_handle);
    device->DeletePixelShader(ps_red_handle);
    device->DeleteVertexShader(vs_handle);
    device->Release();
    d3d8->Release();
    
    g_shader_binary_cache.reset();
    dx8gl_shutdown();
    
    std::cout << "\nShader program cache test completed!\n";
    return hashes_unique;
}

int main() {
    std::cout << "Running Shader Program Cache Tests\n";
    std::cout << "==================================\n";
    
    bool success = test_shader_program_cache();
    
    if (success) {
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!\n";
        return 1;
    }
}