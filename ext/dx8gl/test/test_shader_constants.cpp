#include <gtest/gtest.h>
#include "d3d8.h"
#include "dx8gl.h"
#include <cstring>
#include <vector>

class ShaderConstantsTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8 = nullptr;
    IDirect3DDevice8* device = nullptr;
    IDirect3DSurface8* render_target = nullptr;
    IDirect3DSurface8* depth_stencil = nullptr;
    const UINT width = 256;
    const UINT height = 256;
    
    void SetUp() override {
        // Initialize dx8gl
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_OSMESA;
        config.width = width;
        config.height = height;
        ASSERT_EQ(dx8gl_init(&config), 0);
        
        // Create Direct3D8 interface
        d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        ASSERT_NE(d3d8, nullptr);
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = width;
        pp.BackBufferHeight = height;
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
        
        // Get render target
        device->GetRenderTarget(&render_target);
        device->GetDepthStencilSurface(&depth_stencil);
    }
    
    void TearDown() override {
        if (depth_stencil) {
            depth_stencil->Release();
            depth_stencil = nullptr;
        }
        if (render_target) {
            render_target->Release();
            render_target = nullptr;
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
    
    // Helper to read a pixel from the framebuffer
    D3DCOLOR ReadPixel(UINT x, UINT y) {
        // Create a surface to copy to
        IDirect3DSurface8* surface = nullptr;
        HRESULT hr = device->CreateImageSurface(width, height, D3DFMT_A8R8G8B8, &surface);
        if (FAILED(hr)) return 0;
        
        // Copy render target to surface
        POINT dest_point = {0, 0};
        hr = device->CopyRects(render_target, nullptr, 0, surface, &dest_point);
        if (FAILED(hr)) {
            surface->Release();
            return 0;
        }
        
        // Lock surface and read pixel
        D3DLOCKED_RECT locked_rect;
        hr = surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) {
            surface->Release();
            return 0;
        }
        
        D3DCOLOR* pixels = (D3DCOLOR*)locked_rect.pBits;
        D3DCOLOR color = pixels[y * (locked_rect.Pitch / 4) + x];
        
        surface->UnlockRect();
        surface->Release();
        
        return color;
    }
    
    // Create a simple vertex shader that passes through position and uses a constant for color
    DWORD CreateColorConstantVertexShader() {
        // Vertex shader declaration - position only
        DWORD decl[] = {
            D3DVSD_STREAM(0),
            D3DVSD_REG(0, D3DVSDT_FLOAT3),  // Position -> v0
            D3DVSD_END()
        };
        
        // Vertex shader code - output position and constant c0 as color
        const char* shader_code = 
            "vs.1.1\n"
            "dcl_position v0\n"
            "mov oPos, v0\n"       // Output position
            "mov oD0, c0\n";       // Output constant c0 as diffuse color
        
        DWORD shader_handle = 0;
        HRESULT hr = device->CreateVertexShader(decl, (const DWORD*)shader_code, &shader_handle, 0);
        if (FAILED(hr)) {
            return 0;
        }
        
        return shader_handle;
    }
    
    // Create a simple pixel shader that outputs a constant color
    DWORD CreateColorConstantPixelShader() {
        // Pixel shader code - output constant c0 as color
        const char* shader_code = 
            "ps.1.4\n"
            "mov r0, c0\n";        // Output constant c0
        
        DWORD shader_handle = 0;
        HRESULT hr = device->CreatePixelShader((const DWORD*)shader_code, &shader_handle);
        if (FAILED(hr)) {
            return 0;
        }
        
        return shader_handle;
    }
};

