#ifndef DX8GL_D3D8_CONSTANTS_H
#define DX8GL_D3D8_CONSTANTS_H

/**
 * DirectX 8 Constants and Enumerations
 */

#include "d3d8_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HRESULT values */
#ifndef S_OK
#define S_OK                    ((HRESULT)0x00000000L)
#endif
#ifndef S_FALSE
#define S_FALSE                 ((HRESULT)0x00000001L)
#endif
#ifndef E_FAIL
#define E_FAIL                  ((HRESULT)0x80004005L)
#endif
#ifndef E_INVALIDARG
#define E_INVALIDARG            ((HRESULT)0x80070057L)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY          ((HRESULT)0x8007000EL)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL              ((HRESULT)0x80004001L)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE          ((HRESULT)0x80004002L)
#endif
#ifndef E_POINTER
#define E_POINTER              ((HRESULT)0x80004003L)
#endif
#define D3D_OK                  S_OK
#define D3DERR_INVALIDCALL     ((HRESULT)0x8876086CL)
#define D3DERR_OUTOFVIDEOMEMORY ((HRESULT)0x88760005L)
#define D3DERR_DEVICELOST      ((HRESULT)0x88760868L)
#define D3DERR_DEVICENOTRESET  ((HRESULT)0x88760869L)
#define D3DERR_NOTAVAILABLE    ((HRESULT)0x8876086AL)
#define D3DERR_NOTFOUND        ((HRESULT)0x88760866L)
#define D3DERR_DRIVERINTERNALERROR ((HRESULT)0x88760827L)

/* DirectX constants */
#define MAX_DEVICE_IDENTIFIER_STRING 512

/* Windows utility macros */
#ifndef MAKELONG
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#endif

/* HRESULT macros */
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

/* D3D constants */
#define D3D_SDK_VERSION         120
#define D3DADAPTER_DEFAULT      0

/* D3DLOCK flags */
#define D3DLOCK_READONLY         0x00000010L
#define D3DLOCK_DISCARD          0x00002000L
#define D3DLOCK_NOOVERWRITE      0x00001000L
#define D3DLOCK_NOSYSLOCK        0x00000800L
#define D3DLOCK_NO_DIRTY_UPDATE  0x00008000L

/* D3DCREATE flags */
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040L
#define D3DCREATE_MIXED_VERTEXPROCESSING    0x00000080L

/* D3DPTEXTURECAPS */
#define D3DPTEXTURECAPS_PERSPECTIVE         0x00000001L
#define D3DPTEXTURECAPS_POW2                0x00000002L
#define D3DPTEXTURECAPS_ALPHA               0x00000004L
#define D3DPTEXTURECAPS_SQUAREONLY          0x00000020L
#define D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE 0x00000040L
#define D3DPTEXTURECAPS_ALPHAPALETTE        0x00000080L
#define D3DPTEXTURECAPS_NONPOW2CONDITIONAL  0x00000100L
#define D3DPTEXTURECAPS_PROJECTED           0x00000400L
#define D3DPTEXTURECAPS_CUBEMAP             0x00000800L
#define D3DPTEXTURECAPS_VOLUMEMAP           0x00002000L
#define D3DPTEXTURECAPS_MIPMAP              0x00004000L
#define D3DPTEXTURECAPS_MIPVOLUMEMAP        0x00008000L
#define D3DPTEXTURECAPS_MIPCUBEMAP          0x00010000L
#define D3DPTEXTURECAPS_CUBEMAP_POW2        0x00020000L
#define D3DPTEXTURECAPS_VOLUMEMAP_POW2      0x00040000L

/* D3DCLEAR flags */
#define D3DCLEAR_TARGET                     0x00000001L
#define D3DCLEAR_ZBUFFER                    0x00000002L
#define D3DCLEAR_STENCIL                    0x00000004L

/* D3DCURSOR flags */
#define D3DCURSOR_IMMEDIATE_UPDATE          0x00000001L

/* D3DUSAGE */
#define D3DUSAGE_WRITEONLY                  0x00000008L
#define D3DUSAGE_DONOTCLIP                  0x00000020L
#define D3DUSAGE_POINTS                     0x00000040L
#define D3DUSAGE_RTPATCHES                  0x00000080L
#define D3DUSAGE_NPATCHES                   0x00000100L
#define D3DUSAGE_DYNAMIC                    0x00000200L

/* D3DTSS_TCI flags */
#define D3DTSS_TCI_PASSTHRU                        0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL               0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION             0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR     0x00030000

/* Color macros - defined in d3d8_types.h */

/* Blending modes - Note: Values defined in D3DBLEND enum */

/* Comparison functions - Note: Values defined in D3DCMPFUNC enum */

/* Stencil operations - Note: Values defined in D3DSTENCILOP enum */

