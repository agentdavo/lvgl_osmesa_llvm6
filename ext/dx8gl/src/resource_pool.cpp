#include "resource_pool.h"
#include "logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace dx8gl {

// PoolMetrics implementation

void PoolMetrics::log_summary() const {
    std::stringstream ss;
    ss << "Resource Pool Statistics:\n";
    ss << "  Command Buffers:\n";
    ss << "    Allocated: " << command_buffers_allocated.load() << "\n";
    ss << "    Reused: " << command_buffers_reused.load() << "\n";
    ss << "    Hit Rate: " << std::fixed << std::setprecision(2);
    
    uint64_t total_cb_requests = command_buffer_hits.load() + command_buffer_misses.load();
    if (total_cb_requests > 0) {
        double hit_rate = (100.0 * command_buffer_hits.load()) / total_cb_requests;
        ss << hit_rate << "%\n";
    } else {
        ss << "N/A\n";
    }
    
    ss << "  Texture Cache:\n";
    ss << "    Cached: " << textures_cached.load() << "\n";
    ss << "    Memory: " << (texture_memory_used.load() / (1024.0 * 1024.0)) << " MB\n";
    ss << "    Hit Rate: ";
    
    uint64_t total_tex_requests = texture_cache_hits.load() + texture_cache_misses.load();
    if (total_tex_requests > 0) {
        double hit_rate = (100.0 * texture_cache_hits.load()) / total_tex_requests;
        ss << hit_rate << "%\n";
    } else {
        ss << "N/A\n";
    }
    
    ss << "  Buffer Cache:\n";
    ss << "    Cached: " << buffers_cached.load() << "\n";
    ss << "    Memory: " << (buffer_memory_used.load() / (1024.0 * 1024.0)) << " MB\n";
    ss << "    Hit Rate: ";
    
    uint64_t total_buf_requests = buffer_cache_hits.load() + buffer_cache_misses.load();
    if (total_buf_requests > 0) {
        double hit_rate = (100.0 * buffer_cache_hits.load()) / total_buf_requests;
        ss << hit_rate << "%\n";
    } else {
        ss << "N/A\n";
    }
    
    ss << "  Timing:\n";
    ss << "    Avg Allocation: " << (total_allocation_time_us.load() / 1000.0) << " ms\n";
    ss << "    Avg Deallocation: " << (total_deallocation_time_us.load() / 1000.0) << " ms\n";
    
    DX8GL_INFO("%s", ss.str().c_str());
}

// ResourceKey implementation

bool ResourceKey::operator==(const ResourceKey& other) const {
    return type == other.type &&
           width == other.width &&
           height == other.height &&
           depth == other.depth &&
           format == other.format &&
           usage_flags == other.usage_flags &&
           mip_levels == other.mip_levels &&
           size == other.size;
}

size_t ResourceKeyHash::operator()(const ResourceKey& key) const {
    size_t h1 = std::hash<int>()(key.type);
    size_t h2 = std::hash<uint32_t>()(key.width);
    size_t h3 = std::hash<uint32_t>()(key.height);
    size_t h4 = std::hash<uint32_t>()(key.format);
    size_t h5 = std::hash<uint32_t>()(key.usage_flags);
    size_t h6 = std::hash<size_t>()(key.size);
    
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
}

// EnhancedCommandBufferPool implementation

EnhancedCommandBufferPool::EnhancedCommandBufferPool(const PoolConfiguration& config)
    : config_(config)
    , current_frame_(0) {
    
    // Pre-allocate some buffers if pooling is enabled
    if (config_.enable_command_buffer_pool) {
        size_t initial_count = std::min(size_t(4), config_.max_command_buffers);
        for (size_t i = 0; i < initial_count; ++i) {
            PooledBuffer pooled;
            pooled.buffer = std::make_unique<CommandBuffer>(config_.command_buffer_initial_size);
            pooled.frames_unused = 0;
            pooled.last_size = config_.command_buffer_initial_size;
            available_.push_back(std::move(pooled));
            
            metrics_.command_buffers_allocated++;
        }
    }
    
    DX8GL_INFO("Enhanced command buffer pool initialized (pooling=%s, max=%zu)",
               config_.enable_command_buffer_pool ? "enabled" : "disabled",
               config_.max_command_buffers);
}

EnhancedCommandBufferPool::~EnhancedCommandBufferPool() {
    if (config_.enable_metrics && config_.log_pool_statistics) {
        metrics_.log_summary();
    }
    clear();
}

