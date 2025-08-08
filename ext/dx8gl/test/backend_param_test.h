#ifndef DX8GL_BACKEND_PARAM_TEST_H
#define DX8GL_BACKEND_PARAM_TEST_H

#include <gtest/gtest.h>
#include "dx8gl.h"
#include "d3d8_interface.h"
#include <cstdlib>
#include <string>
#include <vector>

namespace dx8gl {

// Backend type for parameterized testing
enum class TestBackendType {
    OSMesa = DX8GL_BACKEND_OSMESA,
    EGL = DX8GL_BACKEND_EGL,
    WebGPU = DX8GL_BACKEND_WEBGPU
};

// Helper to get backend name for test output
inline std::string GetBackendName(TestBackendType backend) {
    switch (backend) {
        case TestBackendType::OSMesa: return "OSMesa";
        case TestBackendType::EGL: return "EGL";
        case TestBackendType::WebGPU: return "WebGPU";
        default: return "Unknown";
    }
}

// Helper to check if backend is available
inline bool IsBackendAvailable(TestBackendType backend) {
    // Save current backend
    const char* old_backend = std::getenv("DX8GL_BACKEND");
    
    // Set test backend
    std::string backend_str;
    switch (backend) {
        case TestBackendType::OSMesa:
            backend_str = "osmesa";
            break;
        case TestBackendType::EGL:
            backend_str = "egl";
            break;
        case TestBackendType::WebGPU:
            backend_str = "webgpu";
            break;
    }
    
    setenv("DX8GL_BACKEND", backend_str.c_str(), 1);
    
    // Try to initialize
    dx8gl_config config = {};
    config.backend_type = static_cast<DX8GLBackendType>(backend);
    bool available = dx8gl_init(&config);
    
    if (available) {
        dx8gl_shutdown();
    }
    
    // Restore old backend
    if (old_backend) {
        setenv("DX8GL_BACKEND", old_backend, 1);
    } else {
        unsetenv("DX8GL_BACKEND");
    }
    
    return available;
}

// Base test fixture for backend parameterization
class BackendParamTest : public ::testing::TestWithParam<TestBackendType> {
protected:
    IDirect3D8* d3d8_ = nullptr;
    IDirect3DDevice8* device_ = nullptr;
    TestBackendType backend_;
    bool backend_available_ = false;
    
    void SetUp() override {
        backend_ = GetParam();
        
        // Check if backend is available
        backend_available_ = IsBackendAvailable(backend_);
        if (!backend_available_) {
            GTEST_SKIP() << "Backend " << GetBackendName(backend_) << " is not available";
            return;
        }
        
        // Set environment variable for backend
        std::string backend_str;
        switch (backend_) {
            case TestBackendType::OSMesa:
                backend_str = "osmesa";
                break;
            case TestBackendType::EGL:
                backend_str = "egl";
                break;
            case TestBackendType::WebGPU:
                backend_str = "webgpu";
                break;
        }
        
        setenv("DX8GL_BACKEND", backend_str.c_str(), 1);
        
        // Initialize dx8gl with the selected backend
        dx8gl_config config = {};
        config.backend_type = static_cast<DX8GLBackendType>(backend_);
        
        if (!dx8gl_init(&config)) {
            GTEST_SKIP() << "Failed to initialize " << GetBackendName(backend_) << " backend";
            return;
        }
        
        // Create D3D8 interface
        d3d8_ = Direct3DCreate8(D3D_SDK_VERSION);
        if (!d3d8_) {
            dx8gl_shutdown();
            GTEST_SKIP() << "Failed to create Direct3D8 interface for " << GetBackendName(backend_);
            return;
        }
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640;
        pp.BackBufferHeight = 480;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        
        HRESULT hr = d3d8_->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device_
        );
        
        if (FAILED(hr)) {
            d3d8_->Release();
            d3d8_ = nullptr;
            dx8gl_shutdown();
            GTEST_SKIP() << "Failed to create device for " << GetBackendName(backend_);
        }
    }
    
    void TearDown() override {
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        
        if (d3d8_) {
            d3d8_->Release();
            d3d8_ = nullptr;
        }
        
        if (backend_available_) {
            dx8gl_shutdown();
        }
        
        // Clear environment variable
        unsetenv("DX8GL_BACKEND");
    }
    
    // Helper to get backend name for logging
    std::string GetBackendName() const {
        return dx8gl::GetBackendName(backend_);
    }
    
    // Helper to skip if device not created
    void RequireDevice() {
        if (!device_) {
            GTEST_SKIP() << "Device not available for " << GetBackendName();
        }
    }
};

// Helper macro to instantiate parameterized tests for all backends
#define INSTANTIATE_BACKEND_PARAM_TEST(TestSuiteName) \
    INSTANTIATE_TEST_SUITE_P( \
        AllBackends, \
        TestSuiteName, \
        ::testing::Values( \
            TestBackendType::OSMesa, \
            TestBackendType::EGL, \
            TestBackendType::WebGPU \
        ), \
        [](const ::testing::TestParamInfo<TestBackendType>& info) { \
            return GetBackendName(info.param); \
        } \
    )

// Helper macro for backend-specific test expectations
#define EXPECT_BACKEND_SPECIFIC(backend, osmesa_val, egl_val, webgpu_val) \
    ((backend) == TestBackendType::OSMesa ? (osmesa_val) : \
     (backend) == TestBackendType::EGL ? (egl_val) : (webgpu_val))

// List of backends that support specific features
inline bool SupportsVolumeTextures(TestBackendType backend) {
    // Currently only OSMesa fully supports volume textures
    return backend == TestBackendType::OSMesa;
}

inline bool SupportsStencilBuffer(TestBackendType backend) {
    // All backends should support stencil
    return true;
}

inline bool SupportsShaderHotReload(TestBackendType backend) {
    // All backends should support shader hot reload
    return true;
}

inline bool SupportsAsyncOperations(TestBackendType backend) {
    // WebGPU is inherently async, others are synchronous
    return backend == TestBackendType::WebGPU;
}

} // namespace dx8gl

#endif // DX8GL_BACKEND_PARAM_TEST_H