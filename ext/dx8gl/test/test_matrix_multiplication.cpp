#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <cmath>
#include <memory>

using namespace dx8gl;

class MatrixMultiplicationTest : public ::testing::Test {
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
    
    // Helper to create identity matrix
    D3DMATRIX CreateIdentityMatrix() {
        D3DMATRIX mat = {};
        mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
        return mat;
    }
    
    // Helper to create translation matrix
    D3DMATRIX CreateTranslationMatrix(float x, float y, float z) {
        D3DMATRIX mat = CreateIdentityMatrix();
        mat._41 = x;
        mat._42 = y;
        mat._43 = z;
        return mat;
    }
    
    // Helper to create scaling matrix
    D3DMATRIX CreateScalingMatrix(float x, float y, float z) {
        D3DMATRIX mat = {};
        mat._11 = x;
        mat._22 = y;
        mat._33 = z;
        mat._44 = 1.0f;
        return mat;
    }
    
    // Helper to create rotation matrix around Y axis
    D3DMATRIX CreateRotationYMatrix(float angle) {
        D3DMATRIX mat = CreateIdentityMatrix();
        float c = cosf(angle);
        float s = sinf(angle);
        mat._11 = c;
        mat._13 = -s;
        mat._31 = s;
        mat._33 = c;
        return mat;
    }
    
    // Helper to compare matrices
    bool MatricesEqual(const D3DMATRIX& a, const D3DMATRIX& b, float epsilon = 0.0001f) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (fabsf(a.m[i][j] - b.m[i][j]) > epsilon) {
                    return false;
                }
            }
        }
        return true;
    }
    
    // Helper to manually multiply two matrices (for verification)
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
};

TEST_F(MatrixMultiplicationTest, MultiplyIdentityMatrix) {
    // Set world transform to a translation
    D3DMATRIX translation = CreateTranslationMatrix(10.0f, 20.0f, 30.0f);
    HRESULT hr = device_->SetTransform(D3DTS_WORLD, &translation);
    EXPECT_EQ(hr, D3D_OK);
    
    // Multiply by identity (should not change)
    D3DMATRIX identity = CreateIdentityMatrix();
    hr = device_->MultiplyTransform(D3DTS_WORLD, &identity);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get the result
    D3DMATRIX result;
    hr = device_->GetTransform(D3DTS_WORLD, &result);
    EXPECT_EQ(hr, D3D_OK);
    
    // Should still be the translation matrix
    EXPECT_TRUE(MatricesEqual(result, translation));
}

TEST_F(MatrixMultiplicationTest, ChainTranslations) {
    // Set initial translation
    D3DMATRIX trans1 = CreateTranslationMatrix(5.0f, 0.0f, 0.0f);
    HRESULT hr = device_->SetTransform(D3DTS_WORLD, &trans1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Multiply by another translation
    D3DMATRIX trans2 = CreateTranslationMatrix(3.0f, 0.0f, 0.0f);
    hr = device_->MultiplyTransform(D3DTS_WORLD, &trans2);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get the result
    D3DMATRIX result;
    hr = device_->GetTransform(D3DTS_WORLD, &result);
    EXPECT_EQ(hr, D3D_OK);
    
    // Should be translation by (8, 0, 0)
    D3DMATRIX expected = CreateTranslationMatrix(8.0f, 0.0f, 0.0f);
    EXPECT_TRUE(MatricesEqual(result, expected));
}

TEST_F(MatrixMultiplicationTest, ScaleAndTranslate) {
    // Set initial scaling
    D3DMATRIX scale = CreateScalingMatrix(2.0f, 2.0f, 2.0f);
    HRESULT hr = device_->SetTransform(D3DTS_VIEW, &scale);
    EXPECT_EQ(hr, D3D_OK);
    
    // Multiply by translation
    D3DMATRIX trans = CreateTranslationMatrix(10.0f, 5.0f, 0.0f);
    hr = device_->MultiplyTransform(D3DTS_VIEW, &trans);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get the result
    D3DMATRIX result;
    hr = device_->GetTransform(D3DTS_VIEW, &result);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify the combined transformation
    D3DMATRIX expected = MultiplyMatrices(trans, scale);
    EXPECT_TRUE(MatricesEqual(result, expected));
}

TEST_F(MatrixMultiplicationTest, RotateAndScale) {
    // Set initial rotation
    float angle = 3.14159f / 4.0f; // 45 degrees
    D3DMATRIX rotation = CreateRotationYMatrix(angle);
    HRESULT hr = device_->SetTransform(D3DTS_PROJECTION, &rotation);
    EXPECT_EQ(hr, D3D_OK);
    
    // Multiply by scaling
    D3DMATRIX scale = CreateScalingMatrix(2.0f, 3.0f, 4.0f);
    hr = device_->MultiplyTransform(D3DTS_PROJECTION, &scale);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get the result
    D3DMATRIX result;
    hr = device_->GetTransform(D3DTS_PROJECTION, &result);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify the combined transformation
    D3DMATRIX expected = MultiplyMatrices(scale, rotation);
    EXPECT_TRUE(MatricesEqual(result, expected));
}

TEST_F(MatrixMultiplicationTest, TextureMatrixMultiplication) {
    // Test texture matrix multiplication
    for (int i = 0; i < 8; i++) {
        D3DTRANSFORMSTATETYPE texState = static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + i);
        
        // Set initial texture transform
        D3DMATRIX scale = CreateScalingMatrix(2.0f, 2.0f, 1.0f);
        HRESULT hr = device_->SetTransform(texState, &scale);
        EXPECT_EQ(hr, D3D_OK) << "Failed to set texture " << i << " transform";
        
        // Multiply by translation
        D3DMATRIX trans = CreateTranslationMatrix(0.5f, 0.5f, 0.0f);
        hr = device_->MultiplyTransform(texState, &trans);
        EXPECT_EQ(hr, D3D_OK) << "Failed to multiply texture " << i << " transform";
        
        // Get the result
        D3DMATRIX result;
        hr = device_->GetTransform(texState, &result);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get texture " << i << " transform";
        
        // Verify the combined transformation
        D3DMATRIX expected = MultiplyMatrices(trans, scale);
        EXPECT_TRUE(MatricesEqual(result, expected)) << "Texture " << i << " matrix mismatch";
    }
}