std::unique_ptr<CommandBuffer> EnhancedCommandBufferPool::acquire() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    std::unique_ptr<CommandBuffer> buffer;
    
    if (config_.enable_command_buffer_pool && !available_.empty()) {
        // Reuse pooled buffer
        auto& pooled = available_.back();
        buffer = std::move(pooled.buffer);
        available_.pop_back();
        
        buffer->reset();
        
        metrics_.command_buffers_reused++;
        metrics_.command_buffer_hits++;
        
        DX8GL_TRACE("Reused command buffer from pool (available=%zu)", available_.size());
    } else {
        // Allocate new buffer
        buffer = std::make_unique<CommandBuffer>(config_.command_buffer_initial_size);
        
        metrics_.command_buffers_allocated++;
        metrics_.command_buffer_misses++;
        
        DX8GL_TRACE("Allocated new command buffer (total=%llu)", 
                   metrics_.command_buffers_allocated.load());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    metrics_.total_allocation_time_us += duration.count();
    
    // Move the buffer to in_use_ and then return it
    in_use_.push_back(std::move(buffer));
    
    // Return the last element from in_use_ (which we just added)
    return std::move(in_use_.back());
}

void EnhancedCommandBufferPool::release(std::unique_ptr<CommandBuffer> buffer) {
    if (!buffer) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Remove from in-use list using custom predicate
    auto raw_ptr = buffer.get();
    auto it = std::find_if(in_use_.begin(), in_use_.end(), 
        [raw_ptr](const std::unique_ptr<CommandBuffer>& ptr) {
            return ptr.get() == raw_ptr;
        });
    if (it != in_use_.end()) {
        in_use_.erase(it);
    }
    
    if (config_.enable_command_buffer_pool && available_.size() < config_.max_command_buffers) {
        // Return to pool
        PooledBuffer pooled;
        pooled.buffer = std::move(buffer);
        pooled.frames_unused = 0;
        pooled.last_size = pooled.buffer->size();
        
        available_.push_back(std::move(pooled));
        
        DX8GL_TRACE("Returned command buffer to pool (available=%zu)", available_.size());
    } else {
        // Let it be destroyed
        DX8GL_TRACE("Command buffer destroyed (pool full or disabled)");
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    metrics_.total_deallocation_time_us += duration.count();
}

void EnhancedCommandBufferPool::begin_frame() {
    current_frame_++;
    
    if (config_.enable_automatic_cleanup && (current_frame_ % 60) == 0) {
        cleanup_unused_buffers();
    }
    
    if (config_.enable_metrics && config_.log_pool_statistics && 
        (current_frame_ % config_.statistics_interval_frames) == 0) {
        log_statistics();
    }
}

void EnhancedCommandBufferPool::end_frame() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Update frames unused for available buffers
    for (auto& pooled : available_) {
        pooled.frames_unused++;
    }
}

void EnhancedCommandBufferPool::clear() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    available_.clear();
    in_use_.clear();
    
    DX8GL_INFO("Command buffer pool cleared");
}

void EnhancedCommandBufferPool::shrink_to_fit() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Keep only a minimal number of buffers
    size_t keep_count = std::min(size_t(2), available_.size());
    if (available_.size() > keep_count) {
        available_.resize(keep_count);
        DX8GL_INFO("Command buffer pool shrunk to %zu buffers", keep_count);
    }
}

void EnhancedCommandBufferPool::set_configuration(const PoolConfiguration& config) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    config_ = config;
    
    // Adjust pool size if necessary
    if (config_.enable_command_buffer_pool) {
        while (available_.size() > config_.max_command_buffers) {
            available_.pop_back();
        }
    } else {
        available_.clear();
    }
    
    DX8GL_INFO("Command buffer pool configuration updated");
}

void EnhancedCommandBufferPool::cleanup_unused_buffers() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Remove buffers that haven't been used for too long
    available_.erase(
        std::remove_if(available_.begin(), available_.end(),
            [this](const PooledBuffer& pooled) {
                return pooled.frames_unused > config_.max_frames_unused;
            }),
        available_.end()
    );
}

void EnhancedCommandBufferPool::log_statistics() {
    DX8GL_INFO("Command Buffer Pool: available=%zu, in_use=%zu, total_allocated=%llu",
               available_.size(), in_use_.size(), metrics_.command_buffers_allocated.load());
    
    if (config_.enable_metrics) {
        metrics_.log_summary();
    }
}

#ifdef DX8GL_HAS_WEBGPU
// WebGPUResourcePool implementation

WebGPUResourcePool::WebGPUResourcePool(WGpuDevice device, const PoolConfiguration& config)
    : device_(device)
    , config_(config)
    , current_frame_(0) {
    
    DX8GL_INFO("WebGPU resource pool initialized");
}

