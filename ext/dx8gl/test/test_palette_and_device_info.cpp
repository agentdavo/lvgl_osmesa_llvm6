#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <memory>
#include <cstring>

using namespace dx8gl;

class PaletteAndDeviceInfoTest : public ::testing::Test {
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

// Palette tests
TEST_F(PaletteAndDeviceInfoTest, SetAndGetPaletteEntries) {
    PALETTEENTRY entries[256];
    
    // Create a test palette with gradient
    for (int i = 0; i < 256; i++) {
        entries[i].peRed = static_cast<BYTE>(i);
        entries[i].peGreen = static_cast<BYTE>(255 - i);
        entries[i].peBlue = static_cast<BYTE>(i / 2);
        entries[i].peFlags = 0;
    }
    
    // Set palette 0
    HRESULT hr = device_->SetPaletteEntries(0, entries);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get palette back
    PALETTEENTRY retrieved[256];
    hr = device_->GetPaletteEntries(0, retrieved);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify all entries match
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(retrieved[i].peRed, entries[i].peRed) << "Mismatch at index " << i;
        EXPECT_EQ(retrieved[i].peGreen, entries[i].peGreen) << "Mismatch at index " << i;
        EXPECT_EQ(retrieved[i].peBlue, entries[i].peBlue) << "Mismatch at index " << i;
        EXPECT_EQ(retrieved[i].peFlags, entries[i].peFlags) << "Mismatch at index " << i;
    }
}

TEST_F(PaletteAndDeviceInfoTest, MultiplePalettes) {
    PALETTEENTRY palette1[256];
    PALETTEENTRY palette2[256];
    
    // Create two different palettes
    for (int i = 0; i < 256; i++) {
        // Palette 1: Red gradient
        palette1[i].peRed = static_cast<BYTE>(i);
        palette1[i].peGreen = 0;
        palette1[i].peBlue = 0;
        palette1[i].peFlags = 0;
        
        // Palette 2: Blue gradient
        palette2[i].peRed = 0;
        palette2[i].peGreen = 0;
        palette2[i].peBlue = static_cast<BYTE>(i);
        palette2[i].peFlags = 0;
    }
    
    // Set both palettes
    HRESULT hr = device_->SetPaletteEntries(0, palette1);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->SetPaletteEntries(1, palette2);
    EXPECT_EQ(hr, D3D_OK);
    
    // Retrieve and verify both palettes
    PALETTEENTRY retrieved[256];
    
    hr = device_->GetPaletteEntries(0, retrieved);
    EXPECT_EQ(hr, D3D_OK);
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(retrieved[i].peRed, palette1[i].peRed);
        EXPECT_EQ(retrieved[i].peGreen, palette1[i].peGreen);
        EXPECT_EQ(retrieved[i].peBlue, palette1[i].peBlue);
    }
    
    hr = device_->GetPaletteEntries(1, retrieved);
    EXPECT_EQ(hr, D3D_OK);
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(retrieved[i].peRed, palette2[i].peRed);
        EXPECT_EQ(retrieved[i].peGreen, palette2[i].peGreen);
        EXPECT_EQ(retrieved[i].peBlue, palette2[i].peBlue);
    }
}

TEST_F(PaletteAndDeviceInfoTest, SetCurrentTexturePalette) {
    PALETTEENTRY entries[256];
    
    // Create a test palette
    for (int i = 0; i < 256; i++) {
        entries[i].peRed = static_cast<BYTE>(i);
        entries[i].peGreen = static_cast<BYTE>(i);
        entries[i].peBlue = static_cast<BYTE>(i);
        entries[i].peFlags = 0;
    }
    
    // Set palette 5
    HRESULT hr = device_->SetPaletteEntries(5, entries);
    EXPECT_EQ(hr, D3D_OK);
    
    // Set current palette to 5
    hr = device_->SetCurrentTexturePalette(5);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get current palette
    UINT currentPalette = 0;
    hr = device_->GetCurrentTexturePalette(&currentPalette);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_EQ(currentPalette, 5u);
}

TEST_F(PaletteAndDeviceInfoTest, GetUnsetPalette) {
    PALETTEENTRY retrieved[256];
    
    // Get an unset palette (should return black)
    HRESULT hr = device_->GetPaletteEntries(10, retrieved);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify all entries are black
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(retrieved[i].peRed, 0);
        EXPECT_EQ(retrieved[i].peGreen, 0);
        EXPECT_EQ(retrieved[i].peBlue, 0);
        EXPECT_EQ(retrieved[i].peFlags, 0);
    }
}

