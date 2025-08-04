# Changelog

All notable changes to the LVGL OSMesa LLVM project will be documented in this file.

## [Unreleased] - 2025-08-04

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

## [1.0.0] - 2025-08-03

### Initial Release
- LVGL GUI framework integration with OSMesa software rendering
- DirectX 8 API support through dx8gl translation layer
- Texture loading from TGA files with mipmap generation
- Cross-platform support for X11 and SDL2 backends
- Complete software rendering stack with LLVM-based Mesa
- Example applications demonstrating various features