TEST_F(MatrixMultiplicationTest, ComplexTransformChain) {
    // Create a complex chain of transformations
    D3DMATRIX scale = CreateScalingMatrix(2.0f, 2.0f, 2.0f);
    D3DMATRIX rotation = CreateRotationYMatrix(3.14159f / 6.0f); // 30 degrees
    D3DMATRIX translation = CreateTranslationMatrix(10.0f, 5.0f, 3.0f);
    
    // Apply to world transform
    HRESULT hr = device_->SetTransform(D3DTS_WORLD, &scale);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->MultiplyTransform(D3DTS_WORLD, &rotation);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->MultiplyTransform(D3DTS_WORLD, &translation);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get the result
    D3DMATRIX result;
    hr = device_->GetTransform(D3DTS_WORLD, &result);
    EXPECT_EQ(hr, D3D_OK);
    
    // Calculate expected result
    D3DMATRIX temp = MultiplyMatrices(rotation, scale);
    D3DMATRIX expected = MultiplyMatrices(translation, temp);
    
    EXPECT_TRUE(MatricesEqual(result, expected));
}

TEST_F(MatrixMultiplicationTest, InvalidParameters) {
    // Test with null matrix
    HRESULT hr = device_->MultiplyTransform(D3DTS_WORLD, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test with invalid transform state
    D3DMATRIX matrix = CreateIdentityMatrix();
    hr = device_->MultiplyTransform(static_cast<D3DTRANSFORMSTATETYPE>(999), &matrix);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(MatrixMultiplicationTest, PreserveOtherTransforms) {
    // Set different transforms
    D3DMATRIX world = CreateTranslationMatrix(1.0f, 0.0f, 0.0f);
    D3DMATRIX view = CreateTranslationMatrix(0.0f, 2.0f, 0.0f);
    D3DMATRIX proj = CreateTranslationMatrix(0.0f, 0.0f, 3.0f);
    
    HRESULT hr = device_->SetTransform(D3DTS_WORLD, &world);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->SetTransform(D3DTS_VIEW, &view);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->SetTransform(D3DTS_PROJECTION, &proj);
    EXPECT_EQ(hr, D3D_OK);
    
    // Multiply only the view transform
    D3DMATRIX scale = CreateScalingMatrix(2.0f, 2.0f, 2.0f);
    hr = device_->MultiplyTransform(D3DTS_VIEW, &scale);
    EXPECT_EQ(hr, D3D_OK);
    
    // Check that world and projection are unchanged
    D3DMATRIX result_world, result_view, result_proj;
    hr = device_->GetTransform(D3DTS_WORLD, &result_world);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->GetTransform(D3DTS_VIEW, &result_view);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->GetTransform(D3DTS_PROJECTION, &result_proj);
    EXPECT_EQ(hr, D3D_OK);
    
    // World and projection should be unchanged
    EXPECT_TRUE(MatricesEqual(result_world, world));
    EXPECT_TRUE(MatricesEqual(result_proj, proj));
    
    // View should be modified
    D3DMATRIX expected_view = MultiplyMatrices(scale, view);
    EXPECT_TRUE(MatricesEqual(result_view, expected_view));
}

TEST_F(MatrixMultiplicationTest, NonCommutativeMultiplication) {
    // Demonstrate that matrix multiplication is not commutative
    D3DMATRIX rotation = CreateRotationYMatrix(3.14159f / 2.0f); // 90 degrees
    D3DMATRIX translation = CreateTranslationMatrix(10.0f, 0.0f, 0.0f);
    
    // Test 1: Set rotation, then multiply by translation
    HRESULT hr = device_->SetTransform(D3DTS_WORLD, &rotation);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->MultiplyTransform(D3DTS_WORLD, &translation);
    EXPECT_EQ(hr, D3D_OK);
    
    D3DMATRIX result1;
    hr = device_->GetTransform(D3DTS_WORLD, &result1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Test 2: Set translation, then multiply by rotation
    hr = device_->SetTransform(D3DTS_VIEW, &translation);
    EXPECT_EQ(hr, D3D_OK);
    hr = device_->MultiplyTransform(D3DTS_VIEW, &rotation);
    EXPECT_EQ(hr, D3D_OK);
    
    D3DMATRIX result2;
    hr = device_->GetTransform(D3DTS_VIEW, &result2);
    EXPECT_EQ(hr, D3D_OK);
    
    // Results should be different
    EXPECT_FALSE(MatricesEqual(result1, result2));
    
    // Verify correct results
    D3DMATRIX expected1 = MultiplyMatrices(translation, rotation);
    D3DMATRIX expected2 = MultiplyMatrices(rotation, translation);
    
    EXPECT_TRUE(MatricesEqual(result1, expected1));
    EXPECT_TRUE(MatricesEqual(result2, expected2));
}