/* Device caps */
#define D3DDEVCAPS_EXECUTESYSTEMMEMORY      0x00000010L
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY     0x00000040L
#define D3DDEVCAPS_TLVERTEXVIDEOMEMORY      0x00000080L
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY      0x00000100L
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY       0x00000200L
#define D3DDEVCAPS_DRAWPRIMTLVERTEX         0x00000400L
#define D3DDEVCAPS_CANRENDERAFTERFLIP       0x00000800L
#define D3DDEVCAPS_TEXTURENONLOCALVIDMEM    0x00001000L
#define D3DDEVCAPS_DRAWPRIMITIVES2          0x00002000L
#define D3DDEVCAPS_SEPARATETEXTUREMEMORIES  0x00004000L
#define D3DDEVCAPS_DRAWPRIMITIVES2EX        0x00008000L
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT      0x00010000L
#define D3DDEVCAPS_CANBLTSYSTONONLOCAL      0x00020000L
#define D3DDEVCAPS_HWRASTERIZATION          0x00080000L
#define D3DDEVCAPS_PUREDEVICE               0x00100000L
#define D3DDEVCAPS_QUINTICRTPATCHES         0x00200000L
#define D3DDEVCAPS_RTPATCHES                0x00400000L
#define D3DDEVCAPS_RTPATCHHANDLEZERO        0x00800000L
#define D3DDEVCAPS_NPATCHES                 0x01000000L

/* Primitive caps */
#define D3DPMISCCAPS_MASKZ                  0x00000002L
#define D3DPMISCCAPS_LINEPATTERNREP         0x00000004L
#define D3DPMISCCAPS_CULLNONE               0x00000010L
#define D3DPMISCCAPS_CULLCW                 0x00000020L
#define D3DPMISCCAPS_CULLCCW                0x00000040L
#define D3DPMISCCAPS_COLORWRITEENABLE       0x00000100L
#define D3DPMISCCAPS_CLIPPLANESCALEDPOINTS  0x00000200L
#define D3DPMISCCAPS_CLIPTLVERTS            0x00000400L
#define D3DPMISCCAPS_TSSARGTEMP             0x00000800L
#define D3DPMISCCAPS_BLENDOP                0x00001000L
#define D3DPMISCCAPS_NULLREFERENCE          0x00002000L

/* Raster caps */
#define D3DPRASTERCAPS_DITHER               0x00000001L
#define D3DPRASTERCAPS_PAT                  0x00000008L
#define D3DPRASTERCAPS_ZTEST                0x00000010L
#define D3DPRASTERCAPS_FOGVERTEX            0x00000020L
#define D3DPRASTERCAPS_FOGTABLE             0x00000040L
#define D3DPRASTERCAPS_ANTIALIASEDGES       0x00000100L
#define D3DPRASTERCAPS_MIPMAPLODBIAS        0x00000200L
#define D3DPRASTERCAPS_ZBIAS                0x00000400L
#define D3DPRASTERCAPS_ZBUFFERLESSHSR       0x00000800L
#define D3DPRASTERCAPS_FOGRANGE             0x00001000L
#define D3DPRASTERCAPS_ANISOTROPY           0x00002000L
#define D3DPRASTERCAPS_WBUFFER              0x00004000L
#define D3DPRASTERCAPS_WFOG                 0x00008000L
#define D3DPRASTERCAPS_ZFOG                 0x00010000L
#define D3DPRASTERCAPS_COLORPERSPECTIVE     0x00020000L
#define D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE 0x00040000L

/* Texture caps */
#define D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE   0x00000040L
#define D3DPTEXTURECAPS_ALPHAPALETTE               0x00000080L
#define D3DPTEXTURECAPS_NONPOW2CONDITIONAL         0x00000100L
#define D3DPTEXTURECAPS_PROJECTED                  0x00000400L
#define D3DPTEXTURECAPS_VOLUMEMAP_POW2             0x00040000L

/* Z-buffer caps */
#define D3DPZCMPCAPS_NEVER              0x00000001L
#define D3DPZCMPCAPS_LESS               0x00000002L
#define D3DPZCMPCAPS_EQUAL              0x00000004L
#define D3DPZCMPCAPS_LESSEQUAL          0x00000008L
#define D3DPZCMPCAPS_GREATER            0x00000010L
#define D3DPZCMPCAPS_NOTEQUAL           0x00000020L
#define D3DPZCMPCAPS_GREATEREQUAL       0x00000040L
#define D3DPZCMPCAPS_ALWAYS             0x00000080L

/* Blend caps */
#define D3DPBLENDCAPS_ZERO              0x00000001L
#define D3DPBLENDCAPS_ONE               0x00000002L
#define D3DPBLENDCAPS_SRCCOLOR          0x00000004L
#define D3DPBLENDCAPS_INVSRCCOLOR       0x00000008L
#define D3DPBLENDCAPS_SRCALPHA          0x00000010L
#define D3DPBLENDCAPS_INVSRCALPHA       0x00000020L
#define D3DPBLENDCAPS_DESTALPHA         0x00000040L
#define D3DPBLENDCAPS_INVDESTALPHA      0x00000080L
#define D3DPBLENDCAPS_DESTCOLOR         0x00000100L
#define D3DPBLENDCAPS_INVDESTCOLOR      0x00000200L
#define D3DPBLENDCAPS_SRCALPHASAT       0x00000400L
#define D3DPBLENDCAPS_BOTHSRCALPHA      0x00000800L
#define D3DPBLENDCAPS_BOTHINVSRCALPHA   0x00001000L

