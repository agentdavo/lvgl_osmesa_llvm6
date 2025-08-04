# LVGL OSMesa LLVM Project

A demonstration project combining LVGL GUI framework with OSMesa software rendering, DirectX 8 API compatibility, and modern C++ graphics programming.

## Overview

This project showcases:
- **LVGL GUI Applications** with software-rendered 3D graphics
- **OSMesa** (Off-Screen Mesa) for CPU-based OpenGL rendering without GPU
- **DirectX 8 API** support through the dx8gl translation layer
- **Cross-platform** support for X11 and SDL2 backends
- **Modern C++** with GLM for matrix operations
- **Complete software stack** - no GPU or hardware acceleration required

## Quick Start

```bash
# Clone with submodules
git clone --recursive <repository-url>
cd lvgl_osmesa_llvm6

# Build everything (takes 30-45 minutes due to LLVM)
./scripts/compile.sh all

# Or build components individually
./scripts/compile.sh llvm      # Build LLVM first (required)
./scripts/compile.sh mesa      # Then Mesa
./scripts/compile.sh examples  # Finally, the examples

# Run examples using convenience scripts (handles library paths)
./scripts/run_lvgl_hello.sh              # Basic LVGL window
./scripts/run_lvgl_osmesa.sh             # OpenGL triangle in LVGL
./scripts/run_dx8_cube.sh                # DirectX 8 spinning cube (with logging)
./scripts/run_osmesa_test.sh             # Creates test.ppm image
```

## Examples

### lvgl_hello
Basic LVGL application demonstrating window creation and simple UI elements.

### lvgl_osmesa_example
Renders an animated OpenGL triangle using OSMesa and displays it in an LVGL canvas widget.

### dx8_cube
Demonstrates DirectX 8 API usage with a colorful spinning cube, rendered via dx8gl and OSMesa.

### osmesa_test
Standalone OSMesa test that renders a triangle to a PPM image file.

## Architecture

```
Application Code (OpenGL/DirectX 8)
        ↓
  OSMesa/dx8gl Layer
        ↓
  Mesa llvmpipe Driver
        ↓
  LLVM JIT Compiler
        ↓
  CPU Rasterization
        ↓
    RGBA Float Buffer
        ↓
  RGB565 Conversion
        ↓
   LVGL Canvas Widget
        ↓
    GUI Window (X11/SDL2)
```

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
```

Build times (approximate):
- LLVM: 30-45 minutes
- Mesa: 5-10 minutes  
- Examples: < 1 minute each

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