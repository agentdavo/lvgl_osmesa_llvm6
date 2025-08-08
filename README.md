# LVGL OSMesa LLVM Project

A demonstration project combining LVGL GUI framework with OSMesa software rendering, DirectX 8 API compatibility, and modern C++ graphics programming.

## Overview

This project showcases:
- **LVGL GUI Applications** with software-rendered 3D graphics
- **OSMesa** (Off-Screen Mesa) for CPU-based OpenGL rendering without GPU
- **DirectX 8 API** support through the dx8gl translation layer
- **Flexible Rendering Backends** - Switch between OSMesa (software), EGL (hardware), and WebGPU (experimental) at runtime
- **Loading Screen** - Professional 1-second loading animation before rendering starts
- **Error Handling** - Graceful fallback screens when initialization fails
- **Texture loading** from TGA files with full mipmap generation
- **Cross-platform** support for X11 and SDL2 backends
- **Modern C++** with GLM for matrix operations
- **Complete software stack** - no GPU or hardware acceleration required

### Recent Achievements (August 2025)
- ✅ **D3DX Math Library Complete** - Full implementation of plane operations and color manipulation (August 8)
- ✅ **Shader System Enhancements** - Program linking, constant batching, and persistent caching (August 8)
- ✅ **Thread-Safe COM Wrapper Implementation** - Complete resource wrapper classes with mutex synchronization (August 7)
- ✅ **Cube Texture Reset Tracking** - Fixed resource leak prevention during device reset (August 7)
- ✅ **Full DirectX 8 Render State Support** - All DX8Wrapper required states implemented
- ✅ **Enhanced Display Mode Enumeration** - Multiple refresh rates and formats
- ✅ **Cube Texture PreLoad** - Full implementation with seamless filtering
- ✅ **WebGPU Backend Support** - Experimental modern GPU API backend
- ✅ **Backend Abstraction System** - Runtime switching between OSMesa, EGL, and WebGPU
- ✅ **COM Wrapper Architecture** - Comprehensive refactoring with vtables and factory functions

## Quick Start

```bash
# Clone with submodules
git clone --recursive <repository-url>
cd lvgl_osmesa_llvm6

# Build everything (takes 30-45 minutes due to LLVM)
./scripts/compile.sh all

# Build with EGL backend support
./scripts/compile.sh -e all      # Enable EGL backend

# Or build components individually
./scripts/compile.sh llvm      # Build LLVM first (required)
./scripts/compile.sh mesa      # Then Mesa
./scripts/compile.sh examples  # Finally, the examples

# Run examples using convenience scripts (handles library paths)
./scripts/run_lvgl_hello.sh              # Basic LVGL window
./scripts/run_lvgl_osmesa.sh             # OpenGL triangle in LVGL
./scripts/run_dx8_cube.sh                # DirectX 8 spinning cube (with logging)
./scripts/run_osmesa_test.sh             # Creates test.ppm image

# Run with different backends
DX8GL_BACKEND=osmesa ./scripts/run_dx8_cube.sh  # Software rendering (default)
DX8GL_BACKEND=egl ./scripts/run_dx8_cube.sh     # Hardware acceleration
```

## Examples

### lvgl_hello
Basic LVGL application demonstrating window creation and simple UI elements.

### lvgl_osmesa_example
Renders an animated OpenGL triangle using OSMesa and displays it in an LVGL canvas widget.

### dx8_cube
Demonstrates DirectX 8 API usage with:
- Colorful spinning cube with per-vertex lighting
- Textured floor plane using TGA texture loading
- Proper vertex attribute binding for texture coordinates
- Frame-based rendering (100 frames then graceful exit)
- Full DirectX 8 to OpenGL translation via dx8gl

### osmesa_test
Standalone OSMesa test that renders a triangle to a PPM image file.

## Architecture

```
Application Code (OpenGL/DirectX 8)
        ↓
    dx8gl Translation Layer
        ↓
  Render Backend Interface
     ↙         ↘
OSMesa Backend   EGL Backend
     ↓               ↓
Software Render  Hardware GPU
     ↓               ↓
  RGBA Buffer    RGBA Buffer
        ↘         ↙
      RGB565 Conversion
            ↓
       LVGL Canvas Widget
            ↓
      GUI Window (X11/SDL2)
```

