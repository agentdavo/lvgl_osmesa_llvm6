#ifndef DX8GL_RESOURCE_POOL_H
#define DX8GL_RESOURCE_POOL_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include "command_buffer.h"

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"
#endif

namespace dx8gl {

/**
 * Configuration for resource pooling behavior
 */
struct PoolConfiguration {
    // Command buffer pool settings
    bool enable_command_buffer_pool = true;
    size_t max_command_buffers = 64;
    size_t command_buffer_initial_size = 64 * 1024;  // 64KB
    
    // Texture cache settings
    bool enable_texture_cache = true;
    size_t max_cached_textures = 256;
    size_t max_texture_memory = 512 * 1024 * 1024;  // 512MB
    
    // Buffer cache settings
    bool enable_buffer_cache = true;
    size_t max_cached_buffers = 512;
    size_t max_buffer_memory = 256 * 1024 * 1024;  // 256MB
    
    // Resource lifetime settings
    uint32_t max_frames_unused = 60;  // Evict after 60 frames unused
    bool enable_automatic_cleanup = true;
    
    // Performance monitoring
    bool enable_metrics = true;
    bool log_pool_statistics = false;
    uint32_t statistics_interval_frames = 300;  // Log every 300 frames
};

/**
 * Performance metrics for resource pools
 */
struct PoolMetrics {
    // Command buffer metrics
    std::atomic<uint64_t> command_buffers_allocated{0};
    std::atomic<uint64_t> command_buffers_reused{0};
    std::atomic<uint64_t> command_buffer_hits{0};
    std::atomic<uint64_t> command_buffer_misses{0};
    
    // Texture cache metrics
    std::atomic<uint64_t> textures_cached{0};
    std::atomic<uint64_t> texture_cache_hits{0};
    std::atomic<uint64_t> texture_cache_misses{0};
    std::atomic<uint64_t> texture_evictions{0};
    std::atomic<size_t> texture_memory_used{0};
    
    // Buffer cache metrics
    std::atomic<uint64_t> buffers_cached{0};
    std::atomic<uint64_t> buffer_cache_hits{0};
    std::atomic<uint64_t> buffer_cache_misses{0};
    std::atomic<uint64_t> buffer_evictions{0};
    std::atomic<size_t> buffer_memory_used{0};
    
    // Timing metrics
    std::atomic<uint64_t> total_allocation_time_us{0};
    std::atomic<uint64_t> total_deallocation_time_us{0};
    
    void reset() {
        command_buffers_allocated = 0;
        command_buffers_reused = 0;
        command_buffer_hits = 0;
        command_buffer_misses = 0;
        textures_cached = 0;
        texture_cache_hits = 0;
        texture_cache_misses = 0;
        texture_evictions = 0;
        texture_memory_used = 0;
        buffers_cached = 0;
        buffer_cache_hits = 0;
        buffer_cache_misses = 0;
        buffer_evictions = 0;
        buffer_memory_used = 0;
        total_allocation_time_us = 0;
        total_deallocation_time_us = 0;
    }
    
    void log_summary() const;
};

/**
 * Enhanced command buffer pool with metrics and configuration
 */
class EnhancedCommandBufferPool {
public:
    EnhancedCommandBufferPool(const PoolConfiguration& config = {});
    ~EnhancedCommandBufferPool();
    
    // Acquire a command buffer
    std::unique_ptr<CommandBuffer> acquire();
    
    // Release a command buffer back to the pool
    void release(std::unique_ptr<CommandBuffer> buffer);
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Pool management
    void clear();
    void shrink_to_fit();
    
    // Configuration
    void set_configuration(const PoolConfiguration& config);
    const PoolConfiguration& get_configuration() const { return config_; }
    
    // Metrics
    const PoolMetrics& get_metrics() const { return metrics_; }
    void reset_metrics() { metrics_.reset(); }
    
private:
    struct PooledBuffer {
        std::unique_ptr<CommandBuffer> buffer;
        uint32_t frames_unused;
        size_t last_size;
    };
    
