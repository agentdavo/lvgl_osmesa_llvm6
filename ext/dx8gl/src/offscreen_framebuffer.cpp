#include "offscreen_framebuffer.h"
#include "logger.h"
#include <cstring>
#include <algorithm>
#include <cmath>

// Include OpenGL headers for format constants
#include "gl3_headers.h"

namespace dx8gl {

OffscreenFramebuffer::OffscreenFramebuffer(int width, int height, PixelFormat format, bool cpu_accessible)
    : width_(width)
    , height_(height)
    , format_(format)
    , cpu_accessible_(cpu_accessible)
    , gpu_handle_(0)
    , cpu_dirty_(false)
    , gpu_dirty_(false) {
    
    if (cpu_accessible) {
        cpu_buffer_.resize(get_size_bytes());
    }
}

OffscreenFramebuffer::~OffscreenFramebuffer() {
    // Cleanup is handled by vector destructor
}

OffscreenFramebuffer::OffscreenFramebuffer(OffscreenFramebuffer&& other) noexcept
    : width_(other.width_)
    , height_(other.height_)
    , format_(other.format_)
    , cpu_accessible_(other.cpu_accessible_)
    , cpu_buffer_(std::move(other.cpu_buffer_))
    , gpu_handle_(other.gpu_handle_)
    , cpu_dirty_(other.cpu_dirty_)
    , gpu_dirty_(other.gpu_dirty_) {
    
    // Reset moved-from object
    other.width_ = 0;
    other.height_ = 0;
    other.gpu_handle_ = 0;
}

OffscreenFramebuffer& OffscreenFramebuffer::operator=(OffscreenFramebuffer&& other) noexcept {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        format_ = other.format_;
        cpu_accessible_ = other.cpu_accessible_;
        cpu_buffer_ = std::move(other.cpu_buffer_);
        gpu_handle_ = other.gpu_handle_;
        cpu_dirty_ = other.cpu_dirty_;
        gpu_dirty_ = other.gpu_dirty_;
        
        // Reset moved-from object
        other.width_ = 0;
        other.height_ = 0;
        other.gpu_handle_ = 0;
    }
    return *this;
}

bool OffscreenFramebuffer::resize(int width, int height) {
    if (width == width_ && height == height_) {
        return true; // No change needed
    }
    
    width_ = width;
    height_ = height;
    
    if (cpu_accessible_) {
        cpu_buffer_.resize(get_size_bytes());
        cpu_dirty_ = true; // Need to re-read from GPU
    }
    
    gpu_dirty_ = true; // GPU resources need to be recreated
    
    return true;
}

int OffscreenFramebuffer::get_bytes_per_pixel() const {
    switch (format_) {
        case PixelFormat::RGBA8:
        case PixelFormat::BGRA8:
            return 4;
        case PixelFormat::RGB8:
        case PixelFormat::BGR8:
            return 3;
        case PixelFormat::RGB565:
            return 2;
        case PixelFormat::FLOAT_RGBA:
            return 16; // 4 floats * 4 bytes
        default:
            return 4; // Default to RGBA8
    }
}

size_t OffscreenFramebuffer::get_size_bytes() const {
    return static_cast<size_t>(width_) * height_ * get_bytes_per_pixel();
}

uint32_t OffscreenFramebuffer::get_gl_format() const {
    switch (format_) {
        case PixelFormat::RGBA8:
            return GL_RGBA;
        case PixelFormat::RGB8:
            return GL_RGB;
        case PixelFormat::RGB565:
            return GL_RGB;
        case PixelFormat::BGRA8:
            return GL_BGRA;
        case PixelFormat::BGR8:
            return GL_BGR;
        case PixelFormat::FLOAT_RGBA:
            return GL_RGBA;
        default:
            return GL_RGBA;
    }
}