/* Shade caps */
#define D3DPSHADECAPS_COLORGOURAUDRGB       0x00000008L
#define D3DPSHADECAPS_SPECULARGOURAUDRGB    0x00000200L
#define D3DPSHADECAPS_ALPHAGOURAUDBLEND     0x00004000L
#define D3DPSHADECAPS_FOGGOURAUD            0x00008000L

/* Texture filter caps */
#define D3DPTFILTERCAPS_MINFPOINT           0x00000100L
#define D3DPTFILTERCAPS_MINFLINEAR          0x00000200L
#define D3DPTFILTERCAPS_MINFANISOTROPIC     0x00000400L
#define D3DPTFILTERCAPS_MIPFPOINT           0x00010000L
#define D3DPTFILTERCAPS_MIPFLINEAR          0x00020000L
#define D3DPTFILTERCAPS_MAGFPOINT           0x01000000L
#define D3DPTFILTERCAPS_MAGFLINEAR          0x02000000L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC     0x04000000L
#define D3DPTFILTERCAPS_MAGFAFLATCUBIC      0x08000000L
#define D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC   0x10000000L

/* Texture address caps */
#define D3DPTADDRESSCAPS_WRAP               0x00000001L
#define D3DPTADDRESSCAPS_MIRROR             0x00000002L
#define D3DPTADDRESSCAPS_CLAMP              0x00000004L
#define D3DPTADDRESSCAPS_BORDER             0x00000008L
#define D3DPTADDRESSCAPS_INDEPENDENTUV      0x00000010L
#define D3DPTADDRESSCAPS_MIRRORONCE         0x00000020L

/* Line caps */
#define D3DLINECAPS_TEXTURE                 0x00000001L
#define D3DLINECAPS_ZTEST                   0x00000002L
#define D3DLINECAPS_BLEND                   0x00000004L
#define D3DLINECAPS_ALPHACMP                0x00000008L
#define D3DLINECAPS_FOG                     0x00000010L

/* Max texture dimensions */
#define D3DMAXUSERCLIPPLANES    32

/* Stencil caps */
#define D3DSTENCILCAPS_KEEP             0x00000001L
#define D3DSTENCILCAPS_ZERO             0x00000002L
#define D3DSTENCILCAPS_REPLACE          0x00000004L
#define D3DSTENCILCAPS_INCRSAT          0x00000008L
#define D3DSTENCILCAPS_DECRSAT          0x00000010L
#define D3DSTENCILCAPS_INVERT           0x00000020L
#define D3DSTENCILCAPS_INCR             0x00000040L
#define D3DSTENCILCAPS_DECR             0x00000080L

/* FVF caps */
#define D3DFVFCAPS_TEXCOORDCOUNTMASK    0x0000ffffL
#define D3DFVFCAPS_DONOTSTRIPELEMENTS   0x00080000L
#define D3DFVFCAPS_PSIZE                0x00100000L

/* Vertex processing caps */
#define D3DVTXPCAPS_TEXGEN              0x00000001L
#define D3DVTXPCAPS_MATERIALSOURCE7     0x00000002L
#define D3DVTXPCAPS_DIRECTIONALLIGHTS   0x00000008L
#define D3DVTXPCAPS_POSITIONALLIGHTS    0x00000010L
#define D3DVTXPCAPS_LOCALVIEWER         0x00000020L
#define D3DVTXPCAPS_TWEENING            0x00000040L
#define D3DVTXPCAPS_NO_VSDT_UBYTE4      0x00000080L

/* Vertex shader declaration macros */
#define D3DVSD_STREAM(stream)             (stream)
#define D3DVSD_REG(reg, type)             ((reg) | ((type) << 16))
#define D3DVSD_END()                      0xFFFFFFFF
#define D3DVSD_NOP()                      0x00000000
#define D3DVSD_SKIP(count)                (0x10000000 | (count))

/* Vertex shader data types */
#define D3DVSDT_FLOAT1                    0x00
#define D3DVSDT_FLOAT2                    0x01
#define D3DVSDT_FLOAT3                    0x02
#define D3DVSDT_FLOAT4                    0x03
#define D3DVSDT_D3DCOLOR                  0x04
#define D3DVSDT_UBYTE4                    0x05
#define D3DVSDT_SHORT2                    0x06
#define D3DVSDT_SHORT4                    0x07

/* Enumerations */

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN              = 0,
    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,
    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,
    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_W11V11U10            = 65,
    D3DFMT_A2W10V10U10          = 67,
    D3DFMT_D16                  = 80,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D32                  = 71,
    D3DFMT_INDEX16              = 101,
    D3DFMT_INDEX32              = 102,
    D3DFMT_DXT1                 = 827611204,
    D3DFMT_DXT2                 = 844388420,
    D3DFMT_DXT3                 = 861165636,
    D3DFMT_DXT4                 = 877942852,
    D3DFMT_DXT5                 = 894720068,
    D3DFMT_VERTEXDATA           = 100,
} D3DFORMAT;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST             = 1,
    D3DPT_LINELIST              = 2,
    D3DPT_LINESTRIP             = 3,
    D3DPT_TRIANGLELIST          = 4,
    D3DPT_TRIANGLESTRIP         = 5,
    D3DPT_TRIANGLEFAN           = 6,
} D3DPRIMITIVETYPE;

