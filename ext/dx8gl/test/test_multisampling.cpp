#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_surface.h"
#include <memory>

using namespace dx8gl;

class MultisamplingTest : public ::testing::Test {
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
    
    bool CreateDeviceWithMultisampling(D3DMULTISAMPLE_TYPE multisample_type) {
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640;
        pp.BackBufferHeight = 480;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        pp.MultiSampleType = multisample_type;
        
        HRESULT hr = d3d8_->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device_
        );
        
        return SUCCEEDED(hr);
    }
};

// Test CheckDeviceMultiSampleType for different sample counts
TEST_F(MultisamplingTest, CheckDeviceMultiSampleType) {
    // Test no multisampling (should always be supported)
    HRESULT hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_NONE
    );
    EXPECT_EQ(hr, D3D_OK);
    
    // Test 2x MSAA (commonly supported)
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_2_SAMPLES
    );
    EXPECT_EQ(hr, D3D_OK);
    
    // Test 4x MSAA (commonly supported)
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_4_SAMPLES
    );
    EXPECT_EQ(hr, D3D_OK);
    
    // Test 8x MSAA (commonly supported)
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_8_SAMPLES
    );
    EXPECT_EQ(hr, D3D_OK);
    
    // Test 3x MSAA (rarely supported)
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_3_SAMPLES
    );
    EXPECT_EQ(hr, D3DERR_NOTAVAILABLE);
    
    // Test depth formats
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_D24S8,
        TRUE,
        D3DMULTISAMPLE_4_SAMPLES
    );
    EXPECT_EQ(hr, D3D_OK);
}

// Test device creation with multisampling
TEST_F(MultisamplingTest, CreateDeviceWithMSAA) {
    // Test creating device without MSAA
    EXPECT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_NONE));
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
    
    // Test creating device with 2x MSAA
    EXPECT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_2_SAMPLES));
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
    
    // Test creating device with 4x MSAA
    EXPECT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_4_SAMPLES));
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
}

// Test creating multisampled render targets
TEST_F(MultisamplingTest, CreateMultisampledRenderTarget) {
    // Create device first
    ASSERT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_NONE));
    
    // Create non-multisampled render target
    IDirect3DSurface8* rt_no_msaa = nullptr;
    HRESULT hr = device_->CreateRenderTarget(
        512, 512,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        FALSE,
        &rt_no_msaa
    );
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(rt_no_msaa, nullptr);
    
    if (rt_no_msaa) {
        // Check surface description
        D3DSURFACE_DESC desc;
        hr = rt_no_msaa->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 512u);
        EXPECT_EQ(desc.Height, 512u);
        EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);
        EXPECT_EQ(desc.MultiSampleType, D3DMULTISAMPLE_NONE);
        
        rt_no_msaa->Release();
    }
    
    // Create 4x multisampled render target
    IDirect3DSurface8* rt_4x_msaa = nullptr;
    hr = device_->CreateRenderTarget(
        512, 512,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_4_SAMPLES,
        FALSE,
        &rt_4x_msaa
    );
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(rt_4x_msaa, nullptr);
    
    if (rt_4x_msaa) {
        // Check surface description
        D3DSURFACE_DESC desc;
        hr = rt_4x_msaa->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 512u);
        EXPECT_EQ(desc.Height, 512u);
        EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);
        EXPECT_EQ(desc.MultiSampleType, D3DMULTISAMPLE_4_SAMPLES);
        
        rt_4x_msaa->Release();
    }
}

// Test creating multisampled depth stencil surfaces
TEST_F(MultisamplingTest, CreateMultisampledDepthStencil) {
    // Create device first
    ASSERT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_NONE));
    
    // Create non-multisampled depth stencil
    IDirect3DSurface8* ds_no_msaa = nullptr;
    HRESULT hr = device_->CreateDepthStencilSurface(
        512, 512,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE,
        &ds_no_msaa
    );
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(ds_no_msaa, nullptr);
    
    if (ds_no_msaa) {
        ds_no_msaa->Release();
    }
    
    // Create 4x multisampled depth stencil
    IDirect3DSurface8* ds_4x_msaa = nullptr;
    hr = device_->CreateDepthStencilSurface(
        512, 512,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_4_SAMPLES,
        &ds_4x_msaa
    );
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(ds_4x_msaa, nullptr);
    
    if (ds_4x_msaa) {
        ds_4x_msaa->Release();
    }
}

