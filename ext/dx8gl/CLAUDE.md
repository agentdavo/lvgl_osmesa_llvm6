# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with the dx8gl renderer.

## Project Overview

dx8gl is a DirectX 8.1 to OpenGL ES 3.0 / OpenGL 3.3 Core translation library under active development. It provides:
- DirectX 8.1 API surface implementation
- Translation to OpenGL ES 3.0 / OpenGL 3.3 Core (programmable pipeline)
- OSMesa software rendering support (no SDL dependencies)
- Cross-platform support (Linux, Windows, macOS)
- DirectX 8 assembly shader parsing (vertex shader 1.1 and pixel shader 1.4)
- GLSL shader generation from DirectX assembly
- Fixed-function pipeline emulation
- Thread-safe design with shader compilation
- Vertex Array Object (VAO) support for efficient rendering

## Architecture

The dx8gl library acts as a shim between DirectX 8 applications and OpenGL ES 2.0:

```
Application (DirectX 8 API calls)
    ↓
dx8gl Translation Layer
    ↓
OpenGL 3.3 Core / ES 3.0 Commands
    ↓
OSMesa Software Rendering
    ↓
Offscreen Framebuffer
    ↓
LVGL Canvas Widget
```

### Key Components:

**API Headers** (in src/):
- `d3d8.h`, `d3dx8.h`: DirectX 8 API definitions
- `d3d8_interfaces.h`: COM-style interfaces (IDirect3D8, IDirect3DDevice8, etc.)
- `d3d8_types.h`: DirectX 8 data structures
- `dx8gl.h`: Main library interface with initialization
- `hud_system.h`: HUD overlay system for debugging and stats

**Core Implementation**:
- `d3d8_device.cpp`: Main device implementation with state management
- `d3d8_interface.cpp`: Direct3D8 interface and device creation
- `shader_generator.cpp`: Fixed-function to GLSL shader generation
- `shader_bytecode_assembler.cpp`: DirectX shader assembly parsing
- `state_manager.cpp`: Render state tracking and optimization
- `osmesa_context.cpp`: OSMesa context creation and management

**Integration Points**:
- `Direct3DCreate8()`: Main entry point (replaces system DirectX)
- `dx8gl_init()`: Initialize OSMesa and OpenGL context
- `HUD::Create()`: Initialize HUD overlay system
- SDL3 provides cross-platform windowing and input

## Building

### Desktop Build (Linux/Windows/macOS)

```bash
# Configure and build
cmake -S . -B build
cmake --build build -j

# Or use the standalone dx8gl build
cd lib/dx8gl
cmake -S . -B build
cmake --build build -j
```

### Emscripten/WebAssembly Build

```bash
# Source the Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Configure and build
cd lib/dx8gl
cmake -S . -B build-emscripten -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build build-emscripten -j

# Or use the build script
./build_emscripten.sh
```

### Build Configuration
- Standalone CMake project with comprehensive samples
- Links against local SDL3 source (lib/SDL/)
- OpenGL ES 2.0 support via system libraries or WebGL
- Optional features: shader hot reload, binary caching, debug overlays

## Current Status (January 2025)

The dx8gl library is an active DirectX 8 to OpenGL ES 2.0 translation layer with significant functionality implemented.

### ✅ Implemented Features
- **Core DirectX 8 API**: IDirect3D8, IDirect3DDevice8 interfaces
- **Resource Management**: Textures, vertex/index buffers, surfaces
- **Fixed-Function Pipeline**: Dynamic GLSL shader generation based on render states
- **Shader Assembly Parser**: Complete parsing of vs.1.1 and ps.1.4 assembly
  - All major opcodes (MOV, ADD, MUL, DP3, TEXLD, etc.)
  - DCL declarations for vertex attributes
  - DEF instructions for shader constants
  - SINCOS and other special instructions
- **Render States**: Most D3DRS_* states mapped to OpenGL
- **SDL3 Integration**: Cross-platform windowing and context
- **WebAssembly Support**: Full Emscripten build with WebGL 2.0

### 🚧 In Progress
- **Shader Execution**: Shaders parse but need proper program linking
- **Multi-pass Rendering**: Render-to-texture for effects
- **Shader Constants**: Binding and management system
- **Matrix Transformations**: Proper vertex transformation pipeline

### ❌ Not Yet Implemented
- **Shader Program Linking**: Combining vertex/pixel shaders into GL programs
- **Advanced Blend Modes**: Some DirectX blend states
- **Cube Maps and Volume Textures**: 3D texture support
- **Hardware Vertex Processing**: Currently software only
- **D3DX Utilities**: Mesh loading, effects framework

### Sample Applications
- **textured_cube**: Demonstrates basic rendering with textures
- **shader_effects_cube**: Tests DirectX shader assembly parsing
- **spinning_triangle**: Simple geometry rendering
- **minimal_dx8gl_test**: API verification

## Integration with Applications

Applications use dx8gl through standard DirectX 8 API:

```cpp
// Initialize dx8gl (sets up OSMesa)
dx8gl_init(nullptr);

// Create Direct3D8 interface
IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);

// Create device as normal
D3DPRESENT_PARAMETERS pp = { /* ... */ };
IDirect3DDevice8* device;
d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
                   nullptr, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                   &pp, &device);

// Optional: Add HUD overlay
dx8gl::HUD::Create(device);
dx8gl::HUD::Get()->SetFlags(dx8gl::HUD_SHOW_FPS);

// Use standard DirectX 8 API
device->SetRenderState(D3DRS_LIGHTING, TRUE);
device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
```

