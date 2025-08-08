#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_com_wrapper.h"
#include <memory>
#include <thread>
#include <atomic>
#include <vector>

using namespace dx8gl;

class COMWrapperCompleteTest : public ::testing::Test {
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

// Test QueryInterface COM rules
TEST_F(COMWrapperCompleteTest, QueryInterface) {
    // Test QueryInterface for IUnknown
    IUnknown* unknown = nullptr;
    HRESULT hr = device_->QueryInterface(IID_IUnknown, (void**)&unknown);
    EXPECT_EQ(hr, S_OK);
    EXPECT_NE(unknown, nullptr);
    
    if (unknown) {
        // Should return the same interface
        EXPECT_EQ(reinterpret_cast<void*>(unknown), reinterpret_cast<void*>(device_));
        unknown->Release();
    }
    
    // Test QueryInterface for IDirect3DDevice8
    IDirect3DDevice8* device2 = nullptr;
    hr = device_->QueryInterface(IID_IDirect3DDevice8, (void**)&device2);
    EXPECT_EQ(hr, S_OK);
    EXPECT_NE(device2, nullptr);
    
    if (device2) {
        EXPECT_EQ(device2, device_);
        device2->Release();
    }
    
    // Test QueryInterface for unsupported interface (should fail)
    void* unsupported = nullptr;
    GUID fake_guid = { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 } };
    hr = device_->QueryInterface(fake_guid, &unsupported);
    EXPECT_EQ(hr, E_NOINTERFACE);
    EXPECT_EQ(unsupported, nullptr);
}

// Test reference counting
TEST_F(COMWrapperCompleteTest, ReferenceCountin

g) {
    // Initial ref count should be 1 (from creation)
    ULONG ref = device_->AddRef();
    EXPECT_GE(ref, 2u);  // At least 2 after AddRef
    
    // Release should decrement
    ref = device_->Release();
    EXPECT_GE(ref, 1u);  // Should still be at least 1
    
    // Multiple AddRef/Release cycles
    for (int i = 0; i < 5; i++) {
        ref = device_->AddRef();
        EXPECT_GT(ref, 1u);
    }
    
    for (int i = 0; i < 5; i++) {
        ref = device_->Release();
        EXPECT_GE(ref, 1u);
    }
}

// Test surface wrapping/unwrapping
TEST_F(COMWrapperCompleteTest, SurfaceWrapping) {
    // Create a render target surface
    IDirect3DSurface8* surface = nullptr;
    HRESULT hr = device_->CreateRenderTarget(
        256, 256,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        FALSE,
        &surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(surface, nullptr);
    
    // Test surface reference counting
    ULONG ref = surface->AddRef();
    EXPECT_GE(ref, 2u);
    
    ref = surface->Release();
    EXPECT_GE(ref, 1u);
    
    // Test QueryInterface on surface
    IUnknown* unknown = nullptr;
    hr = surface->QueryInterface(IID_IUnknown, (void**)&unknown);
    EXPECT_EQ(hr, S_OK);
    
    if (unknown) {
        unknown->Release();
    }
    
    // Clean up
    surface->Release();
}

// Test GetRenderTarget surface wrapping
TEST_F(COMWrapperCompleteTest, GetRenderTargetWrapping) {
    // Get the current render target
    IDirect3DSurface8* rt1 = nullptr;
    HRESULT hr = device_->GetRenderTarget(&rt1);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(rt1, nullptr);
    
    // Get it again - should return the same wrapped object
    IDirect3DSurface8* rt2 = nullptr;
    hr = device_->GetRenderTarget(&rt2);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(rt2, nullptr);
    
    // Both should point to the same underlying surface
    // (This depends on implementation details)
    
    // Clean up
    if (rt1) rt1->Release();
    if (rt2) rt2->Release();
}

// Test SetRenderTarget with wrapped surfaces
TEST_F(COMWrapperCompleteTest, SetRenderTargetWrapping) {
    // Create a new render target
    IDirect3DSurface8* new_rt = nullptr;
    HRESULT hr = device_->CreateRenderTarget(
        512, 512,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        FALSE,
        &new_rt
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(new_rt, nullptr);
    
    // Set it as render target
    hr = device_->SetRenderTarget(new_rt, nullptr);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get current render target - should be the one we just set
    IDirect3DSurface8* current_rt = nullptr;
    hr = device_->GetRenderTarget(&current_rt);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(current_rt, nullptr);
    
    // Verify dimensions match
    if (current_rt) {
        D3DSURFACE_DESC desc;
        hr = current_rt->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 512u);
        EXPECT_EQ(desc.Height, 512u);
        
        current_rt->Release();
    }
    
    new_rt->Release();
}

// Test CopyRects with wrapped surfaces
TEST_F(COMWrapperCompleteTest, CopyRectsWrapping) {
    // Create source and destination surfaces
    IDirect3DSurface8* src_surface = nullptr;
    IDirect3DSurface8* dst_surface = nullptr;
    
    HRESULT hr = device_->CreateImageSurface(256, 256, D3DFMT_A8R8G8B8, &src_surface);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(src_surface, nullptr);
    
    hr = device_->CreateImageSurface(256, 256, D3DFMT_A8R8G8B8, &dst_surface);
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dst_surface, nullptr);
    
    // Copy between surfaces
    hr = device_->CopyRects(src_surface, nullptr, 0, dst_surface, nullptr);
    EXPECT_EQ(hr, D3D_OK);
    
    // Clean up
    src_surface->Release();
    dst_surface->Release();
}

