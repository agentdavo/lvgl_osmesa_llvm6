#include <iostream>
#include <vector>
#include <memory>
#include "../src/d3d8.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/logger.h"
#include "../src/dx8gl.h"
#include "../src/shader_bytecode_disassembler.h"
#include "../src/dx8_shader_translator.h"

using namespace dx8gl;

// Test vertex shaders with compiled bytecode

// Simple transform shader: vs.1.1, m4x4 oPos, v0, c0
const DWORD g_vs_transform[] = {
    0xFFFE0101,                          // vs.1.1
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x0000FFFF                           // end
};

// Color passthrough: vs.1.1, m4x4 oPos, v0, c0, mov oD0, v1
const DWORD g_vs_color[] = {
    0xFFFE0101,                          // vs.1.1
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x00000001, 0x800F0005, 0x90E40001,  // mov oD0, v1
    0x0000FFFF                           // end
};

// Texture coordinate: vs.1.1, m4x4 oPos, v0, c0, mov oT0, v2
const DWORD g_vs_texcoord[] = {
    0xFFFE0101,                          // vs.1.1
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x00000001, 0x800F0006, 0x90E40002,  // mov oT0, v2
    0x0000FFFF                           // end
};

// Complex shader with multiple operations
const DWORD g_vs_complex[] = {
    0xFFFE0101,                          // vs.1.1
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x00000005, 0x80010001, 0x90000001, 0xA0000004,  // mul r1.x, v1.x, c4.x
    0x00000002, 0x80010001, 0x80000001, 0xA0550004,  // add r1.x, r1.x, c4.y
    0x00000001, 0x800F0005, 0x80000001,  // mov oD0, r1.x
    0x00000001, 0x800F0006, 0x90E40002,  // mov oT0, v2
    0x0000FFFF                           // end
};

// Shader with constants defined
const DWORD g_vs_with_constants[] = {
    0xFFFE0101,                          // vs.1.1
    0x00000051, 0xA00F0004, 0x3F800000, 0x3F000000, 0x00000000, 0x3F800000,  // def c4, 1.0, 0.5, 0.0, 1.0
    0x00000014, 0x800F0000, 0x90E40000, 0xA0E40000,  // m4x4 oPos, v0, c0
    0x00000005, 0x800F0005, 0x90E40001, 0xA0E40004,  // mul oD0, v1, c4
    0x0000FFFF                           // end
};

bool test_vertex_shader_disassembly(const char* name, const DWORD* bytecode, size_t size_in_dwords) {
    std::cout << "\nTesting: " << name << "\n";
    std::cout << "Bytecode size: " << size_in_dwords << " DWORDs\n";
    
    // Convert to vector
    std::vector<DWORD> bytecode_vec(bytecode, bytecode + size_in_dwords);
    
    // Disassemble
    std::string assembly;
    if (!ShaderBytecodeDisassembler::disassemble(bytecode_vec, assembly)) {
        std::cerr << "Failed to disassemble shader\n";
        return false;
    }
    
    std::cout << "Disassembled shader:\n" << assembly << "\n";
    
    // Try to translate to GLSL
    DX8ShaderTranslator translator;
    std::string error_msg;
    
    if (!translator.parse_shader(assembly, error_msg)) {
        std::cerr << "Failed to parse disassembled shader: " << error_msg << "\n";
        return false;
    }
    
    std::string glsl = translator.generate_glsl();
    std::cout << "Generated GLSL:\n" << glsl << "\n";
    
    return true;
}

