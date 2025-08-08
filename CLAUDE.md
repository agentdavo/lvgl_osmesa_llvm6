# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT: Task Management Process

When working on this project, you MUST follow this development process:
1. **Always track tasks and progress** through proper task management
2. **Update task status immediately** when starting work (mark as "in_progress") and when completing work (mark as "completed")
3. **Add new tasks proactively** when discovering issues or identifying work that needs to be done
4. **Check TASKS.md regularly** for the full task list and priorities
5. **Never work on tasks without tracking them** in the task list
6. **Keep only one task "in_progress" at a time** to maintain focus

The task tracking process is critical for project organization and helps manage complex multi-step work. Follow this development process consistently!

## Project Overview

This project combines LLVM, Mesa (OSMesa), LVGL, and dx8gl to create GUI applications with software-based OpenGL rendering. It demonstrates:
- LVGL windows containing OpenGL-rendered content using OSMesa for off-screen rendering
- DirectX 8 API compatibility through dx8gl translation layer
- **Flexible rendering backends**: Switch between OSMesa (software) and EGL (hardware) at runtime
- Texture loading from TGA files with full mipmap generation
- Fixed vertex attribute binding for texture coordinates
- Integration with GLM for modern C++ matrix operations
- Cross-platform GUI support (X11, SDL2) with software rendering
- Complete software rasterization through LLVM's llvmpipe driver
- Frame-based graceful exit and clean memory management

## Build Commands

### Build System Overview
All building should be done using the `scripts/compile.sh` script. This ensures consistent build configuration and proper dependency management. All build and run scripts are located in the `scripts/` directory to keep the project root clean.

### Initial Setup
```bash
# Initialize and update submodules
git submodule update --init --recursive

# Build everything (this takes 30-45 minutes due to LLVM)
./scripts/compile.sh all
```

### Individual Component Builds
```bash
# Build only LLVM (takes 30+ minutes)
./scripts/compile.sh llvm

# Build only Mesa/OSMesa (requires LLVM to be built first)
./scripts/compile.sh mesa

# Build LVGL library
./scripts/compile.sh lvgl

# Build all examples
./scripts/compile.sh examples

# Build with EGL backend support
./scripts/compile.sh -e all

# Build with WebGPU backend support (requires -DDX8GL_ENABLE_WEBGPU=ON)
cmake -S . -B build -DDX8GL_ENABLE_WEBGPU=ON
cmake --build build -j

# Build individual examples
./scripts/compile.sh dx8_cube             # DirectX 8 spinning cube
./scripts/compile.sh osmesa_test          # OSMesa test (outputs PPM)
./scripts/compile.sh lvgl_hello           # Basic LVGL window
./scripts/compile.sh lvgl_osmesa_example  # LVGL + OSMesa integration
```

### Build Options
```bash
# Clean build directory
./scripts/compile.sh -c

# Clean everything including LLVM/Mesa builds
./scripts/compile.sh -C

# Debug build
./scripts/compile.sh -d all

# Verbose output
./scripts/compile.sh -v all

# Set parallel jobs (default: 8)
./scripts/compile.sh -j 4 all

# Enable EGL backend support
./scripts/compile.sh -e all

# Check build status
./scripts/compile.sh status
```

### Running Examples
```bash
# Use the convenience run scripts (handles library paths automatically):
./scripts/run_dx8_cube.sh         # DirectX 8 cube demo with logging
./scripts/run_osmesa_test.sh      # OSMesa test (generates test.ppm)
./scripts/run_lvgl_hello.sh       # Basic LVGL window
./scripts/run_lvgl_osmesa.sh      # LVGL + OSMesa integration demo

# Or run manually with correct library paths:
LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu \
  build/src/dx8_cube/dx8_cube

# dx8_cube now exits gracefully after 100 frames (no timeout needed)

# Run with different rendering backends
DX8GL_BACKEND=osmesa ./scripts/run_dx8_cube.sh  # Software rendering (default)
DX8GL_BACKEND=egl ./scripts/run_dx8_cube.sh     # Hardware acceleration (requires -e build)

# Run EGL backend test (requires -e build)
./scripts/run_egl_test.sh
```

## Architecture Overview