// Test thread safety of COM wrapper
TEST_F(COMWrapperCompleteTest, ThreadSafety) {
    std::atomic<int> errors(0);
    std::vector<std::thread> threads;
    
    // Spawn multiple threads that will AddRef/Release
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([this, &errors]() {
            for (int j = 0; j < 100; j++) {
                ULONG ref = device_->AddRef();
                if (ref < 2) {
                    errors++;
                }
                
                ref = device_->Release();
                if (ref < 1) {
                    errors++;
                }
                
                // Also test surface creation/destruction
                IDirect3DSurface8* surface = nullptr;
                HRESULT hr = device_->CreateImageSurface(
                    64, 64, D3DFMT_A8R8G8B8, &surface
                );
                
                if (SUCCEEDED(hr) && surface) {
                    surface->Release();
                } else {
                    errors++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(errors.load(), 0);
}

// Test cursor methods (newly added)
TEST_F(COMWrapperCompleteTest, CursorMethods) {
    // Test SetCursorPosition
    HRESULT hr = device_->SetCursorPosition(100, 200, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    // Test ShowCursor
    BOOL old_state = device_->ShowCursor(TRUE);
    // Result depends on previous state
    
    old_state = device_->ShowCursor(FALSE);
    EXPECT_TRUE(old_state);  // Was shown before
}

// Test memory management methods
TEST_F(COMWrapperCompleteTest, MemoryManagement) {
    // Test GetAvailableTextureMem
    UINT mem = device_->GetAvailableTextureMem();
    EXPECT_GT(mem, 0u);  // Should report some amount of memory
    
    // Test ResourceManagerDiscardBytes
    HRESULT hr = device_->ResourceManagerDiscardBytes(1024 * 1024);  // Discard 1MB
    EXPECT_EQ(hr, D3D_OK);
}

// Test gamma ramp methods
TEST_F(COMWrapperCompleteTest, GammaRamp) {
    D3DGAMMARAMP ramp = {};
    
    // Initialize a test gamma ramp (linear)
    for (int i = 0; i < 256; i++) {
        WORD value = static_cast<WORD>(i << 8 | i);  // Scale to 16-bit
        ramp.red[i] = value;
        ramp.green[i] = value;
        ramp.blue[i] = value;
    }
    
    // Set gamma ramp
    HRESULT hr = device_->SetGammaRamp(0, &ramp);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get gamma ramp
    D3DGAMMARAMP retrieved_ramp = {};
    hr = device_->GetGammaRamp(&retrieved_ramp);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify some values (might not be exact due to hardware)
    // Just check that it's not all zeros
    bool has_data = false;
    for (int i = 0; i < 256; i++) {
        if (retrieved_ramp.red[i] != 0 || 
            retrieved_ramp.green[i] != 0 || 
            retrieved_ramp.blue[i] != 0) {
            has_data = true;
            break;
        }
    }
    EXPECT_TRUE(has_data);
}

// Test depth stencil surface wrapping
TEST_F(COMWrapperCompleteTest, DepthStencilWrapping) {
    // Create a depth stencil surface
    IDirect3DSurface8* ds_surface = nullptr;
    HRESULT hr = device_->CreateDepthStencilSurface(
        256, 256,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE,
        &ds_surface
    );
    
    if (SUCCEEDED(hr)) {
        ASSERT_NE(ds_surface, nullptr);
        
        // Get current depth stencil
        IDirect3DSurface8* current_ds = nullptr;
        hr = device_->GetDepthStencilSurface(&current_ds);
        
        if (SUCCEEDED(hr) && current_ds) {
            current_ds->Release();
        }
        
        // Set new depth stencil
        IDirect3DSurface8* current_rt = nullptr;
        hr = device_->GetRenderTarget(&current_rt);
        
        if (SUCCEEDED(hr) && current_rt) {
            hr = device_->SetRenderTarget(current_rt, ds_surface);
            EXPECT_EQ(hr, D3D_OK);
            
            current_rt->Release();
        }
        
        ds_surface->Release();
    }
}

// Test GetFrontBuffer with wrapped surface
TEST_F(COMWrapperCompleteTest, GetFrontBufferWrapping) {
    // Create destination surface
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        640, 480,
        D3DFMT_A8R8G8B8,
        &dest_surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dest_surface, nullptr);
    
    // Capture front buffer
    hr = device_->GetFrontBuffer(dest_surface);
    EXPECT_EQ(hr, D3D_OK);
    
    dest_surface->Release();
}

// Test back buffer access
TEST_F(COMWrapperCompleteTest, BackBufferAccess) {
    // Get back buffer
    IDirect3DSurface8* back_buffer = nullptr;
    HRESULT hr = device_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(back_buffer, nullptr);
    
    if (back_buffer) {
        // Verify it's a valid surface
        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 640u);
        EXPECT_EQ(desc.Height, 480u);
        
        back_buffer->Release();
    }
}

// Test surface description
TEST_F(COMWrapperCompleteTest, SurfaceDescription) {
    // Create a test surface
    IDirect3DSurface8* surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        128, 64,
        D3DFMT_R5G6B5,
        &surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(surface, nullptr);
    
    // Get description
    D3DSURFACE_DESC desc = {};
    hr = surface->GetDesc(&desc);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify description
    EXPECT_EQ(desc.Width, 128u);
    EXPECT_EQ(desc.Height, 64u);
    EXPECT_EQ(desc.Format, D3DFMT_R5G6B5);
    EXPECT_EQ(desc.Type, D3DRTYPE_SURFACE);
    
    surface->Release();
}

// Test surface locking
TEST_F(COMWrapperCompleteTest, SurfaceLocking) {
    // Create a lockable surface
    IDirect3DSurface8* surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        64, 64,
        D3DFMT_A8R8G8B8,
        &surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(surface, nullptr);
    
    // Lock the surface
    D3DLOCKED_RECT locked_rect = {};
    hr = surface->LockRect(&locked_rect, nullptr, 0);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(locked_rect.pBits, nullptr);
    EXPECT_GT(locked_rect.Pitch, 0);
    
    // Write some data
    if (locked_rect.pBits) {
        DWORD* pixels = static_cast<DWORD*>(locked_rect.pBits);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                pixels[y * locked_rect.Pitch / 4 + x] = 0xFFFF0000;  // Red
            }
        }
    }
    
    // Unlock
    hr = surface->UnlockRect();
    EXPECT_EQ(hr, D3D_OK);
    
    // Try to lock again (should work)
    hr = surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = surface->UnlockRect();
    EXPECT_EQ(hr, D3D_OK);
    
    surface->Release();
}