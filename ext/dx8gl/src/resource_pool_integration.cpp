#include "resource_pool_integration.h"
#include "d3d8_texture.h"
#include "d3d8_vertexbuffer.h"
#include "d3d8_indexbuffer.h"
#include "logger.h"

namespace dx8gl {

// Static members
std::unordered_map<IDirect3DDevice8*, ResourcePoolIntegration::DeviceContext> 
    ResourcePoolIntegration::device_contexts_;
std::unordered_map<ResourceKey, std::vector<ResourcePoolIntegration::CachedTexture>, ResourceKeyHash>
    ResourcePoolIntegration::texture_cache_;
std::unordered_map<ResourceKey, std::vector<ResourcePoolIntegration::CachedVertexBuffer>, ResourceKeyHash>
    ResourcePoolIntegration::vertex_buffer_cache_;
std::unordered_map<ResourceKey, std::vector<ResourcePoolIntegration::CachedIndexBuffer>, ResourceKeyHash>
    ResourcePoolIntegration::index_buffer_cache_;
std::mutex ResourcePoolIntegration::mutex_;
bool ResourcePoolIntegration::initialized_ = false;
size_t ResourcePoolIntegration::total_texture_memory_ = 0;
size_t ResourcePoolIntegration::total_vertex_buffer_memory_ = 0;
size_t ResourcePoolIntegration::total_index_buffer_memory_ = 0;
uint32_t ResourcePoolIntegration::current_frame_ = 0;

void ResourcePoolIntegration::initialize_for_device(IDirect3DDevice8* device, const PoolConfiguration& config) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        // First device - initialize the pool manager
        ResourcePoolManager::get_instance().initialize(config);
        initialized_ = true;
        
        DX8GL_INFO("Resource pools initialized for first device with config:");
        DX8GL_INFO("  Command buffer pooling: %s (max=%zu)",
                   config.enable_command_buffer_pool ? "enabled" : "disabled",
                   config.max_command_buffers);
        DX8GL_INFO("  Texture caching: %s (max=%zu, memory=%zuMB)",
                   config.enable_texture_cache ? "enabled" : "disabled",
                   config.max_cached_textures,
                   config.max_texture_memory / (1024 * 1024));
        DX8GL_INFO("  Buffer caching: %s (max=%zu, memory=%zuMB)",
                   config.enable_buffer_cache ? "enabled" : "disabled",
                   config.max_cached_buffers,
                   config.max_buffer_memory / (1024 * 1024));
    }
    
    // Track this device
    DeviceContext context;
    context.device = device;
    context.frame_count = 0;
    context.in_scene = false;
    context.last_present_time = std::chrono::high_resolution_clock::now();
    
    device_contexts_[device] = context;
    
    DX8GL_INFO("Resource pools registered for device %p", device);
}

void ResourcePoolIntegration::shutdown_for_device(IDirect3DDevice8* device) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Remove device from tracking
    device_contexts_.erase(device);
    
    // If this was the last device, shutdown pools
    if (device_contexts_.empty() && initialized_) {
        ResourcePoolManager::get_instance().shutdown();
        initialized_ = false;
        
        DX8GL_INFO("Resource pools shut down (last device removed)");
    } else {
        DX8GL_INFO("Resource pools unregistered for device %p", device);
    }
}

void ResourcePoolIntegration::on_begin_scene(IDirect3DDevice8* device) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto it = device_contexts_.find(device);
    if (it != device_contexts_.end()) {
        if (!it->second.in_scene) {
            it->second.in_scene = true;
            
            // Begin frame for pools on first BeginScene
            ResourcePoolManager::get_instance().begin_frame();
            
            DX8GL_TRACE("Begin scene for device %p (frame %u)", 
                       device, it->second.frame_count);
        }
    }
}