// Test setting multisampled render targets
TEST_F(MultisamplingTest, SetMultisampledRenderTarget) {
    // Create device first
    ASSERT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_NONE));
    
    // Create multisampled render target and depth stencil
    IDirect3DSurface8* rt = nullptr;
    IDirect3DSurface8* ds = nullptr;
    
    HRESULT hr = device_->CreateRenderTarget(
        512, 512,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_4_SAMPLES,
        FALSE,
        &rt
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(rt, nullptr);
    
    hr = device_->CreateDepthStencilSurface(
        512, 512,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_4_SAMPLES,
        &ds
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(ds, nullptr);
    
    // Set multisampled render target
    hr = device_->SetRenderTarget(rt, ds);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get current render target to verify
    IDirect3DSurface8* current_rt = nullptr;
    hr = device_->GetRenderTarget(&current_rt);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(current_rt, nullptr);
    
    if (current_rt) {
        D3DSURFACE_DESC desc;
        hr = current_rt->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.MultiSampleType, D3DMULTISAMPLE_4_SAMPLES);
        
        current_rt->Release();
    }
    
    // Cleanup
    rt->Release();
    ds->Release();
}

// Test rendering to multisampled surface
TEST_F(MultisamplingTest, RenderToMultisampledSurface) {
    // Create device with 4x MSAA
    ASSERT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_4_SAMPLES));
    
    // Clear the multisampled back buffer
    HRESULT hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                D3DCOLOR_XRGB(128, 0, 255), 1.0f, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    // Begin scene
    hr = device_->BeginScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Set simple render states
    device_->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // End scene
    hr = device_->EndScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Present (should internally resolve MSAA if needed)
    hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(hr, D3D_OK);
}

// Test MSAA support for different formats
TEST_F(MultisamplingTest, MSAAFormatSupport) {
    // Test common color formats
    D3DFORMAT color_formats[] = {
        D3DFMT_X8R8G8B8,
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A1R5G5B5
    };
    
    for (D3DFORMAT format : color_formats) {
        HRESULT hr = d3d8_->CheckDeviceMultiSampleType(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            format,
            TRUE,
            D3DMULTISAMPLE_4_SAMPLES
        );
        EXPECT_EQ(hr, D3D_OK) << "Format " << format << " should support 4x MSAA";
    }
    
    // Test depth formats
    D3DFORMAT depth_formats[] = {
        D3DFMT_D16,
        D3DFMT_D24S8,
        D3DFMT_D24X8,
        D3DFMT_D32
    };
    
    for (D3DFORMAT format : depth_formats) {
        HRESULT hr = d3d8_->CheckDeviceMultiSampleType(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            format,
            TRUE,
            D3DMULTISAMPLE_4_SAMPLES
        );
        EXPECT_EQ(hr, D3D_OK) << "Depth format " << format << " should support 4x MSAA";
    }
}

// Test invalid MSAA parameters
TEST_F(MultisamplingTest, InvalidMSAAParameters) {
    // Test invalid adapter
    HRESULT hr = d3d8_->CheckDeviceMultiSampleType(
        999,  // Invalid adapter
        D3DDEVTYPE_HAL,
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_4_SAMPLES
    );
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test invalid device type
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_SW,  // Software device
        D3DFMT_X8R8G8B8,
        TRUE,
        D3DMULTISAMPLE_4_SAMPLES
    );
    EXPECT_EQ(hr, D3DERR_NOTAVAILABLE);
    
    // Test unsupported format
    hr = d3d8_->CheckDeviceMultiSampleType(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        D3DFMT_DXT1,  // Compressed texture format
        TRUE,
        D3DMULTISAMPLE_4_SAMPLES
    );
    EXPECT_EQ(hr, D3DERR_NOTAVAILABLE);
}

// Test multisampling with additional swap chains
TEST_F(MultisamplingTest, MultisamplingWithAdditionalSwapChain) {
    // Create device first
    ASSERT_TRUE(CreateDeviceWithMultisampling(D3DMULTISAMPLE_NONE));
    
    // Create additional swap chain with multisampling
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = TRUE;
    
    IDirect3DSwapChain8* swap_chain = nullptr;
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    
    // Note: CreateAdditionalSwapChain might not be fully implemented yet
    if (SUCCEEDED(hr) && swap_chain) {
        // Get back buffer from swap chain
        IDirect3DSurface8* back_buffer = nullptr;
        hr = swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        
        if (SUCCEEDED(hr) && back_buffer) {
            D3DSURFACE_DESC desc;
            hr = back_buffer->GetDesc(&desc);
            EXPECT_EQ(hr, D3D_OK);
            EXPECT_EQ(desc.MultiSampleType, D3DMULTISAMPLE_4_SAMPLES);
            
            back_buffer->Release();
        }
        
        swap_chain->Release();
    }
}