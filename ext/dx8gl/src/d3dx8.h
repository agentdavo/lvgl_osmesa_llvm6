#ifndef DX8GL_D3DX8_H
#define DX8GL_D3DX8_H

/**
 * Direct3D X 8 Compatibility Header
 * 
 * Drop-in replacement for Microsoft's d3dx8.h
 */

// Include appropriate D3D8 header based on context
#ifdef DX8GL_BUILDING_DLL
    #include "d3d8.h"  // Internal dx8gl build
#elif defined(DX8GL_USE_CPP_INTERFACES)
    #include "d3d8_cpp_interfaces.h"  // C++ interfaces for game code
#else
    #include "d3d8.h"  // Default
#endif
#include "d3dx_compat.h"
#include "d3dx8_core.h"

// Include all missing D3DFORMAT constants
#ifndef D3DFMT_L8
#define D3DFMT_L8                   50
#endif
#ifndef D3DFMT_P8
#define D3DFMT_P8                   41
#endif
#ifndef D3DFMT_R3G3B2
#define D3DFMT_R3G3B2               27
#endif
#ifndef D3DFMT_A8R3G3B2
#define D3DFMT_A8R3G3B2             29
#endif
#ifndef D3DFMT_X4R4G4B4
#define D3DFMT_X4R4G4B4             30
#endif
#ifndef D3DFMT_A8P8
#define D3DFMT_A8P8                 40
#endif
#ifndef D3DFMT_A8L8
#define D3DFMT_A8L8                 51
#endif
#ifndef D3DFMT_A4L4
#define D3DFMT_A4L4                 52
#endif
#ifndef D3DFMT_V8U8
#define D3DFMT_V8U8                 60
#endif
#ifndef D3DFMT_L6V5U5
#define D3DFMT_L6V5U5               61
#endif
#ifndef D3DFMT_X8L8V8U8
#define D3DFMT_X8L8V8U8             62
#endif
#ifndef D3DFMT_Q8W8V8U8
#define D3DFMT_Q8W8V8U8             63
#endif
#ifndef D3DFMT_V16U16
#define D3DFMT_V16U16               64
#endif
#ifndef D3DFMT_W11V11U10
#define D3DFMT_W11V11U10            65
#endif
#ifndef D3DFMT_UYVY
#define D3DFMT_UYVY                 MAKEFOURCC('U', 'Y', 'V', 'Y')
#endif
#ifndef D3DFMT_YUY2
#define D3DFMT_YUY2                 MAKEFOURCC('Y', 'U', 'Y', '2')
#endif
#ifndef D3DFMT_D16_LOCKABLE
#define D3DFMT_D16_LOCKABLE         70
#endif
#ifndef D3DFMT_D15S1
#define D3DFMT_D15S1                ((D3DFORMAT)73)
#endif
#ifndef D3DFMT_D24X4S4
#define D3DFMT_D24X4S4              ((D3DFORMAT)79)
#endif

// MAKEFOURCC macro for video formats
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif

// Additional constants
#ifndef D3DDP_MAXTEXCOORD
#define D3DDP_MAXTEXCOORD 8
#endif

#endif // DX8GL_D3DX8_H