void ResourcePoolIntegration::on_end_scene(IDirect3DDevice8* device) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto it = device_contexts_.find(device);
    if (it != device_contexts_.end()) {
        if (it->second.in_scene) {
            it->second.in_scene = false;
            
            // End frame for pools on EndScene
            ResourcePoolManager::get_instance().end_frame();
            
            // Update frames_unused for cached resources
            current_frame_++;
            for (auto& pair : texture_cache_) {
                for (auto& cached : pair.second) {
                    cached.frames_unused++;
                }
            }
            for (auto& pair : vertex_buffer_cache_) {
                for (auto& cached : pair.second) {
                    cached.frames_unused++;
                }
            }
            for (auto& pair : index_buffer_cache_) {
                for (auto& cached : pair.second) {
                    cached.frames_unused++;
                }
            }
            
            // Periodically clean up old resources
            if ((current_frame_ % 60) == 0) {
                const auto& config = ResourcePoolManager::get_instance().get_configuration();
                size_t evicted_count = 0;
                size_t evicted_memory = 0;
                
                for (auto& pair : texture_cache_) {
                    auto& textures = pair.second;
                    for (auto it = textures.begin(); it != textures.end();) {
                        if (it->frames_unused > config.max_frames_unused) {
                            // Release this old texture
                            it->texture->Release();
                            evicted_memory += it->memory_size;
                            total_texture_memory_ -= it->memory_size;
                            it = textures.erase(it);
                            evicted_count++;
                        } else {
                            ++it;
                        }
                    }
                }
                
                if (evicted_count > 0) {
                    DX8GL_INFO("Texture cache cleanup: evicted %zu textures (%zuKB)",
                              evicted_count, evicted_memory / 1024);
                    
                    // Note: Metrics update would need access to the actual pool
                    // For now, just log the eviction
                    // TODO: Add method to update metrics directly on the pool
                }
                
                // Clean up old vertex buffers
                size_t vb_evicted_count = 0;
                size_t vb_evicted_memory = 0;
                for (auto& pair : vertex_buffer_cache_) {
                    auto& buffers = pair.second;
                    for (auto it = buffers.begin(); it != buffers.end();) {
                        if (it->frames_unused > config.max_frames_unused) {
                            // Release this old buffer
                            it->buffer->Release();
                            vb_evicted_memory += it->memory_size;
                            total_vertex_buffer_memory_ -= it->memory_size;
                            it = buffers.erase(it);
                            vb_evicted_count++;
                        } else {
                            ++it;
                        }
                    }
                }
                
                if (vb_evicted_count > 0) {
                    DX8GL_INFO("Vertex buffer cache cleanup: evicted %zu buffers (%zuKB)",
                              vb_evicted_count, vb_evicted_memory / 1024);
                    
                    // Update metrics
                    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
                    // TODO: Update metrics through proper pool API
                    // metrics.buffer_evictions += vb_evicted_count;
                    // metrics.buffer_memory_used = total_vertex_buffer_memory_ + total_index_buffer_memory_;
                }
                
                // Clean up old index buffers
                size_t ib_evicted_count = 0;
                size_t ib_evicted_memory = 0;
                for (auto& pair : index_buffer_cache_) {
                    auto& buffers = pair.second;
                    for (auto it = buffers.begin(); it != buffers.end();) {
                        if (it->frames_unused > config.max_frames_unused) {
                            // Release this old buffer
                            it->buffer->Release();
                            ib_evicted_memory += it->memory_size;
                            total_index_buffer_memory_ -= it->memory_size;
                            it = buffers.erase(it);
                            ib_evicted_count++;
                        } else {
                            ++it;
                        }
                    }
                }
                
                if (ib_evicted_count > 0) {
                    DX8GL_INFO("Index buffer cache cleanup: evicted %zu buffers (%zuKB)",
                              ib_evicted_count, ib_evicted_memory / 1024);
                    
                    // Update metrics
                    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
                    // TODO: Update metrics through proper pool API
                    // metrics.buffer_evictions += ib_evicted_count;
                    metrics.buffer_memory_used = total_vertex_buffer_memory_ + total_index_buffer_memory_;
                }
            }
            
            DX8GL_TRACE("End scene for device %p", device);
        }
    }
}

