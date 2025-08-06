#ifndef DX8GL_D3DX_COMPAT_H
#define DX8GL_D3DX_COMPAT_H

/**
 * DirectX 8 D3DX Utility Functions Compatibility Layer
 * 
 * This header provides D3DX8 utility function declarations
 * for DirectX 8 math, mesh, and texture utilities.
 */

#include "d3d8_types.h"
#include "d3d8_constants.h"
#include "d3d8_cpp_interfaces.h"  // For IUnknown
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* D3DX Types */
typedef float FLOAT;

/* String types */
#ifndef LPSTR
typedef char* LPSTR;
#endif

#ifndef LPCSTR
typedef const char* LPCSTR;
#endif

/* D3DX Image file formats */
typedef enum _D3DXIMAGE_FILEFORMAT {
    D3DXIFF_BMP = 0,
    D3DXIFF_JPG = 1,
    D3DXIFF_TGA = 2,
    D3DXIFF_PNG = 3,
    D3DXIFF_DDS = 4,
    D3DXIFF_PPM = 5,
    D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7,
    D3DXIFF_PFM = 8,
    D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

/* D3DXMATRIX is D3DMATRIX with C++ operators in the official SDK */
#ifdef __cplusplus
struct D3DXMATRIX : public D3DMATRIX
{
    D3DXMATRIX() {}
    D3DXMATRIX(const D3DMATRIX& mat) : D3DMATRIX(mat) {}
    D3DXMATRIX(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44)
    {
        _11 = m11; _12 = m12; _13 = m13; _14 = m14;
        _21 = m21; _22 = m22; _23 = m23; _24 = m24;
        _31 = m31; _32 = m32; _33 = m33; _34 = m34;
        _41 = m41; _42 = m42; _43 = m43; _44 = m44;
    }
    
    // Matrix math operators
    D3DXMATRIX& operator*=(const D3DXMATRIX& rhs) {
        D3DXMATRIX temp = *this;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += temp.m(i, k) * rhs.m(k, j);
                }
                m(i, j) = sum;
            }
        }
        return *this;
    }
    
    D3DXMATRIX operator*(const D3DXMATRIX& rhs) const {
        D3DXMATRIX result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += m(i, k) * rhs.m(k, j);
                }
                result.m(i, j) = sum;
            }
        }
        return result;
    }
};
#else
typedef D3DMATRIX D3DXMATRIX;
#endif

/* Windows types for D3DX font operations */
typedef void* HDC;
typedef void* HFONT;

/* Windows API constants and stubs for non-Windows platforms */
#ifndef _WIN32
#define FW_NORMAL           400
#define DEFAULT_CHARSET     1
#define OUT_DEFAULT_PRECIS  0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY     0
#define DEFAULT_PITCH       0
#define FF_DONTCARE         0

#ifndef WIN32_TYPES_DEFINED
// Only declare if win32_types.h hasn't already declared these
DX8GL_API HDC GetDC(HWND hwnd);
DX8GL_API int ReleaseDC(HWND hwnd, HDC hdc);
DX8GL_API HFONT CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation,
                 int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut,
                 DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision,
                 DWORD fdwQuality, DWORD fdwPitchAndFamily, const char* lpszFace);
DX8GL_API void* SelectObject(HDC hdc, void* hgdiobj);
DX8GL_API BOOL DeleteObject(void* hObject);
#endif
#else
#define ReleaseDC(hwnd, hdc) ((int)0)
#endif

/* Visibility macros for symbol export */
#if defined(_WIN32)
    #ifdef DX8GL_BUILDING_DLL
        #define DX8GL_API __declspec(dllexport)
    #else
        #define DX8GL_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define DX8GL_API __attribute__((visibility("default")))
#else
    #define DX8GL_API
#endif

/* D3DX Constants */
#define D3DX_PI    ((FLOAT)  3.141592654f)
#define D3DX_1BYPI ((FLOAT)  0.318309886f)

/* D3DX filter constants */
#define D3DX_DEFAULT         ((UINT) -1)
#define D3DX_FILTER_NONE     (1 << 0)
#define D3DX_FILTER_POINT    (2 << 0)
#define D3DX_FILTER_LINEAR   (3 << 0)
#define D3DX_FILTER_TRIANGLE (4 << 0)
#define D3DX_FILTER_BOX      (5 << 0)

#define D3DX_FILTER_MIRROR_U (1 << 16)
#define D3DX_FILTER_MIRROR_V (2 << 16)
#define D3DX_FILTER_MIRROR_W (4 << 16)
#define D3DX_FILTER_MIRROR   (7 << 16)
#define D3DX_FILTER_DITHER   (1 << 19)

