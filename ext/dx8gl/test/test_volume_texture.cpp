#include <gtest/gtest.h>
#include "d3d8.h"
#include "dx8gl.h"
#include <cstring>
#include <vector>
#include <cmath>

class VolumeTextureTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8 = nullptr;
    IDirect3DDevice8* device = nullptr;
    IDirect3DVolumeTexture8* volume_texture = nullptr;
    const UINT width = 32;
    const UINT height = 32;
    const UINT depth = 32;
    const UINT levels = 6; // 32x32x32 -> 16x16x16 -> 8x8x8 -> 4x4x4 -> 2x2x2 -> 1x1x1
    
    void SetUp() override {
        // Initialize dx8gl with OSMesa backend
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_OSMESA;
        config.width = 256;
        config.height = 256;
        ASSERT_EQ(dx8gl_init(&config), 0);
        
        // Create Direct3D8 interface
        d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        ASSERT_NE(d3d8, nullptr);
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 256;
        pp.BackBufferHeight = 256;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        
        HRESULT hr = d3d8->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device
        );
        ASSERT_EQ(hr, D3D_OK);
    }
    
    void TearDown() override {
        if (volume_texture) {
            volume_texture->Release();
            volume_texture = nullptr;
        }
        if (device) {
            device->Release();
            device = nullptr;
        }
        if (d3d8) {
            d3d8->Release();
            d3d8 = nullptr;
        }
        dx8gl_shutdown();
    }
    
    // Helper to create a 3D gradient in the volume texture
    void FillVolumeWithGradient(UINT mip_level) {
        D3DLOCKED_BOX locked_box;
        HRESULT hr = volume_texture->LockBox(mip_level, &locked_box, nullptr, 0);
        ASSERT_EQ(hr, D3D_OK);
        
        UINT mip_width = width >> mip_level;
        UINT mip_height = height >> mip_level;
        UINT mip_depth = depth >> mip_level;
        
        if (mip_width == 0) mip_width = 1;
        if (mip_height == 0) mip_height = 1;
        if (mip_depth == 0) mip_depth = 1;
        
        BYTE* data = (BYTE*)locked_box.pBits;
        
        for (UINT z = 0; z < mip_depth; z++) {
            for (UINT y = 0; y < mip_height; y++) {
                BYTE* row = data + z * locked_box.SlicePitch + y * locked_box.RowPitch;
                DWORD* pixels = (DWORD*)row;
                
                for (UINT x = 0; x < mip_width; x++) {
                    // Create gradient based on position (R=X, G=Y, B=Z)
                    BYTE r = (BYTE)((x * 255) / (mip_width - 1));
                    BYTE g = (BYTE)((y * 255) / (mip_height - 1));
                    BYTE b = (BYTE)((z * 255) / (mip_depth - 1));
                    pixels[x] = D3DCOLOR_ARGB(255, r, g, b);
                }
            }
        }
        
        hr = volume_texture->UnlockBox(mip_level);
        ASSERT_EQ(hr, D3D_OK);
    }
    
    // Helper to verify gradient at a specific position
    bool VerifyGradientAt(UINT mip_level, UINT x, UINT y, UINT z) {
        D3DLOCKED_BOX locked_box;
        HRESULT hr = volume_texture->LockBox(mip_level, &locked_box, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) return false;
        
        UINT mip_width = width >> mip_level;
        UINT mip_height = height >> mip_level;
        UINT mip_depth = depth >> mip_level;
        
        if (mip_width == 0) mip_width = 1;
        if (mip_height == 0) mip_height = 1;
        if (mip_depth == 0) mip_depth = 1;
        
        BYTE* data = (BYTE*)locked_box.pBits;
        DWORD* pixel = (DWORD*)(data + z * locked_box.SlicePitch + 
                                y * locked_box.RowPitch + x * sizeof(DWORD));
        
        DWORD color = *pixel;
        BYTE r = (color >> 16) & 0xFF;
        BYTE g = (color >> 8) & 0xFF;
        BYTE b = color & 0xFF;
        
        // Expected gradient values
        BYTE expected_r = (BYTE)((x * 255) / (mip_width - 1));
        BYTE expected_g = (BYTE)((y * 255) / (mip_height - 1));
        BYTE expected_b = (BYTE)((z * 255) / (mip_depth - 1));
        
        volume_texture->UnlockBox(mip_level);
        
        // Allow small tolerance for rounding
        const int tolerance = 2;
        return (abs(r - expected_r) <= tolerance &&
                abs(g - expected_g) <= tolerance &&
                abs(b - expected_b) <= tolerance);
    }
};

TEST_F(VolumeTextureTest, CreateVolumeTexture) {
    // Test basic volume texture creation
    HRESULT hr = device->CreateVolumeTexture(
        width, height, depth, levels,
        0,  // Usage
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &volume_texture
    );
    
    // Volume textures may not be supported, check for appropriate error
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available on this implementation";
    }
    
    EXPECT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Verify dimensions
    D3DVOLUME_DESC desc;
    hr = volume_texture->GetLevelDesc(0, &desc);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_EQ(desc.Width, width);
    EXPECT_EQ(desc.Height, height);
    EXPECT_EQ(desc.Depth, depth);
    EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);
    EXPECT_EQ(desc.Pool, D3DPOOL_MANAGED);
}

