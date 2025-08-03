#ifndef DX8GL_D3D8_GAME_H
#define DX8GL_D3D8_GAME_H

/**
 * DirectX 8 header for game code
 * 
 * Updated for C++17 compatibility - uses direct C++ interfaces
 * instead of COM-style vtable pointers.
 */

// Include basic types first (needed for D3DCOLOR_COLORVALUE macro)
#include "d3d8_types.h"

// Use C++ interfaces directly for C++17 compatibility
#define DX8GL_USE_CPP_INTERFACES
#include "d3d8_cpp_interfaces.h"

// Include D3DX utilities
#include "d3dx8.h"

#endif // DX8GL_D3D8_GAME_H