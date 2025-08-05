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
- ✅ **Vertex shader 1.1 support** - Complete assembly to GLSL translation
  - All vs1.1 opcodes (mov, add, mul, dp3, dp4, m3x2, m3x3, m3x4, m4x3, m4x4, sincos, lit, dst)
  - Address register (a0) and relative addressing support
  - DCL semantic declarations for vertex attributes
  - DEF constant definitions (c0-c95)
- ✅ **Pixel shader 1.4 support** - Full instruction set implementation
  - All ps1.4 opcodes including PHASE, TEXLD, BEM, CND, CMP
  - Bump environment mapping with texture matrices
  - Extended constant registers (c0-c31)
  - GLSL #version 450 core generation for desktop
- ✅ **Shader binary caching** - DX8 bytecode hashing and compiled GLSL caching
  - FNV-1a hash-based cache keys from DX8 bytecode
  - OpenGL program binary storage
  - Persistent disk cache for fast startup

## Rendering Backends

dx8gl supports multiple rendering backends for flexibility:

### OSMesa Backend (Default)
- Software rendering using Mesa's OSMesa library
- No display or GPU required
- Cross-platform compatibility
- Ideal for headless systems and CI/CD

### EGL Backend (Optional)
- Hardware-accelerated rendering via EGL surfaceless context
- Requires EGL_KHR_surfaceless_context extension
- Better performance on systems with GPU
- Useful for embedded systems

### WebGPU Backend (Experimental)
- Modern GPU API for web and native platforms
- Future-proof rendering pipeline
- Works in web browsers with WebGPU support
- Native WebGPU implementations (Dawn, wgpu-native)

### Backend Selection

Select backend at runtime via:

1. **Environment Variable**:
```bash
export DX8GL_BACKEND=osmesa  # or 'egl' or 'webgpu' or 'auto'
./your_app
```

2. **Configuration API**:
```cpp
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_EGL;  // or DX8GL_BACKEND_OSMESA, DX8GL_BACKEND_WEBGPU, DX8GL_BACKEND_DEFAULT
dx8gl_init(&config);
```

Note: `DX8GL_BACKEND_DEFAULT` (or "auto") enables automatic backend selection with fallback chain: WebGPU → EGL → OSMesa

3. **Command Line** (if using provided scripts):
```bash
./scripts/run_dx8_cube.sh --backend=egl
```

## Building

### Desktop (Linux/Windows/macOS)
```bash
cmake -S . -B build
cmake --build build -j
```

### With EGL Backend Support
```bash
cmake -S . -B build -DDX8GL_ENABLE_EGL=ON
cmake --build build -j
```

### With WebGPU Backend Support
```bash
cmake -S . -B build -DDX8GL_ENABLE_WEBGPU=ON
cmake --build build -j
```

### With All Backends
```bash
cmake -S . -B build -DDX8GL_ENABLE_EGL=ON -DDX8GL_ENABLE_WEBGPU=ON
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

Several sample applications demonstrate dx8gl functionality:

1. **minimal_dx8gl_test** - Basic initialization and screen clearing
2. **spinning_triangle** - Vertex buffer usage with transformations
3. **textured_cube** - Advanced features including texturing and index buffers
4. **shader_effects_cube** - DirectX shader assembly parsing
5. **device_reset** - Device reset handling with resource recreation
6. **shader_test** - Comprehensive ps1.4/vs1.1 shader translation and cache tests

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

### Running Tests

dx8gl includes comprehensive tests for shader translation and caching:

```bash
# Run all tests using the test script
./run_tests.sh

# Or run individual tests
./build/samples/dx8gl_shader_test

# Run tests with CTest
cd build
ctest --output-on-failure
```

The shader tests verify:
- VS 1.1 instruction parsing (SINCOS, address register, relative addressing)
- PS 1.4 instruction parsing (BEM, PHASE, CND/CMP)
- GLSL generation correctness
- Binary cache functionality
- Bytecode hashing algorithm

### Backend Testing

Test both backends and compare outputs:

```bash
# Run backend regression test
./scripts/run_backend_test.sh
```

This test:
- Renders the same scene with both OSMesa and EGL backends
- Saves output as PPM files for comparison
- Reports any rendering differences
- Requires EGL backend to be enabled at build time

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