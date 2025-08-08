#include "d3dx_compat.h"
#include <cmath>
#include <cstring>

// D3DX Math Implementation
extern "C" {

// Matrix operations
D3DMATRIX* D3DXMatrixIdentity(D3DMATRIX* pOut) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixMultiply(D3DMATRIX* pOut, const D3DMATRIX* pM1, const D3DMATRIX* pM2) {
    if (!pOut || !pM1 || !pM2) return nullptr;
    
    D3DMATRIX result;
    
    // DirectX uses row-major matrices, so we multiply rows of M1 by columns of M2
    // This follows the standard mathematical convention: result = M1 × M2
    // where transformations are applied from right to left: v' = v × M1 × M2
    
    // Row 1
    result._11 = pM1->_11 * pM2->_11 + pM1->_12 * pM2->_21 + pM1->_13 * pM2->_31 + pM1->_14 * pM2->_41;
    result._12 = pM1->_11 * pM2->_12 + pM1->_12 * pM2->_22 + pM1->_13 * pM2->_32 + pM1->_14 * pM2->_42;
    result._13 = pM1->_11 * pM2->_13 + pM1->_12 * pM2->_23 + pM1->_13 * pM2->_33 + pM1->_14 * pM2->_43;
    result._14 = pM1->_11 * pM2->_14 + pM1->_12 * pM2->_24 + pM1->_13 * pM2->_34 + pM1->_14 * pM2->_44;
    
    // Row 2
    result._21 = pM1->_21 * pM2->_11 + pM1->_22 * pM2->_21 + pM1->_23 * pM2->_31 + pM1->_24 * pM2->_41;
    result._22 = pM1->_21 * pM2->_12 + pM1->_22 * pM2->_22 + pM1->_23 * pM2->_32 + pM1->_24 * pM2->_42;
    result._23 = pM1->_21 * pM2->_13 + pM1->_22 * pM2->_23 + pM1->_23 * pM2->_33 + pM1->_24 * pM2->_43;
    result._24 = pM1->_21 * pM2->_14 + pM1->_22 * pM2->_24 + pM1->_23 * pM2->_34 + pM1->_24 * pM2->_44;
    
    // Row 3
    result._31 = pM1->_31 * pM2->_11 + pM1->_32 * pM2->_21 + pM1->_33 * pM2->_31 + pM1->_34 * pM2->_41;
    result._32 = pM1->_31 * pM2->_12 + pM1->_32 * pM2->_22 + pM1->_33 * pM2->_32 + pM1->_34 * pM2->_42;
    result._33 = pM1->_31 * pM2->_13 + pM1->_32 * pM2->_23 + pM1->_33 * pM2->_33 + pM1->_34 * pM2->_43;
    result._34 = pM1->_31 * pM2->_14 + pM1->_32 * pM2->_24 + pM1->_33 * pM2->_34 + pM1->_34 * pM2->_44;
    
    // Row 4
    result._41 = pM1->_41 * pM2->_11 + pM1->_42 * pM2->_21 + pM1->_43 * pM2->_31 + pM1->_44 * pM2->_41;
    result._42 = pM1->_41 * pM2->_12 + pM1->_42 * pM2->_22 + pM1->_43 * pM2->_32 + pM1->_44 * pM2->_42;
    result._43 = pM1->_41 * pM2->_13 + pM1->_42 * pM2->_23 + pM1->_43 * pM2->_33 + pM1->_44 * pM2->_43;
    result._44 = pM1->_41 * pM2->_14 + pM1->_42 * pM2->_24 + pM1->_43 * pM2->_34 + pM1->_44 * pM2->_44;
    
    *pOut = result;
    return pOut;
}

D3DMATRIX* D3DXMatrixTranspose(D3DMATRIX* pOut, const D3DMATRIX* pM) {
    if (!pOut || !pM) return nullptr;
    
    D3DMATRIX result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m(i, j) = pM->m(j, i);
        }
    }
    *pOut = result;
    return pOut;
}

// Helper function to calculate 3x3 determinant
static float Det3x3(float a11, float a12, float a13,
                   float a21, float a22, float a23,
                   float a31, float a32, float a33) {
    return a11 * (a22 * a33 - a23 * a32) -
           a12 * (a21 * a33 - a23 * a31) +
           a13 * (a21 * a32 - a22 * a31);
}