WebGPUResourcePool::~WebGPUResourcePool() {
    // Clean up all cached resources
    for (auto& encoder : available_encoders_) {
        wgpu_object_destroy(encoder);
    }
    for (auto& encoder : in_use_encoders_) {
        wgpu_object_destroy(encoder);
    }
    
    for (auto& entry : buffer_cache_) {
        wgpu_object_destroy(entry.buffer);
    }
    
    for (auto& entry : texture_cache_) {
        wgpu_object_destroy(entry.texture);
    }
    
    for (auto& pair : bind_group_cache_) {
        for (auto& entry : pair.second) {
            wgpu_object_destroy(entry.group);
        }
    }
    
    if (config_.enable_metrics) {
        metrics_.log_summary();
    }
}

WGpuCommandEncoder WebGPUResourcePool::acquire_command_encoder() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    WGpuCommandEncoder encoder = nullptr;
    
    if (!available_encoders_.empty()) {
        encoder = available_encoders_.back();
        available_encoders_.pop_back();
        metrics_.command_buffer_hits++;
        DX8GL_TRACE("Reused WebGPU command encoder");
    } else {
        WGpuCommandEncoderDescriptor desc = {};
        desc.label = "Pooled Command Encoder";
        encoder = wgpu_device_create_command_encoder(device_, &desc);
        metrics_.command_buffer_misses++;
        metrics_.command_buffers_allocated++;
        DX8GL_TRACE("Created new WebGPU command encoder");
    }
    
    if (encoder) {
        in_use_encoders_.push_back(encoder);
    }
    
    return encoder;
}

void WebGPUResourcePool::release_command_encoder(WGpuCommandEncoder encoder) {
    if (!encoder) return;
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Remove from in-use list
    auto it = std::find(in_use_encoders_.begin(), in_use_encoders_.end(), encoder);
    if (it != in_use_encoders_.end()) {
        in_use_encoders_.erase(it);
    }
    
    if (available_encoders_.size() < config_.max_command_buffers) {
        available_encoders_.push_back(encoder);
        DX8GL_TRACE("Returned WebGPU command encoder to pool");
    } else {
        wgpu_object_destroy(encoder);
        DX8GL_TRACE("Destroyed WebGPU command encoder (pool full)");
    }
}

WGpuBuffer WebGPUResourcePool::acquire_buffer(const WGpuBufferDescriptor& desc) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Track if we found size match but usage mismatch
    bool found_size_match_wrong_usage = false;
    
    // Look for matching buffer in cache
    // Buffer is compatible if it has at least the required usage flags
    for (auto it = buffer_cache_.begin(); it != buffer_cache_.end(); ++it) {
        if (it->size == desc.size) {
            // Check usage compatibility
            // The cached buffer must have all the usage flags requested
            if ((it->usage & desc.usage) == desc.usage) {
                WGpuBuffer buffer = it->buffer;
                metrics_.buffer_memory_used -= it->size;
                buffer_cache_.erase(it);
                
                metrics_.buffer_cache_hits++;
                DX8GL_TRACE("Reused WebGPU buffer (size=%zu, usage=0x%x)", desc.size, desc.usage);
                return buffer;
            } else {
                found_size_match_wrong_usage = true;
                DX8GL_TRACE("Buffer size match but usage incompatible (cached=0x%x, needed=0x%x)", 
                           it->usage, desc.usage);
            }
        }
    }
    
    // Track usage mismatches for metrics
    if (found_size_match_wrong_usage) {
        metrics_.buffer_cache_usage_mismatches++;
    }
    
    // Create new buffer
    WGpuBuffer buffer = wgpu_device_create_buffer(device_, &desc);
    
    metrics_.buffer_cache_misses++;
    metrics_.buffers_cached++;
    metrics_.buffer_memory_used += desc.size;
    
    DX8GL_TRACE("Created new WebGPU buffer (size=%zu, usage=0x%x)", desc.size, desc.usage);
    return buffer;
}

void WebGPUResourcePool::release_buffer(WGpuBuffer buffer, size_t size, uint32_t usage_flags) {
    if (!buffer) return;
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Check if we should cache it
    if (config_.enable_buffer_cache && 
        buffer_cache_.size() < config_.max_cached_buffers &&
        metrics_.buffer_memory_used.load() + size <= config_.max_buffer_memory) {
        
        BufferEntry entry;
        entry.buffer = buffer;
        entry.size = size;
        entry.usage = usage_flags;  // Store the actual usage flags for compatibility checking
        entry.frames_unused = 0;
        
        buffer_cache_.push_back(entry);
        metrics_.buffer_memory_used += size;
        DX8GL_TRACE("Cached WebGPU buffer (size=%zu, usage=0x%x)", size, usage_flags);
    } else {
        wgpu_object_destroy(buffer);
        metrics_.buffer_memory_used -= size;
        DX8GL_TRACE("Destroyed WebGPU buffer (size=%zu, usage=0x%x)", size, usage_flags);
    }
}