### Backend Architecture

The dx8gl library now supports multiple rendering backends:
- **OSMesa Backend**: Pure software rendering using Mesa's llvmpipe driver
- **EGL Backend**: Hardware-accelerated rendering via EGL surfaceless context
- **WebGPU Backend**: Experimental modern GPU API for web and native platforms
- **Runtime Selection**: Choose backend via environment variable, API, or command line

## Dependencies

### Included (as git submodules)
- **LLVM** - JIT compiler for Mesa's llvmpipe software rasterizer
- **Mesa** - OSMesa for software OpenGL implementation
- **LVGL v9.3.0+** - Modern embedded GUI library
- **dx8gl** - DirectX 8 to OpenGL translation layer
- **GLM** - C++ mathematics library for graphics

### Required System Packages
- CMake 3.16+
- Meson & Ninja (for Mesa build)
- SDL2 development libraries
- X11 development libraries
- C++20 compatible compiler

## Build System

The project uses a unified build system with `scripts/compile.sh`:

```bash
./scripts/compile.sh -h          # Show help
./scripts/compile.sh status      # Check build status
./scripts/compile.sh -c          # Clean build
./scripts/compile.sh -C          # Clean everything
./scripts/compile.sh -d all      # Debug build
./scripts/compile.sh -j 4 all    # Use 4 parallel jobs
./scripts/compile.sh -e all      # Enable EGL backend support
```

Build times (approximate):
- LLVM: 30-45 minutes
- Mesa: 5-10 minutes  
- Examples: < 1 minute each

### Recent Updates (December 2025)
- **Volume Texture Support**: Full 3D texture implementation
  - Complete IDirect3DVolumeTexture8 and IDirect3DVolume8 interfaces
  - OpenGL 3D texture creation and management
  - GLSL/WGSL shader support for volumetric effects
  - Ray marching volumetric fog generation helpers
- **Async WebGPU Architecture**: Modern promise/future-based async operations
  - WebGPUAsyncHandler for adapter, device, and buffer operations
  - WebGPUAsyncResource RAII wrapper for resource management
  - WebGPUAsyncCommand for async command submission with callbacks
  - Eliminated all polling loops for better performance
- **Cross-Backend Testing**: Comprehensive rendering consistency verification
  - Automated pixel comparison across OSMesa, EGL, and WebGPU
  - Test scenes: solid triangles, textured quads, alpha blending
  - PPM/PNG output for visual debugging
  - Configurable tolerance for rasterization differences
- **Enhanced Cube Texture Infrastructure**: Complete environment mapping support
  - CubeTextureSupport helper classes for all backends
  - Automatic texture coordinate generation (reflection, normal, sphere mapping)
  - Environment mapping with reflection, refraction, and Fresnel effects
  - Dynamic cube textures for render-to-texture
- Mesa now builds with shared libraries instead of static for better compatibility
- Tests properly link against locally built Mesa 25.0.7 instead of system libraries
- Added WebGPU backend support (experimental)
- Fixed backend selection enum naming issues
- Created OffscreenFramebuffer helper class for unified framebuffer management

## System Requirements

### Minimum
- Linux (tested on Ubuntu/Debian/Fedora)
- 4GB RAM (8GB recommended for building)
- 10GB free disk space
- Multi-core CPU (build times scale with cores)

### Software
- C++20 compiler (GCC 7+ or Clang 6+)
- Python 3.6+ (for Meson)
- pkg-config

## License

Individual components are licensed under their respective licenses:
- LLVM: Apache License 2.0 with LLVM exceptions
- Mesa: MIT License
- LVGL: MIT License
- dx8gl: See ext/dx8gl/LICENSE
- GLM: MIT License

## Contributing

This is a demonstration project. Feel free to fork and experiment!

## Troubleshooting

### Build Issues
- **LLVM build fails**: Check you have enough RAM and disk space
- **Mesa can't find LLVM**: Ensure LLVM build completed successfully first
- **Missing dependencies**: Install required packages:
  ```bash
  # Ubuntu/Debian
  sudo apt install cmake meson ninja-build libsdl2-dev libx11-dev libxext-dev
  
  # Fedora
  sudo dnf install cmake meson ninja-build SDL2-devel libX11-devel libXext-devel
  ```