typedef enum _D3DSWAPEFFECT {
    D3DSWAPEFFECT_DISCARD           = 1,
    D3DSWAPEFFECT_FLIP              = 2,
    D3DSWAPEFFECT_COPY              = 3,
    D3DSWAPEFFECT_COPY_VSYNC        = 4,
} D3DSWAPEFFECT;

typedef enum _D3DDEVTYPE {
    D3DDEVTYPE_HAL                  = 1,
    D3DDEVTYPE_REF                  = 2,
    D3DDEVTYPE_SW                   = 3,
} D3DDEVTYPE;

typedef enum _D3DCULL {
    D3DCULL_NONE                    = 1,
    D3DCULL_CW                      = 2,
    D3DCULL_CCW                     = 3,
} D3DCULL;

typedef enum _D3DZB {
    D3DZB_FALSE                     = 0,
    D3DZB_TRUE                      = 1,
    D3DZB_USEW                      = 2,
} D3DZB;

typedef enum _D3DLIGHTTYPE {
    D3DLIGHT_POINT                  = 1,
    D3DLIGHT_SPOT                   = 2,
    D3DLIGHT_DIRECTIONAL            = 3,
} D3DLIGHTTYPE;

typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW                      = 2,
    D3DTS_PROJECTION                = 3,
    D3DTS_TEXTURE0                  = 16,
    D3DTS_TEXTURE1                  = 17,
    D3DTS_TEXTURE2                  = 18,
    D3DTS_TEXTURE3                  = 19,
    D3DTS_TEXTURE4                  = 20,
    D3DTS_TEXTURE5                  = 21,
    D3DTS_TEXTURE6                  = 22,
    D3DTS_TEXTURE7                  = 23,
    D3DTS_WORLD                     = 256,
    D3DTS_WORLD1                    = 257,
    D3DTS_WORLD2                    = 258,
    D3DTS_WORLD3                    = 259,
} D3DTRANSFORMSTATETYPE;

typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT                 = 0,
    D3DPOOL_MANAGED                 = 1,
    D3DPOOL_SYSTEMMEM               = 2,
    D3DPOOL_SCRATCH                 = 3,
} D3DPOOL;

typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE               = 7,
    D3DRS_FILLMODE              = 8,
    D3DRS_SHADEMODE             = 9,
    D3DRS_LINEPATTERN           = 10,
    D3DRS_ZWRITEENABLE          = 14,
    D3DRS_ALPHATESTENABLE       = 15,
    D3DRS_LASTPIXEL             = 16,
    D3DRS_SRCBLEND              = 19,
    D3DRS_DESTBLEND             = 20,
    D3DRS_CULLMODE              = 22,
    D3DRS_ZFUNC                 = 23,
    D3DRS_ALPHAREF              = 24,
    D3DRS_ALPHAFUNC             = 25,
    D3DRS_DITHERENABLE          = 26,
    D3DRS_ALPHABLENDENABLE      = 27,
    D3DRS_FOGENABLE             = 28,
    D3DRS_SPECULARENABLE        = 29,
    D3DRS_FOGCOLOR              = 34,
    D3DRS_FOGTABLEMODE          = 35,
    D3DRS_FOGSTART              = 36,
    D3DRS_FOGEND                = 37,
    D3DRS_FOGDENSITY            = 38,
    D3DRS_EDGEANTIALIAS         = 40,
    D3DRS_ZBIAS                 = 47,
    D3DRS_RANGEFOGENABLE        = 48,
    D3DRS_STENCILENABLE         = 52,
    D3DRS_STENCILFAIL           = 53,
    D3DRS_STENCILZFAIL          = 54,
    D3DRS_STENCILPASS           = 55,
    D3DRS_STENCILFUNC           = 56,
    D3DRS_STENCILREF            = 57,
    D3DRS_STENCILMASK           = 58,
    D3DRS_STENCILWRITEMASK      = 59,
    D3DRS_TEXTUREFACTOR         = 60,
    D3DRS_WRAP0                 = 128,
    D3DRS_WRAP1                 = 129,
    D3DRS_WRAP2                 = 130,
    D3DRS_WRAP3                 = 131,
    D3DRS_WRAP4                 = 132,
    D3DRS_WRAP5                 = 133,
    D3DRS_WRAP6                 = 134,
    D3DRS_WRAP7                 = 135,
    D3DRS_CLIPPING              = 136,
    D3DRS_LIGHTING              = 137,
    D3DRS_Lighting              = D3DRS_LIGHTING,  /* Alias for case sensitivity */
    D3DRS_AMBIENT               = 139,
    D3DRS_FOGVERTEXMODE         = 140,
    D3DRS_COLORVERTEX           = 141,
    D3DRS_LOCALVIEWER           = 142,
    D3DRS_NORMALIZENORMALS      = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_SPECULARMATERIALSOURCE = 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
    D3DRS_EMISSIVEMATERIALSOURCE = 148,
    D3DRS_VERTEXBLEND           = 151,
    D3DRS_CLIPPLANEENABLE       = 152,
    D3DRS_SOFTWAREVERTEXPROCESSING = 153,
    D3DRS_POINTSIZE             = 154,
    D3DRS_POINTSIZE_MIN         = 155,
    D3DRS_POINTSPRITEENABLE     = 156,
    D3DRS_POINTSCALEENABLE      = 157,
    D3DRS_POINTSCALE_A          = 158,
    D3DRS_POINTSCALE_B          = 159,
    D3DRS_POINTSCALE_C          = 160,
    D3DRS_MULTISAMPLEANTIALIAS  = 161,
    D3DRS_MULTISAMPLEMASK       = 162,
    D3DRS_PATCHEDGESTYLE        = 163,
    D3DRS_PATCHSEGMENTS         = 164,
    D3DRS_DEBUGMONITORTOKEN     = 165,
    D3DRS_POINTSIZE_MAX         = 166,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 167,
    D3DRS_COLORWRITEENABLE      = 168,
    D3DRS_TWEENFACTOR           = 170,
    D3DRS_BLENDOP               = 171,
    D3DRS_POSITIONORDER         = 172,
    D3DRS_NORMALORDER           = 173,
    D3DRS_SCISSORTESTENABLE     = 174,
} D3DRENDERSTATETYPE;