    mutable std::mutex mutex_;
    std::vector<PooledBuffer> available_;
    std::vector<std::unique_ptr<CommandBuffer>> in_use_;
    
    PoolConfiguration config_;
    PoolMetrics metrics_;
    uint32_t current_frame_;
    
    void cleanup_unused_buffers();
    void log_statistics();
};

/**
 * Resource cache key for textures and buffers
 */
struct ResourceKey {
    enum Type {
        TEXTURE_2D,
        TEXTURE_CUBE,
        TEXTURE_VOLUME,
        VERTEX_BUFFER,
        INDEX_BUFFER,
        UNIFORM_BUFFER
    };
    
    Type type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t format;
    uint32_t usage_flags;
    uint32_t mip_levels;
    size_t size;  // For buffers
    
    bool operator==(const ResourceKey& other) const;
};

struct ResourceKeyHash {
    size_t operator()(const ResourceKey& key) const;
};

/**
 * Cached resource wrapper
 */
template<typename T>
struct CachedResource {
    T resource;
    ResourceKey key;
    uint32_t frames_unused;
    uint64_t last_access_frame;
    size_t memory_size;
    bool in_use;
};

/**
 * Generic resource cache template
 */
template<typename ResourceType>
class ResourceCache {
public:
    using ResourcePtr = std::shared_ptr<CachedResource<ResourceType>>;
    using CreateFunc = std::function<ResourceType(const ResourceKey&)>;
    using DestroyFunc = std::function<void(ResourceType)>;
    using SizeFunc = std::function<size_t(const ResourceKey&)>;
    
    ResourceCache(const PoolConfiguration& config,
                  CreateFunc create_func,
                  DestroyFunc destroy_func,
                  SizeFunc size_func);
    ~ResourceCache();
    
    // Get or create a resource
    ResourcePtr acquire(const ResourceKey& key);
    
    // Release a resource back to cache
    void release(ResourcePtr resource);
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Cache management
    void clear();
    void evict_unused();
    void evict_least_recently_used();
    
    // Metrics
    size_t get_cached_count() const;
    size_t get_memory_usage() const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<ResourceKey, std::vector<ResourcePtr>, ResourceKeyHash> cache_;
    std::priority_queue<std::pair<uint64_t, ResourcePtr>> lru_queue_;
    
    CreateFunc create_func_;
    DestroyFunc destroy_func_;
    SizeFunc size_func_;
    
    PoolConfiguration config_;
    PoolMetrics* metrics_;
    uint64_t current_frame_;
    size_t total_memory_;
    
    void cleanup_frame();
    bool should_evict(const CachedResource<ResourceType>& resource) const;
};

#ifdef DX8GL_HAS_WEBGPU
/**
 * WebGPU-specific resource pools
 */
class WebGPUResourcePool {
public:
    WebGPUResourcePool(WGpuDevice device, const PoolConfiguration& config = {});
    ~WebGPUResourcePool();
    
    // Command encoder pool
    WGpuCommandEncoder acquire_command_encoder();
    void release_command_encoder(WGpuCommandEncoder encoder);
    
    // Buffer pool
    WGpuBuffer acquire_buffer(const WGpuBufferDescriptor& desc);
    void release_buffer(WGpuBuffer buffer, size_t size);
    
    // Texture pool
    WGpuTexture acquire_texture(const WGpuTextureDescriptor& desc);
    void release_texture(WGpuTexture texture);
    
    // Bind group pool
    WGpuBindGroup acquire_bind_group(const WGpuBindGroupDescriptor& desc);
    void release_bind_group(WGpuBindGroup group);
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Metrics
    const PoolMetrics& get_metrics() const { return metrics_; }
    
private:
    WGpuDevice device_;
    