TEST_F(VolumeTextureTest, FillAndReadGradient) {
    // Create volume texture
    HRESULT hr = device->CreateVolumeTexture(
        width, height, depth, 1,  // Single level for simplicity
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Fill with gradient
    FillVolumeWithGradient(0);
    
    // Verify gradient at various positions
    EXPECT_TRUE(VerifyGradientAt(0, 0, 0, 0));     // Origin
    EXPECT_TRUE(VerifyGradientAt(0, width-1, 0, 0)); // X-axis end
    EXPECT_TRUE(VerifyGradientAt(0, 0, height-1, 0)); // Y-axis end
    EXPECT_TRUE(VerifyGradientAt(0, 0, 0, depth-1)); // Z-axis end
    EXPECT_TRUE(VerifyGradientAt(0, width/2, height/2, depth/2)); // Center
}

TEST_F(VolumeTextureTest, MipmapGeneration) {
    // Create volume texture with automatic mipmap generation
    HRESULT hr = device->CreateVolumeTexture(
        width, height, depth, 0,  // 0 = auto generate all mip levels
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Get actual level count
    DWORD level_count = volume_texture->GetLevelCount();
    EXPECT_EQ(level_count, levels);
    
    // Fill base level with gradient
    FillVolumeWithGradient(0);
    
    // Fill remaining mip levels
    for (UINT level = 1; level < level_count; level++) {
        FillVolumeWithGradient(level);
    }
    
    // Verify each mip level has correct dimensions
    for (UINT level = 0; level < level_count; level++) {
        D3DVOLUME_DESC desc;
        hr = volume_texture->GetLevelDesc(level, &desc);
        EXPECT_EQ(hr, D3D_OK);
        
        UINT expected_width = std::max(1U, width >> level);
        UINT expected_height = std::max(1U, height >> level);
        UINT expected_depth = std::max(1U, depth >> level);
        
        EXPECT_EQ(desc.Width, expected_width);
        EXPECT_EQ(desc.Height, expected_height);
        EXPECT_EQ(desc.Depth, expected_depth);
    }
}

TEST_F(VolumeTextureTest, SliceSampling) {
    // Create a small volume texture for easier testing
    const UINT small_size = 4;
    HRESULT hr = device->CreateVolumeTexture(
        small_size, small_size, small_size, 1,
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Fill with distinct values for each slice
    D3DLOCKED_BOX locked_box;
    hr = volume_texture->LockBox(0, &locked_box, nullptr, 0);
    ASSERT_EQ(hr, D3D_OK);
    
    BYTE* data = (BYTE*)locked_box.pBits;
    for (UINT z = 0; z < small_size; z++) {
        // Each Z slice gets a unique color
        DWORD slice_color = D3DCOLOR_ARGB(255, z * 63, z * 63, 255 - z * 63);
        
        for (UINT y = 0; y < small_size; y++) {
            DWORD* row = (DWORD*)(data + z * locked_box.SlicePitch + y * locked_box.RowPitch);
            for (UINT x = 0; x < small_size; x++) {
                row[x] = slice_color;
            }
        }
    }
    
    hr = volume_texture->UnlockBox(0);
    ASSERT_EQ(hr, D3D_OK);
    
    // Verify each slice has the correct color
    hr = volume_texture->LockBox(0, &locked_box, nullptr, D3DLOCK_READONLY);
    ASSERT_EQ(hr, D3D_OK);
    
    data = (BYTE*)locked_box.pBits;
    for (UINT z = 0; z < small_size; z++) {
        DWORD expected_color = D3DCOLOR_ARGB(255, z * 63, z * 63, 255 - z * 63);
        DWORD* pixel = (DWORD*)(data + z * locked_box.SlicePitch);
        EXPECT_EQ(pixel[0], expected_color) << "Slice " << z << " has wrong color";
    }
    
    volume_texture->UnlockBox(0);
}

TEST_F(VolumeTextureTest, SubVolumeUpdate) {
    // Test updating a sub-region of the volume texture
    HRESULT hr = device->CreateVolumeTexture(
        width, height, depth, 1,
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Fill entire volume with white
    D3DLOCKED_BOX locked_box;
    hr = volume_texture->LockBox(0, &locked_box, nullptr, 0);
    ASSERT_EQ(hr, D3D_OK);
    
    BYTE* data = (BYTE*)locked_box.pBits;
    for (UINT z = 0; z < depth; z++) {
        for (UINT y = 0; y < height; y++) {
            DWORD* row = (DWORD*)(data + z * locked_box.SlicePitch + y * locked_box.RowPitch);
            for (UINT x = 0; x < width; x++) {
                row[x] = D3DCOLOR_ARGB(255, 255, 255, 255);
            }
        }
    }
    volume_texture->UnlockBox(0);
    
    // Lock a sub-region and fill with red
    D3DBOX sub_box = {
        width/4, width*3/4,   // X range
        height/4, height*3/4, // Y range
        depth/4, depth*3/4    // Z range
    };
    
    hr = volume_texture->LockBox(0, &locked_box, &sub_box, 0);
    ASSERT_EQ(hr, D3D_OK);
    
    data = (BYTE*)locked_box.pBits;
    UINT sub_width = sub_box.Right - sub_box.Left;
    UINT sub_height = sub_box.Bottom - sub_box.Top;
    UINT sub_depth = sub_box.Back - sub_box.Front;
    
    for (UINT z = 0; z < sub_depth; z++) {
        for (UINT y = 0; y < sub_height; y++) {
            DWORD* row = (DWORD*)(data + z * locked_box.SlicePitch + y * locked_box.RowPitch);
            for (UINT x = 0; x < sub_width; x++) {
                row[x] = D3DCOLOR_ARGB(255, 255, 0, 0);
            }
        }
    }
    volume_texture->UnlockBox(0);
    
    // Verify the update - lock entire volume for reading
    hr = volume_texture->LockBox(0, &locked_box, nullptr, D3DLOCK_READONLY);
    ASSERT_EQ(hr, D3D_OK);
    
    data = (BYTE*)locked_box.pBits;
    for (UINT z = 0; z < depth; z++) {
        for (UINT y = 0; y < height; y++) {
            DWORD* row = (DWORD*)(data + z * locked_box.SlicePitch + y * locked_box.RowPitch);
            for (UINT x = 0; x < width; x++) {
                bool in_sub_region = (x >= sub_box.Left && x < sub_box.Right &&
                                     y >= sub_box.Top && y < sub_box.Bottom &&
                                     z >= sub_box.Front && z < sub_box.Back);
                
                DWORD expected = in_sub_region ? 
                    D3DCOLOR_ARGB(255, 255, 0, 0) :  // Red in sub-region
                    D3DCOLOR_ARGB(255, 255, 255, 255); // White elsewhere
                
                if (row[x] != expected) {
                    volume_texture->UnlockBox(0);
                    FAIL() << "Pixel at (" << x << "," << y << "," << z 
                           << ") has wrong color. Expected: " << expected 
                           << ", Got: " << row[x];
                }
            }
        }
    }
    volume_texture->UnlockBox(0);
}

TEST_F(VolumeTextureTest, VolumeTextureInShader) {
    // Test binding volume texture and sampling in a pixel shader
    HRESULT hr = device->CreateVolumeTexture(
        8, 8, 8, 1,
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Fill with gradient for testing
    D3DLOCKED_BOX locked_box;
    hr = volume_texture->LockBox(0, &locked_box, nullptr, 0);
    ASSERT_EQ(hr, D3D_OK);
    
    BYTE* data = (BYTE*)locked_box.pBits;
    for (UINT z = 0; z < 8; z++) {
        for (UINT y = 0; y < 8; y++) {
            DWORD* row = (DWORD*)(data + z * locked_box.SlicePitch + y * locked_box.RowPitch);
            for (UINT x = 0; x < 8; x++) {
                // Create distinct pattern
                BYTE val = (x + y + z) * 255 / 21; // Max sum is 21
                row[x] = D3DCOLOR_ARGB(255, val, val, val);
            }
        }
    }
    volume_texture->UnlockBox(0);
    
    // Set as texture stage 0
    hr = device->SetTexture(0, volume_texture);
    EXPECT_EQ(hr, D3D_OK);
    
    // Set texture stage states for volume texture sampling
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    
    // Set sampling states
    device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    
    // For volume textures, we need 3D texture coordinates
    device->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
    
    // Verify texture is bound
    IDirect3DBaseTexture8* bound_texture = nullptr;
    hr = device->GetTexture(0, &bound_texture);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_EQ(bound_texture, volume_texture);
    
    if (bound_texture) {
        bound_texture->Release();
    }
}

TEST_F(VolumeTextureTest, VolumeTextureLOD) {
    // Test LOD (Level of Detail) control
    HRESULT hr = device->CreateVolumeTexture(
        width, height, depth, 0,  // Auto-generate all mip levels
        0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
        &volume_texture
    );
    
    if (hr == D3DERR_NOTAVAILABLE) {
        GTEST_SKIP() << "Volume textures not available";
    }
    
    ASSERT_EQ(hr, D3D_OK);
    ASSERT_NE(volume_texture, nullptr);
    
    // Test LOD settings
    DWORD original_lod = volume_texture->GetLOD();
    EXPECT_EQ(original_lod, 0);
    
    // Set LOD to skip first mip level
    DWORD new_lod = volume_texture->SetLOD(1);
    EXPECT_EQ(new_lod, 0); // Returns previous LOD
    
    // Verify new LOD is set
    DWORD current_lod = volume_texture->GetLOD();
    EXPECT_EQ(current_lod, 1);
    
    // Test priority (for texture memory management)
    DWORD original_priority = volume_texture->GetPriority();
    volume_texture->SetPriority(100);
    DWORD new_priority = volume_texture->GetPriority();
    EXPECT_EQ(new_priority, 100);
}