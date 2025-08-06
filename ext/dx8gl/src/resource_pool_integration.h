#ifndef DX8GL_RESOURCE_POOL_INTEGRATION_H
#define DX8GL_RESOURCE_POOL_INTEGRATION_H

#include "resource_pool.h"
#include "d3d8_device.h"

namespace dx8gl {

/**
 * Integration helper for resource pools with D3D8 device
 */
class ResourcePoolIntegration {
public:
    // Initialize pools for a device
    static void initialize_for_device(IDirect3DDevice8* device, const PoolConfiguration& config = {});
    
    // Shutdown pools for a device
    static void shutdown_for_device(IDirect3DDevice8* device);
    
    // Frame management hooks
    static void on_begin_scene(IDirect3DDevice8* device);
    static void on_end_scene(IDirect3DDevice8* device);
    static void on_present(IDirect3DDevice8* device);
    
    // Get command buffer with automatic pooling
    static std::unique_ptr<CommandBuffer> acquire_command_buffer();
    static void release_command_buffer(std::unique_ptr<CommandBuffer> buffer);
    
    // Resource caching helpers for textures
    static bool try_reuse_texture(
        uint32_t width,
        uint32_t height,
        uint32_t levels,
        DWORD usage,
        D3DFORMAT format,
        D3DPOOL pool,
        IDirect3DTexture8** out_texture
    );
    
    static void cache_released_texture(IDirect3DTexture8* texture);
    
    // Resource caching helpers for buffers
    static bool try_reuse_vertex_buffer(
        uint32_t length,
        DWORD usage,
        DWORD fvf,
        D3DPOOL pool,
        IDirect3DVertexBuffer8** out_buffer
    );
    
    static void cache_released_vertex_buffer(IDirect3DVertexBuffer8* buffer);
    
    static bool try_reuse_index_buffer(
        uint32_t length,
        DWORD usage,
        D3DFORMAT format,
        D3DPOOL pool,
        IDirect3DIndexBuffer8** out_buffer
    );
    
    static void cache_released_index_buffer(IDirect3DIndexBuffer8* buffer);
    
    // Configuration management
    static void set_pool_configuration(const PoolConfiguration& config);
    static const PoolConfiguration& get_pool_configuration();
    
    // Performance monitoring
    static void log_pool_statistics();
    static PoolMetrics get_pool_metrics();
    
    // Enable/disable pooling at runtime
    static void enable_pooling(bool command_buffers, bool textures, bool buffers);
    
private:
    // Per-device tracking
    struct DeviceContext {
        IDirect3DDevice8* device;
        uint32_t frame_count;
        bool in_scene;
        std::chrono::high_resolution_clock::time_point last_present_time;
    };
    
    static std::unordered_map<IDirect3DDevice8*, DeviceContext> device_contexts_;
    static std::mutex mutex_;
    static bool initialized_;
};

/**
 * RAII helper for automatic command buffer management
 */
class PooledCommandBuffer {
public:
    PooledCommandBuffer() 
        : buffer_(ResourcePoolIntegration::acquire_command_buffer()) {}
    
    ~PooledCommandBuffer() {
        if (buffer_) {
            ResourcePoolIntegration::release_command_buffer(std::move(buffer_));
        }
    }
    
    // Access operators
    CommandBuffer* operator->() { return buffer_.get(); }
    const CommandBuffer* operator->() const { return buffer_.get(); }
    CommandBuffer& operator*() { return *buffer_; }
    const CommandBuffer& operator*() const { return *buffer_; }
    
    // Get raw pointer
    CommandBuffer* get() { return buffer_.get(); }
    const CommandBuffer* get() const { return buffer_.get(); }
    
    // Release ownership
    std::unique_ptr<CommandBuffer> release() {
        return std::move(buffer_);
    }
    
    // Check if valid
    explicit operator bool() const { return buffer_ != nullptr; }
    
private:
    std::unique_ptr<CommandBuffer> buffer_;
    
    // Prevent copying
    PooledCommandBuffer(const PooledCommandBuffer&) = delete;
    PooledCommandBuffer& operator=(const PooledCommandBuffer&) = delete;
};

/**
 * Configuration builder for easy setup
 */
class PoolConfigurationBuilder {
public:
    PoolConfigurationBuilder() = default;
    
