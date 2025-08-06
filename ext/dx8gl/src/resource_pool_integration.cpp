#include "resource_pool_integration.h"
#include "d3d8_texture.h"
#include "d3d8_vertexbuffer.h"
#include "d3d8_indexbuffer.h"
#include "logger.h"

namespace dx8gl {

// Static members
std::unordered_map<IDirect3DDevice8*, ResourcePoolIntegration::DeviceContext> 
    ResourcePoolIntegration::device_contexts_;
std::mutex ResourcePoolIntegration::mutex_;
bool ResourcePoolIntegration::initialized_ = false;

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
    
    if (!initialized_) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_texture_cache) {
        return false;
    }
    
    // Build resource key
    ResourceKey key;
    key.type = ResourceKey::TEXTURE_2D;
    key.width = width;
    key.height = height;
    key.depth = 1;
    key.format = static_cast<uint32_t>(format);
    key.usage_flags = usage;
    key.mip_levels = levels;
    key.size = 0;
    
    // TODO: Implement actual texture cache lookup
    // For now, return false to use normal allocation
    
    DX8GL_TRACE("Texture cache lookup: %ux%u fmt=%u levels=%u (cache not yet implemented)",
               width, height, format, levels);
    
    return false;
}

void ResourcePoolIntegration::cache_released_texture(IDirect3DTexture8* texture) {
    if (!initialized_ || !texture) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_texture_cache) {
        return;
    }
    
    // TODO: Implement actual texture caching
    // Would need to extract texture properties and add to cache
    
    DX8GL_TRACE("Texture %p released (caching not yet implemented)", texture);
}

bool ResourcePoolIntegration::try_reuse_vertex_buffer(
    uint32_t length,
    DWORD usage,
    DWORD fvf,
    D3DPOOL pool,
    IDirect3DVertexBuffer8** out_buffer) {
    
    if (!initialized_) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return false;
    }
    
    // Build resource key
    ResourceKey key;
    key.type = ResourceKey::VERTEX_BUFFER;
    key.width = 0;
    key.height = 0;
    key.depth = 0;
    key.format = fvf;
    key.usage_flags = usage;
    key.mip_levels = 0;
    key.size = length;
    
    // TODO: Implement actual vertex buffer cache lookup
    
    DX8GL_TRACE("Vertex buffer cache lookup: size=%u fvf=0x%X usage=0x%X (cache not yet implemented)",
               length, fvf, usage);
    
    return false;
}

void ResourcePoolIntegration::cache_released_vertex_buffer(IDirect3DVertexBuffer8* buffer) {
    if (!initialized_ || !buffer) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return;
    }
    
    // TODO: Implement actual vertex buffer caching
    
    DX8GL_TRACE("Vertex buffer %p released (caching not yet implemented)", buffer);
}

bool ResourcePoolIntegration::try_reuse_index_buffer(
    uint32_t length,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    IDirect3DIndexBuffer8** out_buffer) {
    
    if (!initialized_) {
        return false;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return false;
    }
    
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
    
    // TODO: Implement actual index buffer cache lookup
    
    DX8GL_TRACE("Index buffer cache lookup: size=%u fmt=%u usage=0x%X (cache not yet implemented)",
               length, format, usage);
    
    return false;
}

void ResourcePoolIntegration::cache_released_index_buffer(IDirect3DIndexBuffer8* buffer) {
    if (!initialized_ || !buffer) {
        return;
    }
    
    const auto& config = ResourcePoolManager::get_instance().get_configuration();
    if (!config.enable_buffer_cache) {
        return;
    }
    
    // TODO: Implement actual index buffer caching
    
    DX8GL_TRACE("Index buffer %p released (caching not yet implemented)", buffer);
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