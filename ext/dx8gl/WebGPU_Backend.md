# WebGPU Backend for dx8gl

This document describes the WebGPU backend implementation for the dx8gl DirectX 8.1 to OpenGL translation library.

## Overview

The WebGPU backend provides modern GPU-accelerated rendering through the WebGPU API, enabling:

- **Cross-platform GPU acceleration**: Works on web browsers, desktop platforms, and mobile devices
- **Modern graphics pipeline**: Utilizes compute shaders and modern GPU features
- **Offscreen rendering**: Supports headless rendering for integration with GUI frameworks like LVGL
- **Async-first design**: Built around WebGPU's asynchronous API model

## Architecture

### Core Components

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   dx8gl API     │────│ DX8WebGPUBackend│────│   lib_webgpu    │
│ (DirectX 8.1)   │    │     (C++)       │    │   (WebGPU C)    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Application    │    │ Render Pipeline │    │   WebGPU API    │
│   (D3D8 calls)  │    │   Management    │    │(Browser/Native) │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Key Classes

- **`DX8WebGPUBackend`**: Main backend implementation inheriting from `DX8RenderBackend`
- **WebGPU Resource Management**: Handles adapters, devices, queues, textures, and buffers
- **Async Callback System**: Manages WebGPU's asynchronous operations
- **Framebuffer Readback**: Provides CPU access to GPU-rendered frames

## Features

### ✅ Implemented

- **Backend Infrastructure**: Complete `DX8RenderBackend` interface implementation
- **WebGPU Initialization**: Adapter and device creation with error handling
- **Offscreen Rendering**: Render texture creation and management
- **Framebuffer Access**: Texture-to-buffer readback for CPU access
- **Resource Management**: Proper cleanup and RAII patterns
- **Configuration Integration**: Environment variables and config struct support

### 🚧 Partial Implementation

- **Render Pipeline**: Basic structure in place, needs D3D8 to WebGPU translation
- **Async Synchronization**: Simple polling implementation, needs improvement
- **Error Handling**: Basic error reporting, could be more comprehensive

### ❌ Not Yet Implemented

- **Shader Translation**: DirectX 8 shaders to WGSL conversion
- **State Management**: D3D8 render states to WebGPU pipeline state mapping
- **Advanced Features**: Multi-render target, compute shader integration
- **Performance Optimization**: Command buffer pooling, resource caching

## Usage

### Configuration

Enable the WebGPU backend through multiple methods:

#### Environment Variable
```bash
export DX8GL_BACKEND=webgpu
```

#### Command Line Arguments
```bash
export DX8GL_ARGS="--backend=webgpu"
```

#### Configuration Struct
```cpp
dx8gl_config config = {};
config.backend_type = DX8GL_BACKEND_WEBGPU;
dx8gl_init(&config);
```

### Build Configuration

Enable WebGPU support in CMake:

```bash
cmake -DDX8GL_ENABLE_WEBGPU=ON .
make
```

### Basic Usage Example

```cpp
#include "dx8gl.h"
#include "d3d8_game.h"

int main() {
    // Configure for WebGPU
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_WEBGPU;
    
    // Initialize
    if (dx8gl_init(&config) != DX8GL_SUCCESS) {
        return -1;
    }
    
    // Use standard DirectX 8 API
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    IDirect3DDevice8* device = nullptr;
    
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 800;
    pp.BackBufferHeight = 600;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.Windowed = TRUE;
    
    d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
                      nullptr, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &pp, &device);
    
    // Render as normal
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Cleanup
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    return 0;
}
```

## Technical Details

### WebGPU Resource Lifecycle

```cpp
// Adapter Creation
WGpuRequestAdapterOptions options = {};
wgpu_request_adapter(&options, adapter_callback, this);

// Device Creation  
WGpuDeviceDescriptor descriptor = {};
wgpu_adapter_request_device(adapter_, &descriptor, device_callback, this);

// Render Texture
WGpuTextureDescriptor texture_desc = {};
texture_desc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;
render_texture_ = wgpu_device_create_texture(device_, &texture_desc);

// Readback Buffer
WGpuBufferDescriptor buffer_desc = {};
buffer_desc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ;
readback_buffer_ = wgpu_device_create_buffer(device_, &buffer_desc);
```

### Framebuffer Readback Process

1. **Render to Texture**: DirectX 8 commands render to WebGPU texture
2. **Copy to Buffer**: GPU texture data copied to staging buffer
3. **Map Buffer**: Staging buffer mapped for CPU access
4. **Data Access**: CPU reads RGBA data from mapped buffer
5. **Unmap Buffer**: Buffer unmapped and ready for next frame