typedef enum _D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE     = 0,
    D3DMULTISAMPLE_2_SAMPLES = 2,
    D3DMULTISAMPLE_3_SAMPLES = 3,
    D3DMULTISAMPLE_4_SAMPLES = 4,
    D3DMULTISAMPLE_5_SAMPLES = 5,
    D3DMULTISAMPLE_6_SAMPLES = 6,
    D3DMULTISAMPLE_7_SAMPLES = 7,
    D3DMULTISAMPLE_8_SAMPLES = 8,
    D3DMULTISAMPLE_9_SAMPLES = 9,
    D3DMULTISAMPLE_10_SAMPLES = 10,
    D3DMULTISAMPLE_11_SAMPLES = 11,
    D3DMULTISAMPLE_12_SAMPLES = 12,
    D3DMULTISAMPLE_13_SAMPLES = 13,
    D3DMULTISAMPLE_14_SAMPLES = 14,
    D3DMULTISAMPLE_15_SAMPLES = 15,
    D3DMULTISAMPLE_16_SAMPLES = 16,
} D3DMULTISAMPLE_TYPE;

typedef enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP           = 1,
    D3DTSS_COLORARG1         = 2,
    D3DTSS_COLORARG2         = 3,
    D3DTSS_ALPHAOP           = 4,
    D3DTSS_ALPHAARG1         = 5,
    D3DTSS_ALPHAARG2         = 6,
    D3DTSS_BUMPENVMAT00      = 7,
    D3DTSS_BUMPENVMAT01      = 8,
    D3DTSS_BUMPENVMAT10      = 9,
    D3DTSS_BUMPENVMAT11      = 10,
    D3DTSS_TEXCOORDINDEX     = 11,
    D3DTSS_ADDRESSU          = 13,
    D3DTSS_ADDRESSV          = 14,
    D3DTSS_BORDERCOLOR       = 15,
    D3DTSS_MAGFILTER         = 16,
    D3DTSS_MINFILTER         = 17,
    D3DTSS_MIPFILTER         = 18,
    D3DTSS_MIPMAPLODBIAS     = 19,
    D3DTSS_MAXMIPLEVEL       = 20,
    D3DTSS_MAXANISOTROPY     = 21,
    D3DTSS_BUMPENVLSCALE     = 22,
    D3DTSS_BUMPENVLOFFSET    = 23,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_ADDRESSW          = 25,
    D3DTSS_COLORARG0         = 26,
    D3DTSS_ALPHAARG0         = 27,
    D3DTSS_RESULTARG         = 28,
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE            = 1,
    D3DRTYPE_VOLUME             = 2,
    D3DRTYPE_TEXTURE            = 3,
    D3DRTYPE_VOLUMETEXTURE      = 4,
    D3DRTYPE_CUBETEXTURE        = 5,
    D3DRTYPE_VERTEXBUFFER       = 6,
    D3DRTYPE_INDEXBUFFER        = 7,
} D3DRESOURCETYPE;

typedef enum _D3DFOGMODE {
    D3DFOG_NONE                 = 0,
    D3DFOG_EXP                  = 1,
    D3DFOG_EXP2                 = 2,
    D3DFOG_LINEAR               = 3,
} D3DFOGMODE;

typedef enum _D3DSHADEMODE {
    D3DSHADE_FLAT               = 1,
    D3DSHADE_GOURAUD            = 2,
    D3DSHADE_PHONG              = 3,
} D3DSHADEMODE;

typedef enum _D3DFILLMODE {
    D3DFILL_POINT               = 1,
    D3DFILL_WIREFRAME           = 2,
    D3DFILL_SOLID               = 3,
} D3DFILLMODE;

