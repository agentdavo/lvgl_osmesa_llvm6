# Changelog

All notable changes to the LVGL OSMesa LLVM project will be documented in this file.

## [Unreleased] - 2025-08-06

### Added
- **Backend Abstraction System**: Complete refactoring of dx8gl to support multiple rendering backends
  - Abstract `DX8RenderBackend` interface for backend implementations
  - `DX8OSMesaBackend` for software rendering via OSMesa
  - `DX8EGLBackend` for hardware-accelerated rendering via EGL surfaceless context
  - Runtime backend selection without recompilation
  
- **Backend Selection Methods**:
  - Environment variable: `DX8GL_BACKEND=osmesa|egl`
  - Command line argument: `DX8GL_ARGS="--backend=egl"`
  - API configuration: `config.backend_type = DX8GL_BACKEND_EGL`
  
- **Build System Enhancements**:
  - Added `-e, --egl` flag to `compile.sh` for enabling EGL backend
  - CMake option `DX8GL_ENABLE_EGL` for conditional EGL compilation
  - Automatic detection of EGL/GLESv2 libraries via pkg-config
  
- **New Examples and Tests**:
  - `egl_backend_test`: Demonstrates EGL backend usage
  - `run_egl_test.sh`: Convenience script for running EGL tests
  
- **LVGL Loading Screen**: Added 1-second loading screen before rendering begins
  - Shows "LOADING..." text with animated progress bar
  - Ensures LVGL window displays properly regardless of dx8gl initialization status
  - Error screen fallback if DirectX 8 initialization fails
  
### Changed
- **dx8gl Initialization**: Now creates and manages a global render backend
- **Direct3DDevice8**: Updated to use shared render backend instead of creating its own context
- **Backend Factory**: `create_render_backend()` function for backend instantiation

### Fixed
- Eliminated duplicate context creation in Direct3DDevice8
- Proper resource management with non-owning backend pointers
- Clean separation between backend interface and implementation

## [1.1.0] - 2025-08-06

### Added
- **WebGPU Backend Support**: Experimental WebGPU rendering backend
  - Added `DX8GL_ENABLE_WEBGPU` CMake option
  - WebGPU backend follows the same `DX8RenderBackend` interface
  - Support for modern GPU API on web and native platforms
  
- **OffscreenFramebuffer Helper Class**: Unified framebuffer management
  - Supports multiple pixel formats (RGBA8, RGB565, FLOAT_RGBA, BGRA8, BGR8)
  - Built-in format conversion utilities
  - CPU/GPU buffer synchronization
  - Lazy buffer allocation for memory efficiency
  
- **Enhanced Test Suite**:
  - `test_backend_selection`: Verifies runtime backend switching
  - `test_framebuffer_correctness`: Tests OffscreenFramebuffer functionality
  - All tests properly link against locally built Mesa libraries

### Changed
- **Mesa Build Configuration**: 
  - Switched from static to shared library build for better compatibility
  - Updated to Mesa 25.0.7 with LLVM 20.1.8 backend
  - CMake now properly detects both static and shared Mesa libraries
  
- **Backend Selection Priority**: 
  - Auto-selection now uses fallback chain: WebGPU → EGL → OSMesa
  - Better error handling and fallback mechanisms

### Fixed
- **Mesa Library Linking**: Tests now correctly use locally built Mesa 25.0.7 instead of system libraries
- **Backend Enum Naming**: Consolidated DX8_BACKEND_* and DX8GL_BACKEND_* to use only DX8GL_BACKEND_*
- **dx8gl Compilation**: Fixed enum naming issues in d3d8_device.cpp
- **Test Linking**: Updated test CMakeLists.txt to use OSMesa target instead of OpenGL::GL

## [1.0.0] - 2025-08-03

### Initial Release
- LVGL GUI framework integration with OSMesa software rendering
- DirectX 8 API support through dx8gl translation layer
- Texture loading from TGA files with mipmap generation
- Cross-platform support for X11 and SDL2 backends
- Complete software rendering stack with LLVM-based Mesa
- Example applications demonstrating various features