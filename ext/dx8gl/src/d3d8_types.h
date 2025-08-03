#ifndef DX8GL_D3D8_TYPES_H
#define DX8GL_D3D8_TYPES_H

/**
 * DirectX 8 Basic Type Definitions
 */

#include <stdint.h>
#include <stdbool.h>

/* Check if basic types are already defined */
#ifdef WIN32_TYPES_DEFINED
  /* Types are already defined by win32_types.h */
  #define DX8GL_TYPES_FROM_WIN32
#else
  /* Define basic Windows types */
  typedef uint32_t DWORD;
  typedef uint32_t D3DCOLOR;
  
  /* DirectX 8 color macros */
  #define D3DCOLOR_ARGB(a,r,g,b) \
      ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
  #define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
  #define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)
  #define D3DCOLOR_COLORVALUE(r,g,b,a) \
      D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))
  typedef uint32_t UINT;
  typedef uint32_t ULONG;
  typedef int32_t LONG;
  typedef int32_t INT;
  typedef int32_t HRESULT;
  typedef void* HWND;
  typedef void* HANDLE;
  typedef void* HMONITOR;
  typedef int BOOL;
  typedef uint16_t WORD;
  typedef uint8_t BYTE;
  typedef uintptr_t DWORD_PTR;
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

#ifdef __cplusplus
extern "C" {
#endif

/* Forward type for REFIID */
#ifndef REFIID
typedef const struct _GUID* REFIID;
#endif
#ifndef REFGUID
typedef const struct _GUID* REFGUID;
#endif

/* Calling conventions */
#ifndef STDMETHODCALLTYPE
#ifdef _WIN32
#define STDMETHODCALLTYPE __stdcall
#else
#define STDMETHODCALLTYPE
#endif
#endif

/* Windows compat functions */
#ifndef _WIN32
#ifndef InterlockedIncrement
#define InterlockedIncrement(p) __sync_add_and_fetch((p), 1)
#endif
#ifndef InterlockedDecrement
#define InterlockedDecrement(p) __sync_sub_and_fetch((p), 1)
#endif
#endif

#ifndef DX8GL_TYPES_FROM_WIN32
/* GUID type for interface IDs */
typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID, IID;

/* Large integer type */
typedef union _LARGE_INTEGER {
    struct {
        uint32_t LowPart;
        int32_t HighPart;
    } s;
    int64_t QuadPart;
} LARGE_INTEGER;

/* Windows structures */
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

typedef struct _RGNDATA {
    struct {
        DWORD dwSize;
        DWORD iType;
        DWORD nCount;
        DWORD nRgnSize;
        RECT rcBound;
    } rdh;
    char Buffer[1];
} RGNDATA;
#else
/* Use definitions from win32_types.h but ensure IID is defined */
#ifndef IID
typedef struct _GUID IID;
#endif
#endif

/* GUID comparison */
#ifndef IsEqualGUID
#define IsEqualGUID(guid1, guid2) \
    ((guid1).Data1 == (guid2).Data1 && \
     (guid1).Data2 == (guid2).Data2 && \
     (guid1).Data3 == (guid2).Data3 && \
     (guid1).Data4[0] == (guid2).Data4[0] && \
     (guid1).Data4[1] == (guid2).Data4[1] && \
     (guid1).Data4[2] == (guid2).Data4[2] && \
     (guid1).Data4[3] == (guid2).Data4[3] && \
     (guid1).Data4[4] == (guid2).Data4[4] && \
     (guid1).Data4[5] == (guid2).Data4[5] && \
     (guid1).Data4[6] == (guid2).Data4[6] && \
     (guid1).Data4[7] == (guid2).Data4[7])
#endif


/* PALETTEENTRY - defined here for d3d8_device_vtable.h */
#ifndef PALETTEENTRY_DEFINED
#define PALETTEENTRY_DEFINED
typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY;
#endif

/* Boolean values */
#ifndef TRUE
#define TRUE                    1
#endif
#ifndef FALSE
#define FALSE                   0
#endif

/* Calling conventions */
#ifndef WINAPI
#define WINAPI
#endif

/* Windows compatibility defines */
#ifndef CONST
#define CONST const
#endif

#ifdef __cplusplus
}
#endif

#endif /* DX8GL_D3D8_TYPES_H */