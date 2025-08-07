#include <gtest/gtest.h>
#include "d3dx_compat.h"
#include <cmath>
#include <cstring>

class D3DXMathTest : public ::testing::Test {
protected:
    // Helper to check if two floats are approximately equal
    bool FloatNear(float a, float b, float epsilon = 1e-5f) {
        return std::abs(a - b) < epsilon;
    }
    
    // Helper to check if two matrices are approximately equal
    bool MatrixNear(const D3DMATRIX& a, const D3DMATRIX& b, float epsilon = 1e-5f) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (!FloatNear(a.m(i, j), b.m(i, j), epsilon)) {
                    return false;
                }
            }
        }
        return true;
    }
    
    // Helper to create identity matrix
    D3DMATRIX IdentityMatrix() {
        D3DMATRIX m = {};
        m._11 = m._22 = m._33 = m._44 = 1.0f;
        return m;
    }
};

TEST_F(D3DXMathTest, MatrixIdentity) {
    D3DMATRIX m;
    D3DXMatrixIdentity(&m);
    
    EXPECT_FLOAT_EQ(m._11, 1.0f);
    EXPECT_FLOAT_EQ(m._22, 1.0f);
    EXPECT_FLOAT_EQ(m._33, 1.0f);
    EXPECT_FLOAT_EQ(m._44, 1.0f);
    
    EXPECT_FLOAT_EQ(m._12, 0.0f);
    EXPECT_FLOAT_EQ(m._13, 0.0f);
    EXPECT_FLOAT_EQ(m._14, 0.0f);
    EXPECT_FLOAT_EQ(m._21, 0.0f);
}

TEST_F(D3DXMathTest, MatrixMultiply) {
    D3DMATRIX a, b, result;
    
    // Test identity multiplication
    D3DXMatrixIdentity(&a);
    D3DXMatrixIdentity(&b);
    D3DXMatrixMultiply(&result, &a, &b);
    EXPECT_TRUE(MatrixNear(result, IdentityMatrix()));
    
    // Test translation * scaling
    D3DXMatrixTranslation(&a, 10.0f, 20.0f, 30.0f);
    D3DXMatrixScaling(&b, 2.0f, 3.0f, 4.0f);
    D3DXMatrixMultiply(&result, &a, &b);
    
    // Result should scale then translate
    EXPECT_FLOAT_EQ(result._11, 2.0f);
    EXPECT_FLOAT_EQ(result._22, 3.0f);
    EXPECT_FLOAT_EQ(result._33, 4.0f);
    EXPECT_FLOAT_EQ(result._41, 10.0f);
    EXPECT_FLOAT_EQ(result._42, 20.0f);
    EXPECT_FLOAT_EQ(result._43, 30.0f);
}

TEST_F(D3DXMathTest, MatrixTranspose) {
    D3DMATRIX m, result;
    
    // Create an asymmetric matrix
    memset(&m, 0, sizeof(D3DMATRIX));
    m._11 = 1.0f; m._12 = 2.0f; m._13 = 3.0f; m._14 = 4.0f;
    m._21 = 5.0f; m._22 = 6.0f; m._23 = 7.0f; m._24 = 8.0f;
    m._31 = 9.0f; m._32 = 10.0f; m._33 = 11.0f; m._34 = 12.0f;
    m._41 = 13.0f; m._42 = 14.0f; m._43 = 15.0f; m._44 = 16.0f;
    
    D3DXMatrixTranspose(&result, &m);
    
    EXPECT_FLOAT_EQ(result._11, 1.0f);
    EXPECT_FLOAT_EQ(result._12, 5.0f);
    EXPECT_FLOAT_EQ(result._13, 9.0f);
    EXPECT_FLOAT_EQ(result._14, 13.0f);
    
    EXPECT_FLOAT_EQ(result._21, 2.0f);
    EXPECT_FLOAT_EQ(result._22, 6.0f);
    EXPECT_FLOAT_EQ(result._23, 10.0f);
    EXPECT_FLOAT_EQ(result._24, 14.0f);
}

TEST_F(D3DXMathTest, MatrixDeterminant) {
    D3DMATRIX m;
    
    // Test identity matrix determinant
    D3DXMatrixIdentity(&m);
    float det = D3DXMatrixDeterminant(&m);
    EXPECT_FLOAT_EQ(det, 1.0f);
    
    // Test scaling matrix determinant
    D3DXMatrixScaling(&m, 2.0f, 3.0f, 4.0f);
    det = D3DXMatrixDeterminant(&m);
    EXPECT_FLOAT_EQ(det, 24.0f); // 2 * 3 * 4
    
    // Test singular matrix
    memset(&m, 0, sizeof(D3DMATRIX));
    m._11 = 1.0f; m._12 = 2.0f; m._13 = 3.0f; m._14 = 0.0f;
    m._21 = 2.0f; m._22 = 4.0f; m._23 = 6.0f; m._24 = 0.0f;
    m._31 = 3.0f; m._32 = 6.0f; m._33 = 9.0f; m._34 = 0.0f;
    m._41 = 0.0f; m._42 = 0.0f; m._43 = 0.0f; m._44 = 1.0f;
    det = D3DXMatrixDeterminant(&m);
    EXPECT_NEAR(det, 0.0f, 1e-5f);
}