void ResourcePoolIntegration::on_present(IDirect3DDevice8* device) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto it = device_contexts_.find(device);
    if (it != device_contexts_.end()) {
        it->second.frame_count++;
        
        // Calculate frame time
        auto now = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(
            now - it->second.last_present_time).count();
        it->second.last_present_time = now;
        
        // Log statistics periodically
        if ((it->second.frame_count % 300) == 0) {
            const auto& config = ResourcePoolManager::get_instance().get_configuration();
            if (config.enable_metrics && config.log_pool_statistics) {
                DX8GL_INFO("Frame %u: Present time = %.2f ms", 
                          it->second.frame_count, frame_time / 1000.0);
                ResourcePoolManager::get_instance().log_all_statistics();
            }
        }
        
        DX8GL_TRACE("Present for device %p (frame %u)", 
                   device, it->second.frame_count);
    }
}

std::unique_ptr<CommandBuffer> ResourcePoolIntegration::acquire_command_buffer() {
    if (!initialized_) {
        // Fallback to direct allocation if pools not initialized
        return std::make_unique<CommandBuffer>();
    }
    
    return ResourcePoolManager::get_instance().get_command_buffer_pool().acquire();
}

void ResourcePoolIntegration::release_command_buffer(std::unique_ptr<CommandBuffer> buffer) {
    if (!initialized_ || !buffer) {
        return;
    }
    
    ResourcePoolManager::get_instance().get_command_buffer_pool().release(std::move(buffer));
}

bool ResourcePoolIntegration::try_reuse_texture(
    uint32_t width,
    uint32_t height,
    uint32_t levels,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    IDirect3DTexture8** out_texture) {
    
    if (!initialized_ || !out_texture) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_texture_cache) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Build resource key for lookup
    ResourceKey key;
    key.type = ResourceKey::TEXTURE_2D;
    key.width = width;
    key.height = height;
    key.depth = 1;
    key.format = static_cast<uint32_t>(format);
    key.usage_flags = usage;
    key.mip_levels = levels;
    key.size = 0;
    
    // Look for matching texture in cache
    auto it = texture_cache_.find(key);
    if (it != texture_cache_.end() && !it->second.empty()) {
        // Found matching textures, get one from the vector
        for (auto tex_it = it->second.begin(); tex_it != it->second.end(); ++tex_it) {
            // Check if the texture matches all requirements including pool
            if (tex_it->pool == pool) {
                // Found a match! Return it and remove from cache
                *out_texture = tex_it->texture;
                
                // Update memory tracking
                total_texture_memory_ -= tex_it->memory_size;
                
                // Remove from cache
                it->second.erase(tex_it);
                
                // Remove the key if no more textures
                if (it->second.empty()) {
                    texture_cache_.erase(it);
                }
                
                DX8GL_TRACE("Texture cache HIT: reused %ux%u fmt=%u levels=%u (remaining cached=%zu)",
                           width, height, format, levels, texture_cache_.size());
                
                // Update metrics
                auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
                // TODO: Update metrics through proper pool API
                // metrics.texture_cache_hits++;
                
                return true;
            }
        }
    }
    
    // No matching texture found
    DX8GL_TRACE("Texture cache MISS: %ux%u fmt=%u levels=%u pool=%u",
               width, height, format, levels, pool);
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.texture_cache_misses++;
    
    return false;
}

