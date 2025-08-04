#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>
#include <list>
#include "GL/gl.h"
#include "GL/glext.h"
#include "d3d8_types.h" // For DWORD

namespace dx8gl {

// Shader binary format information
struct ShaderBinaryFormat {
    GLenum format;
    std::string vendor;
    std::string renderer;
    std::string driver_version;
};

// Cached shader binary data
struct CachedShaderBinary {
    std::vector<uint8_t> binary_data;
    GLenum binary_format;
    std::string source_hash;
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point last_access_time;
    size_t access_count = 0;
    size_t memory_size = 0;
    
    // Version info for validation
    uint32_t gl_version_hash;
    uint32_t extension_hash;
};

// Cache statistics
struct CacheStatistics {
    // Memory cache stats
    size_t memory_cache_hits = 0;
    size_t memory_cache_misses = 0;
    size_t memory_cache_size = 0;
    size_t memory_cache_entries = 0;
    
    // Disk cache stats
    size_t disk_cache_hits = 0;
    size_t disk_cache_misses = 0;
    size_t disk_cache_size = 0;
    size_t disk_cache_entries = 0;
    
    // Performance stats
    std::chrono::microseconds total_save_time{0};
    std::chrono::microseconds total_load_time{0};
    std::chrono::microseconds average_compilation_saved{0};
    
    // Error stats
    size_t binary_load_failures = 0;
    size_t binary_save_failures = 0;
};

// Shader binary cache configuration
struct ShaderBinaryCacheConfig {
    // Memory cache settings
    bool enable_memory_cache = true;
    size_t max_memory_cache_size = 64 * 1024 * 1024;  // 64MB default
    size_t max_memory_entries = 1000;
    
    // Disk cache settings
    bool enable_disk_cache = true;
    std::string disk_cache_directory = "shader_cache";
    size_t max_disk_cache_size = 256 * 1024 * 1024;  // 256MB default
    std::chrono::hours disk_cache_ttl = std::chrono::hours(24 * 30);  // 30 days
    
    // Behavior settings
    bool validate_binaries = true;
    bool compress_disk_cache = true;
    bool async_disk_operations = true;
    
    // LRU eviction for memory cache
    bool use_lru_eviction = true;
    
    ShaderBinaryCacheConfig() = default;
};

// Main shader binary cache class
class ShaderBinaryCache {
public:
    explicit ShaderBinaryCache(const ShaderBinaryCacheConfig& config = ShaderBinaryCacheConfig());
    ~ShaderBinaryCache();
    
    // Initialize cache (creates directories, loads index)
    bool initialize();
    
    // Shutdown cache (saves pending data)
    void shutdown();
    
    // Cache operations
    bool save_shader_binary(GLuint program, const std::string& source_hash);
    bool load_shader_binary(GLuint program, const std::string& source_hash);
    
    // Manual cache management
    void clear_memory_cache();
    void clear_disk_cache();
    void clear_all_caches();
    
    // Preload frequently used shaders into memory
    void preload_shader(const std::string& source_hash);
    void preload_shaders(const std::vector<std::string>& source_hashes);
    
    // Cache maintenance
    void trim_memory_cache();
    void trim_disk_cache();
    void validate_cache_entries();
    
    // Statistics and monitoring
    CacheStatistics get_statistics() const;
    void reset_statistics();
    
    // Configuration
    void set_config(const ShaderBinaryCacheConfig& config);
    const ShaderBinaryCacheConfig& get_config() const { return config_; }
    
    // Utility functions
    static std::string compute_source_hash(const std::string& vertex_source,
                                          const std::string& fragment_source);
    static std::string compute_bytecode_hash(const std::vector<DWORD>& vertex_bytecode,
                                            const std::vector<DWORD>& pixel_bytecode);
    static std::string compute_bytecode_hash(const DWORD* bytecode, size_t dword_count);
    static bool is_binary_caching_supported();
    static std::vector<ShaderBinaryFormat> get_supported_binary_formats();
    
private:
    ShaderBinaryCacheConfig config_;
    mutable std::mutex cache_mutex_;
    
    // Memory cache (LRU)
    std::unordered_map<std::string, std::shared_ptr<CachedShaderBinary>> memory_cache_;
    std::list<std::string> lru_list_;  // For LRU eviction
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;
    