TEST_F(D3DXMathTest, MatrixInverse) {
    D3DMATRIX m, inverse, result;
    float det;
    
    // Test identity matrix inverse
    D3DXMatrixIdentity(&m);
    D3DXMatrixInverse(&inverse, &det, &m);
    EXPECT_FLOAT_EQ(det, 1.0f);
    EXPECT_TRUE(MatrixNear(inverse, IdentityMatrix()));
    
    // Test translation matrix inverse
    D3DXMatrixTranslation(&m, 10.0f, 20.0f, 30.0f);
    D3DXMatrixInverse(&inverse, &det, &m);
    EXPECT_FLOAT_EQ(det, 1.0f);
    
    // Multiply matrix by its inverse should give identity
    D3DXMatrixMultiply(&result, &m, &inverse);
    EXPECT_TRUE(MatrixNear(result, IdentityMatrix()));
    
    // Test scaling matrix inverse
    D3DXMatrixScaling(&m, 2.0f, 4.0f, 8.0f);
    D3DXMatrixInverse(&inverse, &det, &m);
    EXPECT_FLOAT_EQ(det, 64.0f); // 2 * 4 * 8
    
    // Inverse of scaling should have reciprocal scale factors
    EXPECT_FLOAT_EQ(inverse._11, 0.5f);
    EXPECT_FLOAT_EQ(inverse._22, 0.25f);
    EXPECT_FLOAT_EQ(inverse._33, 0.125f);
    
    // Test rotation matrix inverse
    D3DXMatrixRotationY(&m, 3.14159f / 4.0f); // 45 degrees
    D3DXMatrixInverse(&inverse, &det, &m);
    EXPECT_NEAR(det, 1.0f, 1e-5f); // Rotation matrices have determinant 1
    
    // Inverse of rotation is transpose (for orthogonal matrices)
    D3DMATRIX transpose;
    D3DXMatrixTranspose(&transpose, &m);
    EXPECT_TRUE(MatrixNear(inverse, transpose));
    
    // Test singular matrix (should return null)
    memset(&m, 0, sizeof(D3DMATRIX));
    m._11 = 1.0f; m._12 = 2.0f;
    m._21 = 2.0f; m._22 = 4.0f;
    D3DMATRIX* result_ptr = D3DXMatrixInverse(&inverse, &det, &m);
    EXPECT_EQ(result_ptr, nullptr);
    EXPECT_NEAR(det, 0.0f, 1e-5f);
}

TEST_F(D3DXMathTest, MatrixRotation) {
    D3DMATRIX rx, ry, rz;
    float angle = 3.14159f / 2.0f; // 90 degrees
    
    // Test rotation around X axis
    D3DXMatrixRotationX(&rx, angle);
    D3DXVECTOR3 v(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 result;
    D3DXVec3TransformCoord(&result, &v, &rx);
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 0.0f, 1e-5f);
    EXPECT_NEAR(result.z, 1.0f, 1e-5f);
    
    // Test rotation around Y axis
    D3DXMatrixRotationY(&ry, angle);
    v = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    D3DXVec3TransformCoord(&result, &v, &ry);
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 0.0f, 1e-5f);
    EXPECT_NEAR(result.z, -1.0f, 1e-5f);
    
    // Test rotation around Z axis
    D3DXMatrixRotationZ(&rz, angle);
    v = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    D3DXVec3TransformCoord(&result, &v, &rz);
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 1.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST_F(D3DXMathTest, MatrixOrthoLH) {
    D3DMATRIX m;
    float w = 800.0f, h = 600.0f, zn = 0.1f, zf = 100.0f;
    
    D3DXMatrixOrthoLH(&m, w, h, zn, zf);
    
    EXPECT_FLOAT_EQ(m._11, 2.0f / w);
    EXPECT_FLOAT_EQ(m._22, 2.0f / h);
    EXPECT_FLOAT_EQ(m._33, 1.0f / (zf - zn));
    EXPECT_FLOAT_EQ(m._43, -zn / (zf - zn));
    EXPECT_FLOAT_EQ(m._44, 1.0f);
}