void ResourcePoolIntegration::cache_released_texture(IDirect3DTexture8* texture) {
    if (!initialized_ || !texture) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_texture_cache) {
        // Texture caching disabled, just release it
        texture->Release();
        return;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Get texture description to extract properties
    D3DSURFACE_DESC desc;
    HRESULT hr = texture->GetLevelDesc(0, &desc);
    if (FAILED(hr)) {
        // Can't get texture info, just release it
        texture->Release();
        return;
    }
    
    // Get number of mip levels
    DWORD levels = texture->GetLevelCount();
    
    // Calculate approximate memory size
    size_t memory_size = 0;
    for (DWORD level = 0; level < levels; ++level) {
        D3DSURFACE_DESC level_desc;
        if (SUCCEEDED(texture->GetLevelDesc(level, &level_desc))) {
            // Estimate bytes per pixel based on format
            size_t bytes_per_pixel = 4;  // Default to 4 bytes
            switch (level_desc.Format) {
                case D3DFMT_R5G6B5:
                case D3DFMT_X1R5G5B5:
                case D3DFMT_A1R5G5B5:
                    bytes_per_pixel = 2;
                    break;
                case D3DFMT_R8G8B8:
                    bytes_per_pixel = 3;
                    break;
                case D3DFMT_DXT1:
                    bytes_per_pixel = 0;  // Special handling for compressed
                    memory_size += (level_desc.Width * level_desc.Height) / 2;
                    break;
                case D3DFMT_DXT2:
                case D3DFMT_DXT3:
                case D3DFMT_DXT4:
                case D3DFMT_DXT5:
                    bytes_per_pixel = 0;  // Special handling for compressed
                    memory_size += level_desc.Width * level_desc.Height;
                    break;
            }
            if (bytes_per_pixel > 0) {
                memory_size += level_desc.Width * level_desc.Height * bytes_per_pixel;
            }
        }
    }
    
    // Check if we should cache this texture
    if (total_texture_memory_ + memory_size > config.max_texture_memory) {
        // Would exceed memory limit, evict old textures first
        // Evict textures that have been unused for too long
        for (auto& pair : texture_cache_) {
            auto& textures = pair.second;
            for (auto it = textures.begin(); it != textures.end();) {
                if (it->frames_unused > config.max_frames_unused) {
                    // Release this old texture
                    uint32_t frames_unused = it->frames_unused;
                    it->texture->Release();
                    total_texture_memory_ -= it->memory_size;
                    it = textures.erase(it);
                    
                    DX8GL_TRACE("Evicted old texture from cache (frames_unused=%u)", frames_unused);
                    
                    // Check if we have enough space now
                    if (total_texture_memory_ + memory_size <= config.max_texture_memory) {
                        break;
                    }
                } else {
                    ++it;
                }
            }
            if (total_texture_memory_ + memory_size <= config.max_texture_memory) {
                break;
            }
        }
    }
    
    // Check again after potential eviction
    if (total_texture_memory_ + memory_size > config.max_texture_memory ||
        texture_cache_.size() >= config.max_cached_textures) {
        // Still over limit or too many textures, just release this one
        texture->Release();
        DX8GL_TRACE("Texture cache full, releasing texture %p", texture);
        return;
    }
    
    // Build resource key
    ResourceKey key;
    key.type = ResourceKey::TEXTURE_2D;
    key.width = desc.Width;
    key.height = desc.Height;
    key.depth = 1;
    key.format = static_cast<uint32_t>(desc.Format);
    key.usage_flags = desc.Usage;
    key.mip_levels = levels;
    key.size = 0;
    
    // Create cached texture entry
    CachedTexture cached;
    cached.texture = texture;
    cached.width = desc.Width;
    cached.height = desc.Height;
    cached.levels = levels;
    cached.usage = desc.Usage;
    cached.format = desc.Format;
    cached.pool = desc.Pool;
    cached.frames_unused = 0;
    cached.memory_size = memory_size;
    
    // Add to cache
    texture_cache_[key].push_back(cached);
    total_texture_memory_ += memory_size;
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.textures_cached++;
    // metrics.texture_memory_used = total_texture_memory_;
    
    DX8GL_TRACE("Cached texture %p: %ux%u fmt=%u levels=%u pool=%u (total_mem=%zuKB)",
               texture, desc.Width, desc.Height, desc.Format, levels, desc.Pool,
               total_texture_memory_ / 1024);
}

