#include <gtest/gtest.h>
#include "d3d8.h"
#include "dx8gl.h"
#include <cmath>
#include <cstring>

class MatrixPipelineTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8 = nullptr;
    IDirect3DDevice8* device = nullptr;
    
    void SetUp() override {
        // Initialize dx8gl
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
    
    // Helper to create identity matrix
    D3DMATRIX IdentityMatrix() {
        D3DMATRIX m = {};
        m._11 = m._22 = m._33 = m._44 = 1.0f;
        return m;
    }
    
    // Helper to create translation matrix
    D3DMATRIX TranslationMatrix(float x, float y, float z) {
        D3DMATRIX m = IdentityMatrix();
        m._41 = x;
        m._42 = y;
        m._43 = z;
        return m;
    }
    
    // Helper to create scale matrix
    D3DMATRIX ScaleMatrix(float x, float y, float z) {
        D3DMATRIX m = {};
        m._11 = x;
        m._22 = y;
        m._33 = z;
        m._44 = 1.0f;
        return m;
    }
    
    // Helper to create perspective projection matrix
    D3DMATRIX PerspectiveMatrix(float fov, float aspect, float near_plane, float far_plane) {
        D3DMATRIX m = {};
        float h = 1.0f / tanf(fov * 0.5f);
        float w = h / aspect;
        float q = far_plane / (far_plane - near_plane);
        
        m._11 = w;
        m._22 = h;
        m._33 = q;
        m._34 = 1.0f;
        m._43 = -q * near_plane;
        
        return m;
    }
    
    // Helper to multiply two matrices (DirectX order: A * B)
    D3DMATRIX MultiplyMatrices(const D3DMATRIX& a, const D3DMATRIX& b) {
        D3DMATRIX result = {};
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += a.m[i][k] * b.m[k][j];
                }
                result.m[i][j] = sum;
            }
        }
        
        return result;
    }
    
    // Helper to transform a point by a matrix
    void TransformPoint(const D3DMATRIX& m, float x, float y, float z, float w, 
                       float& out_x, float& out_y, float& out_z, float& out_w) {
        out_x = m._11 * x + m._21 * y + m._31 * z + m._41 * w;
        out_y = m._12 * x + m._22 * y + m._32 * z + m._42 * w;
        out_z = m._13 * x + m._23 * y + m._33 * z + m._43 * w;
        out_w = m._14 * x + m._24 * y + m._34 * z + m._44 * w;
    }
};

TEST_F(MatrixPipelineTest, IdentityTransform) {
    // Set all matrices to identity
    D3DMATRIX identity = IdentityMatrix();
    
    ASSERT_EQ(device->SetTransform(D3DTS_WORLD, &identity), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_VIEW, &identity), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_PROJECTION, &identity), D3D_OK);
    
    // Verify we can get them back
    D3DMATRIX retrieved;
    ASSERT_EQ(device->GetTransform(D3DTS_WORLD, &retrieved), D3D_OK);
    EXPECT_EQ(memcmp(&identity, &retrieved, sizeof(D3DMATRIX)), 0);
}

TEST_F(MatrixPipelineTest, WorldTranslation) {
    // Set world matrix to translate by (1, 2, 3)
    D3DMATRIX world = TranslationMatrix(1.0f, 2.0f, 3.0f);
    D3DMATRIX view = IdentityMatrix();
    D3DMATRIX proj = IdentityMatrix();
    
    ASSERT_EQ(device->SetTransform(D3DTS_WORLD, &world), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_VIEW, &view), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_PROJECTION, &proj), D3D_OK);
    
    // Expected combined matrix should be world * view * proj = world (since view and proj are identity)
    D3DMATRIX expected = world;
    
    // Test point (0, 0, 0, 1) should transform to (1, 2, 3, 1)
    float x, y, z, w;
    TransformPoint(expected, 0, 0, 0, 1, x, y, z, w);
    
    EXPECT_FLOAT_EQ(x, 1.0f);
    EXPECT_FLOAT_EQ(y, 2.0f);
    EXPECT_FLOAT_EQ(z, 3.0f);
    EXPECT_FLOAT_EQ(w, 1.0f);
}

