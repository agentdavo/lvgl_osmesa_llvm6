#ifndef DX8GL_WGSL_SHADER_CACHE_H
#define DX8GL_WGSL_SHADER_CACHE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"
#endif

namespace dx8gl {

/**
 * Cache for compiled WGSL shaders in WebGPU backend
 * Similar to shader_binary_cache but for WGSL/WebGPU
 */
class WGSLShaderCache {
public:
    WGSLShaderCache();
    ~WGSLShaderCache();
    
    // Initialize cache with device
    void initialize(WGpuDevice device);
    
    // Shader cache key
    struct ShaderKey {
        enum ShaderType {
            VERTEX_SHADER,
            FRAGMENT_SHADER,
            COMPUTE_SHADER
        };
        
        ShaderType type;
        std::vector<uint8_t> source_hash;  // Hash of WGSL source
        uint32_t state_flags;               // Additional state flags
        
        bool operator==(const ShaderKey& other) const;
    };
    
    struct ShaderKeyHash {
        size_t operator()(const ShaderKey& key) const;
    };
    
    // Cached shader data
    struct CachedShader {
        WGpuShaderModule module;
        std::string wgsl_source;
        uint64_t last_access_time;
        uint32_t use_count;
    };
    
    // Get or compile shader
    WGpuShaderModule get_or_compile_shader(
        const std::string& wgsl_source,
        ShaderKey::ShaderType type,
        uint32_t state_flags = 0
    );
    
    // Get shader if cached
    WGpuShaderModule get_cached_shader(const ShaderKey& key);
    
    // Add compiled shader to cache
    void cache_shader(const ShaderKey& key, WGpuShaderModule module, const std::string& source);
    
    // Clear cache
    void clear();
    
    // Cache statistics
    struct CacheStats {
        uint32_t total_shaders;
        uint32_t cache_hits;
        uint32_t cache_misses;
        uint32_t compilations;
        uint64_t total_compile_time_ms;
        size_t memory_usage;
    };
    
    CacheStats get_stats() const { return stats_; }
    
    // Persistence (save/load from disk)
    bool save_to_file(const std::string& filename);
    bool load_from_file(const std::string& filename);
    
    // Memory management
    void set_max_cache_size(size_t max_shaders);
    void evict_least_recently_used();
    
private:
    // WebGPU device
    WGpuDevice device_;
    
    // Shader cache
    std::unordered_map<ShaderKey, std::unique_ptr<CachedShader>, ShaderKeyHash> cache_;
    
    // Cache statistics
    CacheStats stats_;
    
    // Cache configuration
    size_t max_cache_size_;
    bool enable_disk_cache_;
    std::string cache_directory_;
    
    // Helper functions
    std::vector<uint8_t> compute_hash(const std::string& source);
    WGpuShaderModule compile_shader_internal(const std::string& source, ShaderKey::ShaderType type);
    void update_access_time(const ShaderKey& key);
    size_t estimate_memory_usage(const std::string& source);
};

/**
 * Pipeline cache for complete render/compute pipelines
 */
class WGSLPipelineCache {
public:
    WGSLPipelineCache();
    ~WGSLPipelineCache();
    
    // Initialize with device
    void initialize(WGpuDevice device);
    
    // Pipeline key
    struct PipelineKey {
        WGpuShaderModule vertex_shader;
        WGpuShaderModule fragment_shader;
        uint64_t state_hash;  // Hash of pipeline state (blend, depth, etc.)
        
        bool operator==(const PipelineKey& other) const;
    };
    
    struct PipelineKeyHash {
        size_t operator()(const PipelineKey& key) const;
    };
    
    // Get or create pipeline
    WGpuRenderPipeline get_or_create_render_pipeline(
        const PipelineKey& key,
        const WGpuRenderPipelineDescriptor* desc
    );
    
    // Get or create compute pipeline
    WGpuComputePipeline get_or_create_compute_pipeline(
        WGpuShaderModule compute_shader,
        const WGpuComputePipelineDescriptor* desc
    );
    
    // Clear cache
    void clear();
    
    // Statistics
    struct Stats {
        uint32_t render_pipelines;
        uint32_t compute_pipelines;
        uint32_t cache_hits;
        uint32_t cache_misses;
    };
    
    Stats get_stats() const { return stats_; }
    
private:
    WGpuDevice device_;
    
    // Pipeline caches
    std::unordered_map<PipelineKey, WGpuRenderPipeline, PipelineKeyHash> render_cache_;
    std::unordered_map<WGpuShaderModule, WGpuComputePipeline> compute_cache_;
    
    Stats stats_;
};

/**
 * Combined shader and pipeline manager for WebGPU
 */
class WGSLShaderManager {
public:
    static WGSLShaderManager& get_instance();
    
    // Initialize with WebGPU device
    void initialize(WGpuDevice device);
    
    // Compile shader from WGSL source
    WGpuShaderModule compile_vertex_shader(const std::string& wgsl);
    WGpuShaderModule compile_fragment_shader(const std::string& wgsl);
    
    // Compile shader from DirectX bytecode
    WGpuShaderModule compile_from_dx_bytecode(
        const std::vector<uint32_t>& bytecode,
        bool is_vertex_shader
    );
    
    // Get fixed-function shaders
    struct FixedFunctionKey {
        bool lighting_enabled;
        bool fog_enabled;
        uint32_t texture_stages;
        bool vertex_color;
        bool transform_texcoords;
        
        bool operator==(const FixedFunctionKey& other) const;
    };
    
    struct FixedFunctionKeyHash {
        size_t operator()(const FixedFunctionKey& key) const;
    };
    
    std::pair<WGpuShaderModule, WGpuShaderModule> get_fixed_function_shaders(
        const FixedFunctionKey& key
    );
    
    // Cache management
    void clear_all_caches();
    void save_caches();
    void load_caches();
    
    // Get cache instances
    WGSLShaderCache& get_shader_cache() { return shader_cache_; }
    WGSLPipelineCache& get_pipeline_cache() { return pipeline_cache_; }
    
private:
    WGSLShaderManager();
    ~WGSLShaderManager();
    
    // Prevent copying
    WGSLShaderManager(const WGSLShaderManager&) = delete;
    WGSLShaderManager& operator=(const WGSLShaderManager&) = delete;
    
    // Caches
    WGSLShaderCache shader_cache_;
    WGSLPipelineCache pipeline_cache_;
    
    // Fixed-function shader cache
    std::unordered_map<FixedFunctionKey, std::pair<WGpuShaderModule, WGpuShaderModule>, 
                       FixedFunctionKeyHash> fixed_function_cache_;
    
    // Device reference
    WGpuDevice device_;
    
    // Configuration
    bool initialized_;
};

} // namespace dx8gl

#endif // DX8GL_WGSL_SHADER_CACHE_H