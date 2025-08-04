#include "render_backend.h"
#include "osmesa_backend.h"
#include "egl_backend.h"
#include "logger.h"
#include <memory>

namespace dx8gl {

std::unique_ptr<DX8RenderBackend> create_render_backend(DX8BackendType type) {
    switch (type) {
        case DX8_BACKEND_OSMESA:
            DX8GL_INFO("Creating OSMesa rendering backend");
            return std::make_unique<DX8OSMesaBackend>();
            
        case DX8_BACKEND_EGL:
            DX8GL_INFO("Creating EGL rendering backend");
#ifdef DX8GL_HAS_EGL
            return std::make_unique<DX8EGLBackend>();
#else
            DX8GL_ERROR("EGL backend requested but not compiled in");
            return nullptr;
#endif
            
        default:
            DX8GL_ERROR("Unknown backend type: %d", type);
            return nullptr;
    }
}

} // namespace dx8gl