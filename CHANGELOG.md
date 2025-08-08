# Changelog

All notable changes to the LVGL OSMesa LLVM project will be documented in this file.

## [Unreleased] - 2025-08-08

### Added
- **D3DX Math Library Complete** (2025-08-08):
  - Implemented all D3DX plane utility functions:
    - D3DXPlaneDot, D3DXPlaneDotCoord, D3DXPlaneDotNormal for plane-point operations
    - D3DXPlaneNormalize for normalizing plane equations
    - D3DXPlaneFromPointNormal and D3DXPlaneFromPoints for plane construction
    - D3DXPlaneTransform for matrix transformation of planes
    - Fixed cross product calculation for correct normal direction in D3DXPlaneFromPoints
  - Implemented all D3DX color manipulation functions:
    - D3DXColorAdjustSaturation with proper luminance weights (0.2125, 0.7154, 0.0721)
    - D3DXColorAdjustContrast for contrast adjustment with midpoint
    - D3DXColorLerp for linear interpolation between colors
    - D3DXColorModulate for component-wise multiplication
    - D3DXColorNegative for color inversion preserving alpha
    - D3DXColorScale with HDR support (no clamping)
  - Created comprehensive test coverage with 19 plane tests and 7 color tests

- **Shader System Enhancements** (2025-08-08):
  - **Shader Program Linking Integration**:
    - ShaderProgramManager automatically combines vertex and pixel shaders
    - Program caching to avoid redundant linking operations
    - Support for vertex-only programs with default pixel shader
    - Program invalidation and recompilation on shader changes
    - Created test_shader_program_linking with 5 comprehensive test scenarios
  - **Shader Constant Management System**:
    - Batched constant uploads to minimize GPU state changes
    - Dirty flag tracking for efficient updates
    - Support for float4, matrix4, int4, and bool constant types
    - Global constant cache for sharing uniforms between shaders
    - Performance metrics and memory usage tracking
    - Created test_shader_constant_batching with performance benchmarks
  - **Persistent Shader Caching**:
    - Disk-based shader binary cache with compression
    - Memory cache with LRU eviction policy
    - Async disk operations for non-blocking I/O
    - Cache statistics and validation
    - Memory-mapped file support for ultra-fast access
    - Created test_shader_cache_simple for testing without OpenGL context

## [Unreleased] - 2025-08-07

### Added
- **Test Infrastructure Improvements** (2025-08-07):
  - Fixed resource_pool.cpp compilation errors blocking test suite execution
    - Fixed unique_ptr handling in EnhancedCommandBufferPool::acquire()
    - Fixed atomic value copying with proper .load() and .store() usage
    - Added move constructor to PoolMetrics for atomic members
    - Fixed std::find_if usage for unique_ptr vector operations
  - Added stub vtable definitions for COM interfaces to fix linking errors
  - Created comprehensive test_com_wrapper_threading test for thread safety validation
  - Updated run_dx8gl_tests.sh automated test runner script
  - Tests now properly run from build/Release/ directory with correct library paths

### Added (Earlier Today)
- **COM Wrapper Resource Management** (2025-08-07):
  - Implemented thread-safe wrapper classes for all DirectX 8 resource types
  - Added vtable definitions for IDirect3DSurface8, IDirect3DSwapChain8, IDirect3DVertexBuffer8, IDirect3DIndexBuffer8, IDirect3DCubeTexture8, IDirect3DVolumeTexture8, and IDirect3DVolume8
  - Factory functions for creating COM wrappers with proper reference counting
  - Mutex-based thread synchronization for multi-threaded usage
  - Updated all resource creation methods (CreateRenderTarget, CreateVertexBuffer, etc.) to use wrappers

- **Cube Texture Reset Tracking** (2025-08-07):
  - Fixed missing registration of cube textures for device reset tracking
  - Prevents resource leaks when device is reset
  - Ensures proper recreation of D3DPOOL_DEFAULT cube textures after reset

### Added (Previous)
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

- **DirectX 8 Render States**: Implemented missing render states for DX8Wrapper compatibility
  - D3DRS_RANGEFOGENABLE for range-based fog calculations
  - D3DRS_FOGVERTEXMODE for vertex fog mode selection
  - D3DRS_SPECULARMATERIALSOURCE for specular material source
  - D3DRS_COLORVERTEX for color vertex enable/disable
  - D3DRS_ZBIAS with OpenGL polygon offset mapping

- **Volume Texture Support**: Added stub implementation for volume textures
  - UpdateTexture now handles D3DRTYPE_VOLUMETEXTURE (returns D3DERR_NOTAVAILABLE)
  - CreateVolumeTexture returns D3DERR_NOTAVAILABLE until full implementation

- **Cube Texture PreLoad**: Implemented PreLoad functionality for cube textures
  - Binds texture and sets sampling parameters
  - Enables seamless cube map filtering for OpenGL 3.2+

- **Enhanced Display Mode Enumeration**: Improved compatibility with DX8Wrapper
  - Multiple refresh rates (60, 75, 85, 100, 120 Hz) for each format
  - Support for 16-bit (R5G6B5, X1R5G5B5) and 32-bit (X8R8G8B8, A8R8G8B8) formats
  - Better depth format checking in CheckDepthStencilMatch

- **COM Wrapper Refactoring Tasks**: Created comprehensive task series (CW01-CW15)
  - Base COM object implementation with proper IUnknown support
  - Individual wrappers for Surface, SwapChain, Volume, and VolumeTexture
  - Factory functions and unwrapping utilities
  - Thread safety and performance optimization tasks
  
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