### Error Handling

The backend uses a comprehensive error handling system:

```cpp
// Async callback error handling
static void device_callback(WGpuRequestDeviceStatus status, WGpuDevice device, 
                           const char* message, void* user_data) {
    DX8WebGPUBackend* backend = static_cast<DX8WebGPUBackend*>(user_data);
    
    if (status != WGPU_REQUEST_DEVICE_STATUS_SUCCESS) {
        DX8GL_ERROR("Failed to obtain WebGPU device: %s", message);
        strncpy(backend->error_buffer_, message, sizeof(backend->error_buffer_) - 1);
    }
    
    backend->device_ready_ = true;
}
```

## Platform Support

### Web Browsers
- **Chrome 113+**: Full WebGPU support
- **Firefox 121+**: Behind experimental flag
- **Safari 18+**: WebGPU implementation

### Native Platforms
- **Windows**: Via Dawn (Chrome's WebGPU implementation)
- **macOS**: Via Dawn with Metal backend
- **Linux**: Via Dawn with Vulkan backend

### Mobile Platforms
- **Android**: Limited support via Dawn
- **iOS**: WebGPU in Safari 18+

## Performance Considerations

### Async Operations
WebGPU is inherently asynchronous. The current implementation uses simple polling:

```cpp
// Wait for device creation
int timeout = 5000;
while (!device_ready_ && timeout-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

For production use, consider:
- Event-driven async handling
- Command buffer pooling
- Resource upload optimization

### Memory Management
- **CPU↔GPU transfers**: Minimize readback operations
- **Resource pooling**: Reuse buffers and textures
- **Command batching**: Group operations to reduce API overhead

## Integration with LVGL

The WebGPU backend seamlessly integrates with LVGL:

```cpp
// Get framebuffer for LVGL canvas
int width, height, format;
void* fb_data = backend->get_framebuffer(width, height, format);

// Update LVGL canvas
lv_canvas_set_buffer(canvas, fb_data, width, height, LV_IMG_CF_TRUE_COLOR);
```

## Future Enhancements

### Planned Features
1. **Compute Shader Support**: For advanced effects and GPGPU operations
2. **Multi-threaded Command Recording**: Parallel command buffer generation
3. **Ray Tracing Integration**: Modern rendering techniques
4. **Performance Profiling**: Built-in GPU timing and statistics

### Optimization Opportunities
1. **Shader Caching**: Persistent storage of compiled WGSL shaders
2. **Resource Streaming**: Efficient texture and buffer uploads
3. **Culling Integration**: GPU-driven frustum and occlusion culling
4. **Memory Compression**: Texture and buffer compression techniques

## Debugging and Troubleshooting

### Common Issues

**Backend Not Available**
```
WebGPU backend requested but not compiled in
```
Solution: Build with `-DDX8GL_ENABLE_WEBGPU=ON`

**Adapter Creation Failed**
```
Failed to obtain WebGPU adapter
```
Solution: Ensure WebGPU support in browser/driver

**Device Creation Timeout**
```
Failed to obtain WebGPU device
```
Solution: Check WebGPU feature compatibility

### Debug Build
Enable detailed logging:
```cpp
dx8gl_config config = {};
config.enable_logging = true;
config.enable_validation = true;
```

### Browser Developer Tools
For web builds:
1. Open Developer Console (F12)
2. Check for WebGPU errors
3. Monitor GPU usage in Performance tab

## API Reference

### DX8WebGPUBackend Public Methods

```cpp
class DX8WebGPUBackend : public DX8RenderBackend {
public:
    // DX8RenderBackend interface
    virtual bool initialize(int width, int height) override;
    virtual bool make_current() override;
    virtual void* get_framebuffer(int& width, int& height, int& format) override;
    virtual bool resize(int width, int height) override;
    virtual void shutdown() override;
    virtual DX8BackendType get_type() const override;
    virtual bool has_extension(const char* extension) const override;
    
    // WebGPU-specific accessors
    WGpuDevice get_device() const;
    WGpuQueue get_queue() const;
    WGpuTexture get_render_texture() const;
    WGpuCanvasContext get_canvas_context() const;
};
```

## Contributing

To contribute to the WebGPU backend:

1. **Test Coverage**: Add comprehensive test cases
2. **Performance**: Profile and optimize critical paths
3. **Documentation**: Update this document with changes
4. **Cross-platform**: Ensure compatibility across platforms

## License

The WebGPU backend is part of the dx8gl project and follows the same licensing terms.