#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_additional_swapchain.h"
#include <memory>
#include <vector>

using namespace dx8gl;

class AdditionalSwapChainTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8_ = nullptr;
    IDirect3DDevice8* device_ = nullptr;
    
    void SetUp() override {
        // Initialize dx8gl
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_OSMESA;
        
        ASSERT_EQ(dx8gl_init(&config), DX8GL_SUCCESS);
        
        // Create Direct3D8 interface
        d3d8_ = Direct3DCreate8(D3D_SDK_VERSION);
        ASSERT_NE(d3d8_, nullptr);
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640;
        pp.BackBufferHeight = 480;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        pp.BackBufferCount = 1;
        
        HRESULT hr = d3d8_->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device_
        );
        ASSERT_EQ(hr, D3D_OK);
        ASSERT_NE(device_, nullptr);
    }
    
    void TearDown() override {
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        
        if (d3d8_) {
            d3d8_->Release();
            d3d8_ = nullptr;
        }
        
        dx8gl_shutdown();
    }
};

TEST_F(AdditionalSwapChainTest, CreateSingleAdditionalSwapChain) {
    // Create additional swap chain with different dimensions
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.hDeviceWindow = reinterpret_cast<HWND>(0x1234);  // Dummy window handle
    pp.BackBufferCount = 1;
    
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    
    EXPECT_EQ(hr, D3D_OK) << "CreateAdditionalSwapChain should succeed";
    EXPECT_NE(swap_chain, nullptr) << "Swap chain should not be null";
    
    if (swap_chain) {
        // Get back buffer from swap chain
        IDirect3DSurface8* back_buffer = nullptr;
        hr = swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        EXPECT_EQ(hr, D3D_OK) << "GetBackBuffer should succeed";
        EXPECT_NE(back_buffer, nullptr) << "Back buffer should not be null";
        
        if (back_buffer) {
            // Verify surface description
            D3DSURFACE_DESC desc;
            hr = back_buffer->GetDesc(&desc);
            EXPECT_EQ(hr, D3D_OK);
            EXPECT_EQ(desc.Width, 800u);
            EXPECT_EQ(desc.Height, 600u);
            EXPECT_EQ(desc.Format, D3DFMT_X8R8G8B8);
            
            back_buffer->Release();
        }
        
        swap_chain->Release();
    }
}

TEST_F(AdditionalSwapChainTest, CreateMultipleAdditionalSwapChains) {
    std::vector<IDirect3DSwapChain8*> swap_chains;
    const int num_chains = 3;
    
    // Create multiple swap chains with different dimensions
    for (int i = 0; i < num_chains; i++) {
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640 + i * 100;  // 640, 740, 840
        pp.BackBufferHeight = 480 + i * 50;  // 480, 530, 580
        pp.hDeviceWindow = reinterpret_cast<HWND>(0x1000 + i);
        pp.BackBufferCount = 2;  // Multiple back buffers
        
        IDirect3DSwapChain8* swap_chain = nullptr;
        HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
        
        EXPECT_EQ(hr, D3D_OK) << "Failed to create swap chain " << i;
        EXPECT_NE(swap_chain, nullptr) << "Swap chain " << i << " is null";
        
        if (swap_chain) {
            swap_chains.push_back(swap_chain);
        }
    }
    
    // Verify all swap chains were created
    EXPECT_EQ(swap_chains.size(), static_cast<size_t>(num_chains));
    
    // Verify each swap chain has correct properties
    for (size_t i = 0; i < swap_chains.size(); i++) {
        IDirect3DSurface8* back_buffer = nullptr;
        HRESULT hr = swap_chains[i]->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get back buffer for chain " << i;
        
        if (back_buffer) {
            D3DSURFACE_DESC desc;
            hr = back_buffer->GetDesc(&desc);
            EXPECT_EQ(hr, D3D_OK);
            EXPECT_EQ(desc.Width, 640u + i * 100);
            EXPECT_EQ(desc.Height, 480u + i * 50);
            
            back_buffer->Release();
        }
    }
    
    // Clean up swap chains
    for (auto* chain : swap_chains) {
        chain->Release();
    }
}