### Component Structure
- **LLVM**: Provides software rasterization backend for Mesa's llvmpipe driver
  - Built with X86 target only to minimize build time
  - Static libraries only, with tools enabled for llvm-config
  - RPATH disabled to avoid installation issues
- **Mesa**: OSMesa (off-screen Mesa) for software OpenGL rendering
  - Built with LLVM llvmpipe driver for CPU-based rendering
  - No hardware drivers, pure software implementation
- **LVGL v9.3.0+**: Cross-platform GUI library
  - SDL2 backend (primary, full-featured)
  - X11 backend (basic implementation)
  - Canvas widget for displaying OpenGL framebuffers
- **dx8gl**: DirectX 8.1 to OpenGL 3.3 Core translation layer
  - Allows legacy DirectX 8 code to run on OpenGL
  - **Backend abstraction**: Switch between OSMesa and EGL at runtime
  - OSMesa backend for software rendering
  - EGL backend for hardware acceleration via surfaceless context
- **GLM**: Header-only C++ mathematics library for graphics programming
- **Platform Layer**: C++ abstraction (`src/lvgl_platform/`) providing unified LVGL backend interface

### Rendering Pipeline
1. LVGL creates GUI window using SDL2 or X11 backend
2. OSMesa creates off-screen OpenGL 3.3 context using LLVM-based software rasterization
3. OpenGL content is rendered to RGBA float buffer
4. Custom conversion transforms RGBA float → RGB565 for LVGL canvas widget
5. LVGL canvas displays the rendered content within the GUI window
6. Animation timer drives real-time updates at 30 FPS

### Build Directory Structure
All build artifacts are contained within the `build/` directory:
```
build/
├── llvm-build/          # LLVM build workspace
├── llvm-install/        # LLVM installation
│   ├── bin/            # llvm-config and other tools
│   └── lib/            # LLVM static libraries
├── mesa-build/          # Mesa build workspace  
├── mesa-install/        # Mesa installation
│   └── lib/            # libOSMesa.a
├── liblvgl.a           # LVGL static library
├── ext/dx8gl/          # dx8gl build
│   └── libdx8gl.a      # dx8gl static library
└── src/                # Application builds
    ├── lvgl_platform/  # Platform abstraction
    ├── osmesa_test/    # OSMesa test program
    ├── lvgl_hello/     # Basic LVGL example
    ├── lvgl_osmesa/    # LVGL+OSMesa example
    └── dx8_cube/       # DirectX 8 example
```

## Configuration Files

### LVGL Configuration (`lv_conf.h`)
- Color format: RGB565 (16-bit) for memory efficiency
- Backends: SDL2 (primary), X11 (basic implementation)
- Canvas widget enabled for OSMesa framebuffer display
- Memory management: Standard C library (malloc/free)

### LLVM Build Configuration (CMakeLists.txt)
- Target: X86 only (minimal build for size)
- RTTI disabled for smaller binaries
- Static libraries only (no shared libs)
- Tools enabled (required for llvm-config)
- RPATH disabled to avoid installation issues
- MinSizeRel build type for optimized but smaller binaries

### Mesa Build Configuration (CMakeLists.txt)
- OSMesa-only build (no hardware drivers)
- LLVM llvmpipe driver for software rendering
- Uses custom Meson native file with Fedora security flags
- Shared library build (changed from static for better compatibility)
- No Vulkan, VA-API, VDPAU, or other acceleration APIs
- Version 25.0.7 with LLVM 20.1.8 backend

## Rendering Backend System

### Backend Architecture
The dx8gl library supports multiple rendering backends through a clean abstraction interface:

```cpp
// Select backend at runtime
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_EGL;  // or DX8GL_BACKEND_OSMESA
dx8gl_init(&config);
```

### Available Backends
1. **OSMesa Backend** (Default)
   - Pure software rendering using Mesa's llvmpipe
   - No GPU or display required
   - Consistent behavior across all systems
   - Ideal for testing and CI/CD environments

2. **EGL Backend** (Optional)  
   - Hardware-accelerated rendering via EGL surfaceless context
   - Requires EGL 1.5+ with surfaceless context extension
   - Significantly better performance when GPU is available
   - Falls back gracefully if not available