WGpuTexture WebGPUResourcePool::acquire_texture(const WGpuTextureDescriptor& desc) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Look for matching texture in cache
    for (auto it = texture_cache_.begin(); it != texture_cache_.end(); ++it) {
        if (it->desc.size.width == desc.size.width &&
            it->desc.size.height == desc.size.height &&
            it->desc.size.depthOrArrayLayers == desc.size.depthOrArrayLayers &&
            it->desc.format == desc.format &&
            it->desc.usage == desc.usage &&
            it->desc.mipLevelCount == desc.mipLevelCount) {
            
            WGpuTexture texture = it->texture;
            texture_cache_.erase(it);
            
            metrics_.texture_cache_hits++;
            DX8GL_TRACE("Reused WebGPU texture (%dx%d)", desc.size.width, desc.size.height);
            return texture;
        }
    }
    
    // Create new texture
    WGpuTexture texture = wgpu_device_create_texture(device_, &desc);
    
    metrics_.texture_cache_misses++;
    metrics_.textures_cached++;
    
    // Estimate memory usage
    size_t pixel_size = 4; // Assume 4 bytes per pixel for now
    size_t memory = desc.size.width * desc.size.height * desc.size.depthOrArrayLayers * pixel_size;
    metrics_.texture_memory_used += memory;
    
    DX8GL_TRACE("Created new WebGPU texture (%dx%d)", desc.size.width, desc.size.height);
    return texture;
}

void WebGPUResourcePool::release_texture(WGpuTexture texture) {
    if (!texture) return;
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // For now, just destroy it - caching logic would go here
    wgpu_object_destroy(texture);
    DX8GL_TRACE("Destroyed WebGPU texture");
}

WGpuBindGroup WebGPUResourcePool::acquire_bind_group(const WGpuBindGroupDescriptor& desc) {
    // Simplified - would need proper hashing of descriptor
    uint64_t layout_hash = reinterpret_cast<uint64_t>(desc.layout);
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto& entries = bind_group_cache_[layout_hash];
    
    if (!entries.empty()) {
        BindGroupEntry entry = entries.back();
        entries.pop_back();
        
        metrics_.buffer_cache_hits++; // Reuse buffer metrics for bind groups
        DX8GL_TRACE("Reused WebGPU bind group");
        return entry.group;
    }
    
    // Create new bind group
    WGpuBindGroup group = wgpu_device_create_bind_group(device_, &desc);
    
    metrics_.buffer_cache_misses++;
    DX8GL_TRACE("Created new WebGPU bind group");
    return group;
}

void WebGPUResourcePool::release_bind_group(WGpuBindGroup group) {
    if (!group) return;
    
    // For now, just destroy it
    wgpu_object_destroy(group);
    DX8GL_TRACE("Destroyed WebGPU bind group");
}

void WebGPUResourcePool::begin_frame() {
    current_frame_++;
    
    // Periodic cleanup
    if ((current_frame_ % 60) == 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Clean up unused buffers
        buffer_cache_.erase(
            std::remove_if(buffer_cache_.begin(), buffer_cache_.end(),
                [this](const BufferEntry& entry) {
                    if (entry.frames_unused > config_.max_frames_unused) {
                        wgpu_object_destroy(entry.buffer);
                        metrics_.buffer_memory_used -= entry.size;
                        metrics_.buffer_evictions++;
                        return true;
                    }
                    return false;
                }),
            buffer_cache_.end()
        );
        
        // Clean up unused textures
        texture_cache_.erase(
            std::remove_if(texture_cache_.begin(), texture_cache_.end(),
                [this](const TextureEntry& entry) {
                    if (entry.frames_unused > config_.max_frames_unused) {
                        wgpu_object_destroy(entry.texture);
                        metrics_.texture_evictions++;
                        return true;
                    }
                    return false;
                }),
            texture_cache_.end()
        );
    }
}

void WebGPUResourcePool::end_frame() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Update frames unused
    for (auto& entry : buffer_cache_) {
        entry.frames_unused++;
    }
    for (auto& entry : texture_cache_) {
        entry.frames_unused++;
    }
    for (auto& pair : bind_group_cache_) {
        for (auto& entry : pair.second) {
            entry.frames_unused++;
        }
    }
}
#endif // DX8GL_HAS_WEBGPU