    // Command encoder pool
    std::vector<WGpuCommandEncoder> available_encoders_;
    std::vector<WGpuCommandEncoder> in_use_encoders_;
    
    // Buffer cache
    struct BufferEntry {
        WGpuBuffer buffer;
        size_t size;
        uint32_t usage;
        uint32_t frames_unused;
    };
    std::vector<BufferEntry> buffer_cache_;
    
    // Texture cache
    struct TextureEntry {
        WGpuTexture texture;
        WGpuTextureDescriptor desc;
        uint32_t frames_unused;
    };
    std::vector<TextureEntry> texture_cache_;
    
    // Bind group cache
    struct BindGroupEntry {
        WGpuBindGroup group;
        uint64_t layout_hash;
        uint32_t frames_unused;
    };
    std::unordered_map<uint64_t, std::vector<BindGroupEntry>> bind_group_cache_;
    
    mutable std::mutex mutex_;
    PoolConfiguration config_;
    PoolMetrics metrics_;
    uint32_t current_frame_;
};
#endif // DX8GL_HAS_WEBGPU

/**
 * Main resource pool manager singleton
 */
class ResourcePoolManager {
public:
    static ResourcePoolManager& get_instance();
    
    // Initialize with configuration
    void initialize(const PoolConfiguration& config = {});
    void shutdown();
    
    // Get pools
    EnhancedCommandBufferPool& get_command_buffer_pool() { return *command_buffer_pool_; }
    
    template<typename T>
    ResourceCache<T>& get_resource_cache();
    
#ifdef DX8GL_HAS_WEBGPU
    WebGPUResourcePool& get_webgpu_pool() { return *webgpu_pool_; }
    void initialize_webgpu(WGpuDevice device);
#endif
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Global metrics
    PoolMetrics get_combined_metrics() const;
    void log_all_statistics() const;
    
    // Configuration
    void set_configuration(const PoolConfiguration& config);
    const PoolConfiguration& get_configuration() const { return config_; }
    
private:
    ResourcePoolManager();
    ~ResourcePoolManager();
    
    // Prevent copying
    ResourcePoolManager(const ResourcePoolManager&) = delete;
    ResourcePoolManager& operator=(const ResourcePoolManager&) = delete;
    
    std::unique_ptr<EnhancedCommandBufferPool> command_buffer_pool_;
    
#ifdef DX8GL_HAS_WEBGPU
    std::unique_ptr<WebGPUResourcePool> webgpu_pool_;
#endif
    
    PoolConfiguration config_;
    bool initialized_;
    uint32_t frame_counter_;
    
    // Resource caches for different types
    std::unordered_map<std::string, std::shared_ptr<void>> resource_caches_;
};

// Convenience functions
inline EnhancedCommandBufferPool& get_command_buffer_pool() {
    return ResourcePoolManager::get_instance().get_command_buffer_pool();
}

// RAII wrapper for automatic command buffer release
class ScopedCommandBuffer {
public:
    explicit ScopedCommandBuffer(EnhancedCommandBufferPool& pool)
        : pool_(pool), buffer_(pool.acquire()) {}
    
    ~ScopedCommandBuffer() {
        if (buffer_) {
            pool_.release(std::move(buffer_));
        }
    }
    
    CommandBuffer* operator->() { return buffer_.get(); }
    const CommandBuffer* operator->() const { return buffer_.get(); }
    CommandBuffer& operator*() { return *buffer_; }
    const CommandBuffer& operator*() const { return *buffer_; }
    
    CommandBuffer* get() { return buffer_.get(); }
    const CommandBuffer* get() const { return buffer_.get(); }
    
    std::unique_ptr<CommandBuffer> release() {
        return std::move(buffer_);
    }
    
private:
    EnhancedCommandBufferPool& pool_;
    std::unique_ptr<CommandBuffer> buffer_;
};

} // namespace dx8gl

#endif // DX8GL_RESOURCE_POOL_H