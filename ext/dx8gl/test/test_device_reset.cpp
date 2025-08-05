#include <iostream>
#include <vector>
#include <memory>
#include "../src/d3d8.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_texture.h"
#include "../src/d3d8_vertexbuffer.h"
#include "../src/d3d8_indexbuffer.h"
#include "../src/logger.h"
#include "../src/dx8gl.h"

using namespace dx8gl;

// Test device reset with default pool resources
bool test_device_reset() {
    std::cout << "=== Test: Device Reset with Default Pool Resources ===" << std::endl;
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return false;
    }
    
    // Create device
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                   &pp, &device);
    if (FAILED(hr)) {
        std::cerr << "Failed to create device: " << hr << std::endl;
        d3d8->Release();
        return false;
    }
    
    std::cout << "Device created successfully" << std::endl;
    
    // Create resources in different pools
    IDirect3DTexture8* defaultTexture = nullptr;
    IDirect3DTexture8* managedTexture = nullptr;
    IDirect3DVertexBuffer8* defaultVB = nullptr;
    IDirect3DVertexBuffer8* managedVB = nullptr;
    IDirect3DIndexBuffer8* defaultIB = nullptr;
    IDirect3DIndexBuffer8* managedIB = nullptr;
    
    // Create texture in D3DPOOL_DEFAULT
    hr = device->CreateTexture(256, 256, 1, D3DUSAGE_RENDERTARGET, 
                               D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &defaultTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create default pool texture: " << hr << std::endl;
    } else {
        std::cout << "Created texture in D3DPOOL_DEFAULT" << std::endl;
    }
    
    // Create texture in D3DPOOL_MANAGED
    hr = device->CreateTexture(256, 256, 0, 0, 
                               D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &managedTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create managed pool texture: " << hr << std::endl;
    } else {
        std::cout << "Created texture in D3DPOOL_MANAGED" << std::endl;
    }
    
    // Create vertex buffer in D3DPOOL_DEFAULT
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    hr = device->CreateVertexBuffer(1024, D3DUSAGE_WRITEONLY, fvf, 
                                   D3DPOOL_DEFAULT, &defaultVB);
    if (FAILED(hr)) {
        std::cerr << "Failed to create default pool vertex buffer: " << hr << std::endl;
    } else {
        std::cout << "Created vertex buffer in D3DPOOL_DEFAULT" << std::endl;
    }
    
    // Create vertex buffer in D3DPOOL_MANAGED
    hr = device->CreateVertexBuffer(1024, 0, fvf, 
                                   D3DPOOL_MANAGED, &managedVB);
    if (FAILED(hr)) {
        std::cerr << "Failed to create managed pool vertex buffer: " << hr << std::endl;
    } else {
        std::cout << "Created vertex buffer in D3DPOOL_MANAGED" << std::endl;
    }
    
    // Create index buffer in D3DPOOL_DEFAULT
    hr = device->CreateIndexBuffer(512, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, 
                                  D3DPOOL_DEFAULT, &defaultIB);
    if (FAILED(hr)) {
        std::cerr << "Failed to create default pool index buffer: " << hr << std::endl;
    } else {
        std::cout << "Created index buffer in D3DPOOL_DEFAULT" << std::endl;
    }
    
    // Create index buffer in D3DPOOL_MANAGED
    hr = device->CreateIndexBuffer(512, 0, D3DFMT_INDEX16, 
                                  D3DPOOL_MANAGED, &managedIB);
    if (FAILED(hr)) {
        std::cerr << "Failed to create managed pool index buffer: " << hr << std::endl;
    } else {
        std::cout << "Created index buffer in D3DPOOL_MANAGED" << std::endl;
    }
    
    // Get GL resource IDs before reset
    GLuint texGL = 0, vbGL = 0, ibGL = 0;
    if (defaultTexture) {
        auto* tex = static_cast<Direct3DTexture8*>(defaultTexture);
        texGL = tex->get_gl_texture();
        std::cout << "Default texture GL ID before reset: " << texGL << std::endl;
    }
    if (defaultVB) {
        auto* vb = static_cast<Direct3DVertexBuffer8*>(defaultVB);
        vbGL = vb->get_vbo();
        std::cout << "Default VB GL ID before reset: " << vbGL << std::endl;
    }
    if (defaultIB) {
        auto* ib = static_cast<Direct3DIndexBuffer8*>(defaultIB);
        ibGL = ib->get_ibo();
        std::cout << "Default IB GL ID before reset: " << ibGL << std::endl;
    }
    
    std::cout << "\nPerforming device reset..." << std::endl;
    
    // Reset device with new parameters
    D3DPRESENT_PARAMETERS newPP = pp;
    newPP.BackBufferWidth = 800;
    newPP.BackBufferHeight = 600;
    
    hr = device->Reset(&newPP);
    if (FAILED(hr)) {
        std::cerr << "Device reset failed: " << hr << std::endl;
        // Cleanup
        if (defaultTexture) defaultTexture->Release();
        if (managedTexture) managedTexture->Release();
        if (defaultVB) defaultVB->Release();
        if (managedVB) managedVB->Release();
        if (defaultIB) defaultIB->Release();
        if (managedIB) managedIB->Release();
        device->Release();
        d3d8->Release();
        return false;
    }
    
    std::cout << "Device reset successful!" << std::endl;
    
    // Check GL resource IDs after reset
    bool success = true;
    if (defaultTexture) {
        auto* tex = static_cast<Direct3DTexture8*>(defaultTexture);
        GLuint newTexGL = tex->get_gl_texture();
        std::cout << "Default texture GL ID after reset: " << newTexGL;
        if (newTexGL != texGL && newTexGL != 0) {
            std::cout << " (recreated)" << std::endl;
        } else if (newTexGL == 0) {
            std::cout << " (FAILED to recreate)" << std::endl;
            success = false;
        } else {
            std::cout << " (unchanged - ERROR)" << std::endl;
            success = false;
        }
    }
    
    if (defaultVB) {
        auto* vb = static_cast<Direct3DVertexBuffer8*>(defaultVB);
        GLuint newVbGL = vb->get_vbo();
        std::cout << "Default VB GL ID after reset: " << newVbGL;
        if (newVbGL != vbGL && newVbGL != 0) {
            std::cout << " (recreated)" << std::endl;
        } else if (newVbGL == 0) {
            std::cout << " (FAILED to recreate)" << std::endl;
            success = false;
        } else {
            std::cout << " (unchanged - ERROR)" << std::endl;
            success = false;
        }
    }
    
    if (defaultIB) {
        auto* ib = static_cast<Direct3DIndexBuffer8*>(defaultIB);
        GLuint newIbGL = ib->get_ibo();
        std::cout << "Default IB GL ID after reset: " << newIbGL;
        if (newIbGL != ibGL && newIbGL != 0) {
            std::cout << " (recreated)" << std::endl;
        } else if (newIbGL == 0) {
            std::cout << " (FAILED to recreate)" << std::endl;
            success = false;
        } else {
            std::cout << " (unchanged - ERROR)" << std::endl;
            success = false;
        }
    }
    
    // Test that we can still use the resources
    std::cout << "\nTesting resource usage after reset..." << std::endl;
    
    // Clear to verify device is working
    hr = device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                       D3DCOLOR_XRGB(64, 128, 192), 1.0f, 0);
    if (FAILED(hr)) {
        std::cerr << "Clear failed after reset: " << hr << std::endl;
        success = false;
    } else {
        std::cout << "Clear successful after reset" << std::endl;
    }
    
    // Cleanup
    if (defaultTexture) defaultTexture->Release();
    if (managedTexture) managedTexture->Release();
    if (defaultVB) defaultVB->Release();
    if (managedVB) managedVB->Release();
    if (defaultIB) defaultIB->Release();
    if (managedIB) managedIB->Release();
    device->Release();
    d3d8->Release();
    
    return success;
}

int main() {
    std::cout << "Running Device Reset Tests" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    dx8gl_error init_result = dx8gl_init(&config);
    if (init_result != DX8GL_SUCCESS) {
        std::cerr << "Failed to initialize dx8gl: error code " << init_result << std::endl;
        return 1;
    }
    
    bool result = test_device_reset();
    
    std::cout << "\n=========================" << std::endl;
    if (result) {
        std::cout << "All tests PASSED!" << std::endl;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
    }
    
    dx8gl_shutdown();
    
    return result ? 0 : 1;
}