### Runtime Issues
- **Black screen**: Run `osmesa_test` first to verify OpenGL rendering
- **Slow performance**: This uses software rendering; performance depends on CPU
- **X11 errors**: Try SDL2 backend instead (better support)

### Development Tips
- Use `./scripts/compile.sh status` to check what's built
- Build with `-v` flag for verbose output when debugging
- Check `build/` directory for all outputs

## Testing

### Test Suite Overview
The dx8gl library includes a comprehensive test suite with automated test runners and multiple test categories:

### Running Tests

#### Quick Test Run
```bash
# Run all dx8gl tests using the automated test runner
./scripts/run_dx8gl_tests.sh

# Run with specific options
./scripts/run_dx8gl_tests.sh --filter shader   # Run only shader tests
./scripts/run_dx8gl_tests.sh --valgrind        # Run with memory leak detection
./scripts/run_dx8gl_tests.sh --coverage        # Generate code coverage report
```

#### Manual Test Execution
```bash
# Tests are built in the Release directory
cd build

# Set library paths for OSMesa/LLVM
export LD_LIBRARY_PATH=llvm-install/lib:mesa-install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# Run individual tests
./Release/test_shader_translator      # Shader translation tests
./Release/test_render_states         # Render state management
./Release/test_com_wrapper_threading # COM wrapper thread safety
./Release/test_backend_selection     # Backend switching tests
./Release/test_alpha_blending        # Alpha blending operations
```

### Test Categories

#### Core API Tests
- `test_dx8gl_core_api` - DirectX 8 API implementation
- `test_device_reset` - Device reset and resource recreation
- `test_swapchain_presentation` - Swap chain functionality

#### Shader Tests
- `test_shader_translator` - DirectX to GLSL translation
- `test_shader_cache_resize` - Shader binary caching
- `test_shader_cache_simple` - Shader cache without OpenGL context
- `test_shader_program_linking` - Vertex/pixel shader linking
- `test_shader_constant_batching` - Efficient uniform updates
- `test_multi_texcoords` - Multiple texture coordinate handling

#### Math & Utility Tests
- `test_d3dx_math` - Complete D3DX math library (19 tests)
- `test_d3dx_color` - Color manipulation functions (7 tests)

#### Resource Tests
- `test_cube_texture` - Cube texture functionality
- `test_surface_format` - Surface format conversions
- `test_texture_simple` - Basic texture operations

#### Threading Tests
- `test_com_wrapper_threading` - Thread-safe COM wrapper operations
- `test_async_simple` - Asynchronous command execution

#### State Management
- `test_render_states` - Render state validation
- `test_state_manager_validation` - State manager correctness
- `test_alpha_blending` - Blending state operations

### Building Tests
```bash
# Build all tests
./scripts/compile.sh all

# Build only dx8gl library and tests
cd build
make dx8gl
make -C ext/dx8gl/test

# Build specific test
make test_render_states
```

### Cross-Backend Testing
```bash
# Run backend comparison tests
./scripts/run_backend_test.sh

# Test specific backend
DX8GL_BACKEND=osmesa ./Release/test_backend_selection
DX8GL_BACKEND=egl ./Release/test_backend_selection
```

### Debugging Failed Tests
```bash
# Enable debug output
export DX8GL_LOG_LEVEL=DEBUG
export DX8GL_LOG_FILE=/tmp/dx8gl_test.log

# Run with GDB
gdb ./Release/test_render_states

# Run with Valgrind for memory issues
valgrind --leak-check=full --track-origins=yes ./Release/test_shader_translator
```

Test scenarios:
- **Solid Triangle**: Basic rasterization
- **Textured Quad**: Texture filtering and sampling
- **Alpha Blending**: Transparency and blending operations

### Manual Testing
```bash
# Test OSMesa backend
DX8GL_BACKEND=osmesa ./scripts/run_dx8_cube.sh

# Test EGL backend (if available)
DX8GL_BACKEND=egl ./scripts/run_dx8_cube.sh

# Test WebGPU backend (if available)
DX8GL_BACKEND=webgpu ./scripts/run_dx8_cube.sh
```