/* D3DX Vector types */
typedef struct D3DXVECTOR2 {
    float x, y;
#ifdef __cplusplus
    D3DXVECTOR2() : x(0), y(0) {}
    D3DXVECTOR2(float _x, float _y) : x(_x), y(_y) {}
#endif
} D3DXVECTOR2;

typedef struct D3DXVECTOR3 {
    float x, y, z;
#ifdef __cplusplus
    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
#endif
} D3DXVECTOR3;

typedef struct D3DXVECTOR4 {
    float x, y, z, w;
#ifdef __cplusplus
    D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
    D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
#endif
} D3DXVECTOR4;

/* D3DX Quaternion */
typedef struct D3DXQUATERNION {
    float x, y, z, w;
} D3DXQUATERNION;

/* D3DX Plane */
typedef struct D3DXPLANE {
    float a, b, c, d;
} D3DXPLANE;

/* D3DX Color */
typedef struct D3DXCOLOR {
    float r, g, b, a;
} D3DXCOLOR;

/* Forward declarations for D3DX interfaces */
struct ID3DXMesh;
struct ID3DXBuffer;

#ifdef __cplusplus
// Pure C++ interfaces for D3DX

// ID3DXBuffer interface
struct ID3DXBuffer : public IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};

// ID3DXMesh interface
struct ID3DXMesh : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE DrawSubset(DWORD AttribId) = 0;
    virtual DWORD STDMETHODCALLTYPE GetNumFaces() = 0;
    virtual DWORD STDMETHODCALLTYPE GetNumVertices() = 0;
    virtual DWORD STDMETHODCALLTYPE GetFVF() = 0;
    // Add more methods as needed
};

#else
// C compatibility - opaque pointers
typedef struct ID3DXMesh ID3DXMesh;
typedef struct ID3DXBuffer ID3DXBuffer;
#endif