typedef enum _D3DTEXTUREOP {
    D3DTOP_DISABLE              = 1,
    D3DTOP_SELECTARG1           = 2,
    D3DTOP_SELECTARG2           = 3,
    D3DTOP_MODULATE             = 4,
    D3DTOP_MODULATE2X           = 5,
    D3DTOP_MODULATE4X           = 6,
    D3DTOP_ADD                  = 7,
    D3DTOP_ADDSIGNED            = 8,
    D3DTOP_ADDSIGNED2X          = 9,
    D3DTOP_SUBTRACT             = 10,
    D3DTOP_ADDSMOOTH            = 11,
    D3DTOP_BLENDDIFFUSEALPHA    = 12,
    D3DTOP_BLENDTEXTUREALPHA    = 13,
    D3DTOP_BLENDFACTORALPHA     = 14,
    D3DTOP_BLENDTEXTUREALPHAPM  = 15,
    D3DTOP_BLENDCURRENTALPHA    = 16,
    D3DTOP_PREMODULATE          = 17,
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    D3DTOP_BUMPENVMAP           = 22,
    D3DTOP_BUMPENVMAPLUMINANCE  = 23,
    D3DTOP_DOTPRODUCT3          = 24,
    D3DTOP_MULTIPLYADD          = 25,
    D3DTOP_FORCE_DWORD          = 0x7fffffff,
} D3DTEXTUREOP;

typedef enum _D3DCUBEMAP_FACES {
    D3DCUBEMAP_FACE_POSITIVE_X = 0,
    D3DCUBEMAP_FACE_NEGATIVE_X = 1,
    D3DCUBEMAP_FACE_POSITIVE_Y = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
    D3DCUBEMAP_FACE_POSITIVE_Z = 4,
    D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
} D3DCUBEMAP_FACES;

typedef enum _D3DBACKBUFFER_TYPE {
    D3DBACKBUFFER_TYPE_MONO   = 0,
    D3DBACKBUFFER_TYPE_LEFT   = 1,
    D3DBACKBUFFER_TYPE_RIGHT  = 2,
} D3DBACKBUFFER_TYPE;

typedef enum _D3DSTATEBLOCKTYPE {
    D3DSBT_ALL         = 1,
    D3DSBT_PIXELSTATE  = 2,
    D3DSBT_VERTEXSTATE = 3,
} D3DSTATEBLOCKTYPE;

typedef enum _D3DMATERIALCOLORSOURCE {
    D3DMCS_MATERIAL = 0,
    D3DMCS_COLOR1   = 1,
    D3DMCS_COLOR2   = 2,
} D3DMATERIALCOLORSOURCE;

typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER        = 1,
    D3DCMP_LESS         = 2,
    D3DCMP_EQUAL        = 3,
    D3DCMP_LESSEQUAL    = 4,
    D3DCMP_GREATER      = 5,
    D3DCMP_NOTEQUAL     = 6,
    D3DCMP_GREATEREQUAL = 7,
    D3DCMP_ALWAYS       = 8,
} D3DCMPFUNC;

typedef enum _D3DVERTEXBLENDFLAGS {
    D3DVBF_DISABLE  = 0,
    D3DVBF_1WEIGHTS = 1,
    D3DVBF_2WEIGHTS = 2,
    D3DVBF_3WEIGHTS = 3,
    D3DVBF_TWEENING = 255,
    D3DVBF_0WEIGHTS = 256,
} D3DVERTEXBLENDFLAGS;

typedef enum _D3DPATCHEDGE {
    D3DPATCHEDGE_DISCRETE    = 0,
    D3DPATCHEDGE_CONTINUOUS  = 1,
} D3DPATCHEDGE;

typedef enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP        = 1,
    D3DTADDRESS_MIRROR      = 2,
    D3DTADDRESS_CLAMP       = 3,
    D3DTADDRESS_BORDER      = 4,
    D3DTADDRESS_MIRRORONCE  = 5,
} D3DTEXTUREADDRESS;

typedef enum _D3DTEXTUREFILTERTYPE {
    D3DTEXF_NONE            = 0,
    D3DTEXF_POINT           = 1,
    D3DTEXF_LINEAR          = 2,
    D3DTEXF_ANISOTROPIC     = 3,
    D3DTEXF_FLATCUBIC       = 4,
    D3DTEXF_GAUSSIANCUBIC   = 5,
} D3DTEXTUREFILTERTYPE;

typedef enum _D3DBLEND {
    D3DBLEND_ZERO               = 1,
    D3DBLEND_ONE                = 2,
    D3DBLEND_SRCCOLOR           = 3,
    D3DBLEND_INVSRCCOLOR        = 4,
    D3DBLEND_SRCALPHA           = 5,
    D3DBLEND_INVSRCALPHA        = 6,
    D3DBLEND_DESTALPHA          = 7,
    D3DBLEND_INVDESTALPHA       = 8,
    D3DBLEND_DESTCOLOR          = 9,
    D3DBLEND_INVDESTCOLOR       = 10,
    D3DBLEND_SRCALPHASAT        = 11,
    D3DBLEND_BOTHSRCALPHA       = 12,
    D3DBLEND_BOTHINVSRCALPHA    = 13,
} D3DBLEND;

typedef enum _D3DBLENDOP {
    D3DBLENDOP_ADD         = 1,
    D3DBLENDOP_SUBTRACT    = 2,
    D3DBLENDOP_REVSUBTRACT = 3,
    D3DBLENDOP_MIN         = 4,
    D3DBLENDOP_MAX         = 5,
} D3DBLENDOP;

typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP        = 1,
    D3DSTENCILOP_ZERO        = 2,
    D3DSTENCILOP_REPLACE     = 3,
    D3DSTENCILOP_INCRSAT     = 4,
    D3DSTENCILOP_DECRSAT     = 5,
    D3DSTENCILOP_INVERT      = 6,
    D3DSTENCILOP_INCR        = 7,
    D3DSTENCILOP_DECR        = 8,
} D3DSTENCILOP;

