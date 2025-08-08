#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <memory>
#include <cstring>
#include <vector>

using namespace dx8gl;

class VolumeTextureAndFrontBufferTest : public ::testing::Test {
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
    
    // Helper to fill volume texture with test data
    void FillVolumeTexture(IDirect3DVolumeTexture8* texture, UINT level, BYTE value) {
        D3DLOCKED_BOX locked_box;
        HRESULT hr = texture->LockBox(level, &locked_box, nullptr, 0);
        ASSERT_EQ(hr, D3D_OK);
        
        D3DVOLUME_DESC desc;
        texture->GetLevelDesc(level, &desc);
        
        BYTE* data = static_cast<BYTE*>(locked_box.pBits);
        
        // Fill with test pattern
        for (UINT z = 0; z < desc.Depth; z++) {
            for (UINT y = 0; y < desc.Height; y++) {
                BYTE* row = data + z * locked_box.SlicePitch + y * locked_box.RowPitch;
                for (UINT x = 0; x < desc.Width * 4; x++) { // Assume 4 bytes per pixel
                    row[x] = value + static_cast<BYTE>(x + y + z);
                }
            }
        }
        
        texture->UnlockBox(level);
    }
    
    // Helper to verify volume texture data
    bool VerifyVolumeTexture(IDirect3DVolumeTexture8* texture, UINT level, BYTE expected_value) {
        D3DLOCKED_BOX locked_box;
        HRESULT hr = texture->LockBox(level, &locked_box, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) return false;
        
        D3DVOLUME_DESC desc;
        texture->GetLevelDesc(level, &desc);
        
        BYTE* data = static_cast<BYTE*>(locked_box.pBits);
        bool matches = true;
        
        for (UINT z = 0; z < desc.Depth && matches; z++) {
            for (UINT y = 0; y < desc.Height && matches; y++) {
                BYTE* row = data + z * locked_box.SlicePitch + y * locked_box.RowPitch;
                for (UINT x = 0; x < desc.Width * 4 && matches; x++) {
                    BYTE expected = expected_value + static_cast<BYTE>(x + y + z);
                    if (row[x] != expected) {
                        matches = false;
                    }
                }
            }
        }
        
        texture->UnlockBox(level);
        return matches;
    }
};

// Volume texture tests
TEST_F(VolumeTextureAndFrontBufferTest, CreateVolumeTexture) {
    IDirect3DVolumeTexture8* volume = nullptr;
    
    // Create a volume texture
    HRESULT hr = device_->CreateVolumeTexture(
        64, 64, 32,  // Width, Height, Depth
        1,           // Levels
        0,           // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &volume
    );
    
    // Volume textures might not be supported
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not supported";
    }
    
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(volume, nullptr);
    
    if (volume) {
        // Verify properties
        D3DVOLUME_DESC desc;
        hr = volume->GetLevelDesc(0, &desc);
        EXPECT_EQ(hr, D3D_OK);
        EXPECT_EQ(desc.Width, 64u);
        EXPECT_EQ(desc.Height, 64u);
        EXPECT_EQ(desc.Depth, 32u);
        EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);
        
        volume->Release();
    }
}