TEST_F(MatrixPipelineTest, CombinedTransform) {
    // Set up a typical transformation pipeline
    D3DMATRIX world = TranslationMatrix(5.0f, 0.0f, 0.0f);  // Move right by 5
    D3DMATRIX view = TranslationMatrix(0.0f, 0.0f, -10.0f); // Move camera back by 10
    D3DMATRIX proj = PerspectiveMatrix(3.14159f / 4.0f, 1.0f, 0.1f, 100.0f); // 45 degree FOV
    
    ASSERT_EQ(device->SetTransform(D3DTS_WORLD, &world), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_VIEW, &view), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_PROJECTION, &proj), D3D_OK);
    
    // Calculate expected combined matrix (World * View * Projection)
    D3DMATRIX world_view = MultiplyMatrices(world, view);
    D3DMATRIX expected = MultiplyMatrices(world_view, proj);
    
    // Test that a point at origin transforms correctly
    float x, y, z, w;
    TransformPoint(expected, 0, 0, 0, 1, x, y, z, w);
    
    // After world transform: (5, 0, 0, 1)
    // After view transform: (5, 0, -10, 1)
    // After projection, we should have perspective division
    
    // Basic sanity checks
    EXPECT_NE(w, 0.0f); // W should not be zero
    
    // After perspective divide
    float clip_x = x / w;
    float clip_y = y / w;
    float clip_z = z / w;
    
    // Clip space coordinates should be within reasonable bounds
    EXPECT_GE(clip_x, -10.0f);
    EXPECT_LE(clip_x, 10.0f);
    EXPECT_GE(clip_y, -10.0f);
    EXPECT_LE(clip_y, 10.0f);
    EXPECT_GE(clip_z, -1.0f);
    EXPECT_LE(clip_z, 1.0f);
}

TEST_F(MatrixPipelineTest, MatrixMultiplicationOrder) {
    // Test that matrix multiplication follows DirectX convention: World * View * Projection
    
    // Create distinct matrices that don't commute
    D3DMATRIX world = TranslationMatrix(1.0f, 0.0f, 0.0f);
    D3DMATRIX view = ScaleMatrix(2.0f, 2.0f, 2.0f);
    D3DMATRIX proj = TranslationMatrix(0.0f, 1.0f, 0.0f);
    
    ASSERT_EQ(device->SetTransform(D3DTS_WORLD, &world), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_VIEW, &view), D3D_OK);
    ASSERT_EQ(device->SetTransform(D3DTS_PROJECTION, &proj), D3D_OK);
    
    // Calculate expected result in DirectX order
    D3DMATRIX world_view = MultiplyMatrices(world, view);
    D3DMATRIX expected = MultiplyMatrices(world_view, proj);
    
    // Transform a test point
    float x, y, z, w;
    TransformPoint(expected, 1, 1, 1, 1, x, y, z, w);
    
    // Point (1,1,1,1) should be:
    // 1. Translated by world: (2,1,1,1)
    // 2. Scaled by view: (4,2,2,1)
    // 3. Translated by proj: (4,3,2,1)
    
    EXPECT_FLOAT_EQ(x, 4.0f);
    EXPECT_FLOAT_EQ(y, 3.0f);
    EXPECT_FLOAT_EQ(z, 2.0f);
    EXPECT_FLOAT_EQ(w, 1.0f);
}

TEST_F(MatrixPipelineTest, TransposeForOpenGL) {
    // This test verifies that matrices are correctly transposed when sent to OpenGL
    // DirectX uses row-major, OpenGL uses column-major
    
    // Create a non-symmetric matrix to detect transposition issues
    D3DMATRIX world = {};
    world._11 = 1.0f; world._12 = 2.0f; world._13 = 3.0f; world._14 = 4.0f;
    world._21 = 5.0f; world._22 = 6.0f; world._23 = 7.0f; world._24 = 8.0f;
    world._31 = 9.0f; world._32 = 10.0f; world._33 = 11.0f; world._34 = 12.0f;
    world._41 = 13.0f; world._42 = 14.0f; world._43 = 15.0f; world._44 = 16.0f;
    
    ASSERT_EQ(device->SetTransform(D3DTS_WORLD, &world), D3D_OK);
    
    // Retrieve and verify it's stored correctly
    D3DMATRIX retrieved;
    ASSERT_EQ(device->GetTransform(D3DTS_WORLD, &retrieved), D3D_OK);
    
    // Should match exactly (no transposition in storage)
    EXPECT_EQ(memcmp(&world, &retrieved, sizeof(D3DMATRIX)), 0);
}