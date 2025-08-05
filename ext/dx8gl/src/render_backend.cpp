#include "render_backend.h"
#include "osmesa_backend.h"
#include "egl_backend.h"
#include "webgpu_backend.h"
#include "logger.h"
#include <memory>

namespace dx8gl {

std::unique_ptr<DX8RenderBackend> create_render_backend(DX8BackendType type) {
    switch (type) {
        case DX8GL_BACKEND_OSMESA:
            DX8GL_INFO("Creating OSMesa rendering backend");
            return std::make_unique<DX8OSMesaBackend>();
            
        case DX8GL_BACKEND_EGL:
            DX8GL_INFO("Creating EGL rendering backend");
#ifdef DX8GL_HAS_EGL
            return std::make_unique<DX8EGLBackend>();
#else
            DX8GL_ERROR("EGL backend requested but not compiled in");
            return nullptr;
#endif
            
        case DX8GL_BACKEND_WEBGPU:
            DX8GL_INFO("Creating WebGPU rendering backend");
#ifdef DX8GL_HAS_WEBGPU
            return std::make_unique<DX8WebGPUBackend>();
#else
            DX8GL_ERROR("WebGPU backend requested but not compiled in");  
            return nullptr;
#endif
            
        case DX8GL_BACKEND_DEFAULT:
            DX8GL_INFO("Auto-selecting best available backend");
            // Fallback chain: Try WebGPU → EGL → OSMesa
#ifdef DX8GL_HAS_WEBGPU
            {
                auto backend = std::make_unique<DX8WebGPUBackend>();
                if (backend->initialize(800, 600)) {
                    DX8GL_INFO("Auto-selected WebGPU backend");
                    return backend;
                }
            }
#endif
#ifdef DX8GL_HAS_EGL
            {
                auto backend = std::make_unique<DX8EGLBackend>();
                if (backend->initialize(800, 600)) {
                    DX8GL_INFO("Auto-selected EGL backend");
                    return backend;
                }
            }
#endif
            DX8GL_INFO("Auto-selected OSMesa backend (fallback)");
            return std::make_unique<DX8OSMesaBackend>();
            
        default:
            DX8GL_ERROR("Unknown backend type: %d", type);
            return nullptr;
    }
}

} // namespace dx8gl