TEST_F(PaletteAndDeviceInfoTest, SetCurrentPaletteToUnset) {
    // Try to set current palette to an unset palette
    HRESULT hr = device_->SetCurrentTexturePalette(50);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(PaletteAndDeviceInfoTest, InvalidPaletteNumber) {
    PALETTEENTRY entries[256] = {};
    
    // Try to set palette beyond maximum (256)
    HRESULT hr = device_->SetPaletteEntries(256, entries);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    hr = device_->SetPaletteEntries(1000, entries);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Try to get palette beyond maximum
    hr = device_->GetPaletteEntries(256, entries);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(PaletteAndDeviceInfoTest, NullPointerHandling) {
    PALETTEENTRY entries[256] = {};
    
    // Test null pointer for SetPaletteEntries
    HRESULT hr = device_->SetPaletteEntries(0, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test null pointer for GetPaletteEntries
    hr = device_->GetPaletteEntries(0, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test null pointer for GetCurrentTexturePalette
    hr = device_->GetCurrentTexturePalette(nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

// Device info tests
TEST_F(PaletteAndDeviceInfoTest, GetInfoVCache) {
    // Test vertex cache info
    struct VCacheInfo {
        DWORD OptMethod;
        DWORD CacheSize;
        DWORD MagicNumber;
    };
    
    VCacheInfo info = {};
    const DWORD D3DDEVINFOID_VCACHE = 1;
    
    HRESULT hr = device_->GetInfo(D3DDEVINFOID_VCACHE, &info, sizeof(VCacheInfo));
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify we got reasonable values
    EXPECT_EQ(info.OptMethod, 1u);  // Simple optimization
    EXPECT_EQ(info.CacheSize, 32u); // Typical cache size
    EXPECT_EQ(info.MagicNumber, 0u); // No magic number for software renderer
}

TEST_F(PaletteAndDeviceInfoTest, GetInfoResourceManager) {
    // Test resource manager info
    struct ResourceManagerInfo {
        DWORD NumCreated;
        DWORD NumManaged;
        DWORD NumEvictions;
        DWORD BytesDownloaded;
    };
    
    ResourceManagerInfo info = {};
    const DWORD D3DDEVINFOID_RESOURCEMANAGER = 2;
    
    HRESULT hr = device_->GetInfo(D3DDEVINFOID_RESOURCEMANAGER, &info, sizeof(ResourceManagerInfo));
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify we got zeros (software renderer has no resource management)
    EXPECT_EQ(info.NumCreated, 0u);
    EXPECT_EQ(info.NumManaged, 0u);
    EXPECT_EQ(info.NumEvictions, 0u);
    EXPECT_EQ(info.BytesDownloaded, 0u);
}

TEST_F(PaletteAndDeviceInfoTest, GetInfoVertexStats) {
    // Test vertex statistics
    struct VertexStats {
        DWORD NumRenderedTriangles;
        DWORD NumExtraClippingTriangles;
    };
    
    VertexStats info = {};
    const DWORD D3DDEVINFOID_VERTEXSTATS = 3;
    
    HRESULT hr = device_->GetInfo(D3DDEVINFOID_VERTEXSTATS, &info, sizeof(VertexStats));
    EXPECT_EQ(hr, D3D_OK);
    
    // Stats should be zero initially
    EXPECT_EQ(info.NumRenderedTriangles, 0u);
    EXPECT_EQ(info.NumExtraClippingTriangles, 0u);
}

TEST_F(PaletteAndDeviceInfoTest, GetInfoInvalidID) {
    DWORD buffer[10] = {};
    
    // Test with unknown info ID
    HRESULT hr = device_->GetInfo(9999, buffer, sizeof(buffer));
    EXPECT_EQ(hr, D3DERR_NOTAVAILABLE);
}

TEST_F(PaletteAndDeviceInfoTest, GetInfoNullPointer) {
    const DWORD D3DDEVINFOID_VCACHE = 1;
    
    // Test with null pointer
    HRESULT hr = device_->GetInfo(D3DDEVINFOID_VCACHE, nullptr, 16);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(PaletteAndDeviceInfoTest, GetInfoInsufficientBuffer) {
    struct VCacheInfo {
        DWORD OptMethod;
        DWORD CacheSize;
        DWORD MagicNumber;
    };
    
    VCacheInfo info = {};
    const DWORD D3DDEVINFOID_VCACHE = 1;
    
    // Test with buffer too small
    HRESULT hr = device_->GetInfo(D3DDEVINFOID_VCACHE, &info, sizeof(DWORD));
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(PaletteAndDeviceInfoTest, PalettePersistence) {
    PALETTEENTRY entries[256];
    
    // Create a unique palette
    for (int i = 0; i < 256; i++) {
        entries[i].peRed = static_cast<BYTE>(i ^ 0xAA);
        entries[i].peGreen = static_cast<BYTE>(i ^ 0x55);
        entries[i].peBlue = static_cast<BYTE>(i ^ 0xFF);
        entries[i].peFlags = 0;
    }
    
    // Set palette 42
    HRESULT hr = device_->SetPaletteEntries(42, entries);
    EXPECT_EQ(hr, D3D_OK);
    
    // Set and get other palettes
    PALETTEENTRY other[256] = {};
    hr = device_->SetPaletteEntries(0, other);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->SetPaletteEntries(100, other);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify palette 42 is still intact
    PALETTEENTRY retrieved[256];
    hr = device_->GetPaletteEntries(42, retrieved);
    EXPECT_EQ(hr, D3D_OK);
    
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(retrieved[i].peRed, entries[i].peRed);
        EXPECT_EQ(retrieved[i].peGreen, entries[i].peGreen);
        EXPECT_EQ(retrieved[i].peBlue, entries[i].peBlue);
        EXPECT_EQ(retrieved[i].peFlags, entries[i].peFlags);
    }
}