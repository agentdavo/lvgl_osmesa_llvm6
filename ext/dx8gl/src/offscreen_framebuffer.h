#ifndef DX8GL_OFFSCREEN_FRAMEBUFFER_H
#define DX8GL_OFFSCREEN_FRAMEBUFFER_H

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace dx8gl {

/**
 * Pixel format enumeration for framebuffers
 */
enum class PixelFormat {
    RGBA8,      // 32-bit RGBA (8 bits per channel)
    RGB8,       // 24-bit RGB (8 bits per channel)
    RGB565,     // 16-bit RGB (5-6-5 bits)
    BGRA8,      // 32-bit BGRA (8 bits per channel)
    BGR8,       // 24-bit BGR (8 bits per channel)
    FLOAT_RGBA  // 128-bit RGBA (32-bit float per channel)
};

/**
 * Helper class for managing offscreen framebuffers across different backends
 * 
 * This class provides a unified interface for framebuffer management,
 * handling CPU/GPU memory allocation, format conversions, and readback
 * operations for OSMesa, EGL, and WebGPU backends.
 */
class OffscreenFramebuffer {
public:
    /**
     * Constructor
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param format Pixel format for the framebuffer
     * @param cpu_accessible Whether the framebuffer should be CPU-accessible
     * @param sample_count Number of samples for multisampling (1 = no MSAA)
     */
    OffscreenFramebuffer(int width, int height, PixelFormat format, 
                        bool cpu_accessible = true, int sample_count = 1);
    
    /**
     * Destructor
     */
    ~OffscreenFramebuffer();
    
    // Disable copy construction and assignment
    OffscreenFramebuffer(const OffscreenFramebuffer&) = delete;
    OffscreenFramebuffer& operator=(const OffscreenFramebuffer&) = delete;
    
    // Enable move construction and assignment
    OffscreenFramebuffer(OffscreenFramebuffer&& other) noexcept;
    OffscreenFramebuffer& operator=(OffscreenFramebuffer&& other) noexcept;
    
    /**
     * Resize the framebuffer
     * @param width New width in pixels
     * @param height New height in pixels
     * @return true if resize successful, false otherwise
     */
    bool resize(int width, int height);
    
    /**
     * Get framebuffer dimensions
     */
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    
    /**
     * Get pixel format
     */
    PixelFormat get_format() const { return format_; }
    
    /**
     * Get bytes per pixel for the current format
     */
    int get_bytes_per_pixel() const;
    
    /**
     * Get total size in bytes
     */
    size_t get_size_bytes() const;
    
    /**
     * Get OpenGL format constant for the pixel format
     */
    uint32_t get_gl_format() const;
    
    /**
     * Get OpenGL type constant for the pixel format
     */
    uint32_t get_gl_type() const;
    
    /**
     * Get raw framebuffer data (CPU-accessible)
     * @return Pointer to framebuffer data, or nullptr if not CPU-accessible
     */
    void* get_data() { return cpu_buffer_.data(); }
    const void* get_data() const { return cpu_buffer_.data(); }
    
    /**
     * Get typed framebuffer data
     */
    template<typename T>
    T* get_data_as() { return reinterpret_cast<T*>(cpu_buffer_.data()); }
    
    template<typename T>
    const T* get_data_as() const { return reinterpret_cast<const T*>(cpu_buffer_.data()); }
    
    /**
     * Clear the framebuffer to a specific color
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha component (0.0-1.0)
     */
    void clear(float r, float g, float b, float a = 1.0f);
    
    /**
     * Convert framebuffer to a different format
     * @param target_format Desired pixel format
     * @param output Output buffer (must be pre-allocated)
     * @return true if conversion successful, false otherwise
     */
    bool convert_to(PixelFormat target_format, void* output) const;
    
    /**
     * Convert framebuffer to a different format (allocating new buffer)
     * @param target_format Desired pixel format
     * @return New framebuffer with converted data, or nullptr on failure
     */
    std::unique_ptr<OffscreenFramebuffer> convert_to(PixelFormat target_format) const;
    
    /**
     * Copy framebuffer data from GPU to CPU buffer
     * @param gpu_read_func Function to read GPU data (backend-specific)
     * @return true if successful, false otherwise
     */
    bool read_from_gpu(std::function<bool(void* dest, size_t size)> gpu_read_func);
    
    /**
     * Write framebuffer data from CPU to GPU buffer
     * @param gpu_write_func Function to write GPU data (backend-specific)
     * @return true if successful, false otherwise
     */
    bool write_to_gpu(std::function<bool(const void* src, size_t size)> gpu_write_func);
    
    /**
     * Set GPU resource handle (backend-specific)
     * @param handle GPU resource handle (texture ID, buffer handle, etc.)
     */
    void set_gpu_handle(uintptr_t handle) { gpu_handle_ = handle; }
    
    /**
     * Get GPU resource handle
     */
    uintptr_t get_gpu_handle() const { return gpu_handle_; }
    
    /**
     * Mark the CPU buffer as dirty (needs GPU readback)
     */
    void mark_cpu_dirty() { cpu_dirty_ = true; }
    
    /**
     * Mark the GPU buffer as dirty (needs CPU upload)
     */
    void mark_gpu_dirty() { gpu_dirty_ = true; }
    
    /**
     * Check if CPU buffer needs update from GPU
     */
    bool is_cpu_dirty() const { return cpu_dirty_; }
    
    /**
     * Check if GPU buffer needs update from CPU
     */
    bool is_gpu_dirty() const { return gpu_dirty_; }
    
    /**
     * Get the sample count for multisampling
     */
    int get_sample_count() const { return sample_count_; }
    
    /**
     * Check if this framebuffer is multisampled
     */
    bool is_multisampled() const { return sample_count_ > 1; }
    
    /**
     * Set multisample resolve framebuffer (for MSAA rendering)
     * @param handle GPU handle to the resolve framebuffer
     */
    void set_resolve_handle(uintptr_t handle) { resolve_handle_ = handle; }
    
    /**
     * Get multisample resolve framebuffer handle
     */
    uintptr_t get_resolve_handle() const { return resolve_handle_; }

private:
    int width_;
    int height_;
    PixelFormat format_;
    bool cpu_accessible_;
    int sample_count_;  // Number of samples for MSAA
    
    // CPU-side buffer
    std::vector<uint8_t> cpu_buffer_;
    
    // GPU resource handle (backend-specific)
    uintptr_t gpu_handle_;
    
    // Resolve framebuffer handle for MSAA (backend-specific)
    uintptr_t resolve_handle_;
    
    // Dirty flags for synchronization
    bool cpu_dirty_;
    bool gpu_dirty_;
    
    // Helper methods for format conversion
    void convert_rgba8_to_rgb565(const void* src, void* dst) const;
    void convert_rgba8_to_rgb8(const void* src, void* dst) const;
    void convert_rgba8_to_bgra8(const void* src, void* dst) const;
    void convert_rgb8_to_rgba8(const void* src, void* dst) const;
    void convert_rgb565_to_rgba8(const void* src, void* dst) const;
    void convert_float_rgba_to_rgba8(const void* src, void* dst) const;
    void convert_rgba8_to_float_rgba(const void* src, void* dst) const;
};

} // namespace dx8gl

#endif // DX8GL_OFFSCREEN_FRAMEBUFFER_H