typedef enum _D3DBASISTYPE {
    D3DBASIS_BEZIER = 0,
    D3DBASIS_BSPLINE = 1,
    D3DBASIS_INTERPOLATE = 2,
} D3DBASISTYPE;

typedef enum _D3DORDERTYPE {
    D3DORDER_LINEAR = 1,
    D3DORDER_QUADRATIC = 2,
    D3DORDER_CUBIC = 3,
    D3DORDER_QUINTIC = 5,
} D3DORDERTYPE;

/* FVF codes - Enhanced */
#define D3DFVF_RESERVED0        0x0001
#define D3DFVF_XYZ              0x0002
#define D3DFVF_XYZRHW           0x0004
#define D3DFVF_XYZB1            0x0006
#define D3DFVF_XYZB2            0x0008
#define D3DFVF_XYZB3            0x000A
#define D3DFVF_XYZB4            0x000C
#define D3DFVF_XYZB5            0x000E
#define D3DFVF_NORMAL           0x0010
#define D3DFVF_PSIZE            0x0020
#define D3DFVF_DIFFUSE          0x0040
#define D3DFVF_SPECULAR         0x0080

/* Texture coordinate counts */
#define D3DFVF_TEX0             0x0000
#define D3DFVF_TEX1             0x0100
#define D3DFVF_TEX2             0x0200
#define D3DFVF_TEX3             0x0300
#define D3DFVF_TEX4             0x0400
#define D3DFVF_TEX5             0x0500
#define D3DFVF_TEX6             0x0600
#define D3DFVF_TEX7             0x0700
#define D3DFVF_TEX8             0x0800

#define D3DFVF_LASTBETA_UBYTE4  0x1000
#define D3DFVF_LASTBETA_D3DCOLOR 0x8000

/* Texture coordinate sizes for each texture unit */
#define D3DFVF_TEXTUREFORMAT1   3  /* one floating point value */
#define D3DFVF_TEXTUREFORMAT2   0  /* two floating point values */
#define D3DFVF_TEXTUREFORMAT3   1  /* three floating point values */
#define D3DFVF_TEXTUREFORMAT4   2  /* four floating point values */

/* Helper macros for FVF parsing */
#define D3DFVF_TEXCOUNT_MASK    0x00000F00
#define D3DFVF_TEXCOUNT_SHIFT   8
#define D3DFVF_POSITION_MASK    0x0000000E

/* Texture coordinate format for texture unit n */
#define D3DFVF_TEXCOORDSIZE1(n) (D3DFVF_TEXTUREFORMAT1 << (n*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(n) (D3DFVF_TEXTUREFORMAT2 << (n*2 + 16))
#define D3DFVF_TEXCOORDSIZE3(n) (D3DFVF_TEXTUREFORMAT3 << (n*2 + 16))
#define D3DFVF_TEXCOORDSIZE4(n) (D3DFVF_TEXTUREFORMAT4 << (n*2 + 16))

/* Common vertex formats */
#define D3DFVF_VERTEX           (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
#define D3DFVF_LVERTEX          (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
#define D3DFVF_TLVERTEX         (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

/* D3DPRESENT flags */
#define D3DPRESENTFLAG_LOCKABLE_BACKBUFFER 0x00000001
#define D3DPRESENTFLAG_DEVICECLIP          0x00000004
#define D3DPRESENTFLAG_VIDEO               0x00000010

/* D3DPRESENT_RATE constants */
#define D3DPRESENT_RATE_DEFAULT            0x00000000

/* D3DENUM flags */
#define D3DENUM_NO_WHQL_LEVEL              0x00000002L

/* D3DCREATE flags - Additional */
#define D3DCREATE_FPU_PRESERVE             0x00000002L
#define D3DCREATE_MULTITHREADED            0x00000004L
#define D3DCREATE_PUREDEVICE               0x00000010L
#define D3DCREATE_DISABLE_DRIVER_MANAGEMENT 0x00000100L
#define D3DCREATE_ADAPTERGROUP_DEVICE       0x00000200L

/* D3DSGR flags */
#define D3DSGR_NO_CALIBRATION              0x00000000L
#define D3DSGR_CALIBRATE                   0x00000001L

/* Debug monitor tokens */
#define D3DDMT_ENABLE  1
#define D3DDMT_DISABLE 0

/* Texture transform flags */
#define D3DTTFF_DISABLE    0
#define D3DTTFF_COUNT1     1
#define D3DTTFF_COUNT2     2
#define D3DTTFF_COUNT3     3
#define D3DTTFF_COUNT4     4
#define D3DTTFF_PROJECTED  256

/* Texture coordinate wrap flags */
#define D3DWRAP_U          1
#define D3DWRAP_V          2
#define D3DWRAP_W          4

/* Texture argument */
#define D3DTA_SELECTMASK        0x0000000f
#define D3DTA_DIFFUSE           0x00000000
#define D3DTA_CURRENT           0x00000001
#define D3DTA_TEXTURE           0x00000002
#define D3DTA_TFACTOR           0x00000003
#define D3DTA_SPECULAR          0x00000004
#define D3DTA_TEMP              0x00000005
#define D3DTA_COMPLEMENT        0x00000010
#define D3DTA_ALPHAREPLICATE    0x00000020

