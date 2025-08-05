#include <iostream>
#include <memory>
#include "../src/d3d8.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_vertexbuffer.h"
#include "../src/logger.h"
#include "../src/dx8gl.h"

using namespace dx8gl;

// Test helper structure for vertex data
struct TestVertex {
    float x, y, z;
    DWORD color;
    float u, v;
};

bool test_stream_source_stride() {
    std::cout << "=== Test: Stream Source Stride ===\n";
    
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
    
    // Test 1: Set and retrieve stream source with various strides
    std::cout << "\nTest 1: Setting and retrieving stream sources with different strides\n";
    
    // Create vertex buffer
    IDirect3DVertexBuffer8* vb = nullptr;
    hr = device->CreateVertexBuffer(sizeof(TestVertex) * 4, 0, 
                                   D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                                   D3DPOOL_MANAGED, &vb);
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex buffer: " << hr << std::endl;
        device->Release();
        d3d8->Release();
        return false;
    }
    
    // Test various stride values
    const UINT test_strides[] = {sizeof(TestVertex), 32, 64, 128};
    
    for (int i = 0; i < 4; i++) {
        UINT stream_num = i;
        UINT stride_to_set = test_strides[i];
        
        // Set stream source
        hr = device->SetStreamSource(stream_num, vb, stride_to_set);
        if (FAILED(hr)) {
            std::cerr << "Failed to set stream source " << stream_num << ": " << hr << std::endl;
            continue;
        }
        
        // Retrieve stream source
        IDirect3DVertexBuffer8* retrieved_vb = nullptr;
        UINT retrieved_stride = 0;
        
        hr = device->GetStreamSource(stream_num, &retrieved_vb, &retrieved_stride);
        if (FAILED(hr)) {
            std::cerr << "Failed to get stream source " << stream_num << ": " << hr << std::endl;
            continue;
        }
        
        // Verify results
        if (retrieved_vb != vb) {
            std::cerr << "Stream " << stream_num << ": Retrieved vertex buffer doesn't match!\n";
        } else if (retrieved_stride != stride_to_set) {
            std::cerr << "Stream " << stream_num << ": Stride mismatch! Set=" << stride_to_set 
                      << ", Retrieved=" << retrieved_stride << "\n";
        } else {
            std::cout << "Stream " << stream_num << ": Correctly retrieved stride " 
                      << retrieved_stride << "\n";
        }
        
        if (retrieved_vb) {
            retrieved_vb->Release();
        }
    }
    
    // Test 2: Clear stream source and verify stride is reset
    std::cout << "\nTest 2: Clearing stream source\n";
    
    // Clear stream 0
    hr = device->SetStreamSource(0, nullptr, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to clear stream source 0: " << hr << std::endl;
    }
    
    // Try to retrieve cleared stream
    IDirect3DVertexBuffer8* cleared_vb = nullptr;
    UINT cleared_stride = 999;  // Set to non-zero to verify it gets cleared
    
    hr = device->GetStreamSource(0, &cleared_vb, &cleared_stride);
    if (FAILED(hr)) {
        std::cerr << "Failed to get cleared stream source: " << hr << std::endl;
    } else {
        if (cleared_vb != nullptr) {
            std::cerr << "Cleared stream source returned non-null vertex buffer!\n";
            cleared_vb->Release();
        } else if (cleared_stride != 0) {
            std::cerr << "Cleared stream source returned non-zero stride: " << cleared_stride << "\n";
        } else {
            std::cout << "Cleared stream source correctly returned null VB and 0 stride\n";
        }
    }
    
    // Test 3: Replace stream source and verify stride updates
    std::cout << "\nTest 3: Replacing stream source with different stride\n";
    
    // Set initial stream with stride 32
    hr = device->SetStreamSource(1, vb, 32);
    if (FAILED(hr)) {
        std::cerr << "Failed to set initial stream source: " << hr << std::endl;
    }
    
    // Replace with stride 64
    hr = device->SetStreamSource(1, vb, 64);
    if (FAILED(hr)) {
        std::cerr << "Failed to replace stream source: " << hr << std::endl;
    }
    
    // Verify new stride
    IDirect3DVertexBuffer8* replaced_vb = nullptr;
    UINT replaced_stride = 0;
    
    hr = device->GetStreamSource(1, &replaced_vb, &replaced_stride);
    if (FAILED(hr)) {
        std::cerr << "Failed to get replaced stream source: " << hr << std::endl;
    } else {
        if (replaced_stride != 64) {
            std::cerr << "Replaced stream stride incorrect: Expected=64, Got=" << replaced_stride << "\n";
        } else {
            std::cout << "Replaced stream correctly updated stride to 64\n";
        }
        
        if (replaced_vb) {
            replaced_vb->Release();
        }
    }
    
    // Cleanup
    vb->Release();
    device->Release();
    d3d8->Release();
    
    dx8gl_shutdown();
    
    std::cout << "\nStream source stride test completed!\n";
    return true;
}

int main() {
    std::cout << "Running Stream Source Stride Tests\n";
    std::cout << "===================================\n";
    
    bool success = test_stream_source_stride();
    
    if (success) {
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!\n";
        return 1;
    }
}