TEST_F(VolumeTextureAndFrontBufferTest, UpdateVolumeTexture) {
    IDirect3DVolumeTexture8* src_volume = nullptr;
    IDirect3DVolumeTexture8* dst_volume = nullptr;
    
    // Create source volume texture
    HRESULT hr = device_->CreateVolumeTexture(
        32, 32, 16,  // Width, Height, Depth
        1,           // Levels
        0,           // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM,
        &src_volume
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not supported";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(src_volume, nullptr);
    
    // Create destination volume texture
    hr = device_->CreateVolumeTexture(
        32, 32, 16,  // Same dimensions
        1,           // Levels
        0,           // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT,
        &dst_volume
    );
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dst_volume, nullptr);
    
    // Fill source with test data
    FillVolumeTexture(src_volume, 0, 42);
    
    // Update destination from source
    hr = device_->UpdateTexture(src_volume, dst_volume);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify the data was copied (if we can lock the destination)
    // Note: D3DPOOL_DEFAULT textures might not be lockable
    
    // Clean up
    src_volume->Release();
    dst_volume->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, UpdateVolumeTextureWithMipLevels) {
    IDirect3DVolumeTexture8* src_volume = nullptr;
    IDirect3DVolumeTexture8* dst_volume = nullptr;
    
    // Create source volume texture with mip levels
    HRESULT hr = device_->CreateVolumeTexture(
        64, 64, 32,  // Width, Height, Depth
        0,           // Levels (0 = all mip levels)
        0,           // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM,
        &src_volume
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not supported";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(src_volume, nullptr);
    
    // Create destination volume texture with mip levels
    hr = device_->CreateVolumeTexture(
        64, 64, 32,  // Same dimensions
        0,           // Levels (0 = all mip levels)
        0,           // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT,
        &dst_volume
    );
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dst_volume, nullptr);
    
    // Get level counts
    DWORD src_levels = src_volume->GetLevelCount();
    DWORD dst_levels = dst_volume->GetLevelCount();
    EXPECT_GT(src_levels, 1u);
    EXPECT_EQ(src_levels, dst_levels);
    
    // Fill each mip level with different test data
    for (DWORD level = 0; level < src_levels; level++) {
        FillVolumeTexture(src_volume, level, static_cast<BYTE>(level * 10));
    }
    
    // Update destination from source
    hr = device_->UpdateTexture(src_volume, dst_volume);
    EXPECT_EQ(hr, D3D_OK);
    
    // Clean up
    src_volume->Release();
    dst_volume->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, UpdateVolumeTextureDifferentFormats) {
    IDirect3DVolumeTexture8* src_volume = nullptr;
    IDirect3DVolumeTexture8* dst_volume = nullptr;
    
    // Create source volume texture with one format
    HRESULT hr = device_->CreateVolumeTexture(
        32, 32, 16,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM,
        &src_volume
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not supported";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(src_volume, nullptr);
    
    // Create destination with different format (should fail to update)
    hr = device_->CreateVolumeTexture(
        32, 32, 16,
        1,
        0,
        D3DFMT_X8R8G8B8,  // Different format
        D3DPOOL_DEFAULT,
        &dst_volume
    );
    
    if (SUCCEEDED(hr)) {
        // Try to update - might fail due to format mismatch
        hr = device_->UpdateTexture(src_volume, dst_volume);
        // DirectX might allow this or might fail - implementation specific
        
        dst_volume->Release();
    }
    
    src_volume->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, UpdateVolumeTextureInvalidParams) {
    IDirect3DVolumeTexture8* volume = nullptr;
    IDirect3DTexture8* texture = nullptr;
    
    // Create a volume texture
    HRESULT hr = device_->CreateVolumeTexture(
        32, 32, 16,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &volume
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not supported";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume, nullptr);
    
    // Create a regular 2D texture
    hr = device_->CreateTexture(
        32, 32,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &texture
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(texture, nullptr);
    
    // Try to update between different resource types (should fail)
    hr = device_->UpdateTexture(
        reinterpret_cast<IDirect3DBaseTexture8*>(volume),
        reinterpret_cast<IDirect3DBaseTexture8*>(texture)
    );
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Try with null pointers
    hr = device_->UpdateTexture(nullptr, volume);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    hr = device_->UpdateTexture(volume, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    volume->Release();
    texture->Release();
}

// Front buffer tests
TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBuffer) {
    // Create a destination surface in system memory
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        640, 480,
        D3DFMT_A8R8G8B8,
        &dest_surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dest_surface, nullptr);
    
    // Clear the back buffer to a known color
    hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF00FF00, 1.0f, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    // Present to make it the "front buffer"
    hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(hr, D3D_OK);
    
    // Capture the front buffer
    hr = device_->GetFrontBuffer(dest_surface);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify we got something (lock and check a pixel)
    D3DLOCKED_RECT locked;
    hr = dest_surface->LockRect(&locked, nullptr, D3DLOCK_READONLY);
    EXPECT_EQ(hr, D3D_OK);
    
    if (SUCCEEDED(hr)) {
        // Check that we have data (not all zeros)
        DWORD* pixels = static_cast<DWORD*>(locked.pBits);
        bool has_data = false;
        for (int i = 0; i < 100; i++) {
            if (pixels[i] != 0) {
                has_data = true;
                break;
            }
        }
        EXPECT_TRUE(has_data) << "Front buffer appears to be empty";
        
        dest_surface->UnlockRect();
    }
    
    dest_surface->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBufferDifferentSize) {
    // Create a smaller destination surface
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        320, 240,  // Half size
        D3DFMT_A8R8G8B8,
        &dest_surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dest_surface, nullptr);
    
    // Capture the front buffer (should work with size mismatch)
    hr = device_->GetFrontBuffer(dest_surface);
    EXPECT_EQ(hr, D3D_OK);
    
    dest_surface->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBufferDifferentFormat) {
    // Create destination surface with different format
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        640, 480,
        D3DFMT_R5G6B5,  // 16-bit format
        &dest_surface
    );
    
    // Some formats might not be supported for image surfaces
    if (SUCCEEDED(hr)) {
        // Capture the front buffer (format conversion should happen)
        hr = device_->GetFrontBuffer(dest_surface);
        EXPECT_EQ(hr, D3D_OK);
        
        dest_surface->Release();
    }
}

TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBufferInvalidParams) {
    // Test with null pointer
    HRESULT hr = device_->GetFrontBuffer(nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test with non-system memory surface (render target)
    IDirect3DSurface8* rt_surface = nullptr;
    hr = device_->CreateRenderTarget(
        256, 256,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        FALSE,
        &rt_surface
    );
    
    if (SUCCEEDED(hr)) {
        // Should fail because destination is not system memory
        hr = device_->GetFrontBuffer(rt_surface);
        EXPECT_EQ(hr, D3DERR_INVALIDCALL);
        
        rt_surface->Release();
    }
}

TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBufferAfterRendering) {
    // Create destination surface
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        640, 480,
        D3DFMT_A8R8G8B8,
        &dest_surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dest_surface, nullptr);
    
    // Draw something
    hr = device_->BeginScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Clear to red
    hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFFFF0000, 1.0f, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->EndScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Present
    hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(hr, D3D_OK);
    
    // Capture the front buffer
    hr = device_->GetFrontBuffer(dest_surface);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify the captured content
    D3DLOCKED_RECT locked;
    hr = dest_surface->LockRect(&locked, nullptr, D3DLOCK_READONLY);
    EXPECT_EQ(hr, D3D_OK);
    
    if (SUCCEEDED(hr)) {
        // Check center pixel (should be reddish)
        DWORD* pixels = static_cast<DWORD*>(locked.pBits);
        DWORD center_pixel = pixels[240 * (locked.Pitch / 4) + 320];
        
        // Extract color components
        BYTE r = (center_pixel >> 16) & 0xFF;
        BYTE g = (center_pixel >> 8) & 0xFF;
        BYTE b = center_pixel & 0xFF;
        
        // Should be mostly red
        EXPECT_GT(r, 200) << "Red component should be high";
        EXPECT_LT(g, 50) << "Green component should be low";
        EXPECT_LT(b, 50) << "Blue component should be low";
        
        dest_surface->UnlockRect();
    }
    
    dest_surface->Release();
}

TEST_F(VolumeTextureAndFrontBufferTest, GetFrontBufferMultipleTimes) {
    // Create destination surface
    IDirect3DSurface8* dest_surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(
        640, 480,
        D3DFMT_A8R8G8B8,
        &dest_surface
    );
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(dest_surface, nullptr);
    
    // Capture multiple times
    for (int i = 0; i < 3; i++) {
        // Clear to different color each time
        DWORD colors[] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF };
        hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET, colors[i], 1.0f, 0);
        EXPECT_EQ(hr, D3D_OK);
        
        hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
        EXPECT_EQ(hr, D3D_OK);
        
        hr = device_->GetFrontBuffer(dest_surface);
        EXPECT_EQ(hr, D3D_OK);
    }
    
    dest_surface->Release();
}