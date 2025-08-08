#include "backend_param_test.h"
#include "golden_image_utils.h"
#include "../src/d3d8_device.h"
#include "../src/d3dx8.h"
#include <vector>
#include <cmath>

using namespace dx8gl;

class CrossBackendEnhancedTest : public BackendParamTest {
protected:
    // Helper to render a volume-textured cube
    void RenderVolumeTexturedCube() {
        RequireDevice();
        
        if (!SupportsVolumeTextures(backend_)) {
            GTEST_SKIP() << "Backend " << GetBackendName() << " doesn't support volume textures";
        }
        
        // Clear the scene
        device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                      D3DCOLOR_XRGB(32, 32, 64), 1.0f, 0);
        
        // Create a volume texture
        IDirect3DVolumeTexture8* volume_tex = nullptr;
        HRESULT hr = device_->CreateVolumeTexture(
            32, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &volume_tex);
        
        if (FAILED(hr) || !volume_tex) {
            GTEST_SKIP() << "Failed to create volume texture";
        }
        
        // Fill volume texture with gradient data
        D3DLOCKED_BOX locked_box;
        hr = volume_tex->LockBox(0, &locked_box, nullptr, 0);
        if (SUCCEEDED(hr)) {
            uint32_t* data = static_cast<uint32_t*>(locked_box.pBits);
            for (int z = 0; z < 32; z++) {
                for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 32; x++) {
                        uint8_t r = (x * 255) / 31;
                        uint8_t g = (y * 255) / 31;
                        uint8_t b = (z * 255) / 31;
                        int idx = z * (locked_box.SlicePitch / 4) + 
                                 y * (locked_box.RowPitch / 4) + x;
                        data[idx] = D3DCOLOR_ARGB(255, r, g, b);
                    }
                }
            }
            volume_tex->UnlockBox(0);
        }
        
        // Set up matrices
        D3DXMATRIX world, view, proj;
        D3DXMatrixIdentity(&world);
        D3DXMatrixLookAtLH(&view, 
            &D3DXVECTOR3(0, 0, -3),
            &D3DXVECTOR3(0, 0, 0),
            &D3DXVECTOR3(0, 1, 0));
        D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, 1.0f, 0.1f, 100.0f);
        
        device_->SetTransform(D3DTS_WORLD, &world);
        device_->SetTransform(D3DTS_VIEW, &view);
        device_->SetTransform(D3DTS_PROJECTION, &proj);
        
        // Set texture and states
        device_->SetTexture(0, volume_tex);
        device_->SetRenderState(D3DRS_LIGHTING, FALSE);
        device_->SetRenderState(D3DRS_ZENABLE, TRUE);
        
        // Define cube vertices with 3D texture coords
        struct Vertex3D {
            float x, y, z;
            float u, v, w;
        };
        
        Vertex3D vertices[] = {
            // Front face
            {-1, -1, -1,  0, 1, 0}, {-1,  1, -1,  0, 0, 0},
            { 1,  1, -1,  1, 0, 0}, { 1, -1, -1,  1, 1, 0},
            // Back face  
            { 1, -1,  1,  0, 1, 1}, { 1,  1,  1,  0, 0, 1},
            {-1,  1,  1,  1, 0, 1}, {-1, -1,  1,  1, 1, 1}
        };
        
        WORD indices[] = {
            0,1,2, 0,2,3,  // Front
            4,5,6, 4,6,7,  // Back
            7,6,1, 7,1,0,  // Left
            3,2,5, 3,5,4,  // Right
            1,6,5, 1,5,2,  // Top
            7,0,3, 7,3,4   // Bottom
        };
        
        // Draw the cube
        device_->BeginScene();
        device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
        device_->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 8, 12,
                                       indices, D3DFMT_INDEX16,
                                       vertices, sizeof(Vertex3D));
        device_->EndScene();
        
        // Clean up
        volume_tex->Release();
    }
    
    // Helper to render a scene with updated shader constants
    void RenderShaderConstantScene() {
        RequireDevice();
        
        // Clear the scene
        device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                      D3DCOLOR_XRGB(16, 16, 32), 1.0f, 0);
        
        // Create and set vertex shader with constants
        const char* vs_source = 
            "vs.1.1\n"
            "dcl_position v0\n"
            "dcl_color v1\n"
            "mov r0, v0\n"
            "mul r0, r0, c0\n"  // Scale by constant 0
            "add r0, r0, c1\n"  // Offset by constant 1
            "mov oPos, r0\n"
            "mul oD0, v1, c2\n"; // Modulate color by constant 2
        
        // Assemble shader (simplified - in real implementation would use assembler)
        DWORD vs_handle = 0;
        std::vector<DWORD> vs_bytecode = {
            0xFFFE0101,  // vs_1_1
            0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
            0x0000001F, 0x80000005, 0x900F0001,  // dcl_color v1
            0x00000001, 0x800F0000, 0x90E40000,  // mov r0, v0
            0x00000005, 0x800F0000, 0x80E40000, 0xA0E40000,  // mul r0, r0, c0
            0x00000002, 0x800F0000, 0x80E40000, 0xA0E40001,  // add r0, r0, c1
            0x00000001, 0xC00F0000, 0x80E40000,  // mov oPos, r0
            0x00000005, 0xD00F0000, 0x90E40001, 0xA0E40002,  // mul oD0, v1, c2
            0x0000FFFF  // end
        };
        
        HRESULT hr = device_->CreateVertexShader(nullptr, vs_bytecode.data(), &vs_handle, 0);
        if (SUCCEEDED(hr) && vs_handle) {
            device_->SetVertexShader(vs_handle);
            
            // Set shader constants with different values
            float scale[4] = {1.5f, 1.5f, 1.5f, 1.0f};
            float offset[4] = {0.0f, 0.2f, 0.0f, 0.0f};
            float color_mod[4] = {1.0f, 0.5f, 2.0f, 1.0f};
            
            device_->SetVertexShaderConstant(0, scale, 1);
            device_->SetVertexShaderConstant(1, offset, 1);
            device_->SetVertexShaderConstant(2, color_mod, 1);
            
            // Draw a triangle with the shader
            struct ColorVertex {
                float x, y, z;
                DWORD color;
            };
            
            ColorVertex triangle[] = {
                {-0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
                { 0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(0, 255, 0)},
                { 0.0f,  0.5f, 0.5f, D3DCOLOR_XRGB(0, 0, 255)}
            };
            
            device_->BeginScene();
            device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, triangle, sizeof(ColorVertex));
            device_->EndScene();
            
            // Clean up shader
            device_->DeleteVertexShader(vs_handle);
        } else {
            // Fallback to fixed-function if shaders not supported
            device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
            
            struct ColorVertex {
                float x, y, z;
                DWORD color;
            };
            
            ColorVertex triangle[] = {
                {-0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(128, 64, 64)},
                { 0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(64, 128, 64)},
                { 0.0f,  0.5f, 0.5f, D3DCOLOR_XRGB(64, 64, 128)}
            };
            
            device_->BeginScene();
            device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, triangle, sizeof(ColorVertex));
            device_->EndScene();
        }
    }
    
    // Helper to render geometry with depth and stencil tests
    void RenderDepthStencilScene() {
        RequireDevice();
        
        // Clear with stencil value 0
        device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                      D3DCOLOR_XRGB(32, 32, 32), 1.0f, 0);
        
        // Enable depth testing
        device_->SetRenderState(D3DRS_ZENABLE, TRUE);
        device_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        device_->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        
        // First pass: Draw to stencil
        device_->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        device_->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
        device_->SetRenderState(D3DRS_STENCILREF, 1);
        device_->SetRenderState(D3DRS_STENCILMASK, 0xFF);
        device_->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);
        device_->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
        device_->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
        device_->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
        
        // Draw first quad (writes to stencil)
        struct Vertex {
            float x, y, z;
            DWORD color;
        };
        
        Vertex quad1[] = {
            {-0.7f, -0.7f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
            { 0.3f, -0.7f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
            { 0.3f,  0.3f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
            {-0.7f,  0.3f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)}
        };
        
        device_->BeginScene();
        device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad1, sizeof(Vertex));
        
        // Second pass: Draw only where stencil == 1
        device_->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
        device_->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
        
        // Draw overlapping quad (only visible where stencil == 1)
        Vertex quad2[] = {
            {-0.3f, -0.3f, 0.3f, D3DCOLOR_XRGB(0, 255, 0)},
            { 0.7f, -0.3f, 0.3f, D3DCOLOR_XRGB(0, 255, 0)},
            { 0.7f,  0.7f, 0.3f, D3DCOLOR_XRGB(0, 255, 0)},
            {-0.3f,  0.7f, 0.3f, D3DCOLOR_XRGB(0, 255, 0)}
        };
        
        device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad2, sizeof(Vertex));
        
        // Third pass: Draw with depth test (behind first quad)
        device_->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        
        Vertex quad3[] = {
            {-0.5f, -0.5f, 0.7f, D3DCOLOR_XRGB(0, 0, 255)},
            { 0.5f, -0.5f, 0.7f, D3DCOLOR_XRGB(0, 0, 255)},
            { 0.5f,  0.5f, 0.7f, D3DCOLOR_XRGB(0, 0, 255)},
            {-0.5f,  0.5f, 0.7f, D3DCOLOR_XRGB(0, 0, 255)}
        };
        
        device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad3, sizeof(Vertex));
        
        device_->EndScene();
    }
    
    // Capture current framebuffer to PPM image
    PPMImage CaptureFramebuffer() {
        // Get the backbuffer
        IDirect3DSurface8* backbuffer = nullptr;
        HRESULT hr = device_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
        
        if (FAILED(hr) || !backbuffer) {
            return PPMImage();
        }
        
        // Lock and read the surface
        D3DLOCKED_RECT locked_rect;
        hr = backbuffer->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);
        
        PPMImage image;
        if (SUCCEEDED(hr)) {
            // Assuming X8R8G8B8 format
            image = GoldenImageUtils::FramebufferToPPM(
                locked_rect.pBits, 640, 480, false, true);
            backbuffer->UnlockRect();
        }
        
        backbuffer->Release();
        return image;
    }
};