uint32_t OffscreenFramebuffer::get_gl_type() const {
    switch (format_) {
        case PixelFormat::RGBA8:
        case PixelFormat::RGB8:
        case PixelFormat::BGRA8:
        case PixelFormat::BGR8:
            return GL_UNSIGNED_BYTE;
        case PixelFormat::RGB565:
            return GL_UNSIGNED_SHORT_5_6_5;
        case PixelFormat::FLOAT_RGBA:
            return GL_FLOAT;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

void OffscreenFramebuffer::clear(float r, float g, float b, float a) {
    if (!cpu_accessible_ || cpu_buffer_.empty()) {
        return;
    }
    
    switch (format_) {
        case PixelFormat::RGBA8: {
            uint8_t r8 = static_cast<uint8_t>(r * 255.0f);
            uint8_t g8 = static_cast<uint8_t>(g * 255.0f);
            uint8_t b8 = static_cast<uint8_t>(b * 255.0f);
            uint8_t a8 = static_cast<uint8_t>(a * 255.0f);
            
            uint32_t* pixels = reinterpret_cast<uint32_t*>(cpu_buffer_.data());
            uint32_t color = (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
            std::fill(pixels, pixels + (width_ * height_), color);
            break;
        }
        case PixelFormat::RGB8: {
            uint8_t r8 = static_cast<uint8_t>(r * 255.0f);
            uint8_t g8 = static_cast<uint8_t>(g * 255.0f);
            uint8_t b8 = static_cast<uint8_t>(b * 255.0f);
            
            uint8_t* pixels = cpu_buffer_.data();
            for (int i = 0; i < width_ * height_; i++) {
                pixels[i * 3 + 0] = r8;
                pixels[i * 3 + 1] = g8;
                pixels[i * 3 + 2] = b8;
            }
            break;
        }
        case PixelFormat::RGB565: {
            uint16_t r5 = static_cast<uint16_t>(r * 31.0f);
            uint16_t g6 = static_cast<uint16_t>(g * 63.0f);
            uint16_t b5 = static_cast<uint16_t>(b * 31.0f);
            
            uint16_t* pixels = reinterpret_cast<uint16_t*>(cpu_buffer_.data());
            uint16_t color = (r5 << 11) | (g6 << 5) | b5;
            std::fill(pixels, pixels + (width_ * height_), color);
            break;
        }
        case PixelFormat::FLOAT_RGBA: {
            float* pixels = reinterpret_cast<float*>(cpu_buffer_.data());
            for (int i = 0; i < width_ * height_; i++) {
                pixels[i * 4 + 0] = r;
                pixels[i * 4 + 1] = g;
                pixels[i * 4 + 2] = b;
                pixels[i * 4 + 3] = a;
            }
            break;
        }
        // Add other formats as needed
        default:
            break;
    }
    
    gpu_dirty_ = true; // CPU buffer has been modified
}

bool OffscreenFramebuffer::convert_to(PixelFormat target_format, void* output) const {
    if (!cpu_accessible_ || cpu_buffer_.empty() || !output) {
        return false;
    }
    
    if (format_ == target_format) {
        // No conversion needed, just copy
        std::memcpy(output, cpu_buffer_.data(), get_size_bytes());
        return true;
    }
    
    // Perform format conversion
    const void* src = cpu_buffer_.data();
    
    // RGBA8 as intermediate format for complex conversions
    std::vector<uint8_t> temp_buffer;
    if (format_ != PixelFormat::RGBA8 && target_format != PixelFormat::RGBA8) {
        // Convert to RGBA8 first
        temp_buffer.resize(width_ * height_ * 4);
        
        switch (format_) {
            case PixelFormat::RGB8:
                convert_rgb8_to_rgba8(src, temp_buffer.data());
                break;
            case PixelFormat::RGB565:
                convert_rgb565_to_rgba8(src, temp_buffer.data());
                break;
            case PixelFormat::FLOAT_RGBA:
                convert_float_rgba_to_rgba8(src, temp_buffer.data());
                break;
            default:
                return false;
        }
        
        src = temp_buffer.data();
    }
    
    // Convert from source (or RGBA8) to target format
    switch (target_format) {
        case PixelFormat::RGBA8:
            if (format_ == PixelFormat::RGB8) {
                convert_rgb8_to_rgba8(src, output);
            } else if (format_ == PixelFormat::RGB565) {
                convert_rgb565_to_rgba8(src, output);
            } else if (format_ == PixelFormat::FLOAT_RGBA) {
                convert_float_rgba_to_rgba8(src, output);
            } else if (format_ == PixelFormat::BGRA8) {
                convert_rgba8_to_bgra8(src, output); // BGRA->RGBA is same operation
            }
            break;
            
        case PixelFormat::RGB8:
            convert_rgba8_to_rgb8(src, output);
            break;
            
        case PixelFormat::RGB565:
            convert_rgba8_to_rgb565(src, output);
            break;
            
        case PixelFormat::BGRA8:
            convert_rgba8_to_bgra8(src, output);
            break;
            
        case PixelFormat::FLOAT_RGBA:
            convert_rgba8_to_float_rgba(src, output);
            break;
            
        default:
            return false;
    }
    
    return true;
}

std::unique_ptr<OffscreenFramebuffer> OffscreenFramebuffer::convert_to(PixelFormat target_format) const {
    auto result = std::make_unique<OffscreenFramebuffer>(width_, height_, target_format, true);
    
    if (convert_to(target_format, result->get_data())) {
        return result;
    }
    
    return nullptr;
}

bool OffscreenFramebuffer::read_from_gpu(std::function<bool(void* dest, size_t size)> gpu_read_func) {
    if (!cpu_accessible_ || !gpu_read_func) {
        return false;
    }
    
    if (cpu_buffer_.empty()) {
        cpu_buffer_.resize(get_size_bytes());
    }
    
    if (gpu_read_func(cpu_buffer_.data(), get_size_bytes())) {
        cpu_dirty_ = false;
        return true;
    }
    
    return false;
}

bool OffscreenFramebuffer::write_to_gpu(std::function<bool(const void* src, size_t size)> gpu_write_func) {
    if (!cpu_accessible_ || cpu_buffer_.empty() || !gpu_write_func) {
        return false;
    }
    
    if (gpu_write_func(cpu_buffer_.data(), get_size_bytes())) {
        gpu_dirty_ = false;
        return true;
    }
    
    return false;
}

// Format conversion implementations
void OffscreenFramebuffer::convert_rgba8_to_rgb565(const void* src, void* dst) const {
    const uint8_t* src_pixels = static_cast<const uint8_t*>(src);
    uint16_t* dst_pixels = static_cast<uint16_t*>(dst);
    
    for (int i = 0; i < width_ * height_; i++) {
        uint8_t r = src_pixels[i * 4 + 0];
        uint8_t g = src_pixels[i * 4 + 1];
        uint8_t b = src_pixels[i * 4 + 2];
        
        // Convert 8-bit to 5/6/5-bit
        uint16_t r5 = (r >> 3) & 0x1F;
        uint16_t g6 = (g >> 2) & 0x3F;
        uint16_t b5 = (b >> 3) & 0x1F;
        
        dst_pixels[i] = (r5 << 11) | (g6 << 5) | b5;
    }
}

void OffscreenFramebuffer::convert_rgba8_to_rgb8(const void* src, void* dst) const {
    const uint8_t* src_pixels = static_cast<const uint8_t*>(src);
    uint8_t* dst_pixels = static_cast<uint8_t*>(dst);
    
    for (int i = 0; i < width_ * height_; i++) {
        dst_pixels[i * 3 + 0] = src_pixels[i * 4 + 0]; // R
        dst_pixels[i * 3 + 1] = src_pixels[i * 4 + 1]; // G
        dst_pixels[i * 3 + 2] = src_pixels[i * 4 + 2]; // B
        // Skip alpha
    }
}

void OffscreenFramebuffer::convert_rgba8_to_bgra8(const void* src, void* dst) const {
    const uint8_t* src_pixels = static_cast<const uint8_t*>(src);
    uint8_t* dst_pixels = static_cast<uint8_t*>(dst);
    
    for (int i = 0; i < width_ * height_; i++) {
        dst_pixels[i * 4 + 0] = src_pixels[i * 4 + 2]; // B
        dst_pixels[i * 4 + 1] = src_pixels[i * 4 + 1]; // G
        dst_pixels[i * 4 + 2] = src_pixels[i * 4 + 0]; // R
        dst_pixels[i * 4 + 3] = src_pixels[i * 4 + 3]; // A
    }
}

void OffscreenFramebuffer::convert_rgb8_to_rgba8(const void* src, void* dst) const {
    const uint8_t* src_pixels = static_cast<const uint8_t*>(src);
    uint8_t* dst_pixels = static_cast<uint8_t*>(dst);
    
    for (int i = 0; i < width_ * height_; i++) {
        dst_pixels[i * 4 + 0] = src_pixels[i * 3 + 0]; // R
        dst_pixels[i * 4 + 1] = src_pixels[i * 3 + 1]; // G
        dst_pixels[i * 4 + 2] = src_pixels[i * 3 + 2]; // B
        dst_pixels[i * 4 + 3] = 255; // A = opaque
    }
}

void OffscreenFramebuffer::convert_rgb565_to_rgba8(const void* src, void* dst) const {
    const uint16_t* src_pixels = static_cast<const uint16_t*>(src);
    uint8_t* dst_pixels = static_cast<uint8_t*>(dst);
    
    for (int i = 0; i < width_ * height_; i++) {
        uint16_t pixel = src_pixels[i];
        
        // Extract 5/6/5-bit components
        uint8_t r5 = (pixel >> 11) & 0x1F;
        uint8_t g6 = (pixel >> 5) & 0x3F;
        uint8_t b5 = pixel & 0x1F;
        
        // Convert to 8-bit (with proper scaling)
        dst_pixels[i * 4 + 0] = (r5 << 3) | (r5 >> 2); // R
        dst_pixels[i * 4 + 1] = (g6 << 2) | (g6 >> 4); // G
        dst_pixels[i * 4 + 2] = (b5 << 3) | (b5 >> 2); // B
        dst_pixels[i * 4 + 3] = 255; // A = opaque
    }
}

void OffscreenFramebuffer::convert_float_rgba_to_rgba8(const void* src, void* dst) const {
    const float* src_pixels = static_cast<const float*>(src);
    uint8_t* dst_pixels = static_cast<uint8_t*>(dst);
    
    for (int i = 0; i < width_ * height_ * 4; i++) {
        float value = src_pixels[i];
        // Clamp to [0, 1] range
        value = std::max(0.0f, std::min(1.0f, value));
        dst_pixels[i] = static_cast<uint8_t>(value * 255.0f);
    }
}

void OffscreenFramebuffer::convert_rgba8_to_float_rgba(const void* src, void* dst) const {
    const uint8_t* src_pixels = static_cast<const uint8_t*>(src);
    float* dst_pixels = static_cast<float*>(dst);
    
    for (int i = 0; i < width_ * height_ * 4; i++) {
        dst_pixels[i] = src_pixels[i] / 255.0f;
    }
}

} // namespace dx8gl