3. **WebGPU Backend** (Experimental)
   - Modern GPU API for next-generation rendering
   - Cross-platform support (web browsers and native)
   - Future-proof rendering pipeline
   - Requires WebGPU-enabled browser or native implementation

### Backend Selection Methods
```bash
# Environment variable
export DX8GL_BACKEND=egl  # or osmesa, webgpu, auto

# Command line argument  
export DX8GL_ARGS="--backend=egl"

# API configuration
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_DEFAULT;  # auto-select with fallback
dx8gl_init(&config);
```

The "auto" backend selection uses a fallback chain: WebGPU → EGL → OSMesa

## Known Issues and Solutions

### LLVM Build Issues
- **RPATH Error**: Fixed by adding `-DCMAKE_SKIP_INSTALL_RPATH=TRUE`
- **Static Linking Warnings**: Expected when building static LLVM, can be ignored
- **Build Time**: LLVM takes 30+ minutes even with parallel builds

### Mesa Build Requirements
- Requires Meson build system and Ninja
- Must use our custom-built LLVM (system LLVM won't work)
- Needs explicit path to llvm-config in native file

### Platform Compatibility
- **X11 Backend**: Basic implementation, SDL2 recommended
- **Static Builds**: All libraries built as static for portability

### Fixed Issues
- **Texture Coordinate Crash**: Fixed by properly enabling texcoord0 attribute in command buffer
- **Double Free Error**: Fixed by removing redundant dx8gl_init() call
- **Floor Position**: Fixed Y-axis orientation when saving PPM files
- **Mesa Library Linking**: Fixed tests linking to system Mesa instead of locally built version
- **Backend Enum Naming**: Fixed duplicated backend enum definitions (DX8_BACKEND_* vs DX8GL_BACKEND_*)
- **Build Configuration**: Changed Mesa from static to shared libraries for better compatibility

## Development Workflow

### Adding New Examples
1. Create new directory under `src/`
2. Add CMakeLists.txt with proper LVGL and OSMesa linking
3. Link against `lvgl`, `lvgl_platform`, and required libraries (OSMesa, dx8gl)
4. Use platform abstraction layer for window creation
5. For DirectX 8 examples, include GLM for matrix operations

### Debugging OSMesa Issues
1. Run `osmesa_test` first to verify OSMesa setup
2. Check `test.ppm` output image for correct rendering
3. LVGL canvas requires RGB565 format - verify color conversion
4. Use `MESA_DEBUG=1` environment variable for Mesa debugging
5. For runtime issues, ensure proper `LD_LIBRARY_PATH` is set to find custom LLVM/Mesa libraries

### Runtime Environment
Examples that use OpenGL/OSMesa require the correct library path:
```bash
export LD_LIBRARY_PATH=build/llvm-install/lib:build/mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
```

### Color Format Conversion
- OSMesa renders to RGBA float (32-bit per channel)
- LVGL canvas expects RGB565 (16-bit total)
- Critical conversion code in `src/lvgl_osmesa/main.cpp`

## External Dependencies

### Git Submodules
- `ext/llvm-project` - LLVM (tag: llvmorg-20.1.8)
- `ext/mesa` - Mesa (tag: mesa-25.0.4)  
- `ext/lvgl` - LVGL latest (main branch)
- `ext/dx8gl` - DirectX 8 to OpenGL translation layer
- `ext/glm` - GLM mathematics library for graphics

### System Requirements
- CMake 3.16+
- Meson and Ninja (for Mesa)
- SDL2 development libraries
- X11 development libraries (libx11-dev, libxext-dev)
- OpenGL/GLU development headers
- Standard C/C++ toolchain (GCC 7+ or Clang 6+)

### dx8gl performance Considerations

1. **State Filtering**: Cache and filter redundant state changes
2. **Resource Pooling**: Pool temporary buffers for immediate mode
3. **Batch Optimization**: Combine draw calls where possible
4. **Memory Management**: Implement proper eviction for MANAGED pool
5. **Threading**: Use command buffers for deferred execution

### dx8gl testing Requirements

Each implemented method needs:
1. Parameter validation tests
2. State interaction tests
3. Resource lifetime tests
4. Error condition tests
5. Performance benchmarks

## Testing Infrastructure

### Test Suite
The dx8gl library has comprehensive unit tests located in `ext/dx8gl/test/`. Tests are built in the `build/Release/` directory.

### Running Tests
```bash
# Automated test runner (recommended)
./scripts/run_dx8gl_tests.sh

# Run with options
./scripts/run_dx8gl_tests.sh --filter shader   # Run only shader tests
./scripts/run_dx8gl_tests.sh --valgrind        # Memory leak detection
./scripts/run_dx8gl_tests.sh --coverage        # Generate coverage report

# Manual test execution
cd build
export LD_LIBRARY_PATH=llvm-install/lib:mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
./Release/test_shader_translator
./Release/test_render_states
./Release/test_com_wrapper_threading
./Release/test_d3dx_math
./Release/test_d3dx_color
```

### Test Categories
- **Core API**: test_dx8gl_core_api, test_device_reset, test_swapchain_presentation
- **Shaders**: test_shader_translator, test_shader_cache_resize, test_shader_cache_simple, test_shader_program_linking, test_shader_constant_batching, test_multi_texcoords
- **Math & Utilities**: test_d3dx_math (19 tests), test_d3dx_color (7 tests)
- **Resources**: test_cube_texture, test_surface_format, test_texture_simple
- **Threading**: test_com_wrapper_threading, test_async_simple
- **State**: test_render_states, test_state_manager_validation, test_alpha_blending
- **Backend**: test_backend_selection, test_framebuffer_correctness

### Building Tests
```bash
# Build all tests with the main build
./scripts/compile.sh all

# Build specific test
cd build
make test_render_states

# Build all tests in dx8gl
make -C ext/dx8gl/test
```

### Known Test Issues
- OSMesa context warnings in multi-threaded tests are expected (OpenGL is not thread-safe)
- test_vertex_shader_disassembly may have PIE linking issues
- Some tests require stub COM vtable implementations (already added to d3d8_com_wrapper.cpp)
- Resource pool metrics updates are currently commented out (need proper API)

### Test Infrastructure Files
- `scripts/run_dx8gl_tests.sh` - Main test runner script
- `ext/dx8gl/TEST_AUTOMATION.md` - Comprehensive test automation guide
- `ext/dx8gl/test/CMakeLists.txt` - Test build configuration

## Recent Completed Tasks (August 2025)

### D3DX Math Library Extensions ✅ (August 8)
- **Plane Utility Functions**: Complete implementation of all D3DX plane operations
  - D3DXPlaneDot, D3DXPlaneDotCoord, D3DXPlaneDotNormal
  - D3DXPlaneNormalize, D3DXPlaneFromPointNormal, D3DXPlaneFromPoints
  - D3DXPlaneTransform with matrix multiplication
  - Fixed cross product calculation for correct plane normal direction
- **Color Manipulation Helpers**: Full set of color operations
  - D3DXColorAdjustSaturation with proper luminance weights (0.2125, 0.7154, 0.0721)
  - D3DXColorAdjustContrast, D3DXColorLerp, D3DXColorModulate
  - D3DXColorNegative, D3DXColorScale with HDR support
  - Comprehensive test coverage with edge cases

### Shader System Enhancements ✅ (August 8)
- **Shader Program Linking Integration**: Automatic vertex/pixel shader combination
  - ShaderProgramManager handles program creation and caching
  - Supports vertex-only programs with default pixel shader
  - Program invalidation and recompilation on shader changes
  - Comprehensive test suite with 5 test scenarios
- **Shader Constant Management System**: Optimized uniform updates
  - Batched constant uploads to minimize GPU calls
  - Dirty flag tracking for changed constants
  - Support for float4, matrix4, int4, and bool constants
  - Global constant cache for shared uniforms
  - Performance metrics and memory tracking
- **Persistent Shader Caching**: Disk-based shader binary cache
  - Memory and disk caching with LRU eviction
  - Compression support for disk storage
  - Async disk operations for non-blocking I/O
  - Cache statistics and validation
  - Memory-mapped file support for ultra-fast access

## Recent Completed Tasks (August 2025)

### COM Wrapper Resource Management (August 7, 2025) ✅
- Implemented thread-safe wrapper classes for all DirectX 8 resource types:
  - Direct3DSurface8_COM_Wrapper with mutex synchronization
  - Direct3DSwapChain8_COM_Wrapper for swap chain management
  - Direct3DVertexBuffer8_COM_Wrapper and Direct3DIndexBuffer8_COM_Wrapper for buffer resources
  - Direct3DCubeTexture8_COM_Wrapper and Direct3DVolumeTexture8_COM_Wrapper for texture resources
  - Direct3DVolume8_COM_Wrapper for volume slices
- Added complete vtable definitions for all COM interfaces
- Created factory functions for wrapper instantiation with proper reference counting
- Updated all resource creation methods to use wrappers instead of direct pass-through

### Cube Texture Device Reset Tracking (August 7, 2025) ✅
- Fixed missing registration of cube textures in CreateCubeTexture
- Cube textures now properly tracked in all_cube_textures_ vector
- Prevents resource leaks during device reset operations
- Ensures D3DPOOL_DEFAULT cube textures are recreated after reset

### DirectX 8 Render States Implementation ✅
- Added all missing render states required by DX8Wrapper:
  - D3DRS_RANGEFOGENABLE for range-based fog calculations
  - D3DRS_FOGVERTEXMODE for vertex fog mode selection
  - D3DRS_SPECULARMATERIALSOURCE for specular material source
  - D3DRS_COLORVERTEX for color vertex enable/disable
  - D3DRS_ZBIAS with OpenGL polygon offset mapping
- Extended StateManager with new fields and proper OpenGL state mapping
- Added comprehensive test_render_states.cpp validation

### Volume Texture Support ✅
- Added stub implementation in UpdateTexture for D3DRTYPE_VOLUMETEXTURE
- CreateVolumeTexture returns D3DERR_NOTAVAILABLE as placeholder
- Foundation laid for future 3D texture implementation

### Cube Texture PreLoad Implementation ✅
- Fully implemented PreLoad() method for cube textures
- Sets proper texture sampling parameters (min/mag filters, wrap modes)
- Enables GL_TEXTURE_CUBE_MAP_SEAMLESS for OpenGL 3.2+
- Ensures all mip levels are properly allocated and uploaded

### Display Mode Enumeration Enhancement ✅
- Added multiple refresh rates (60, 75, 85, 100, 120 Hz) per format
- Support for 16-bit (R5G6B5, X1R5G5B5) and 32-bit (X8R8G8B8, A8R8G8B8) formats
- Improved CheckDepthStencilMatch for better depth format support
- Full compatibility with DX8Wrapper's Find_Color_Mode and Find_Z_Mode logic

### COM Wrapper Refactoring Plan ✅
- Created comprehensive 15-task series (CW01-CW15) for COM interface overhaul
- Planned base COM object implementation with proper IUnknown
- Defined tasks for Surface, SwapChain, Volume, and VolumeTexture wrappers
- Included testing, optimization, and documentation tasks
- Updated TASKS.md with proper dependencies and execution order

### Previous Infrastructure Tasks

#### Backend Enum Fixes
- Fixed duplicated backend enum definitions in dx8gl headers
- Consolidated DX8_BACKEND_* and DX8GL_BACKEND_* to use only DX8GL_BACKEND_*
- Added proper backend selection logic in dx8gl initialization

#### OffscreenFramebuffer Helper Class
- Created unified framebuffer management class for all backends
- Supports multiple pixel formats (RGBA8, RGB565, FLOAT_RGBA, etc.)
- Includes format conversion utilities
- Provides CPU/GPU buffer synchronization

#### WebGPU Backend Support
- Added WebGPU backend compilation flags to build system
- Created WebGPU backend interface following the DX8RenderBackend pattern
- Added build documentation for WebGPU compilation

#### Test Suite Improvements
- Created backend selection tests to verify runtime backend switching
- Added framebuffer correctness tests for the new helper class
- Fixed test CMakeLists.txt to properly link against OSMesa target

#### Mesa Library Linking Fix
- Changed Mesa build from static to shared libraries in CMakeLists.txt
- Updated CMake configuration to detect both static and shared Mesa libraries
- Fixed test linking to use locally built Mesa 25.0.7 instead of system libraries
- Verified all dx8gl tests pass with the correct Mesa version