TEST_F(D3DXMathTest, MatrixOrthoRH) {
    D3DMATRIX m;
    float w = 800.0f, h = 600.0f, zn = 0.1f, zf = 100.0f;
    
    D3DXMatrixOrthoRH(&m, w, h, zn, zf);
    
    EXPECT_FLOAT_EQ(m._11, 2.0f / w);
    EXPECT_FLOAT_EQ(m._22, 2.0f / h);
    EXPECT_FLOAT_EQ(m._33, 1.0f / (zn - zf));
    EXPECT_FLOAT_EQ(m._43, zn / (zn - zf));
    EXPECT_FLOAT_EQ(m._44, 1.0f);
}

TEST_F(D3DXMathTest, MatrixPerspectiveFov) {
    D3DMATRIX m;
    float fovy = 3.14159f / 4.0f; // 45 degrees
    float aspect = 16.0f / 9.0f;
    float zn = 0.1f, zf = 100.0f;
    
    // Test LH version
    D3DXMatrixPerspectiveFovLH(&m, fovy, aspect, zn, zf);
    float yScale = 1.0f / tanf(fovy * 0.5f);
    float xScale = yScale / aspect;
    
    EXPECT_FLOAT_EQ(m._11, xScale);
    EXPECT_FLOAT_EQ(m._22, yScale);
    EXPECT_FLOAT_EQ(m._34, 1.0f);
    
    // Test RH version
    D3DXMatrixPerspectiveFovRH(&m, fovy, aspect, zn, zf);
    EXPECT_FLOAT_EQ(m._11, xScale);
    EXPECT_FLOAT_EQ(m._22, yScale);
    EXPECT_FLOAT_EQ(m._34, -1.0f);
}