bool test_vertex_shader_loading() {
    std::cout << "=== Test: Vertex Shader Loading with Compiled vs_1_1 Shaders ===\n";
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    dx8gl_error init_result = dx8gl_init(&config);
    if (init_result != DX8GL_SUCCESS) {
        std::cerr << "Failed to initialize dx8gl: error code " << init_result << std::endl;
        return false;
    }
    
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
    
    // Test 1: Test disassembly of various shaders
    bool all_passed = true;
    
    all_passed &= test_vertex_shader_disassembly("Simple Transform", 
                                                 g_vs_transform, 
                                                 sizeof(g_vs_transform) / sizeof(DWORD));
    
    all_passed &= test_vertex_shader_disassembly("Color Passthrough", 
                                                 g_vs_color, 
                                                 sizeof(g_vs_color) / sizeof(DWORD));
    
    all_passed &= test_vertex_shader_disassembly("Texture Coordinate", 
                                                 g_vs_texcoord, 
                                                 sizeof(g_vs_texcoord) / sizeof(DWORD));
    
    all_passed &= test_vertex_shader_disassembly("Complex Operations", 
                                                 g_vs_complex, 
                                                 sizeof(g_vs_complex) / sizeof(DWORD));
    
    all_passed &= test_vertex_shader_disassembly("With Constants", 
                                                 g_vs_with_constants, 
                                                 sizeof(g_vs_with_constants) / sizeof(DWORD));
    
    // Test 2: Create actual vertex shaders in the device
    std::cout << "\nTest 2: Creating vertex shaders in device\n";
    
    struct ShaderTest {
        const char* name;
        const DWORD* bytecode;
        size_t size;
    };
    
    ShaderTest shader_tests[] = {
        {"Simple Transform", g_vs_transform, sizeof(g_vs_transform)},
        {"Color Passthrough", g_vs_color, sizeof(g_vs_color)},
        {"Texture Coordinate", g_vs_texcoord, sizeof(g_vs_texcoord)},
        {"Complex Operations", g_vs_complex, sizeof(g_vs_complex)},
        {"With Constants", g_vs_with_constants, sizeof(g_vs_with_constants)}
    };
    
    std::vector<DWORD> created_handles;
    
    for (const auto& test : shader_tests) {
        DWORD handle = 0;
        hr = device->CreateVertexShader(nullptr, test.bytecode, &handle, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to create vertex shader '" << test.name << "': " << hr << std::endl;
            all_passed = false;
        } else {
            std::cout << "Created vertex shader '" << test.name << "': handle=" << handle << "\n";
            created_handles.push_back(handle);
            
            // Try to set it as active
            hr = device->SetVertexShader(handle);
            if (FAILED(hr)) {
                std::cerr << "Failed to set vertex shader '" << test.name << "': " << hr << std::endl;
                all_passed = false;
            } else {
                std::cout << "Successfully set vertex shader '" << test.name << "' as active\n";
            }
        }
    }
    
    // Test 3: Verify shader function retrieval
    std::cout << "\nTest 3: Verifying shader function retrieval\n";
    
    if (!created_handles.empty()) {
        DWORD test_handle = created_handles[0];
        std::vector<DWORD> retrieved_function(256); // Buffer for retrieved function
        DWORD size_needed = retrieved_function.size() * sizeof(DWORD);
        
        hr = device->GetVertexShaderFunction(test_handle, retrieved_function.data(), &size_needed);
        if (FAILED(hr)) {
            std::cerr << "Failed to get vertex shader function: " << hr << std::endl;
            all_passed = false;
        } else {
            std::cout << "Retrieved vertex shader function, size: " << size_needed << " bytes\n";
            
            // Verify it matches original
            size_t original_size = sizeof(g_vs_transform);
            if (size_needed == original_size) {
                if (memcmp(retrieved_function.data(), g_vs_transform, original_size) == 0) {
                    std::cout << "PASS: Retrieved function matches original bytecode\n";
                } else {
                    std::cerr << "FAIL: Retrieved function doesn't match original\n";
                    all_passed = false;
                }
            } else {
                std::cerr << "FAIL: Retrieved function size doesn't match original\n";
                all_passed = false;
            }
        }
    }
    
    // Cleanup
    for (DWORD handle : created_handles) {
        device->DeleteVertexShader(handle);
    }
    
    device->Release();
    d3d8->Release();
    
    dx8gl_shutdown();
    
    std::cout << "\nVertex shader loading test completed!\n";
    return all_passed;
}

int main() {
    std::cout << "Running Vertex Shader Disassembly and Loading Tests\n";
    std::cout << "==================================================\n";
    
    bool success = test_vertex_shader_loading();
    
    if (success) {
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!\n";
        return 1;
    }
}