float D3DXMatrixDeterminant(const D3DMATRIX* pM) {
    if (!pM) return 0.0f;
    
    // Calculate determinant using cofactor expansion along first row
    float det = 0.0f;
    
    det += pM->_11 * Det3x3(pM->_22, pM->_23, pM->_24,
                            pM->_32, pM->_33, pM->_34,
                            pM->_42, pM->_43, pM->_44);
    
    det -= pM->_12 * Det3x3(pM->_21, pM->_23, pM->_24,
                            pM->_31, pM->_33, pM->_34,
                            pM->_41, pM->_43, pM->_44);
    
    det += pM->_13 * Det3x3(pM->_21, pM->_22, pM->_24,
                            pM->_31, pM->_32, pM->_34,
                            pM->_41, pM->_42, pM->_44);
    
    det -= pM->_14 * Det3x3(pM->_21, pM->_22, pM->_23,
                            pM->_31, pM->_32, pM->_33,
                            pM->_41, pM->_42, pM->_43);
    
    return det;
}

D3DMATRIX* D3DXMatrixInverse(D3DMATRIX* pOut, float* pDeterminant, const D3DMATRIX* pM) {
    if (!pOut || !pM) return nullptr;
    
    // Calculate determinant
    float det = D3DXMatrixDeterminant(pM);
    if (pDeterminant) *pDeterminant = det;
    
    // Check for singular matrix
    if (fabsf(det) < 1e-7f) {
        // Matrix is singular, return identity
        D3DXMatrixIdentity(pOut);
        return nullptr;
    }
    
    // Calculate cofactor matrix
    D3DMATRIX cofactor;
    
    // Row 1
    cofactor._11 = Det3x3(pM->_22, pM->_23, pM->_24,
                          pM->_32, pM->_33, pM->_34,
                          pM->_42, pM->_43, pM->_44);
    cofactor._12 = -Det3x3(pM->_21, pM->_23, pM->_24,
                           pM->_31, pM->_33, pM->_34,
                           pM->_41, pM->_43, pM->_44);
    cofactor._13 = Det3x3(pM->_21, pM->_22, pM->_24,
                          pM->_31, pM->_32, pM->_34,
                          pM->_41, pM->_42, pM->_44);
    cofactor._14 = -Det3x3(pM->_21, pM->_22, pM->_23,
                           pM->_31, pM->_32, pM->_33,
                           pM->_41, pM->_42, pM->_43);
    
    // Row 2
    cofactor._21 = -Det3x3(pM->_12, pM->_13, pM->_14,
                           pM->_32, pM->_33, pM->_34,
                           pM->_42, pM->_43, pM->_44);
    cofactor._22 = Det3x3(pM->_11, pM->_13, pM->_14,
                          pM->_31, pM->_33, pM->_34,
                          pM->_41, pM->_43, pM->_44);
    cofactor._23 = -Det3x3(pM->_11, pM->_12, pM->_14,
                           pM->_31, pM->_32, pM->_34,
                           pM->_41, pM->_42, pM->_44);
    cofactor._24 = Det3x3(pM->_11, pM->_12, pM->_13,
                          pM->_31, pM->_32, pM->_33,
                          pM->_41, pM->_42, pM->_43);
    
    // Row 3
    cofactor._31 = Det3x3(pM->_12, pM->_13, pM->_14,
                          pM->_22, pM->_23, pM->_24,
                          pM->_42, pM->_43, pM->_44);
    cofactor._32 = -Det3x3(pM->_11, pM->_13, pM->_14,
                           pM->_21, pM->_23, pM->_24,
                           pM->_41, pM->_43, pM->_44);
    cofactor._33 = Det3x3(pM->_11, pM->_12, pM->_14,
                          pM->_21, pM->_22, pM->_24,
                          pM->_41, pM->_42, pM->_44);
    cofactor._34 = -Det3x3(pM->_11, pM->_12, pM->_13,
                           pM->_21, pM->_22, pM->_23,
                           pM->_41, pM->_42, pM->_43);
    
    // Row 4
    cofactor._41 = -Det3x3(pM->_12, pM->_13, pM->_14,
                           pM->_22, pM->_23, pM->_24,
                           pM->_32, pM->_33, pM->_34);
    cofactor._42 = Det3x3(pM->_11, pM->_13, pM->_14,
                          pM->_21, pM->_23, pM->_24,
                          pM->_31, pM->_33, pM->_34);
    cofactor._43 = -Det3x3(pM->_11, pM->_12, pM->_14,
                           pM->_21, pM->_22, pM->_24,
                           pM->_31, pM->_32, pM->_34);
    cofactor._44 = Det3x3(pM->_11, pM->_12, pM->_13,
                          pM->_21, pM->_22, pM->_23,
                          pM->_31, pM->_32, pM->_33);
    
    // Transpose cofactor matrix and divide by determinant to get inverse
    float invDet = 1.0f / det;
    pOut->_11 = cofactor._11 * invDet;
    pOut->_12 = cofactor._21 * invDet;
    pOut->_13 = cofactor._31 * invDet;
    pOut->_14 = cofactor._41 * invDet;
    
    pOut->_21 = cofactor._12 * invDet;
    pOut->_22 = cofactor._22 * invDet;
    pOut->_23 = cofactor._32 * invDet;
    pOut->_24 = cofactor._42 * invDet;
    
    pOut->_31 = cofactor._13 * invDet;
    pOut->_32 = cofactor._23 * invDet;
    pOut->_33 = cofactor._33 * invDet;
    pOut->_34 = cofactor._43 * invDet;
    
    pOut->_41 = cofactor._14 * invDet;
    pOut->_42 = cofactor._24 * invDet;
    pOut->_43 = cofactor._34 * invDet;
    pOut->_44 = cofactor._44 * invDet;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixScaling(D3DMATRIX* pOut, float sx, float sy, float sz) {
    if (!pOut) return nullptr;
    
    D3DXMatrixIdentity(pOut);
    pOut->_11 = sx;
    pOut->_22 = sy;
    pOut->_33 = sz;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixTranslation(D3DMATRIX* pOut, float x, float y, float z) {
    if (!pOut) return nullptr;
    
    D3DXMatrixIdentity(pOut);
    pOut->_41 = x;
    pOut->_42 = y;
    pOut->_43 = z;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixRotationX(D3DMATRIX* pOut, float angle) {
    if (!pOut) return nullptr;
    
    float c = cosf(angle);
    float s = sinf(angle);
    
    D3DXMatrixIdentity(pOut);
    pOut->_22 = c;
    pOut->_23 = s;
    pOut->_32 = -s;
    pOut->_33 = c;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixRotationY(D3DMATRIX* pOut, float angle) {
    if (!pOut) return nullptr;
    
    float c = cosf(angle);
    float s = sinf(angle);
    
    D3DXMatrixIdentity(pOut);
    pOut->_11 = c;
    pOut->_13 = -s;
    pOut->_31 = s;
    pOut->_33 = c;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixRotationZ(D3DMATRIX* pOut, float angle) {
    if (!pOut) return nullptr;
    
    float c = cosf(angle);
    float s = sinf(angle);
    
    D3DXMatrixIdentity(pOut);
    pOut->_11 = c;
    pOut->_12 = s;
    pOut->_21 = -s;
    pOut->_22 = c;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixRotationYawPitchRoll(D3DMATRIX* pOut, float yaw, float pitch, float roll) {
    if (!pOut) return nullptr;
    
    // DirectX convention: Apply rotations in the order Roll (Z), Pitch (X), Yaw (Y)
    // This matches the standard airplane convention where:
    // - Yaw is rotation around Y (vertical) axis
    // - Pitch is rotation around X (lateral) axis  
    // - Roll is rotation around Z (longitudinal) axis
    
    // Calculate sin/cos for all three angles
    float cy = cosf(yaw);
    float sy = sinf(yaw);
    float cp = cosf(pitch);
    float sp = sinf(pitch);
    float cr = cosf(roll);
    float sr = sinf(roll);
    
    // Combined rotation matrix (Roll × Pitch × Yaw)
    pOut->_11 = cr * cy + sr * sp * sy;
    pOut->_12 = sr * cp;
    pOut->_13 = sr * sp * cy - cr * sy;
    pOut->_14 = 0.0f;
    
    pOut->_21 = cr * sp * sy - sr * cy;
    pOut->_22 = cr * cp;
    pOut->_23 = sr * sy + cr * sp * cy;
    pOut->_24 = 0.0f;
    
    pOut->_31 = cp * sy;
    pOut->_32 = -sp;
    pOut->_33 = cp * cy;
    pOut->_34 = 0.0f;
    
    pOut->_41 = 0.0f;
    pOut->_42 = 0.0f;
    pOut->_43 = 0.0f;
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixPerspectiveFovLH(D3DMATRIX* pOut, float fovy, float aspect, float zn, float zf) {
    if (!pOut) return nullptr;
    
    float yScale = 1.0f / tanf(fovy * 0.5f);
    float xScale = yScale / aspect;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = xScale;
    pOut->_22 = yScale;
    pOut->_33 = zf / (zf - zn);
    pOut->_34 = 1.0f;
    pOut->_43 = -zn * zf / (zf - zn);
    
    return pOut;
}

D3DMATRIX* D3DXMatrixLookAtLH(D3DMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp) {
    if (!pOut || !pEye || !pAt || !pUp) return nullptr;
    
    // Calculate look direction
    D3DXVECTOR3 zaxis;
    zaxis.x = pAt->x - pEye->x;
    zaxis.y = pAt->y - pEye->y;
    zaxis.z = pAt->z - pEye->z;
    D3DXVec3Normalize(&zaxis, &zaxis);
    
    // Calculate right vector (cross product of up and look)
    D3DXVECTOR3 xaxis;
    D3DXVec3Cross(&xaxis, const_cast<D3DXVECTOR3*>(pUp), &zaxis);
    D3DXVec3Normalize(&xaxis, &xaxis);
    
    // Calculate actual up vector (cross product of look and right)
    D3DXVECTOR3 yaxis;
    D3DXVec3Cross(&yaxis, &zaxis, &xaxis);
    
    // Build the view matrix
    pOut->_11 = xaxis.x;
    pOut->_21 = xaxis.y;
    pOut->_31 = xaxis.z;
    pOut->_41 = -D3DXVec3Dot(&xaxis, pEye);
    
    pOut->_12 = yaxis.x;
    pOut->_22 = yaxis.y;
    pOut->_32 = yaxis.z;
    pOut->_42 = -D3DXVec3Dot(&yaxis, pEye);
    
    pOut->_13 = zaxis.x;
    pOut->_23 = zaxis.y;
    pOut->_33 = zaxis.z;
    pOut->_43 = -D3DXVec3Dot(&zaxis, pEye);
    
    pOut->_14 = 0.0f;
    pOut->_24 = 0.0f;
    pOut->_34 = 0.0f;
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixOrthoLH(D3DMATRIX* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f / w;
    pOut->_22 = 2.0f / h;
    pOut->_33 = 1.0f / (zf - zn);
    pOut->_43 = -zn / (zf - zn);
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixOrthoRH(D3DMATRIX* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f / w;
    pOut->_22 = 2.0f / h;
    pOut->_33 = 1.0f / (zn - zf);
    pOut->_43 = zn / (zn - zf);
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixOrthoOffCenterLH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f / (r - l);
    pOut->_22 = 2.0f / (t - b);
    pOut->_33 = 1.0f / (zf - zn);
    pOut->_41 = (l + r) / (l - r);
    pOut->_42 = (t + b) / (b - t);
    pOut->_43 = zn / (zn - zf);
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixOrthoOffCenterRH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f / (r - l);
    pOut->_22 = 2.0f / (t - b);
    pOut->_33 = 1.0f / (zn - zf);
    pOut->_41 = (l + r) / (l - r);
    pOut->_42 = (t + b) / (b - t);
    pOut->_43 = zn / (zn - zf);
    pOut->_44 = 1.0f;
    
    return pOut;
}

D3DMATRIX* D3DXMatrixPerspectiveLH(D3DMATRIX* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f * zn / w;
    pOut->_22 = 2.0f * zn / h;
    pOut->_33 = zf / (zf - zn);
    pOut->_34 = 1.0f;
    pOut->_43 = -zn * zf / (zf - zn);
    
    return pOut;
}

D3DMATRIX* D3DXMatrixPerspectiveRH(D3DMATRIX* pOut, float w, float h, float zn, float zf) {
    if (!pOut) return nullptr;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = 2.0f * zn / w;
    pOut->_22 = 2.0f * zn / h;
    pOut->_33 = zf / (zn - zf);
    pOut->_34 = -1.0f;
    pOut->_43 = zn * zf / (zn - zf);
    
    return pOut;
}

D3DMATRIX* D3DXMatrixPerspectiveFovRH(D3DMATRIX* pOut, float fovy, float aspect, float zn, float zf) {
    if (!pOut) return nullptr;
    
    float yScale = 1.0f / tanf(fovy * 0.5f);
    float xScale = yScale / aspect;
    
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = xScale;
    pOut->_22 = yScale;
    pOut->_33 = zf / (zn - zf);
    pOut->_34 = -1.0f;
    pOut->_43 = zn * zf / (zn - zf);
    
    return pOut;
}

D3DMATRIX* D3DXMatrixLookAtRH(D3DMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp) {
    if (!pOut || !pEye || !pAt || !pUp) return nullptr;
    
    // Calculate look direction (negated for RH)
    D3DXVECTOR3 zaxis;
    zaxis.x = pEye->x - pAt->x;
    zaxis.y = pEye->y - pAt->y;
    zaxis.z = pEye->z - pAt->z;
    D3DXVec3Normalize(&zaxis, &zaxis);
    
    // Calculate right vector (cross product of up and look)
    D3DXVECTOR3 xaxis;
    D3DXVec3Cross(&xaxis, const_cast<D3DXVECTOR3*>(pUp), &zaxis);
    D3DXVec3Normalize(&xaxis, &xaxis);
    
    // Calculate actual up vector (cross product of look and right)
    D3DXVECTOR3 yaxis;
    D3DXVec3Cross(&yaxis, &zaxis, &xaxis);
    
    // Build the view matrix
    pOut->_11 = xaxis.x;
    pOut->_21 = xaxis.y;
    pOut->_31 = xaxis.z;
    pOut->_41 = -D3DXVec3Dot(&xaxis, pEye);
    
    pOut->_12 = yaxis.x;
    pOut->_22 = yaxis.y;
    pOut->_32 = yaxis.z;
    pOut->_42 = -D3DXVec3Dot(&yaxis, pEye);
    
    pOut->_13 = zaxis.x;
    pOut->_23 = zaxis.y;
    pOut->_33 = zaxis.z;
    pOut->_43 = -D3DXVec3Dot(&zaxis, pEye);
    
    pOut->_14 = 0.0f;
    pOut->_24 = 0.0f;
    pOut->_34 = 0.0f;
    pOut->_44 = 1.0f;
    
    return pOut;
}

// Vector operations
float WINAPI D3DXVec3Length(const D3DXVECTOR3* pV) {
    if (!pV) return 0.0f;
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
}

float WINAPI D3DXVec3LengthSq(const D3DXVECTOR3* pV) {
    if (!pV) return 0.0f;
    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z;
}

float WINAPI D3DXVec3Dot(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pV1 || !pV2) return 0.0f;
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

D3DXVECTOR3* WINAPI D3DXVec3Cross(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    D3DXVECTOR3 result;
    result.x = pV1->y * pV2->z - pV1->z * pV2->y;
    result.y = pV1->z * pV2->x - pV1->x * pV2->z;
    result.z = pV1->x * pV2->y - pV1->y * pV2->x;
    
    *pOut = result;
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV) {
    if (!pOut || !pV) return nullptr;
    
    float length = D3DXVec3Length(pV);
    if (length == 0.0f) {
        pOut->x = pOut->y = pOut->z = 0.0f;
    } else {
        pOut->x = pV->x / length;
        pOut->y = pV->y / length;
        pOut->z = pV->z / length;
    }
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Add(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    pOut->z = pV1->z + pV2->z;
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Subtract(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Minimize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    pOut->x = (pV1->x < pV2->x) ? pV1->x : pV2->x;
    pOut->y = (pV1->y < pV2->y) ? pV1->y : pV2->y;
    pOut->z = (pV1->z < pV2->z) ? pV1->z : pV2->z;
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Maximize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    pOut->x = (pV1->x > pV2->x) ? pV1->x : pV2->x;
    pOut->y = (pV1->y > pV2->y) ? pV1->y : pV2->y;
    pOut->z = (pV1->z > pV2->z) ? pV1->z : pV2->z;
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Scale(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, float s) {
    if (!pOut || !pV) return nullptr;
    
    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    pOut->z = pV->z * s;
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3Lerp(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, float s) {
    if (!pOut || !pV1 || !pV2) return nullptr;
    
    // Linear interpolation: result = V1 + s * (V2 - V1)
    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    pOut->z = pV1->z + s * (pV2->z - pV1->z);
    
    return pOut;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM) {
    if (!pOut || !pV || !pM) return nullptr;
    
    D3DXVECTOR4 result;
    result.x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pM->_41;
    result.y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pM->_42;
    result.z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pM->_43;
    result.w = pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pM->_44;
    
    *pOut = result;
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM) {
    if (!pOut || !pV || !pM) return nullptr;
    
    D3DXVECTOR4 v4;
    D3DXVec3Transform(&v4, pV, pM);
    
    if (v4.w != 0.0f) {
        pOut->x = v4.x / v4.w;
        pOut->y = v4.y / v4.w;
        pOut->z = v4.z / v4.w;
    } else {
        pOut->x = v4.x;
        pOut->y = v4.y;
        pOut->z = v4.z;
    }
    
    return pOut;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM) {
    if (!pOut || !pV || !pM) return nullptr;
    
    // Transform as direction vector (ignore translation)
    D3DXVECTOR3 result;
    result.x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31;
    result.y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32;
    result.z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33;
    
    *pOut = result;
    return pOut;
}

// Additional vector operations
float WINAPI D3DXVec2Length(const D3DXVECTOR2* pV) {
    if (!pV) return 0.0f;
    return sqrtf(pV->x * pV->x + pV->y * pV->y);
}

float WINAPI D3DXVec2LengthSq(const D3DXVECTOR2* pV) {
    if (!pV) return 0.0f;
    return pV->x * pV->x + pV->y * pV->y;
}

float WINAPI D3DXVec2Dot(const D3DXVECTOR2* pV1, const D3DXVECTOR2* pV2) {
    if (!pV1 || !pV2) return 0.0f;
    return pV1->x * pV2->x + pV1->y * pV2->y;
}

D3DXVECTOR2* WINAPI D3DXVec2Normalize(D3DXVECTOR2* pOut, const D3DXVECTOR2* pV) {
    if (!pOut || !pV) return nullptr;
    
    float length = D3DXVec2Length(pV);
    if (length == 0.0f) {
        pOut->x = pOut->y = 0.0f;
    } else {
        pOut->x = pV->x / length;
        pOut->y = pV->y / length;
    }
    
    return pOut;
}

float WINAPI D3DXVec4Length(const D3DXVECTOR4* pV) {
    if (!pV) return 0.0f;
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w);
}

float WINAPI D3DXVec4LengthSq(const D3DXVECTOR4* pV) {
    if (!pV) return 0.0f;
    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w;
}

float WINAPI D3DXVec4Dot(const D3DXVECTOR4* pV1, const D3DXVECTOR4* pV2) {
    if (!pV1 || !pV2) return 0.0f;
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z + pV1->w * pV2->w;
}

D3DXVECTOR4* WINAPI D3DXVec4Normalize(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV) {
    if (!pOut || !pV) return nullptr;
    
    float length = D3DXVec4Length(pV);
    if (length == 0.0f) {
        pOut->x = pOut->y = pOut->z = pOut->w = 0.0f;
    } else {
        pOut->x = pV->x / length;
        pOut->y = pV->y / length;
        pOut->z = pV->z / length;
        pOut->w = pV->w / length;
    }
    
    return pOut;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DMATRIX* pM) {
    if (!pOut || !pV || !pM) return nullptr;
    
    D3DXVECTOR4 result;
    result.x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pV->w * pM->_41;
    result.y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pV->w * pM->_42;
    result.z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pV->w * pM->_43;
    result.w = pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pV->w * pM->_44;
    
    *pOut = result;
    return pOut;
}

// FVF utility functions
UINT D3DXGetFVFVertexSize(DWORD FVF) {
    UINT size = 0;
    
    // Position
    if (FVF & D3DFVF_XYZ) size += 12; // 3 floats
    else if (FVF & D3DFVF_XYZRHW) size += 16; // 4 floats
    else if (FVF & D3DFVF_XYZB1) size += 16; // 4 floats
    else if (FVF & D3DFVF_XYZB2) size += 20; // 5 floats
    else if (FVF & D3DFVF_XYZB3) size += 24; // 6 floats
    else if (FVF & D3DFVF_XYZB4) size += 28; // 7 floats
    else if (FVF & D3DFVF_XYZB5) size += 32; // 8 floats
    
    // Normal
    if (FVF & D3DFVF_NORMAL) size += 12; // 3 floats
    
    // Point size
    if (FVF & D3DFVF_PSIZE) size += 4; // 1 float
    
    // Diffuse color
    if (FVF & D3DFVF_DIFFUSE) size += 4; // 1 DWORD
    
    // Specular color
    if (FVF & D3DFVF_SPECULAR) size += 4; // 1 DWORD
    
    // Texture coordinates
    DWORD tex_count = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (DWORD i = 0; i < tex_count; i++) {
        // Default to 2D texture coordinates
        size += 8; // 2 floats
    }
    
    return size;
}

// Plane operations
float D3DXPlaneDot(const D3DXPLANE* pP, const D3DXVECTOR4* pV) {
    if (!pP || !pV) return 0.0f;
    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d * pV->w;
}

float D3DXPlaneDotCoord(const D3DXPLANE* pP, const D3DXVECTOR3* pV) {
    if (!pP || !pV) return 0.0f;
    // For a point in 3D space, w = 1
    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

float D3DXPlaneDotNormal(const D3DXPLANE* pP, const D3DXVECTOR3* pV) {
    if (!pP || !pV) return 0.0f;
    // For a normal vector, w = 0 (no translation component)
    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z;
}

D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP) {
    if (!pOut || !pP) return nullptr;
    
    // Normalize the plane equation: ax + by + cz + d = 0
    // The normal vector is (a, b, c), normalize it
    float length = sqrtf(pP->a * pP->a + pP->b * pP->b + pP->c * pP->c);
    
    if (length < 1e-6f) {
        // Degenerate plane
        pOut->a = 0.0f;
        pOut->b = 0.0f;
        pOut->c = 0.0f;
        pOut->d = 0.0f;
        return pOut;
    }
    
    float invLength = 1.0f / length;
    pOut->a = pP->a * invLength;
    pOut->b = pP->b * invLength;
    pOut->c = pP->c * invLength;
    pOut->d = pP->d * invLength;
    
    return pOut;
}

D3DXPLANE* D3DXPlaneFromPointNormal(D3DXPLANE* pOut, const D3DXVECTOR3* pPoint, const D3DXVECTOR3* pNormal) {
    if (!pOut || !pPoint || !pNormal) return nullptr;
    
    // Plane equation: normal · (p - point) = 0
    // Expanding: normal · p - normal · point = 0
    // So: ax + by + cz + d = 0, where d = -normal · point
    pOut->a = pNormal->x;
    pOut->b = pNormal->y;
    pOut->c = pNormal->z;
    pOut->d = -(pNormal->x * pPoint->x + pNormal->y * pPoint->y + pNormal->z * pPoint->z);
    
    return pOut;
}

D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, const D3DXVECTOR3* pV3) {
    if (!pOut || !pV1 || !pV2 || !pV3) return nullptr;
    
    // Create two vectors from the three points
    D3DXVECTOR3 v12, v13;
    v12.x = pV2->x - pV1->x;
    v12.y = pV2->y - pV1->y;
    v12.z = pV2->z - pV1->z;
    
    v13.x = pV3->x - pV1->x;
    v13.y = pV3->y - pV1->y;
    v13.z = pV3->z - pV1->z;
    
    // Cross product gives the normal to the plane
    D3DXVECTOR3 normal;
    D3DXVec3Cross(&normal, &v12, &v13);
    
    // Create plane from point and normal
    return D3DXPlaneFromPointNormal(pOut, pV1, &normal);
}

D3DXPLANE* D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DMATRIX* pM) {
    if (!pOut || !pP || !pM) return nullptr;
    
    // To transform a plane, we need to use the inverse transpose of the matrix
    // For now, we'll do a simplified version that works for orthogonal matrices
    // Full implementation would require computing inverse transpose
    
    // Transform the plane as a 4D vector
    D3DXVECTOR4 plane(pP->a, pP->b, pP->c, pP->d);
    D3DXVECTOR4 result;
    
    // For proper plane transformation, we'd use inverse transpose
    // This simplified version works for rotation and uniform scaling
    result.x = plane.x * pM->_11 + plane.y * pM->_21 + plane.z * pM->_31 + plane.w * pM->_41;
    result.y = plane.x * pM->_12 + plane.y * pM->_22 + plane.z * pM->_32 + plane.w * pM->_42;
    result.z = plane.x * pM->_13 + plane.y * pM->_23 + plane.z * pM->_33 + plane.w * pM->_43;
    result.w = plane.x * pM->_14 + plane.y * pM->_24 + plane.z * pM->_34 + plane.w * pM->_44;
    
    pOut->a = result.x;
    pOut->b = result.y;
    pOut->c = result.z;
    pOut->d = result.w;
    
    return pOut;
}

// Color operations
D3DXCOLOR* D3DXColorAdjustSaturation(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float s) {
    if (!pOut || !pC) return nullptr;
    
    // Convert RGB to grayscale using luminance weights
    const float lumR = 0.2125f;
    const float lumG = 0.7154f;
    const float lumB = 0.0721f;
    
    float gray = pC->r * lumR + pC->g * lumG + pC->b * lumB;
    
    // Interpolate between grayscale and original color
    pOut->r = gray + s * (pC->r - gray);
    pOut->g = gray + s * (pC->g - gray);
    pOut->b = gray + s * (pC->b - gray);
    pOut->a = pC->a;
    
    // Clamp to [0, 1]
    pOut->r = (pOut->r < 0.0f) ? 0.0f : (pOut->r > 1.0f) ? 1.0f : pOut->r;
    pOut->g = (pOut->g < 0.0f) ? 0.0f : (pOut->g > 1.0f) ? 1.0f : pOut->g;
    pOut->b = (pOut->b < 0.0f) ? 0.0f : (pOut->b > 1.0f) ? 1.0f : pOut->b;
    
    return pOut;
}

D3DXCOLOR* D3DXColorAdjustContrast(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float c) {
    if (!pOut || !pC) return nullptr;
    
    // Adjust contrast around 0.5 midpoint
    const float midpoint = 0.5f;
    
    pOut->r = midpoint + c * (pC->r - midpoint);
    pOut->g = midpoint + c * (pC->g - midpoint);
    pOut->b = midpoint + c * (pC->b - midpoint);
    pOut->a = pC->a;
    
    // Clamp to [0, 1]
    pOut->r = (pOut->r < 0.0f) ? 0.0f : (pOut->r > 1.0f) ? 1.0f : pOut->r;
    pOut->g = (pOut->g < 0.0f) ? 0.0f : (pOut->g > 1.0f) ? 1.0f : pOut->g;
    pOut->b = (pOut->b < 0.0f) ? 0.0f : (pOut->b > 1.0f) ? 1.0f : pOut->b;
    
    return pOut;
}

D3DXCOLOR* D3DXColorLerp(D3DXCOLOR* pOut, const D3DXCOLOR* pC1, const D3DXCOLOR* pC2, float s) {
    if (!pOut || !pC1 || !pC2) return nullptr;
    
    // Linear interpolation: result = c1 + s * (c2 - c1)
    pOut->r = pC1->r + s * (pC2->r - pC1->r);
    pOut->g = pC1->g + s * (pC2->g - pC1->g);
    pOut->b = pC1->b + s * (pC2->b - pC1->b);
    pOut->a = pC1->a + s * (pC2->a - pC1->a);
    
    return pOut;
}

D3DXCOLOR* D3DXColorModulate(D3DXCOLOR* pOut, const D3DXCOLOR* pC1, const D3DXCOLOR* pC2) {
    if (!pOut || !pC1 || !pC2) return nullptr;
    
    // Component-wise multiplication
    pOut->r = pC1->r * pC2->r;
    pOut->g = pC1->g * pC2->g;
    pOut->b = pC1->b * pC2->b;
    pOut->a = pC1->a * pC2->a;
    
    return pOut;
}

D3DXCOLOR* D3DXColorNegative(D3DXCOLOR* pOut, const D3DXCOLOR* pC) {
    if (!pOut || !pC) return nullptr;
    
    // Invert the color (1 - c)
    pOut->r = 1.0f - pC->r;
    pOut->g = 1.0f - pC->g;
    pOut->b = 1.0f - pC->b;
    pOut->a = pC->a;  // Alpha typically not inverted
    
    return pOut;
}

D3DXCOLOR* D3DXColorScale(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float s) {
    if (!pOut || !pC) return nullptr;
    
    // Scale all components by s
    pOut->r = pC->r * s;
    pOut->g = pC->g * s;
    pOut->b = pC->b * s;
    pOut->a = pC->a * s;
    
    // Note: No clamping here as scale might be used for HDR colors
    
    return pOut;
}

} // extern "C"