/* Additional render states */
#define D3DRS_ZVISIBLE              30

/* Color write enable flags */
#define D3DCOLORWRITEENABLE_RED     0x00000001L
#define D3DCOLORWRITEENABLE_GREEN   0x00000002L
#define D3DCOLORWRITEENABLE_BLUE    0x00000004L
#define D3DCOLORWRITEENABLE_ALPHA   0x00000008L

/* D3DUSAGE flags - Additional */
#define D3DUSAGE_RENDERTARGET      0x00000001L
#define D3DUSAGE_DEPTHSTENCIL      0x00000002L
#define D3DUSAGE_SOFTWAREPROCESSING 0x00000010L

/* Max texture dimensions */
#define D3DMAXUSERCLIPPLANES    32

/* Missing texture operation capabilities */
#define D3DTEXOPCAPS_MULTIPLYADD       0x01000000L
#define D3DTEXOPCAPS_LERP              0x02000000L

/* Additional missing texture operation capabilities */
#define D3DTEXOPCAPS_DISABLE           0x00000001L
#define D3DTEXOPCAPS_SELECTARG1        0x00000002L
#define D3DTEXOPCAPS_SELECTARG2        0x00000004L
#define D3DTEXOPCAPS_MODULATE          0x00000008L
#define D3DTEXOPCAPS_MODULATE2X        0x00000010L
#define D3DTEXOPCAPS_MODULATE4X        0x00000020L
#define D3DTEXOPCAPS_ADD               0x00000040L
#define D3DTEXOPCAPS_ADDSIGNED         0x00000080L
#define D3DTEXOPCAPS_ADDSIGNED2X       0x00000100L
#define D3DTEXOPCAPS_SUBTRACT          0x00000200L
#define D3DTEXOPCAPS_ADDSMOOTH         0x00000400L
#define D3DTEXOPCAPS_BLENDDIFFUSEALPHA 0x00000800L
#define D3DTEXOPCAPS_BLENDTEXTUREALPHA 0x00001000L
#define D3DTEXOPCAPS_BLENDFACTORALPHA  0x00002000L
#define D3DTEXOPCAPS_BLENDTEXTUREALPHAPM 0x00004000L
#define D3DTEXOPCAPS_BLENDCURRENTALPHA 0x00008000L
#define D3DTEXOPCAPS_PREMODULATE       0x00010000L
#define D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR 0x00020000L
#define D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA 0x00040000L
#define D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR 0x00080000L
#define D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA 0x00100000L
#define D3DTEXOPCAPS_BUMPENVMAP        0x00200000L
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 0x00400000L
#define D3DTEXOPCAPS_DOTPRODUCT3       0x00800000L

/* Device capabilities */
#define D3DCAPS_READ_SCANLINE          0x00020000L

/* Device capabilities 2 */
#define D3DCAPS2_CANMANAGERESOURCE     0x10000000L
#define D3DCAPS2_DYNAMICTEXTURES       0x20000000L
#define D3DCAPS2_FULLSCREENGAMMA       0x20000L

/* Device capabilities 3 */
#define D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD 0x00000020L

/* Presentation intervals */
#define D3DPRESENT_INTERVAL_DEFAULT    0x00000000L
#define D3DPRESENT_INTERVAL_ONE        0x00000001L
#define D3DPRESENT_INTERVAL_TWO        0x00000002L
#define D3DPRESENT_INTERVAL_THREE      0x00000004L
#define D3DPRESENT_INTERVAL_FOUR       0x00000008L
#define D3DPRESENT_INTERVAL_IMMEDIATE  0x80000000L

/* Cursor capabilities */
#define D3DCURSORCAPS_COLOR            0x00000001L
#define D3DCURSORCAPS_LOWRES           0x00000002L

/* Device capabilities - execution */
#define D3DDEVCAPS_EXECUTEVIDEOMEMORY  0x00000080L

/* Z-Compare capabilities (corrected names) */
#define D3DPCMPCAPS_NEVER              0x00000001L
#define D3DPCMPCAPS_LESS               0x00000002L
#define D3DPCMPCAPS_EQUAL              0x00000004L
#define D3DPCMPCAPS_LESSEQUAL          0x00000008L
#define D3DPCMPCAPS_GREATER            0x00000010L
#define D3DPCMPCAPS_NOTEQUAL           0x00000020L
#define D3DPCMPCAPS_GREATEREQUAL       0x00000040L
#define D3DPCMPCAPS_ALWAYS             0x00000080L

/* Shader version macros */
#define D3DVS_VERSION(major, minor)    (0xFFFE0000 | ((major) << 8) | (minor))
#define D3DPS_VERSION(major, minor)    (0xFFFF0000 | ((major) << 8) | (minor))
#define D3DSHADER_VERSION_MAJOR(version) (((version) >> 8) & 0xFF)
#define D3DSHADER_VERSION_MINOR(version) ((version) & 0xFF)

#ifdef __cplusplus
}
#endif

#endif /* DX8GL_D3D8_CONSTANTS_H */