bool ResourcePoolIntegration::try_reuse_vertex_buffer(
    uint32_t length,
    DWORD usage,
    DWORD fvf,
    D3DPOOL pool,
    IDirect3DVertexBuffer8** out_buffer) {
    
    if (!initialized_ || !out_buffer) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Build resource key derived from length, FVF, and usage flags
    ResourceKey key;
    key.type = ResourceKey::VERTEX_BUFFER;
    key.width = 0;
    key.height = 0;
    key.depth = 0;
    key.format = fvf;  // Store FVF in format field
    key.usage_flags = usage;
    key.mip_levels = 0;
    key.size = length;
    
    // Look for matching vertex buffer in cache
    auto it = vertex_buffer_cache_.find(key);
    if (it != vertex_buffer_cache_.end() && !it->second.empty()) {
        // Found matching vertex buffers, get one from the vector
        for (auto buf_it = it->second.begin(); buf_it != it->second.end(); ++buf_it) {
            // Check if the buffer matches all requirements including pool
            if (buf_it->pool == pool) {
                // Found a match! Return it and remove from cache
                *out_buffer = buf_it->buffer;
                
                // Update memory tracking
                total_vertex_buffer_memory_ -= buf_it->memory_size;
                
                // Remove from cache
                it->second.erase(buf_it);
                
                // Remove the key if no more buffers
                if (it->second.empty()) {
                    vertex_buffer_cache_.erase(it);
                }
                
                DX8GL_TRACE("Vertex buffer cache HIT: reused %u bytes, fvf=0x%X, usage=0x%X (remaining cached=%zu)",
                           length, fvf, usage, vertex_buffer_cache_.size());
                
                // Update metrics
                auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
                // TODO: Update metrics through proper pool API
                // metrics.buffer_cache_hits++;
                
                return true;
            }
        }
    }
    
    // No matching buffer found
    DX8GL_TRACE("Vertex buffer cache MISS: %u bytes, fvf=0x%X, usage=0x%X, pool=%u",
               length, fvf, usage, pool);
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.buffer_cache_misses++;
    
    return false;
}

void ResourcePoolIntegration::cache_released_vertex_buffer(IDirect3DVertexBuffer8* buffer) {
    if (!initialized_ || !buffer) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        // Buffer caching disabled, just release it
        buffer->Release();
        return;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Get buffer description to extract properties
    D3DVERTEXBUFFER_DESC desc;
    HRESULT hr = buffer->GetDesc(&desc);
    if (FAILED(hr)) {
        // Can't get buffer info, just release it
        buffer->Release();
        return;
    }
    
    // Calculate buffer memory size
    size_t memory_size = desc.Size;
    
    // Check if we should cache this buffer
    if (total_vertex_buffer_memory_ + memory_size > config.max_buffer_memory) {
        // Would exceed memory limit, evict old buffers first
        for (auto& pair : vertex_buffer_cache_) {
            auto& buffers = pair.second;
            for (auto it = buffers.begin(); it != buffers.end();) {
                if (it->frames_unused > config.max_frames_unused) {
                    // Release this old buffer
                    uint32_t frames_unused = it->frames_unused;
                    it->buffer->Release();
                    total_vertex_buffer_memory_ -= it->memory_size;
                    it = buffers.erase(it);
                    
                    DX8GL_TRACE("Evicted old vertex buffer from cache (frames_unused=%u)", frames_unused);
                    
                    // Check if we have enough space now
                    if (total_vertex_buffer_memory_ + memory_size <= config.max_buffer_memory) {
                        break;
                    }
                } else {
                    ++it;
                }
            }
            if (total_vertex_buffer_memory_ + memory_size <= config.max_buffer_memory) {
                break;
            }
        }
    }
    
    // Check again after potential eviction
    if (total_vertex_buffer_memory_ + memory_size > config.max_buffer_memory ||
        vertex_buffer_cache_.size() >= config.max_cached_buffers) {
        // Still over limit or too many buffers, just release this one
        buffer->Release();
        DX8GL_TRACE("Vertex buffer cache full, releasing buffer %p", buffer);
        return;
    }
    
    // Build resource key from buffer properties
    ResourceKey key;
    key.type = ResourceKey::VERTEX_BUFFER;
    key.width = 0;
    key.height = 0;
    key.depth = 0;
    key.format = desc.FVF;  // Store FVF in format field
    key.usage_flags = desc.Usage;
    key.mip_levels = 0;
    key.size = desc.Size;
    
    // Create cached buffer entry
    CachedVertexBuffer cached;
    cached.buffer = buffer;
    cached.length = desc.Size;
    cached.usage = desc.Usage;
    cached.fvf = desc.FVF;
    cached.pool = desc.Pool;
    cached.frames_unused = 0;
    cached.memory_size = memory_size;
    
    // Add to cache
    vertex_buffer_cache_[key].push_back(cached);
    total_vertex_buffer_memory_ += memory_size;
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.buffers_cached++;
    // metrics.buffer_memory_used = total_vertex_buffer_memory_ + total_index_buffer_memory_;
    
    DX8GL_TRACE("Cached vertex buffer %p: %u bytes, fvf=0x%X, usage=0x%X, pool=%u (total_mem=%zuKB)",
               buffer, desc.Size, desc.FVF, desc.Usage, desc.Pool,
               total_vertex_buffer_memory_ / 1024);
}