## Development Guidelines

### OSMesa Context Management

The library uses OSMesa for software rendering and OpenGL context:

```cpp
// OSMesa initialization (in dx8gl_init)
const int attribs[] = {
    OSMESA_FORMAT, OSMESA_RGBA,
    OSMESA_DEPTH_BITS, 24,
    OSMESA_STENCIL_BITS, 8,
    OSMESA_PROFILE, OSMESA_CORE_PROFILE,
    OSMESA_CONTEXT_MAJOR_VERSION, 3,
    OSMESA_CONTEXT_MINOR_VERSION, 3,
    0
};

OSMesaContext context = OSMesaCreateContextAttribs(attribs, NULL);
OSMesaMakeCurrent(context, framebuffer, GL_UNSIGNED_BYTE, width, height);
```

### Shader Generation Strategy

The fixed-function pipeline is emulated through dynamic shader generation:

1. **State-based shader keys**: Render state combinations generate unique shader programs
2. **Shader caching**: Compiled shaders are cached to avoid regeneration
3. **Uniform management**: Fixed-function parameters become shader uniforms
4. **Feature flags**: Lighting, fog, texturing, etc. are controlled via preprocessor defines

### Performance Optimization

1. **State tracking**: Minimize redundant OpenGL state changes
2. **Batch rendering**: Group draw calls with similar state
3. **Shader binary cache**: Store compiled shaders to disk for faster loading
4. **Thread pool**: Multi-threaded shader compilation (optional)

### Debugging Features

- **HUD system**: Real-time stats, FPS, debug info
- **Shader hot reload**: Modify shaders without restarting
- **State validation**: Verify DirectX state combinations
- **OpenGL debug output**: Enable via GL_KHR_debug extension

## DirectX 8 API Mapping

### Key Interfaces Implemented

**IDirect3D8**:
- `GetAdapterCount()`: Returns available graphics adapters
- `GetAdapterIdentifier()`: Returns adapter info (vendor, renderer, driver version)
- `CreateDevice()`: Creates IDirect3DDevice8 instance with OpenGL context

**IDirect3DDevice8**:
- Render state management (SetRenderState, GetRenderState)
- Transform matrices (SetTransform, GetTransform)
- Texture management (SetTexture, GetTexture)
- Vertex/Index buffers (CreateVertexBuffer, CreateIndexBuffer)
- Drawing (DrawPrimitive, DrawIndexedPrimitive)

### DirectX to OpenGL 3.3 Core / ES 3.0 State Mapping

```
D3DRS_ZENABLE          → glEnable/Disable(GL_DEPTH_TEST)
D3DRS_ZWRITEENABLE     → glDepthMask()
D3DRS_ALPHATESTENABLE  → Handled in fragment shader
D3DRS_SRCBLEND         → glBlendFunc() src parameter
D3DRS_DESTBLEND        → glBlendFunc() dst parameter
D3DRS_CULLMODE         → glCullFace() + glEnable(GL_CULL_FACE)
D3DRS_LIGHTING         → Shader uniform flag
D3DRS_FOGENABLE        → Shader uniform flag
```

### Fixed-Function to Shader Pipeline

Since OpenGL 3.3 Core / ES 3.0 has no fixed-function pipeline, dx8gl must generate shaders:

1. **Vertex Shader Generation**:
   - Transform vertices by modelview + projection matrices
   - Apply lighting calculations if enabled
   - Pass through texture coordinates and colors

2. **Fragment Shader Generation**:
   - Sample textures
   - Apply fog if enabled
   - Perform alpha test via discard
   - Output final color

## Platform Support

### Desktop Platforms
- **Linux**: Full support via OSMesa software rendering
- **Windows**: Full support via OSMesa software rendering
- **macOS**: Support via OSMesa with OpenGL 3.3 Core

### Integration with LVGL
- **OSMesa**: Software rendering to offscreen buffer
- **LVGL Canvas**: Display rendered content in LVGL windows
- **No SDL**: Pure OSMesa implementation for LVGL integration

### Mobile Platforms (Future)
- **Android**: SDL3 + OpenGL ES 2.0 native support
- **iOS**: SDL3 + OpenGL ES 2.0 native support

## Next Development Steps

### High Priority
1. **Shader Program Linking**: Combine vertex and pixel shaders into unified GL programs
2. **Shader Constant Management**: Implement proper constant buffer binding system
3. **Matrix Pipeline**: Fix vertex transformation with proper matrix multiplication
4. **Render-to-Texture**: Enable multi-pass rendering effects

### Medium Priority
5. **Texture Coordinate Fix**: Initialize pixel shader texture coordinates from varyings
6. **Multiple Texture Stages**: Support for multi-texturing in shaders
7. **Shader Caching**: Persistent shader cache for faster startup
8. **Performance Optimization**: Reduce shader compilation overhead

### Future Enhancements
9. **Advanced Shader Instructions**: Implement remaining opcodes (MAX, etc.)
10. **Debugging Tools**: Shader debugger, state inspector, performance profiler
11. **D3DX Support**: Implement D3DX utility functions
12. **Hardware Vertex Processing**: Move from software to hardware T&L

## Important Notes

- dx8gl is integrated with LVGL via OSMesa for software rendering
- No SDL dependencies - pure OSMesa implementation
- All rendering uses OpenGL 3.3 Core / ES 3.0 with software rasterization
- The library maintains DirectX 8 semantics while leveraging modern OpenGL
- Vertex Array Objects (VAOs) are used for efficient vertex attribute management
- Shader generation produces GLSL 3.30 core / ES 3.00 shaders