    // Disk cache index
    std::unordered_map<std::string, std::string> disk_index_;  // hash -> filename
    
    // Statistics
    mutable CacheStatistics stats_;
    
    // Version hashes for validation
    uint32_t current_gl_version_hash_;
    uint32_t current_extension_hash_;
    
    // Helper methods
    bool save_to_memory_cache(const std::string& hash, 
                             std::shared_ptr<CachedShaderBinary> binary);
    std::shared_ptr<CachedShaderBinary> load_from_memory_cache(const std::string& hash);
    
    bool save_to_disk_cache(const std::string& hash, 
                           const CachedShaderBinary& binary);
    std::shared_ptr<CachedShaderBinary> load_from_disk_cache(const std::string& hash);
    
    void evict_lru_entries();
    void update_lru(const std::string& hash);
    
    std::string get_cache_filename(const std::string& hash) const;
    bool create_cache_directory();
    void load_disk_index();
    void save_disk_index();
    
    uint32_t compute_gl_version_hash() const;
    uint32_t compute_extension_hash() const;
    bool validate_binary(const CachedShaderBinary& binary) const;
    
    // Compression helpers
    std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed);
};

// RAII helper for shader binary operations
class ShaderBinaryCacheScope {
public:
    ShaderBinaryCacheScope(ShaderBinaryCache* cache, GLuint program, 
                          const std::string& source_hash)
        : cache_(cache), program_(program), source_hash_(source_hash) {
        if (cache_) {
            loaded_from_cache_ = cache_->load_shader_binary(program_, source_hash_);
        }
    }
    
    ~ShaderBinaryCacheScope() {
        if (cache_ && !loaded_from_cache_ && compilation_succeeded_) {
            cache_->save_shader_binary(program_, source_hash_);
        }
    }
    
    bool was_loaded_from_cache() const { return loaded_from_cache_; }
    void set_compilation_succeeded(bool success) { compilation_succeeded_ = success; }
    
private:
    ShaderBinaryCache* cache_;
    GLuint program_;
    std::string source_hash_;
    bool loaded_from_cache_ = false;
    bool compilation_succeeded_ = false;
};

// Memory-mapped file cache for ultra-fast access
class MemoryMappedShaderCache {
public:
    explicit MemoryMappedShaderCache(const std::string& cache_file);
    ~MemoryMappedShaderCache();
    
    bool initialize(size_t max_size);
    void shutdown();
    
    bool store_binary(const std::string& hash, const void* data, size_t size);
    bool load_binary(const std::string& hash, std::vector<uint8_t>& data);
    
    bool is_valid() const { return mapped_memory_ != nullptr; }
    
private:
    std::string cache_file_;
    void* mapped_memory_ = nullptr;
    size_t mapped_size_ = 0;
    int file_descriptor_ = -1;
    
    struct CacheHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t entry_count;
        uint32_t total_size;
    };
    
    struct CacheEntry {
        char hash[64];
        uint32_t offset;
        uint32_t size;
        uint64_t timestamp;
    };
    
    CacheHeader* header_ = nullptr;
    std::unordered_map<std::string, CacheEntry*> entry_map_;
};

// Global shader binary cache instance
extern std::unique_ptr<ShaderBinaryCache> g_shader_binary_cache;

// Initialize global shader binary cache
bool initialize_shader_binary_cache(const ShaderBinaryCacheConfig& config = ShaderBinaryCacheConfig());

// Shutdown global shader binary cache
void shutdown_shader_binary_cache();

// Convenience macros
#ifdef ENABLE_SHADER_CACHE
    #define SHADER_CACHE_SCOPE(program, vs_source, fs_source) \
        ShaderBinaryCacheScope _cache_scope(g_shader_binary_cache.get(), program, \
            ShaderBinaryCache::compute_source_hash(vs_source, fs_source))
    
    #define SHADER_CACHE_SET_SUCCESS(success) \
        _cache_scope.set_compilation_succeeded(success)
    
    #define SHADER_CACHE_WAS_LOADED() \
        _cache_scope.was_loaded_from_cache()
#else
    #define SHADER_CACHE_SCOPE(program, vs_source, fs_source) ((void)0)
    #define SHADER_CACHE_SET_SUCCESS(success) ((void)0)
    #define SHADER_CACHE_WAS_LOADED() (false)
#endif

} // namespace dx8gl