    PoolConfigurationBuilder& with_command_buffer_pooling(bool enabled, size_t max_buffers = 64) {
        config_.enable_command_buffer_pool = enabled;
        config_.max_command_buffers = max_buffers;
        return *this;
    }
    
    PoolConfigurationBuilder& with_command_buffer_size(size_t initial_size) {
        config_.command_buffer_initial_size = initial_size;
        return *this;
    }
    
    PoolConfigurationBuilder& with_texture_caching(bool enabled, size_t max_textures = 256) {
        config_.enable_texture_cache = enabled;
        config_.max_cached_textures = max_textures;
        return *this;
    }
    
    PoolConfigurationBuilder& with_texture_memory_limit(size_t max_bytes) {
        config_.max_texture_memory = max_bytes;
        return *this;
    }
    
    PoolConfigurationBuilder& with_buffer_caching(bool enabled, size_t max_buffers = 512) {
        config_.enable_buffer_cache = enabled;
        config_.max_cached_buffers = max_buffers;
        return *this;
    }
    
    PoolConfigurationBuilder& with_buffer_memory_limit(size_t max_bytes) {
        config_.max_buffer_memory = max_bytes;
        return *this;
    }
    
    PoolConfigurationBuilder& with_automatic_cleanup(bool enabled, uint32_t max_frames_unused = 60) {
        config_.enable_automatic_cleanup = enabled;
        config_.max_frames_unused = max_frames_unused;
        return *this;
    }
    
    PoolConfigurationBuilder& with_metrics(bool enabled, bool log_stats = false) {
        config_.enable_metrics = enabled;
        config_.log_pool_statistics = log_stats;
        return *this;
    }
    
    PoolConfigurationBuilder& with_statistics_interval(uint32_t frames) {
        config_.statistics_interval_frames = frames;
        return *this;
    }
    
    PoolConfiguration build() const { return config_; }
    
    // Preset configurations
    static PoolConfiguration minimal() {
        return PoolConfigurationBuilder()
            .with_command_buffer_pooling(true, 8)
            .with_texture_caching(false)
            .with_buffer_caching(false)
            .with_metrics(false)
            .build();
    }
    
    static PoolConfiguration balanced() {
        return PoolConfigurationBuilder()
            .with_command_buffer_pooling(true, 32)
            .with_texture_caching(true, 128)
            .with_texture_memory_limit(256 * 1024 * 1024)
            .with_buffer_caching(true, 256)
            .with_buffer_memory_limit(128 * 1024 * 1024)
            .with_automatic_cleanup(true, 60)
            .with_metrics(true, false)
            .build();
    }
    
    static PoolConfiguration aggressive() {
        return PoolConfigurationBuilder()
            .with_command_buffer_pooling(true, 128)
            .with_command_buffer_size(128 * 1024)
            .with_texture_caching(true, 512)
            .with_texture_memory_limit(1024 * 1024 * 1024)
            .with_buffer_caching(true, 1024)
            .with_buffer_memory_limit(512 * 1024 * 1024)
            .with_automatic_cleanup(true, 120)
            .with_metrics(true, true)
            .with_statistics_interval(600)
            .build();
    }
    
    static PoolConfiguration debug() {
        return PoolConfigurationBuilder()
            .with_command_buffer_pooling(true, 16)
            .with_texture_caching(true, 64)
            .with_buffer_caching(true, 128)
            .with_automatic_cleanup(true, 30)
            .with_metrics(true, true)
            .with_statistics_interval(60)
            .build();
    }
    
private:
    PoolConfiguration config_;
};

// Convenience macros for pool usage
#define POOLED_COMMAND_BUFFER() dx8gl::PooledCommandBuffer _pooled_cmd_buffer
#define CMD_BUFFER (_pooled_cmd_buffer.get())

// Global functions for easy access
inline void initialize_resource_pools(const PoolConfiguration& config = PoolConfigurationBuilder::balanced()) {
    ResourcePoolManager::get_instance().initialize(config);
}

inline void shutdown_resource_pools() {
    ResourcePoolManager::get_instance().shutdown();
}

inline void begin_frame() {
    ResourcePoolManager::get_instance().begin_frame();
}

inline void end_frame() {
    ResourcePoolManager::get_instance().end_frame();
}

} // namespace dx8gl

#endif // DX8GL_RESOURCE_POOL_INTEGRATION_H