// ResourcePoolManager implementation

ResourcePoolManager& ResourcePoolManager::get_instance() {
    static ResourcePoolManager instance;
    return instance;
}

ResourcePoolManager::ResourcePoolManager()
    : initialized_(false)
    , frame_counter_(0) {
}

ResourcePoolManager::~ResourcePoolManager() {
    shutdown();
}

void ResourcePoolManager::initialize(const PoolConfiguration& config) {
    if (initialized_) {
        DX8GL_WARNING("Resource pool manager already initialized");
        return;
    }
    
    config_ = config;
    
    // Create command buffer pool
    command_buffer_pool_ = std::make_unique<EnhancedCommandBufferPool>(config);
    
    initialized_ = true;
    
    DX8GL_INFO("Resource pool manager initialized");
}

void ResourcePoolManager::shutdown() {
    if (!initialized_) return;
    
    // Log final statistics
    if (config_.enable_metrics) {
        log_all_statistics();
    }
    
    // Clear all pools
    command_buffer_pool_.reset();
    
#ifdef DX8GL_HAS_WEBGPU
    webgpu_pool_.reset();
#endif
    
    resource_caches_.clear();
    
    initialized_ = false;
    
    DX8GL_INFO("Resource pool manager shut down");
}

#ifdef DX8GL_HAS_WEBGPU
void ResourcePoolManager::initialize_webgpu(WGpuDevice device) {
    if (!initialized_) {
        DX8GL_ERROR("Resource pool manager not initialized");
        return;
    }
    
    webgpu_pool_ = std::make_unique<WebGPUResourcePool>(device, config_);
    
    DX8GL_INFO("WebGPU resource pool initialized");
}
#endif

void ResourcePoolManager::begin_frame() {
    if (!initialized_) return;
    
    frame_counter_++;
    
    if (command_buffer_pool_) {
        command_buffer_pool_->begin_frame();
    }
    
#ifdef DX8GL_HAS_WEBGPU
    if (webgpu_pool_) {
        webgpu_pool_->begin_frame();
    }
#endif
}

void ResourcePoolManager::end_frame() {
    if (!initialized_) return;
    
    if (command_buffer_pool_) {
        command_buffer_pool_->end_frame();
    }
    
#ifdef DX8GL_HAS_WEBGPU
    if (webgpu_pool_) {
        webgpu_pool_->end_frame();
    }
#endif
}

PoolMetrics ResourcePoolManager::get_combined_metrics() const {
    // Create a local PoolMetrics and populate it atomically
    PoolMetrics combined;
    
    if (command_buffer_pool_) {
        const auto& cb_metrics = command_buffer_pool_->get_metrics();
        combined.command_buffers_allocated.store(cb_metrics.command_buffers_allocated.load());
        combined.command_buffers_reused.store(cb_metrics.command_buffers_reused.load());
        combined.command_buffer_hits.store(cb_metrics.command_buffer_hits.load());
        combined.command_buffer_misses.store(cb_metrics.command_buffer_misses.load());
    }
    
#ifdef DX8GL_HAS_WEBGPU
    if (webgpu_pool_) {
        const auto& wp_metrics = webgpu_pool_->get_metrics();
        combined.textures_cached.store(wp_metrics.textures_cached.load());
        combined.texture_cache_hits.store(wp_metrics.texture_cache_hits.load());
        combined.texture_cache_misses.store(wp_metrics.texture_cache_misses.load());
        combined.texture_evictions.store(wp_metrics.texture_evictions.load());
        combined.texture_memory_used.store(wp_metrics.texture_memory_used.load());
        combined.buffers_cached.store(wp_metrics.buffers_cached.load());
        combined.buffer_cache_hits.store(wp_metrics.buffer_cache_hits.load());
        combined.buffer_cache_misses.store(wp_metrics.buffer_cache_misses.load());
        combined.buffer_evictions.store(wp_metrics.buffer_evictions.load());
        combined.buffer_memory_used.store(wp_metrics.buffer_memory_used.load());
    }
#endif
    
    // Return by value (uses move constructor)
    return combined;
}

void ResourcePoolManager::log_all_statistics() const {
    DX8GL_INFO("=== Resource Pool Manager Statistics ===");
    DX8GL_INFO("Total frames: %u", frame_counter_);
    
    auto combined = get_combined_metrics();
    combined.log_summary();
}

void ResourcePoolManager::set_configuration(const PoolConfiguration& config) {
    config_ = config;
    
    if (command_buffer_pool_) {
        command_buffer_pool_->set_configuration(config);
    }
    
    DX8GL_INFO("Resource pool configuration updated");
}

} // namespace dx8gl