/* Matrix operations */
DX8GL_API D3DMATRIX* D3DXMatrixIdentity(D3DMATRIX* pOut);
DX8GL_API D3DMATRIX* D3DXMatrixMultiply(D3DMATRIX* pOut, const D3DMATRIX* pM1, const D3DMATRIX* pM2);
DX8GL_API D3DMATRIX* D3DXMatrixTranspose(D3DMATRIX* pOut, const D3DMATRIX* pM);
DX8GL_API D3DMATRIX* D3DXMatrixInverse(D3DMATRIX* pOut, float* pDeterminant, const D3DMATRIX* pM);
DX8GL_API D3DMATRIX* D3DXMatrixScaling(D3DMATRIX* pOut, float sx, float sy, float sz);
DX8GL_API D3DMATRIX* D3DXMatrixTranslation(D3DMATRIX* pOut, float x, float y, float z);
DX8GL_API D3DMATRIX* D3DXMatrixRotationX(D3DMATRIX* pOut, float angle);
DX8GL_API D3DMATRIX* D3DXMatrixRotationY(D3DMATRIX* pOut, float angle);
DX8GL_API D3DMATRIX* D3DXMatrixRotationZ(D3DMATRIX* pOut, float angle);
DX8GL_API D3DMATRIX* D3DXMatrixRotationAxis(D3DMATRIX* pOut, const D3DXVECTOR3* pV, float angle);
DX8GL_API D3DMATRIX* D3DXMatrixRotationQuaternion(D3DMATRIX* pOut, const D3DXQUATERNION* pQ);
DX8GL_API D3DMATRIX* D3DXMatrixRotationYawPitchRoll(D3DMATRIX* pOut, float yaw, float pitch, float roll);
DX8GL_API D3DMATRIX* D3DXMatrixLookAtLH(D3DMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp);
DX8GL_API D3DMATRIX* D3DXMatrixLookAtRH(D3DMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveFovLH(D3DMATRIX* pOut, float fovy, float aspect, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveFovRH(D3DMATRIX* pOut, float fovy, float aspect, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveLH(D3DMATRIX* pOut, float w, float h, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveRH(D3DMATRIX* pOut, float w, float h, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveOffCenterLH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixPerspectiveOffCenterRH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixOrthoLH(D3DMATRIX* pOut, float w, float h, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixOrthoRH(D3DMATRIX* pOut, float w, float h, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixOrthoOffCenterLH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf);
DX8GL_API D3DMATRIX* D3DXMatrixOrthoOffCenterRH(D3DMATRIX* pOut, float l, float r, float b, float t, float zn, float zf);

/* Vector operations */
DX8GL_API float WINAPI D3DXVec3Length(const D3DXVECTOR3* pV);
DX8GL_API float WINAPI D3DXVec3LengthSq(const D3DXVECTOR3* pV);
DX8GL_API float WINAPI D3DXVec3Dot(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API float WINAPI D3DXVec4Dot(const D3DXVECTOR4* pV1, const D3DXVECTOR4* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Cross(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Add(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Subtract(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Minimize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Maximize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Scale(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, float s);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Lerp(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, float s);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV);
DX8GL_API D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM);
DX8GL_API D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DMATRIX* pM);
DX8GL_API D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DMATRIX* pM);

/* Quaternion operations */
DX8GL_API float D3DXQuaternionLength(const D3DXQUATERNION* pQ);
DX8GL_API float D3DXQuaternionLengthSq(const D3DXQUATERNION* pQ);
DX8GL_API float D3DXQuaternionDot(const D3DXQUATERNION* pQ1, const D3DXQUATERNION* pQ2);
DX8GL_API D3DXQUATERNION* D3DXQuaternionIdentity(D3DXQUATERNION* pOut);
DX8GL_API D3DXQUATERNION* D3DXQuaternionConjugate(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ);
DX8GL_API D3DXQUATERNION* D3DXQuaternionInverse(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ);
DX8GL_API D3DXQUATERNION* D3DXQuaternionNormalize(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ);
DX8GL_API D3DXQUATERNION* D3DXQuaternionMultiply(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ1, const D3DXQUATERNION* pQ2);
DX8GL_API D3DXQUATERNION* D3DXQuaternionRotationMatrix(D3DXQUATERNION* pOut, const D3DMATRIX* pM);
DX8GL_API D3DXQUATERNION* D3DXQuaternionRotationAxis(D3DXQUATERNION* pOut, const D3DXVECTOR3* pV, float angle);
DX8GL_API D3DXQUATERNION* D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION* pOut, float yaw, float pitch, float roll);
DX8GL_API D3DXQUATERNION* D3DXQuaternionSlerp(D3DXQUATERNION* pOut, const D3DXQUATERNION* pQ1, const D3DXQUATERNION* pQ2, float t);

/* Plane operations */
float D3DXPlaneDot(const D3DXPLANE* pP, const D3DXVECTOR4* pV);
float D3DXPlaneDotCoord(const D3DXPLANE* pP, const D3DXVECTOR3* pV);
float D3DXPlaneDotNormal(const D3DXPLANE* pP, const D3DXVECTOR3* pV);
D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP);
D3DXPLANE* D3DXPlaneFromPointNormal(D3DXPLANE* pOut, const D3DXVECTOR3* pPoint, const D3DXVECTOR3* pNormal);
D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, const D3DXVECTOR3* pV3);
D3DXPLANE* D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DMATRIX* pM);

/* Color operations */
DX8GL_API D3DXCOLOR* D3DXColorAdjustSaturation(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float s);
DX8GL_API D3DXCOLOR* D3DXColorAdjustContrast(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float c);
DX8GL_API D3DXCOLOR* D3DXColorLerp(D3DXCOLOR* pOut, const D3DXCOLOR* pC1, const D3DXCOLOR* pC2, float s);
DX8GL_API D3DXCOLOR* D3DXColorModulate(D3DXCOLOR* pOut, const D3DXCOLOR* pC1, const D3DXCOLOR* pC2);
DX8GL_API D3DXCOLOR* D3DXColorNegative(D3DXCOLOR* pOut, const D3DXCOLOR* pC);
DX8GL_API D3DXCOLOR* D3DXColorScale(D3DXCOLOR* pOut, const D3DXCOLOR* pC, float s);

/* Mesh utility functions */
DX8GL_API HRESULT D3DXComputeBoundingBox(const D3DXVECTOR3* pFirstPosition, DWORD NumVertices, DWORD dwStride, D3DXVECTOR3* pMin, D3DXVECTOR3* pMax);
DX8GL_API HRESULT D3DXComputeBoundingSphere(const D3DXVECTOR3* pFirstPosition, DWORD NumVertices, DWORD dwStride, D3DXVECTOR3* pCenter, float* pRadius);

/* FVF utility functions */
DX8GL_API UINT D3DXGetFVFVertexSize(DWORD FVF);

/* Error string function - declared with WINAPI for compatibility */
HRESULT WINAPI D3DXGetErrorStringA(HRESULT hr, LPSTR pBuffer, UINT BufferLen);

/* Mesh creation functions */
DX8GL_API HRESULT D3DXCreateBox(IDirect3DDevice8* pDevice, float Width, float Height, float Depth, void** ppMesh, void** ppAdjacency);
DX8GL_API HRESULT D3DXCreateCylinder(IDirect3DDevice8* pDevice, float Radius1, float Radius2, float Length, DWORD Slices, DWORD Stacks, void** ppMesh, void** ppAdjacency);
DX8GL_API HRESULT D3DXCreateSphere(IDirect3DDevice8* pDevice, float Radius, DWORD Slices, DWORD Stacks, void** ppMesh, void** ppAdjacency);
DX8GL_API HRESULT D3DXCreateTorus(IDirect3DDevice8* pDevice, float InnerRadius, float OuterRadius, DWORD Sides, DWORD Rings, void** ppMesh, void** ppAdjacency);
DX8GL_API HRESULT D3DXCreateTeapot(IDirect3DDevice8* pDevice, void** ppMesh, void** ppAdjacency);
DX8GL_API HRESULT D3DXCreateText(IDirect3DDevice8* pDevice, void* hDC, const char* pText, float Deviation, float Extrusion, void** ppMesh, void** ppAdjacency, void* pGlyphMetrics);

/* Texture creation functions */
DX8GL_API HRESULT D3DXCreateTexture(IDirect3DDevice8* pDevice, DWORD Width, DWORD Height, DWORD MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture);
DX8GL_API HRESULT D3DXCreateTextureFromFile(IDirect3DDevice8* pDevice, const char* pSrcFile, IDirect3DTexture8** ppTexture);
DX8GL_API HRESULT D3DXCreateTextureFromFileEx(IDirect3DDevice8* pDevice, const char* pSrcFile, DWORD Width, DWORD Height, DWORD MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, void* pSrcInfo, void* pPalette, IDirect3DTexture8** ppTexture);
DX8GL_API HRESULT D3DXCreateTextureFromFileExA(IDirect3DDevice8* pDevice, const char* pSrcFile, DWORD Width, DWORD Height, DWORD MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, void* pSrcInfo, void* pPalette, IDirect3DTexture8** ppTexture);
HRESULT D3DXCreateTextureFromResource(IDirect3DDevice8* pDevice, void* hSrcModule, const char* pSrcResource, IDirect3DTexture8** ppTexture);
HRESULT D3DXCreateTextureFromResourceEx(IDirect3DDevice8* pDevice, void* hSrcModule, const char* pSrcResource, DWORD Width, DWORD Height, DWORD MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, void* pSrcInfo, void* pPalette, IDirect3DTexture8** ppTexture);
HRESULT D3DXCreateTextureFromFileInMemory(IDirect3DDevice8* pDevice, const void* pSrcData, DWORD SrcDataSize, IDirect3DTexture8** ppTexture);
HRESULT D3DXCreateTextureFromFileInMemoryEx(IDirect3DDevice8* pDevice, const void* pSrcData, DWORD SrcDataSize, DWORD Width, DWORD Height, DWORD MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, void* pSrcInfo, void* pPalette, IDirect3DTexture8** ppTexture);

/* Texture utilities */
HRESULT D3DXFilterTexture(IDirect3DTexture8* pTexture, void* pPalette, DWORD SrcLevel, DWORD Filter);
HRESULT D3DXFillTexture(IDirect3DTexture8* pTexture, void* pFunction, void* pData);
HRESULT D3DXComputeNormalMap(IDirect3DTexture8* pTexture, IDirect3DTexture8* pSrcTexture, void* pSrcPalette, DWORD Flags, DWORD Channel, float Amplitude);

/* Surface utilities */
DX8GL_API HRESULT WINAPI D3DXLoadSurfaceFromFile(IDirect3DSurface8* pDestSurface, const PALETTEENTRY* pDestPalette, const RECT* pDestRect, const char* pSrcFile, const RECT* pSrcRect, DWORD Filter, D3DCOLOR ColorKey, void* pSrcInfo);
DX8GL_API HRESULT WINAPI D3DXLoadSurfaceFromMemory(IDirect3DSurface8* pDestSurface, const PALETTEENTRY* pDestPalette, const RECT* pDestRect, const void* pSrcMemory, D3DFORMAT SrcFormat, UINT SrcPitch, const PALETTEENTRY* pSrcPalette, const RECT* pSrcRect, DWORD Filter, D3DCOLOR ColorKey);
DX8GL_API HRESULT D3DXLoadSurfaceFromSurface(IDirect3DSurface8* pDestSurface, CONST PALETTEENTRY* pDestPalette, CONST RECT* pDestRect, IDirect3DSurface8* pSrcSurface, CONST PALETTEENTRY* pSrcPalette, CONST RECT* pSrcRect, DWORD Filter, D3DCOLOR ColorKey);
DX8GL_API HRESULT WINAPI D3DXSaveSurfaceToFile(LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, IDirect3DSurface8* pSrcSurface, const PALETTEENTRY* pSrcPalette, const RECT* pSrcRect);

/* Shader compilation functions */
DX8GL_API HRESULT D3DXAssembleShader(const char* pSrcData, DWORD SrcDataLen, DWORD Flags, ID3DXBuffer** ppConstants, ID3DXBuffer** ppCompiledShader, ID3DXBuffer** ppCompilationErrors);

#ifdef __cplusplus
}
#endif

#endif /* DX8GL_D3DX_COMPAT_H */