bool ResourcePoolIntegration::try_reuse_index_buffer(
    uint32_t length,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    IDirect3DIndexBuffer8** out_buffer) {
    
    if (!initialized_ || !out_buffer) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Build resource key
    ResourceKey key;
    key.type = ResourceKey::INDEX_BUFFER;
    key.width = 0;
    key.height = 0;
    key.depth = 0;
    key.format = static_cast<uint32_t>(format);
    key.usage_flags = usage;
    key.mip_levels = 0;
    key.size = length;
    
    // Look for matching index buffer in cache
    auto it = index_buffer_cache_.find(key);
    if (it != index_buffer_cache_.end() && !it->second.empty()) {
        // Found matching index buffers, get one from the vector
        for (auto buf_it = it->second.begin(); buf_it != it->second.end(); ++buf_it) {
            // Check if the buffer matches all requirements including pool
            if (buf_it->pool == pool) {
                // Found a match! Return it and remove from cache
                *out_buffer = buf_it->buffer;
                
                // Update memory tracking
                total_index_buffer_memory_ -= buf_it->memory_size;
                
                // Remove from cache
                it->second.erase(buf_it);
                
                // Remove the key if no more buffers
                if (it->second.empty()) {
                    index_buffer_cache_.erase(it);
                }
                
                DX8GL_TRACE("Index buffer cache HIT: reused %u bytes, fmt=%u, usage=0x%X (remaining cached=%zu)",
                           length, format, usage, index_buffer_cache_.size());
                
                // Update metrics
                auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
                // TODO: Update metrics through proper pool API
                // metrics.buffer_cache_hits++;
                
                return true;
            }
        }
    }
    
    // No matching buffer found
    DX8GL_TRACE("Index buffer cache MISS: %u bytes, fmt=%u, usage=0x%X, pool=%u",
               length, format, usage, pool);
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.buffer_cache_misses++;
    
    return false;
}