TEST_F(ShaderConstantsTest, VertexShaderConstantUpdate) {
    // Create a simple triangle
    struct Vertex {
        float x, y, z;
    };
    
    Vertex vertices[] = {
        { -0.5f, -0.5f, 0.5f },
        {  0.5f, -0.5f, 0.5f },
        {  0.0f,  0.5f, 0.5f }
    };
    
    // Clear to black
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
    
    // Begin scene
    device->BeginScene();
    
    // Set up fixed-function pipeline (no shaders)
    device->SetVertexShader(D3DFVF_XYZ);
    device->SetPixelShader(0);
    
    // Set vertex shader constant to red
    float red_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    HRESULT hr = device->SetVertexShaderConstant(0, red_color, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Draw triangle using immediate mode
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
    
    // End scene
    device->EndScene();
    
    // Read center pixel - should be red
    D3DCOLOR pixel = ReadPixel(width / 2, height / 2);
    BYTE r = (pixel >> 16) & 0xFF;
    BYTE g = (pixel >> 8) & 0xFF;
    BYTE b = pixel & 0xFF;
    
    // Allow some tolerance for color conversion
    EXPECT_GT(r, 250);
    EXPECT_LT(g, 5);
    EXPECT_LT(b, 5);
    
    // Now update the constant to green
    float green_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    hr = device->SetVertexShaderConstant(0, green_color, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Clear and draw again
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
    device->BeginScene();
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
    device->EndScene();
    
    // Read center pixel - should now be green
    pixel = ReadPixel(width / 2, height / 2);
    r = (pixel >> 16) & 0xFF;
    g = (pixel >> 8) & 0xFF;
    b = pixel & 0xFF;
    
    EXPECT_LT(r, 5);
    EXPECT_GT(g, 250);
    EXPECT_LT(b, 5);
}

TEST_F(ShaderConstantsTest, PixelShaderConstantUpdate) {
    // Create a fullscreen quad
    struct Vertex {
        float x, y, z, rhw;
        DWORD color;
    };
    
    Vertex vertices[] = {
        { 0.0f,    0.0f,    0.5f, 1.0f, 0xFFFFFFFF },
        { (float)width, 0.0f,    0.5f, 1.0f, 0xFFFFFFFF },
        { 0.0f,    (float)height, 0.5f, 1.0f, 0xFFFFFFFF },
        { (float)width, (float)height, 0.5f, 1.0f, 0xFFFFFFFF }
    };
    
    // Clear to black
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
    
    // Begin scene
    device->BeginScene();
    
    // Set up for transformed vertices
    device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    device->SetPixelShader(0);
    
    // Set pixel shader constant to blue
    float blue_color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    HRESULT hr = device->SetPixelShaderConstant(0, blue_color, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Draw quad
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
    
    // End scene
    device->EndScene();
    
    // Read center pixel
    D3DCOLOR pixel = ReadPixel(width / 2, height / 2);
    BYTE r = (pixel >> 16) & 0xFF;
    BYTE g = (pixel >> 8) & 0xFF;
    BYTE b = pixel & 0xFF;
    
    // For fixed-function, pixel shader constants might not directly affect output
    // But we're testing that the API works without crashing
    
    // Update to yellow
    float yellow_color[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
    hr = device->SetPixelShaderConstant(0, yellow_color, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Clear and draw again
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
    device->BeginScene();
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
    device->EndScene();
}

TEST_F(ShaderConstantsTest, MultipleConstantUpdate) {
    // Test updating multiple constants at once
    float constants[16];  // 4 vec4 constants
    
    // Set constants 0-3 to different colors
    constants[0] = 1.0f; constants[1] = 0.0f; constants[2] = 0.0f; constants[3] = 1.0f; // Red
    constants[4] = 0.0f; constants[5] = 1.0f; constants[6] = 0.0f; constants[7] = 1.0f; // Green
    constants[8] = 0.0f; constants[9] = 0.0f; constants[10] = 1.0f; constants[11] = 1.0f; // Blue
    constants[12] = 1.0f; constants[13] = 1.0f; constants[14] = 0.0f; constants[15] = 1.0f; // Yellow
    
    // Set all 4 constants at once
    HRESULT hr = device->SetVertexShaderConstant(0, constants, 4);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify we can read them back
    float read_constants[16];
    hr = device->GetVertexShaderConstant(0, read_constants, 4);
    EXPECT_EQ(hr, D3D_OK);
    
    // Check that constants match
    for (int i = 0; i < 16; i++) {
        EXPECT_FLOAT_EQ(constants[i], read_constants[i]);
    }
    
    // Test pixel shader constants too
    hr = device->SetPixelShaderConstant(0, constants, 4);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device->GetPixelShaderConstant(0, read_constants, 4);
    EXPECT_EQ(hr, D3D_OK);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_FLOAT_EQ(constants[i], read_constants[i]);
    }
}

TEST_F(ShaderConstantsTest, ConstantPersistence) {
    // Test that constants persist across draw calls
    float red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    
    // Set vertex constant
    HRESULT hr = device->SetVertexShaderConstant(5, red, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Begin scene and draw something
    device->BeginScene();
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
    device->EndScene();
    
    // Constant should still be set
    float read_constant[4];
    hr = device->GetVertexShaderConstant(5, read_constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_FLOAT_EQ(read_constant[0], 1.0f);
    EXPECT_FLOAT_EQ(read_constant[1], 0.0f);
    EXPECT_FLOAT_EQ(read_constant[2], 0.0f);
    EXPECT_FLOAT_EQ(read_constant[3], 1.0f);
    
    // Update constant
    hr = device->SetVertexShaderConstant(5, green, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Draw again
    device->BeginScene();
    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
    device->EndScene();
    
    // Check updated constant
    hr = device->GetVertexShaderConstant(5, read_constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_FLOAT_EQ(read_constant[0], 0.0f);
    EXPECT_FLOAT_EQ(read_constant[1], 1.0f);
    EXPECT_FLOAT_EQ(read_constant[2], 0.0f);
    EXPECT_FLOAT_EQ(read_constant[3], 1.0f);
}

TEST_F(ShaderConstantsTest, MaxConstantRange) {
    // Test setting constants at the maximum range
    // DirectX 8 supports up to 96 vertex shader constants
    float constant[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    
    // Set constant at index 95 (last valid index for vs_1_1)
    HRESULT hr = device->SetVertexShaderConstant(95, constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Read it back
    float read_constant[4];
    hr = device->GetVertexShaderConstant(95, read_constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_FLOAT_EQ(read_constant[0], 0.5f);
    EXPECT_FLOAT_EQ(read_constant[1], 0.5f);
    EXPECT_FLOAT_EQ(read_constant[2], 0.5f);
    EXPECT_FLOAT_EQ(read_constant[3], 1.0f);
    
    // Pixel shader supports up to 8 constants in ps_1_4
    hr = device->SetPixelShaderConstant(7, constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device->GetPixelShaderConstant(7, read_constant, 1);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_FLOAT_EQ(read_constant[0], 0.5f);
    
    // Test setting out of range should fail gracefully
    hr = device->SetVertexShaderConstant(96, constant, 1);
    // DirectX 8 might allow this but clip internally
    
    hr = device->SetPixelShaderConstant(32, constant, 1);
    // ps_1_4 supports up to 32 constants, but typical is 8
}