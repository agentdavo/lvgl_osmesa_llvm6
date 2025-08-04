#ifndef DX8GL_RENDER_BACKEND_H
#define DX8GL_RENDER_BACKEND_H

#include <cstdint>
#include <memory>

namespace dx8gl {

/**
 * Backend type enumeration for different rendering implementations
 */
enum DX8BackendType {
    DX8_BACKEND_OSMESA,    // OSMesa software rendering backend
    DX8_BACKEND_EGL        // EGL surfaceless context backend
};

/**
 * Abstract interface for rendering backends in dx8gl
 * 
 * This interface allows dx8gl to support multiple rendering backends
 * such as OSMesa for software rendering and EGL for hardware acceleration.
 */
class DX8RenderBackend {
public:
    virtual ~DX8RenderBackend() = default;
    
    /**
     * Initialize the rendering backend
     * @param width Initial framebuffer width
     * @param height Initial framebuffer height
     * @return true if initialization successful, false otherwise
     */
    virtual bool initialize(int width, int height) = 0;
    
    /**
     * Make the rendering context current for this thread
     * @return true if successful, false otherwise
     */
    virtual bool make_current() = 0;
    
    /**
     * Get pointer to the framebuffer data
     * @param width Output parameter for framebuffer width
     * @param height Output parameter for framebuffer height
     * @param format Output parameter for pixel format (e.g., GL_RGBA)
     * @return Pointer to framebuffer data, or nullptr if not available
     */
    virtual void* get_framebuffer(int& width, int& height, int& format) = 0;
    
    /**
     * Resize the framebuffer
     * @param width New framebuffer width
     * @param height New framebuffer height
     * @return true if resize successful, false otherwise
     */
    virtual bool resize(int width, int height) = 0;
    
    /**
     * Shutdown the rendering backend and release resources
     */
    virtual void shutdown() = 0;
    
    /**
     * Get the backend type
     * @return The type of this backend
     */
    virtual DX8BackendType get_type() const = 0;
    
    /**
     * Check if the backend supports a specific OpenGL extension
     * @param extension Extension name to check
     * @return true if supported, false otherwise
     */
    virtual bool has_extension(const char* extension) const = 0;
};

/**
 * Factory function to create a rendering backend
 * @param type The type of backend to create
 * @return Unique pointer to the created backend, or nullptr on failure
 */
std::unique_ptr<DX8RenderBackend> create_render_backend(DX8BackendType type);

} // namespace dx8gl

#endif // DX8GL_RENDER_BACKEND_H