// Test volume textured cube rendering
TEST_P(CrossBackendEnhancedTest, VolumeTexturedCube) {
    RenderVolumeTexturedCube();
    
    PPMImage captured = CaptureFramebuffer();
    if (captured.IsValid()) {
        EXPECT_IMAGE_MATCHES_GOLDEN(captured, "CrossBackend", "VolumeTexturedCube", 
                                   GetBackendName(), 5.0, 0.01);
    }
}

// Test shader constant updates
TEST_P(CrossBackendEnhancedTest, ShaderConstantScene) {
    RenderShaderConstantScene();
    
    PPMImage captured = CaptureFramebuffer();
    if (captured.IsValid()) {
        EXPECT_IMAGE_MATCHES_GOLDEN(captured, "CrossBackend", "ShaderConstants",
                                   GetBackendName(), 5.0, 0.01);
    }
}

// Test depth and stencil operations
TEST_P(CrossBackendEnhancedTest, DepthStencilScene) {
    RenderDepthStencilScene();
    
    PPMImage captured = CaptureFramebuffer();
    if (captured.IsValid()) {
        EXPECT_IMAGE_MATCHES_GOLDEN(captured, "CrossBackend", "DepthStencil",
                                   GetBackendName(), 5.0, 0.01);
    }
}

// Test combined complex scene
TEST_P(CrossBackendEnhancedTest, ComplexCombinedScene) {
    RequireDevice();
    
    // Clear to a unique color
    device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                  D3DCOLOR_XRGB(48, 32, 64), 1.0f, 0);
    
    // Render multiple elements
    device_->BeginScene();
    
    // Enable states for complex rendering
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    // Draw multiple overlapping triangles with different blend modes
    struct AlphaVertex {
        float x, y, z;
        DWORD color;
    };
    
    // Opaque triangle
    AlphaVertex tri1[] = {
        {-0.8f, -0.5f, 0.8f, D3DCOLOR_ARGB(255, 255, 0, 0)},
        { 0.0f, -0.5f, 0.8f, D3DCOLOR_ARGB(255, 255, 0, 0)},
        {-0.4f,  0.3f, 0.8f, D3DCOLOR_ARGB(255, 255, 0, 0)}
    };
    
    device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, tri1, sizeof(AlphaVertex));
    
    // Semi-transparent triangle
    AlphaVertex tri2[] = {
        {-0.2f, -0.5f, 0.6f, D3DCOLOR_ARGB(128, 0, 255, 0)},
        { 0.6f, -0.5f, 0.6f, D3DCOLOR_ARGB(128, 0, 255, 0)},
        { 0.2f,  0.3f, 0.6f, D3DCOLOR_ARGB(128, 0, 255, 0)}
    };
    
    device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, tri2, sizeof(AlphaVertex));
    
    // Very transparent triangle
    AlphaVertex tri3[] = {
        { 0.2f, -0.3f, 0.4f, D3DCOLOR_ARGB(64, 0, 0, 255)},
        { 0.8f, -0.3f, 0.4f, D3DCOLOR_ARGB(64, 0, 0, 255)},
        { 0.5f,  0.5f, 0.4f, D3DCOLOR_ARGB(64, 0, 0, 255)}
    };
    
    device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, tri3, sizeof(AlphaVertex));
    
    device_->EndScene();
    
    PPMImage captured = CaptureFramebuffer();
    if (captured.IsValid()) {
        EXPECT_IMAGE_MATCHES_GOLDEN(captured, "CrossBackend", "ComplexCombined",
                                   GetBackendName(), 5.0, 0.01);
    }
}

// Instantiate tests for all backends
INSTANTIATE_BACKEND_PARAM_TEST(CrossBackendEnhancedTest);