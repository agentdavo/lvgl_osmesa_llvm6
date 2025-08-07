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
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += pM1->m(i, k) * pM2->m(k, j);
            }
            result.m(i, j) = sum;
        }
    }
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

} // extern "C"