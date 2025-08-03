# dx8gl - DirectX 8 to OpenGL ES 2.0 Translation Layer

## Overview

dx8gl is a complete DirectX 8.1 to OpenGL ES 2.0 translation library that enables DirectX 8 applications to run on modern platforms including Linux, Windows, macOS, and WebAssembly (via Emscripten).

## Features

### Core DirectX 8 API Implementation
- ✅ **Complete IDirect3D8 interface** - Device enumeration, adapter queries, device creation
- ✅ **Full IDirect3DDevice8 implementation** - All rendering methods, state management, resource creation
- ✅ **Resource types** - Textures, vertex/index buffers, surfaces, swap chains
- ✅ **Fixed-function pipeline emulation** - Dynamic GLSL shader generation from D3D8 states
- ✅ **Immediate mode rendering** - DrawPrimitiveUP/DrawIndexedPrimitiveUP support
- ✅ **Multi-stream vertex data** - Full 16-stream vertex buffer support
- ✅ **Render targets** - SetRenderTarget/GetRenderTarget with FBO management
- ✅ **Private data storage** - SetPrivateData/GetPrivateData/FreePrivateData for all resources

### Advanced Features
- ✅ **Vertex buffer orphaning** - Optimized DYNAMIC buffer updates to avoid GPU stalls
- ✅ **Command buffer architecture** - Deferred rendering for optimal batching
- ✅ **Thread-safe operations** - Multi-threaded shader compilation support
- ✅ **SDL3 integration** - Cross-platform windowing and context management
- ✅ **Emscripten/WebAssembly** - Full web browser support via WebGL 2.0

### Shader System
- ✅ **Fixed-function to GLSL** - Automatic shader generation based on render states
- ✅ **Vertex shader 1.1 support** - Assembly to GLSL translation
- ✅ **Pixel shader 1.4 support** - Full instruction set implementation
- ✅ **Shader caching** - Binary and source caching for performance

## Building

### Desktop (Linux/Windows/macOS)
```bash
cmake -S . -B build
cmake --build build -j
```

### Emscripten/WebAssembly
```bash
# Source Emscripten SDK first
source /path/to/emsdk/emsdk_env.sh

# Build with provided script
./build_emscripten.sh

# Or manually
cmake -S . -B build-emscripten -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build-emscripten -j
```

## Sample Applications

Three sample applications demonstrate dx8gl functionality:

1. **minimal_dx8gl_test** - Basic initialization and screen clearing
2. **spinning_triangle** - Vertex buffer usage with transformations
3. **textured_cube** - Advanced features including texturing and index buffers

### Running Samples

#### Desktop
```bash
./build/samples/dx8gl_spinning_triangle
```

#### WebAssembly
```bash
# Start web server
./serve_emscripten.py

# Open browser to http://localhost:8000
# Navigate to build-emscripten/Release/
```

## Integration Guide

### Basic Usage
```cpp
#include "dx8gl.h"
#include "d3d8.h"

// Initialize dx8gl
dx8gl_init(nullptr);

// Create Direct3D8 interface
IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);

// Create device
D3DPRESENT_PARAMETERS pp = {};
pp.Windowed = TRUE;
pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
pp.BackBufferFormat = D3DFMT_X8R8G8B8;

IDirect3DDevice8* device;
d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);

// Use standard DirectX 8 API
device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);
device->Present(nullptr, nullptr, nullptr, nullptr);
```

## Architecture

### Translation Pipeline
```
DirectX 8 API calls
       ↓
dx8gl interfaces (IDirect3D8, IDirect3DDevice8, etc.)
       ↓
State tracking and validation
       ↓
Command buffer (optional batching)
       ↓
OpenGL ES 2.0 commands
       ↓
SDL3 context management
       ↓
Platform display (X11/Wayland/Windows/WebGL)
```

### Key Components

- **d3d8_device.cpp** - Core device implementation with all D3D8 methods
- **shader_generator.cpp** - Fixed-function to GLSL shader generation
- **command_buffer.cpp** - Deferred rendering command execution
- **state_manager.cpp** - Render state tracking and optimization
- **private_data.cpp** - Resource private data storage
- **fvf_utils.cpp** - Flexible Vertex Format parsing

## Implementation Status

### ✅ Fully Implemented
- Device creation and management
- All primitive types (points, lines, triangles, strips, fans)
- Texture management (2D textures, mipmaps, dynamic updates)
- Vertex and index buffers (including orphaning optimization)
- Render targets and depth/stencil buffers
- Fixed-function pipeline states
- Matrix transformations
- Immediate mode rendering
- Multi-texture support (8 stages)
- Private data storage for all resources

### 🚧 Limitations
- No cube texture support (returns D3DERR_NOTAVAILABLE)
- No volume texture support
- No hardware vertex processing (software only)
- No antialiasing support
- No stencil operations (depth only)

## Performance Optimizations

1. **Vertex Buffer Orphaning** - Multiple buffer versions for DYNAMIC usage
2. **State Caching** - Minimizes redundant OpenGL state changes
3. **Shader Caching** - Compiled shaders cached to disk
4. **Command Batching** - Groups similar draw calls
5. **Thread Pool** - Multi-threaded shader compilation

## License

See LICENSE file in the project root.

## Contributing

Contributions are welcome! Please ensure:
- All DirectX 8 semantics are preserved
- Proper HRESULT error codes are returned
- Thread safety is maintained where required
- Emscripten compatibility is preserved