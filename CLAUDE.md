# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT: Task Management Requirements

When working on this project, you MUST:
1. **Always use the TodoWrite tool** to track tasks and progress
2. **Update task status immediately** when starting work (mark as "in_progress") and when completing work (mark as "completed")
3. **Add new tasks proactively** when discovering issues or identifying work that needs to be done
4. **Check TASKS.md regularly** for the full task list and priorities
5. **Never work on tasks without tracking them** in the todo list
6. **Keep only one task "in_progress" at a time** to maintain focus

The todo list is critical for project organization and helps track complex multi-step work. Use it liberally!

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
- Static library build for easier distribution
- No Vulkan, VA-API, VDPAU, or other acceleration APIs

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

### Backend Selection Methods
```bash
# Environment variable
export DX8GL_BACKEND=egl

# Command line argument
export DX8GL_ARGS="--backend=egl"

# API configuration (shown above)
```

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