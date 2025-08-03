#ifndef DX8GL_D3DX8CORE_H
#define DX8GL_D3DX8CORE_H

/**
 * D3DX8 Core Header - Compatibility layer
 * 
 * This header provides core D3DX8 functionality stubs
 */

#include <stddef.h>  /* for wchar_t */
#ifdef __cplusplus
#include "d3d8_game.h"  /* Use game header for COM compatibility */
#else
#include "d3d8.h"
#endif
#include "d3dx8.h"

/* Windows types if not already defined */
#ifndef WIN32_TYPES_DEFINED
#ifndef CHAR
typedef char CHAR;
#endif
#ifndef WCHAR
typedef wchar_t WCHAR;
#endif
#endif

/* Windows GDI types for font creation */
#ifndef LOGFONT_DEFINED
#define LOGFONT_DEFINED
typedef struct tagLOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[32];
} LOGFONTA, *PLOGFONTA;

typedef struct tagLOGFONTW {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[32];
} LOGFONTW, *PLOGFONTW;

#ifdef UNICODE
typedef LOGFONTW LOGFONT;
typedef PLOGFONTW PLOGFONT;
#else
typedef LOGFONTA LOGFONT;
typedef PLOGFONTA PLOGFONT;
#endif
#endif /* LOGFONT_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

/* D3DX Sprite interface - stub for now */
typedef struct ID3DXSprite ID3DXSprite;
typedef ID3DXSprite* LPD3DXSPRITE;

/* D3DX Font interface - stub for now */
typedef struct ID3DXFont ID3DXFont;
typedef ID3DXFont* LPD3DXFONT;

/* D3DX RenderToSurface interface - stub for now */
typedef struct ID3DXRenderToSurface ID3DXRenderToSurface;
typedef ID3DXRenderToSurface* LPD3DXRENDERTOSURFACE;

/* D3DX RenderToEnvMap interface - stub for now */
typedef struct ID3DXRenderToEnvMap ID3DXRenderToEnvMap;
typedef ID3DXRenderToEnvMap* LPD3DXRENDERTOENVMAP;

/* D3DX Line interface - stub for now */
typedef struct ID3DXLine ID3DXLine;
typedef ID3DXLine* LPD3DXLINE;

/* Function declarations - basic stubs */
HRESULT WINAPI D3DXCreateSprite(
    LPDIRECT3DDEVICE8 pDevice,
    LPD3DXSPRITE* ppSprite
);

HRESULT WINAPI D3DXCreateFont(
    LPDIRECT3DDEVICE8 pDevice,
    HFONT hFont,
    LPD3DXFONT* ppFont
);

HRESULT WINAPI D3DXCreateFontIndirect(
    LPDIRECT3DDEVICE8 pDevice,
    CONST LOGFONT* pLogFont,
    LPD3DXFONT* ppFont
);

HRESULT WINAPI D3DXCreateRenderToSurface(
    LPDIRECT3DDEVICE8 pDevice,
    UINT Width,
    UINT Height,
    D3DFORMAT Format,
    BOOL DepthStencil,
    D3DFORMAT DepthStencilFormat,
    LPD3DXRENDERTOSURFACE* ppRenderToSurface
);

HRESULT WINAPI D3DXCreateRenderToEnvMap(
    LPDIRECT3DDEVICE8 pDevice,
    UINT Size,
    D3DFORMAT Format,
    BOOL DepthStencil,
    D3DFORMAT DepthStencilFormat,
    LPD3DXRENDERTOENVMAP* ppRenderToEnvMap
);

HRESULT WINAPI D3DXCreateLine(
    LPDIRECT3DDEVICE8 pDevice,
    LPD3DXLINE* ppLine
);

/* D3DX Effect interface - stub */
typedef struct ID3DXEffect ID3DXEffect;
typedef ID3DXEffect* LPD3DXEFFECT;

/* D3DX Buffer interface is already defined in d3dx_compat.h */

#ifdef __cplusplus
}
#endif

#endif /* DX8GL_D3DX8CORE_H */