TEST_F(AdditionalSwapChainTest, PresentToWindow) {
    // Create additional swap chain
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.hDeviceWindow = reinterpret_cast<HWND>(0x5678);
    pp.BackBufferCount = 1;
    
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(swap_chain, nullptr);
    
    // Get back buffer
    IDirect3DSurface8* back_buffer = nullptr;
    hr = swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(back_buffer, nullptr);
    
    // Lock and fill back buffer with test pattern
    D3DLOCKED_RECT locked_rect;
    hr = back_buffer->LockRect(&locked_rect, nullptr, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    if (SUCCEEDED(hr)) {
        uint32_t* pixels = (uint32_t*)locked_rect.pBits;
        for (int y = 0; y < 600; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * locked_rect.Pitch);
            for (int x = 0; x < 800; x++) {
                // Create gradient pattern
                uint8_t r = (x * 255) / 800;
                uint8_t g = (y * 255) / 600;
                uint8_t b = 128;
                row[x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
        
        hr = back_buffer->UnlockRect();
        EXPECT_EQ(hr, D3D_OK);
    }
    
    // Present the swap chain (will attempt to display to window)
    hr = swap_chain->Present(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(hr, D3D_OK) << "Present should succeed";
    
    // Clean up
    back_buffer->Release();
    swap_chain->Release();
}

TEST_F(AdditionalSwapChainTest, DeviceResetWithAdditionalSwapChains) {
    // Create two additional swap chains
    std::vector<D3DPRESENT_PARAMETERS> chain_params;
    
    for (int i = 0; i < 2; i++) {
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640 + i * 100;
        pp.BackBufferHeight = 480 + i * 100;
        pp.hDeviceWindow = reinterpret_cast<HWND>(0x2000 + i);
        pp.BackBufferCount = 1;
        
        chain_params.push_back(pp);
        
        IDirect3DSwapChain8* swap_chain = nullptr;
        HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
        EXPECT_EQ(hr, D3D_OK);
        
        if (swap_chain) {
            // Don't hold reference during reset
            swap_chain->Release();
        }
    }
    
    // Reset device with new parameters
    D3DPRESENT_PARAMETERS reset_params = {};
    reset_params.Windowed = TRUE;
    reset_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    reset_params.BackBufferFormat = D3DFMT_X8R8G8B8;
    reset_params.BackBufferWidth = 1024;
    reset_params.BackBufferHeight = 768;
    reset_params.EnableAutoDepthStencil = TRUE;
    reset_params.AutoDepthStencilFormat = D3DFMT_D24S8;
    reset_params.BackBufferCount = 1;
    
    HRESULT hr = device_->Reset(&reset_params);
    EXPECT_EQ(hr, D3D_OK) << "Device reset should succeed";
    
    // Verify device is functional after reset
    IDirect3DSurface8* back_buffer = nullptr;
    hr = device_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
    EXPECT_EQ(hr, D3D_OK);
    
    if (back_buffer) {
        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 1024u);
        EXPECT_EQ(desc.Height, 768u);
        
        back_buffer->Release();
    }
}

TEST_F(AdditionalSwapChainTest, MultipleBackBuffers) {
    // Create swap chain with multiple back buffers
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.hDeviceWindow = reinterpret_cast<HWND>(0x3000);
    pp.BackBufferCount = 3;  // Triple buffering
    
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(swap_chain, nullptr);
    
    // Get all back buffers
    std::vector<IDirect3DSurface8*> back_buffers;
    for (UINT i = 0; i < 3; i++) {
        IDirect3DSurface8* buffer = nullptr;
        hr = swap_chain->GetBackBuffer(i, D3DBACKBUFFER_TYPE_MONO, &buffer);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get back buffer " << i;
        EXPECT_NE(buffer, nullptr) << "Back buffer " << i << " is null";
        
        if (buffer) {
            back_buffers.push_back(buffer);
            
            // Verify each buffer has correct dimensions
            D3DSURFACE_DESC desc;
            hr = buffer->GetDesc(&desc);
            EXPECT_EQ(hr, D3D_OK);
            EXPECT_EQ(desc.Width, 800u);
            EXPECT_EQ(desc.Height, 600u);
        }
    }
    
    // Should have gotten all 3 buffers
    EXPECT_EQ(back_buffers.size(), 3u);
    
    // Clean up
    for (auto* buffer : back_buffers) {
        buffer->Release();
    }
    swap_chain->Release();
}

TEST_F(AdditionalSwapChainTest, InvalidParameters) {
    // Test with null parameters
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(nullptr, &swap_chain);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL) << "Should fail with null parameters";
    
    // Test with null output pointer
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    hr = device_->CreateAdditionalSwapChain(&pp, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL) << "Should fail with null output pointer";
    
    // Test with zero dimensions
    pp.BackBufferWidth = 0;
    pp.BackBufferHeight = 480;
    hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL) << "Should fail with zero width";
    
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 0;
    hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL) << "Should fail with zero height";
}

// Test to verify swap chain reference counting
TEST_F(AdditionalSwapChainTest, ReferenceCountingTest) {
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.hDeviceWindow = reinterpret_cast<HWND>(0x4000);
    pp.BackBufferCount = 1;
    
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(swap_chain, nullptr);
    
    // Initial ref count should be 1
    ULONG ref = swap_chain->AddRef();
    EXPECT_EQ(ref, 2u) << "After AddRef, count should be 2";
    
    ref = swap_chain->Release();
    EXPECT_EQ(ref, 1u) << "After Release, count should be 1";
    
    // Final release should destroy the object
    ref = swap_chain->Release();
    EXPECT_EQ(ref, 0u) << "Final release should return 0";
}