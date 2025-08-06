# WebGPU Backend Guide

## Overview

The dx8gl library now includes a WebGPU backend that provides modern GPU acceleration for web platforms and native applications. This guide covers building, configuring, and using the WebGPU backend with OffscreenCanvas support.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building with WebGPU](#building-with-webgpu)
3. [Threading Modes](#threading-modes)
4. [OffscreenCanvas Management](#offscreencanvas-management)
5. [API Usage](#api-usage)
6. [HTML/JS Integration](#htmljs-integration)
7. [Performance Considerations](#performance-considerations)
8. [Troubleshooting](#troubleshooting)

## Prerequisites

### For Web Builds (Emscripten)
- Emscripten SDK 3.1.50 or later
- Chrome 113+ or Edge 113+ (WebGPU support)
- Python 3.x for local development server

### For Native Builds
- Dawn (Google's WebGPU implementation) OR
- wgpu-native (Rust WebGPU implementation)
- CMake 3.16+
- C++20 compatible compiler

## Building with WebGPU

### Desktop Build with WebGPU

```bash
# Configure with WebGPU and Dawn
cmake -S . -B build \
    -DDX8GL_ENABLE_WEBGPU=ON \
    -DDX8GL_WEBGPU_USE_DAWN=ON

# Or with wgpu-native
cmake -S . -B build \
    -DDX8GL_ENABLE_WEBGPU=ON \
    -DDX8GL_WEBGPU_USE_WGPU=ON

# Build
cmake --build build -j
```

### Emscripten Build

```bash
# Source Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Use the dedicated WebGPU build script
./scripts/build_webgpu.sh

# Build with specific threading mode
./scripts/build_webgpu.sh -t pthreads
./scripts/build_webgpu.sh -t wasm-workers

# Build specific target
./scripts/build_webgpu.sh webgpu_complex_scene
```

### Using the Main Build Script

```bash
# Enable WebGPU backend
./scripts/compile.sh -w all

# With threading configuration
./scripts/compile.sh -w --webgpu-threading pthreads all

# With native WebGPU implementation
./scripts/compile.sh -w --webgpu-dawn all
```

## Threading Modes

The WebGPU backend supports multiple threading configurations for optimal performance:

### 1. None (Main Thread)
Default mode where all rendering occurs on the main thread.

```cmake
-DDX8GL_WEBGPU_THREADING=none
```

**Pros:**
- Simplest setup
- No special browser requirements
- Works everywhere WebGPU is supported

**Cons:**
- Can block UI interactions during heavy rendering
- Limited parallelism

### 2. Pthreads
Uses Emscripten's pthread support for true multi-threading.

```cmake
-DDX8GL_WEBGPU_THREADING=pthreads
```

**Requirements:**
- COOP/COEP headers on web server:
  ```
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp
  ```
- SharedArrayBuffer support in browser

**Pros:**
- True parallelism
- Better performance for CPU-bound tasks
- Shared memory between threads

**Cons:**
- Requires special headers
- Not all hosting providers support COOP/COEP

### 3. Wasm Workers
Uses WebAssembly Workers for parallel execution without SharedArrayBuffer.

```cmake
-DDX8GL_WEBGPU_THREADING=wasm-workers
```

**Pros:**
- Works without special headers
- Good parallelism
- Broader browser compatibility

**Cons:**
- No shared memory (message passing only)
- Slightly higher overhead than pthreads

## OffscreenCanvas Management

The WebGPU backend now properly manages OffscreenCanvas lifecycle:

### Automatic Canvas Creation

```cpp
// The backend automatically creates an OffscreenCanvas if needed
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_WEBGPU;
config.width = 1024;
config.height = 768;
dx8gl_init(&config);
```

### Transfer from HTML Canvas

```cpp
// Transfer control from existing HTML canvas
DX8WebGPUBackend* backend = get_webgpu_backend();
backend->transfer_canvas_control("#myCanvas");
```

### Custom Canvas ID

```cpp
// Use specific canvas ID for multiple instances
backend->set_canvas_id(42);
backend->initialize(width, height);
```

### Canvas Lifecycle

The backend handles:
- Canvas creation when not present
- Validation of canvas state
- Proper cleanup on shutdown
- Resize operations

## API Usage

### Basic Initialization

```cpp
#include "dx8gl.h"

int main() {
    // Configure for WebGPU
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_WEBGPU;
    config.width = 1024;
    config.height = 768;
    
    // Initialize
    if (!dx8gl_init(&config)) {
        printf("Failed to initialize WebGPU backend\n");
        return 1;
    }
    
    // Create DirectX 8 interface as usual
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    // ... rest of DirectX 8 code
}
```

### Async Framebuffer Readback

```cpp
// Request framebuffer asynchronously (recommended)
void on_framebuffer_ready(void* data, int width, int height, 
                          int format, void* user_data) {
    if (data) {
        // Process framebuffer
        save_screenshot(data, width, height);
    }
}

// In render loop
backend->request_framebuffer_async(on_framebuffer_ready, nullptr);
```

### Runtime Backend Selection

```cpp
// Environment variable
export DX8GL_BACKEND=webgpu

// Or in code
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_DEFAULT; // Auto-select

// Fallback chain: WebGPU → EGL → OSMesa
```

## HTML/JS Integration

### Basic HTML Setup

```html
<!DOCTYPE html>
<html>
<head>
    <title>dx8gl WebGPU Demo</title>
</head>
<body>
    <canvas id="canvas" width="1024" height="768"></canvas>
    <script src="dx8gl_app.js"></script>
</body>
</html>
```

### Worker-based Rendering

```javascript
// Create OffscreenCanvas and transfer to worker
const canvas = document.getElementById('canvas');
const offscreen = canvas.transferControlToOffscreen();

const worker = new Worker('webgpu_worker.js');
worker.postMessage({
    type: 'init',
    canvas: offscreen,
    width: 1024,
    height: 768
}, [offscreen]);
```

### Using the Provided Harness

The project includes a complete HTML/JS harness:

```bash
# Build with harness
./scripts/build_webgpu.sh

# Serve with appropriate headers
cd build-webgpu
python3 serve_webgpu.py

# Open in browser
# http://localhost:8080/webgpu_harness.html
```

## Performance Considerations

### 1. Async Operations
Always prefer async operations to avoid blocking:

```cpp
// Good: Async readback
backend->request_framebuffer_async(callback, data);

// Avoid: Synchronous readback in worker
void* fb = backend->get_framebuffer(w, h, fmt); // May stall
```

### 2. Buffer Management
WebGPU uses explicit buffer management:

```cpp
// Efficient: Reuse buffers
static WGpuBuffer vertex_buffer = nullptr;
if (!vertex_buffer) {
    vertex_buffer = create_vertex_buffer();
}
update_buffer(vertex_buffer, data);

// Inefficient: Creating buffers each frame
WGpuBuffer buffer = create_vertex_buffer(); // Avoid
```

### 3. Command Buffer Batching
Batch operations for better performance:

```cpp
// Record multiple operations
command_buffer->begin();
command_buffer->set_pipeline(pipeline1);
command_buffer->draw(vertices1);
command_buffer->set_pipeline(pipeline2);
command_buffer->draw(vertices2);
command_buffer->end();
command_buffer->submit(); // Single submission
```

## Troubleshooting

### Common Issues

#### 1. WebGPU Not Available
**Error:** "WebGPU is not supported in this browser"

**Solution:** 
- Use Chrome 113+ or Edge 113+
- Enable WebGPU flag: `chrome://flags/#enable-unsafe-webgpu`
- Check navigator.gpu availability

#### 2. OffscreenCanvas Creation Failed
**Error:** "Failed to create OffscreenCanvas"

**Solution:**
- Ensure canvas element exists in DOM
- Check browser supports OffscreenCanvas
- Verify not trying to transfer twice

#### 3. Threading Mode Issues
**Error:** "SharedArrayBuffer is not defined"

**Solution for pthreads:**
- Add COOP/COEP headers to server
- Use provided `serve_webgpu.py` script
- Or switch to wasm-workers mode

#### 4. Framebuffer Readback Timeout
**Error:** "Framebuffer readback timed out"

**Solution:**
- Use async readback instead
- Check WebGPU device not lost
- Verify render pipeline is valid

### Debug Tips

1. **Enable Verbose Logging:**
   ```cpp
   dx8gl_set_log_level(DX8GL_LOG_VERBOSE);
   ```

2. **Check WebGPU Errors:**
   ```javascript
   device.pushErrorScope('validation');
   // ... operations ...
   device.popErrorScope().then(error => {
       if (error) console.error(error.message);
   });
   ```

3. **Monitor Performance:**
   ```cpp
   // In render loop
   auto stats = backend->get_performance_stats();
   printf("FPS: %.1f, Draw Calls: %d\n", 
          stats.fps, stats.draw_calls);
   ```

## Best Practices

1. **Always Handle Async Operations**
   - Use callbacks for framebuffer readback
   - Check operation completion status
   - Implement proper error handling

2. **Optimize for Web Deployment**
   - Minimize WASM size with -O3 optimization
   - Use MINIMAL_RUNTIME when possible
   - Enable WASM SIMD if supported

3. **Test Multiple Configurations**
   - Test all threading modes
   - Verify on different browsers
   - Check performance metrics

4. **Resource Management**
   - Properly release WebGPU resources
   - Avoid resource leaks in workers
   - Clean up on context loss

## Example Projects

### 1. Complex Scene Demo
Demonstrates multiple rotating objects with dynamic updates:
```bash
./scripts/build_webgpu.sh webgpu_complex_scene
```

### 2. WebGPU Backend Tests
Comprehensive test suite for backend functionality:
```bash
cd ext/dx8gl
cmake -B build -DDX8GL_ENABLE_WEBGPU=ON
cmake --build build
ctest --test-dir build
```

### 3. Integration Sample
Full application using WebGPU backend:
```cpp
// See ext/dx8gl/samples/webgpu_complex_scene.cpp
```

## Further Resources

- [WebGPU Specification](https://www.w3.org/TR/webgpu/)
- [Emscripten WebGPU Docs](https://emscripten.org/docs/api_reference/html5.h.html#webgpu)
- [Dawn Project](https://dawn.googlesource.com/dawn)
- [wgpu-native](https://github.com/gfx-rs/wgpu-native)
- [OffscreenCanvas MDN](https://developer.mozilla.org/en-US/docs/Web/API/OffscreenCanvas)

## Migration from Other Backends

### From OSMesa
```cpp
// Before (OSMesa)
config.backend_type = DX8GL_BACKEND_OSMESA;

// After (WebGPU)
config.backend_type = DX8GL_BACKEND_WEBGPU;
// Rest of code remains the same!
```

### From EGL
```cpp
// Before (EGL)
config.backend_type = DX8GL_BACKEND_EGL;
void* fb = backend->get_framebuffer(w, h, fmt);

// After (WebGPU with async)
config.backend_type = DX8GL_BACKEND_WEBGPU;
backend->request_framebuffer_async(callback, data);
```

The DirectX 8 API remains unchanged - only the backend configuration differs!