void ResourcePoolIntegration::cache_released_index_buffer(IDirect3DIndexBuffer8* buffer) {
    if (!initialized_ || !buffer) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        // Buffer caching disabled, just release it
        buffer->Release();
        return;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Get buffer description to extract properties
    D3DINDEXBUFFER_DESC desc;
    HRESULT hr = buffer->GetDesc(&desc);
    if (FAILED(hr)) {
        // Can't get buffer info, just release it
        buffer->Release();
        return;
    }
    
    // Calculate buffer memory size
    size_t memory_size = desc.Size;
    
    // Check if we should cache this buffer
    if (total_index_buffer_memory_ + memory_size > config.max_buffer_memory) {
        // Would exceed memory limit, evict old buffers first
        for (auto& pair : index_buffer_cache_) {
            auto& buffers = pair.second;
            for (auto it = buffers.begin(); it != buffers.end();) {
                if (it->frames_unused > config.max_frames_unused) {
                    // Release this old buffer
                    uint32_t frames_unused = it->frames_unused;
                    it->buffer->Release();
                    total_index_buffer_memory_ -= it->memory_size;
                    it = buffers.erase(it);
                    
                    DX8GL_TRACE("Evicted old index buffer from cache (frames_unused=%u)", frames_unused);
                    
                    // Check if we have enough space now
                    if (total_index_buffer_memory_ + memory_size <= config.max_buffer_memory) {
                        break;
                    }
                } else {
                    ++it;
                }
            }
            if (total_index_buffer_memory_ + memory_size <= config.max_buffer_memory) {
                break;
            }
        }
    }
    
    // Check again after potential eviction
    if (total_index_buffer_memory_ + memory_size > config.max_buffer_memory ||
        index_buffer_cache_.size() >= config.max_cached_buffers) {
        // Still over limit or too many buffers, just release this one
        buffer->Release();
        DX8GL_TRACE("Index buffer cache full, releasing buffer %p", buffer);
        return;
    }
    
    // Build resource key from buffer properties
    ResourceKey key;
    key.type = ResourceKey::INDEX_BUFFER;
    key.width = 0;
    key.height = 0;
    key.depth = 0;
    key.format = static_cast<uint32_t>(desc.Format);
    key.usage_flags = desc.Usage;
    key.mip_levels = 0;
    key.size = desc.Size;
    
    // Create cached buffer entry
    CachedIndexBuffer cached;
    cached.buffer = buffer;
    cached.length = desc.Size;
    cached.usage = desc.Usage;
    cached.format = desc.Format;
    cached.pool = desc.Pool;
    cached.frames_unused = 0;
    cached.memory_size = memory_size;
    
    // Add to cache
    index_buffer_cache_[key].push_back(cached);
    total_index_buffer_memory_ += memory_size;
    
    // Update metrics
    auto metrics = ResourcePoolManager::get_instance().get_combined_metrics();
    // TODO: Update metrics through proper pool API
    // metrics.buffers_cached++;
    // metrics.buffer_memory_used = total_vertex_buffer_memory_ + total_index_buffer_memory_;
    
    DX8GL_TRACE("Cached index buffer %p: %u bytes, fmt=%u, usage=0x%X, pool=%u (total_mem=%zuKB)",
               buffer, desc.Size, desc.Format, desc.Usage, desc.Pool,
               total_index_buffer_memory_ / 1024);
}

void ResourcePoolIntegration::set_pool_configuration(const PoolConfiguration& config) {
    if (!initialized_) {
        DX8GL_WARNING("Cannot set pool configuration: pools not initialized");
        return;
    }
    
    ResourcePoolManager::get_instance().set_configuration(config);
    
    DX8GL_INFO("Pool configuration updated");
}

const PoolConfiguration& ResourcePoolIntegration::get_pool_configuration() {
    static PoolConfiguration default_config;
    
    if (!initialized_) {
        return default_config;
    }
    
    return ResourcePoolManager::get_instance().get_configuration();
}

void ResourcePoolIntegration::log_pool_statistics() {
    if (!initialized_) {
        DX8GL_INFO("Resource pools not initialized");
        return;
    }
    
    ResourcePoolManager::get_instance().log_all_statistics();
}

PoolMetrics ResourcePoolIntegration::get_pool_metrics() {
    if (!initialized_) {
        return PoolMetrics();
    }
    
    return ResourcePoolManager::get_instance().get_combined_metrics();
}

void ResourcePoolIntegration::enable_pooling(bool command_buffers, bool textures, bool buffers) {
    if (!initialized_) {
        DX8GL_WARNING("Cannot enable/disable pooling: pools not initialized");
        return;
    }
    
    auto config = ResourcePoolManager::get_instance().get_configuration();
    config.enable_command_buffer_pool = command_buffers;
    config.enable_texture_cache = textures;
    config.enable_buffer_cache = buffers;
    
    ResourcePoolManager::get_instance().set_configuration(config);
    
    DX8GL_INFO("Pooling updated: command_buffers=%s, textures=%s, buffers=%s",
               command_buffers ? "enabled" : "disabled",
               textures ? "enabled" : "disabled",
               buffers ? "enabled" : "disabled");
}

} // namespace dx8gl