TEST_F(D3DXMathTest, Vec3Operations) {
    D3DXVECTOR3 v1(3.0f, 4.0f, 0.0f);
    D3DXVECTOR3 v2(1.0f, 0.0f, 0.0f);
    D3DXVECTOR3 result;
    
    // Test length
    float len = D3DXVec3Length(&v1);
    EXPECT_FLOAT_EQ(len, 5.0f); // 3-4-5 triangle
    
    // Test length squared
    float lenSq = D3DXVec3LengthSq(&v1);
    EXPECT_FLOAT_EQ(lenSq, 25.0f);
    
    // Test dot product
    float dot = D3DXVec3Dot(&v1, &v2);
    EXPECT_FLOAT_EQ(dot, 3.0f);
    
    // Test cross product
    v1 = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    v2 = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    D3DXVec3Cross(&result, &v1, &v2);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
    
    // Test normalization
    v1 = D3DXVECTOR3(3.0f, 4.0f, 0.0f);
    D3DXVec3Normalize(&result, &v1);
    EXPECT_FLOAT_EQ(result.x, 0.6f);
    EXPECT_FLOAT_EQ(result.y, 0.8f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
    
    // Normalized vector should have length 1
    len = D3DXVec3Length(&result);
    EXPECT_NEAR(len, 1.0f, 1e-5f);
}

TEST_F(D3DXMathTest, Vec2Operations) {
    D3DXVECTOR2 v1(3.0f, 4.0f);
    D3DXVECTOR2 v2(1.0f, 0.0f);
    D3DXVECTOR2 result;
    
    // Test length
    float len = D3DXVec2Length(&v1);
    EXPECT_FLOAT_EQ(len, 5.0f);
    
    // Test length squared
    float lenSq = D3DXVec2LengthSq(&v1);
    EXPECT_FLOAT_EQ(lenSq, 25.0f);
    
    // Test dot product
    float dot = D3DXVec2Dot(&v1, &v2);
    EXPECT_FLOAT_EQ(dot, 3.0f);
    
    // Test normalization
    D3DXVec2Normalize(&result, &v1);
    EXPECT_FLOAT_EQ(result.x, 0.6f);
    EXPECT_FLOAT_EQ(result.y, 0.8f);
    
    len = D3DXVec2Length(&result);
    EXPECT_NEAR(len, 1.0f, 1e-5f);
}

TEST_F(D3DXMathTest, Vec4Operations) {
    D3DXVECTOR4 v1(1.0f, 2.0f, 2.0f, 0.0f);
    D3DXVECTOR4 v2(1.0f, 0.0f, 0.0f, 1.0f);
    D3DXVECTOR4 result;
    
    // Test length
    float len = D3DXVec4Length(&v1);
    EXPECT_FLOAT_EQ(len, 3.0f); // sqrt(1 + 4 + 4 + 0) = 3
    
    // Test length squared
    float lenSq = D3DXVec4LengthSq(&v1);
    EXPECT_FLOAT_EQ(lenSq, 9.0f);
    
    // Test dot product
    float dot = D3DXVec4Dot(&v1, &v2);
    EXPECT_FLOAT_EQ(dot, 1.0f);
    
    // Test normalization
    D3DXVec4Normalize(&result, &v1);
    EXPECT_NEAR(result.x, 1.0f/3.0f, 1e-5f);
    EXPECT_NEAR(result.y, 2.0f/3.0f, 1e-5f);
    EXPECT_NEAR(result.z, 2.0f/3.0f, 1e-5f);
    EXPECT_FLOAT_EQ(result.w, 0.0f);
    
    len = D3DXVec4Length(&result);
    EXPECT_NEAR(len, 1.0f, 1e-5f);
}

TEST_F(D3DXMathTest, Vec3Transform) {
    D3DMATRIX m;
    D3DXVECTOR3 v(1.0f, 0.0f, 0.0f);
    D3DXVECTOR4 result4;
    D3DXVECTOR3 result3;
    
    // Test translation
    D3DXMatrixTranslation(&m, 10.0f, 20.0f, 30.0f);
    D3DXVec3Transform(&result4, &v, &m);
    EXPECT_FLOAT_EQ(result4.x, 11.0f);
    EXPECT_FLOAT_EQ(result4.y, 20.0f);
    EXPECT_FLOAT_EQ(result4.z, 30.0f);
    EXPECT_FLOAT_EQ(result4.w, 1.0f);
    
    // Test TransformCoord (applies perspective divide)
    D3DXVec3TransformCoord(&result3, &v, &m);
    EXPECT_FLOAT_EQ(result3.x, 11.0f);
    EXPECT_FLOAT_EQ(result3.y, 20.0f);
    EXPECT_FLOAT_EQ(result3.z, 30.0f);
    
    // Test TransformNormal (ignores translation)
    D3DXVec3TransformNormal(&result3, &v, &m);
    EXPECT_FLOAT_EQ(result3.x, 1.0f);
    EXPECT_FLOAT_EQ(result3.y, 0.0f);
    EXPECT_FLOAT_EQ(result3.z, 0.0f);
}

TEST_F(D3DXMathTest, MatrixLookAt) {
    D3DMATRIX view;
    D3DXVECTOR3 eye(0.0f, 0.0f, -10.0f);
    D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    
    // Test LH version
    D3DXMatrixLookAtLH(&view, &eye, &at, &up);
    
    // Camera looking down +Z axis should have identity rotation
    // but with translation to move world by -eye position
    EXPECT_NEAR(view._11, 1.0f, 1e-5f);
    EXPECT_NEAR(view._22, 1.0f, 1e-5f);
    EXPECT_NEAR(view._33, 1.0f, 1e-5f);
    EXPECT_NEAR(view._43, 10.0f, 1e-5f); // -(-10) = 10
    
    // Test RH version
    D3DXMatrixLookAtRH(&view, &eye, &at, &up);
    
    // RH version should flip Z axis
    EXPECT_NEAR(view._11, -1.0f, 1e-5f); // X axis flipped
    EXPECT_NEAR(view._22, 1.0f, 1e-5f);
    EXPECT_NEAR(view._33, -1.0f, 1e-5f); // Z axis flipped
    EXPECT_NEAR(view._43, -10.0f, 1e-5f);
}

TEST_F(D3DXMathTest, ComplexMatrixChain) {
    // Test a complex transformation chain
    D3DMATRIX scale, rotY, trans, combined, temp;
    
    D3DXMatrixScaling(&scale, 2.0f, 2.0f, 2.0f);
    D3DXMatrixRotationY(&rotY, 3.14159f / 4.0f); // 45 degrees
    D3DXMatrixTranslation(&trans, 10.0f, 0.0f, 0.0f);
    
    // Combine: Scale -> Rotate -> Translate
    D3DXMatrixMultiply(&temp, &scale, &rotY);
    D3DXMatrixMultiply(&combined, &temp, &trans);
    
    // Test with a point
    D3DXVECTOR3 point(1.0f, 1.0f, 0.0f);
    D3DXVECTOR3 result;
    D3DXVec3TransformCoord(&result, &point, &combined);
    
    // Point should be scaled by 2, rotated 45 degrees, then translated
    float sqrt2 = sqrtf(2.0f);
    EXPECT_NEAR(result.x, 10.0f + sqrt2, 1e-5f);
    EXPECT_NEAR(result.y, 2.0f, 1e-5f);
    EXPECT_NEAR(result.z, -sqrt2, 1e-5f);
    
    // Verify inverse works
    D3DMATRIX inverse;
    float det;
    D3DXMatrixInverse(&inverse, &det, &combined);
    D3DXMatrixMultiply(&temp, &combined, &inverse);
    EXPECT_TRUE(MatrixNear(temp, IdentityMatrix()));
}