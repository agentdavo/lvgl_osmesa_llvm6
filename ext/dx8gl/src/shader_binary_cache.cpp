#include "shader_binary_cache.h"
#include "logger.h"
#include "osmesa_gl_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
// OpenSSL and ZLIB removed - not needed for DirectX 8 to OpenGL ES translation

namespace dx8gl {

// Global shader binary cache instance
std::unique_ptr<ShaderBinaryCache> g_shader_binary_cache;

// Check if binary format is supported
static bool check_binary_format_support() {
    // In OpenGL 3.3 Core profile, use GL_ARB_get_program_binary (GL_OES_get_program_binary is OpenGL ES only)
    // Note: glGetProgramBinary is core in OpenGL 4.1+, but we check for ARB extension for 3.3 compatibility
    return has_extension("GL_ARB_get_program_binary");
}

//////////////////////////////////////////////////////////////////////////////
// ShaderBinaryCache Implementation
//////////////////////////////////////////////////////////////////////////////

ShaderBinaryCache::ShaderBinaryCache(const ShaderBinaryCacheConfig& config)
    : config_(config) {
    current_gl_version_hash_ = compute_gl_version_hash();
    current_extension_hash_ = compute_extension_hash();
}

ShaderBinaryCache::~ShaderBinaryCache() {
    shutdown();
}

bool ShaderBinaryCache::initialize() {
    DX8GL_INFO("Initializing shader binary cache");
    
    // Check if binary caching is supported
    if (!is_binary_caching_supported()) {
        DX8GL_WARNING("Shader binary caching not supported on this system");
        config_.enable_memory_cache = false;
        config_.enable_disk_cache = false;
        return false;
    }
    
    // Create cache directory if disk cache is enabled
    if (config_.enable_disk_cache) {
        if (!create_cache_directory()) {
            DX8GL_ERROR("Failed to create shader cache directory");
            config_.enable_disk_cache = false;
        } else {
            load_disk_index();
        }
    }
    
    DX8GL_INFO("Shader binary cache initialized (memory: %s, disk: %s)",
               config_.enable_memory_cache ? "enabled" : "disabled",
               config_.enable_disk_cache ? "enabled" : "disabled");
    
    return true;
}

void ShaderBinaryCache::shutdown() {
    DX8GL_INFO("Shutting down shader binary cache");
    
    // Save disk index
    if (config_.enable_disk_cache) {
        save_disk_index();
    }
    
    // Log final statistics
    auto stats = get_statistics();
    DX8GL_INFO("Cache statistics - Memory hits: %zu, misses: %zu, Disk hits: %zu, misses: %zu",
               stats.memory_cache_hits, stats.memory_cache_misses,
               stats.disk_cache_hits, stats.disk_cache_misses);
}

bool ShaderBinaryCache::save_shader_binary(GLuint program, const std::string& source_hash) {
    if (!config_.enable_memory_cache && !config_.enable_disk_cache) {
        return false;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Get binary length
    GLint binary_length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binary_length);
    
    if (binary_length <= 0) {
        DX8GL_WARNING("Program %u has no binary representation", program);
        return false;
    }
    
    // Get the binary
    auto binary = std::make_shared<CachedShaderBinary>();
    binary->binary_data.resize(binary_length);
    GLsizei actual_length = 0;
    
    glGetProgramBinary(program, binary_length, &actual_length,
                      &binary->binary_format, binary->binary_data.data());
    
    if (actual_length != binary_length) {
        DX8GL_WARNING("Binary size mismatch for program %u: expected %d, got %d",
                      program, binary_length, actual_length);
        stats_.binary_save_failures++;
        return false;
    }
    
    // Fill in metadata
    binary->source_hash = source_hash;
    binary->creation_time = std::chrono::system_clock::now();
    binary->last_access_time = binary->creation_time;
    binary->access_count = 0;
    binary->memory_size = binary->binary_data.size();
    binary->gl_version_hash = current_gl_version_hash_;
    binary->extension_hash = current_extension_hash_;
    
    bool success = false;
    
    // Save to memory cache
    if (config_.enable_memory_cache) {
        success = save_to_memory_cache(source_hash, binary);
    }
    
    // Save to disk cache
    if (config_.enable_disk_cache) {
        success = save_to_disk_cache(source_hash, *binary) || success;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    stats_.total_save_time += duration;
    
    if (success) {
        DX8GL_DEBUG("Saved shader binary for hash %s (size: %zu bytes, time: %ld μs)",
                    source_hash.c_str(), binary->binary_data.size(), duration.count());
    }
    
    return success;
}

bool ShaderBinaryCache::load_shader_binary(GLuint program, const std::string& source_hash) {
    if (!config_.enable_memory_cache && !config_.enable_disk_cache) {
        return false;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::shared_ptr<CachedShaderBinary> binary;
    
    // Try memory cache first
    if (config_.enable_memory_cache) {
        binary = load_from_memory_cache(source_hash);
        if (binary) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            stats_.memory_cache_hits++;
        }
    }
    
    // Try disk cache if not in memory
    if (!binary && config_.enable_disk_cache) {
        binary = load_from_disk_cache(source_hash);
        if (binary) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            stats_.disk_cache_hits++;
            
            // Add to memory cache for faster access next time
            if (config_.enable_memory_cache) {
                save_to_memory_cache(source_hash, binary);
            }
        }
    }
    
    if (!binary) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (config_.enable_memory_cache) stats_.memory_cache_misses++;
        if (config_.enable_disk_cache) stats_.disk_cache_misses++;
        return false;
    }
    
    // Validate binary if configured
    if (config_.validate_binaries && !validate_binary(*binary)) {
        DX8GL_WARNING("Shader binary validation failed for hash %s", source_hash.c_str());
        stats_.binary_load_failures++;
        return false;
    }
    
    // Load the binary into the program
    glProgramBinary(program, binary->binary_format,
                   binary->binary_data.data(), binary->binary_data.size());
    
    // Check if it loaded successfully
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    
    if (link_status != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        DX8GL_WARNING("Failed to load shader binary: %s", log);
        stats_.binary_load_failures++;
        return false;
    }
    
    // Update access info
    binary->last_access_time = std::chrono::system_clock::now();
    binary->access_count++;
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    stats_.total_load_time += duration;
    
    DX8GL_DEBUG("Loaded shader binary for hash %s (size: %zu bytes, time: %ld μs)",
                source_hash.c_str(), binary->binary_data.size(), duration.count());
    
    return true;
}

void ShaderBinaryCache::clear_memory_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    memory_cache_.clear();
    lru_list_.clear();
    lru_map_.clear();
    
    stats_.memory_cache_size = 0;
    stats_.memory_cache_entries = 0;
    
    DX8GL_INFO("Memory cache cleared");
}

void ShaderBinaryCache::clear_disk_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Remove all cache files
    for (const auto& pair : disk_index_) {
        std::string filepath = config_.disk_cache_directory + "/" + pair.second;
        std::remove(filepath.c_str());
    }
    
    disk_index_.clear();
    save_disk_index();
    
    stats_.disk_cache_size = 0;
    stats_.disk_cache_entries = 0;
    
    DX8GL_INFO("Disk cache cleared");
}

void ShaderBinaryCache::clear_all_caches() {
    clear_memory_cache();
    clear_disk_cache();
}

void ShaderBinaryCache::preload_shader(const std::string& source_hash) {
    if (!config_.enable_memory_cache) return;
    
    // Check if already in memory
    if (memory_cache_.find(source_hash) != memory_cache_.end()) {
        return;
    }
    
    // Try to load from disk
    if (config_.enable_disk_cache) {
        auto binary = load_from_disk_cache(source_hash);
        if (binary) {
            save_to_memory_cache(source_hash, binary);
        }
    }
}

void ShaderBinaryCache::preload_shaders(const std::vector<std::string>& source_hashes) {
    for (const auto& hash : source_hashes) {
        preload_shader(hash);
    }
}

void ShaderBinaryCache::trim_memory_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Remove entries until we're under the size limit
    while (stats_.memory_cache_size > config_.max_memory_cache_size && 
           !lru_list_.empty()) {
        evict_lru_entries();
    }
}

void ShaderBinaryCache::trim_disk_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_remove;
    
    // Check each disk entry
    for (const auto& pair : disk_index_) {
        std::string filepath = config_.disk_cache_directory + "/" + pair.second;
        
        struct stat file_stat;
        if (stat(filepath.c_str(), &file_stat) == 0) {
            auto file_time = std::chrono::system_clock::from_time_t(file_stat.st_mtime);
            auto age = now - file_time;
            
            if (age > config_.disk_cache_ttl) {
                to_remove.push_back(pair.first);
            }
        }
    }
    
    // Remove old entries
    for (const auto& hash : to_remove) {
        std::string filepath = config_.disk_cache_directory + "/" + disk_index_[hash];
        std::remove(filepath.c_str());
        disk_index_.erase(hash);
    }
    
    if (!to_remove.empty()) {
        save_disk_index();
        DX8GL_INFO("Removed %zu old entries from disk cache", to_remove.size());
    }
}

void ShaderBinaryCache::validate_cache_entries() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::vector<std::string> invalid_entries;
    
    // Validate memory cache
    for (const auto& pair : memory_cache_) {
        if (!validate_binary(*pair.second)) {
            invalid_entries.push_back(pair.first);
        }
    }
    
    // Remove invalid entries
    for (const auto& hash : invalid_entries) {
        memory_cache_.erase(hash);
        
        // Remove from LRU
        auto it = lru_map_.find(hash);
        if (it != lru_map_.end()) {
            lru_list_.erase(it->second);
            lru_map_.erase(it);
        }
    }
    
    if (!invalid_entries.empty()) {
        DX8GL_WARNING("Removed %zu invalid entries from cache", invalid_entries.size());
    }
}

CacheStatistics ShaderBinaryCache::get_statistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return stats_;
}

void ShaderBinaryCache::reset_statistics() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    stats_ = CacheStatistics();
    
    // Recalculate current sizes
    stats_.memory_cache_entries = memory_cache_.size();
    stats_.memory_cache_size = 0;
    for (const auto& pair : memory_cache_) {
        stats_.memory_cache_size += pair.second->memory_size;
    }
    
    stats_.disk_cache_entries = disk_index_.size();
}

void ShaderBinaryCache::set_config(const ShaderBinaryCacheConfig& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    config_ = config;
    
    // Apply new limits
    if (config_.enable_memory_cache && config_.use_lru_eviction) {
        trim_memory_cache();
    }
}

std::string ShaderBinaryCache::compute_source_hash(const std::string& vertex_source,
                                                  const std::string& fragment_source) {
    // Combine sources
    std::string combined = vertex_source + "\n---FRAGMENT---\n" + fragment_source;
    
    // Simple hash function - sufficient for shader caching
    std::hash<std::string> hasher;
    size_t hash_value = hasher(combined);
    std::stringstream ss;
    ss << std::hex << hash_value;
    return ss.str();
}

std::string ShaderBinaryCache::compute_bytecode_hash(const std::vector<DWORD>& vertex_bytecode,
                                                    const std::vector<DWORD>& pixel_bytecode) {
    // Combine bytecodes for hashing
    size_t total_size = vertex_bytecode.size() + pixel_bytecode.size();
    
    // Use FNV-1a hash for good distribution
    const uint64_t FNV_prime = 1099511628211ULL;
    const uint64_t FNV_offset_basis = 14695981039346656037ULL;
    
    uint64_t hash = FNV_offset_basis;
    
    // Hash vertex bytecode
    for (DWORD dword : vertex_bytecode) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&dword);
        for (int i = 0; i < 4; i++) {
            hash ^= bytes[i];
            hash *= FNV_prime;
        }
    }
    
    // Add separator
    hash ^= 0xFF;
    hash *= FNV_prime;
    
    // Hash pixel bytecode
    for (DWORD dword : pixel_bytecode) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&dword);
        for (int i = 0; i < 4; i++) {
            hash ^= bytes[i];
            hash *= FNV_prime;
        }
    }
    
    // Include shader version info in hash
    hash ^= (vertex_bytecode.empty() ? 0 : vertex_bytecode[0]); // Version token
    hash *= FNV_prime;
    hash ^= (pixel_bytecode.empty() ? 0 : pixel_bytecode[0]); // Version token
    hash *= FNV_prime;
    
    // Convert to string
    std::stringstream ss;
    ss << "dx8_" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::string ShaderBinaryCache::compute_bytecode_hash(const DWORD* bytecode, size_t dword_count) {
    if (!bytecode || dword_count == 0) {
        return "dx8_empty";
    }
    
    // Use FNV-1a hash
    const uint64_t FNV_prime = 1099511628211ULL;
    const uint64_t FNV_offset_basis = 14695981039346656037ULL;
    
    uint64_t hash = FNV_offset_basis;
    
    for (size_t i = 0; i < dword_count; i++) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&bytecode[i]);
        for (int j = 0; j < 4; j++) {
            hash ^= bytes[j];
            hash *= FNV_prime;
        }
    }
    
    // Include version token in hash
    hash ^= bytecode[0]; // First DWORD is version
    hash *= FNV_prime;
    
    std::stringstream ss;
    ss << "dx8_" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

bool ShaderBinaryCache::is_binary_caching_supported() {
    return check_binary_format_support();
}

std::vector<ShaderBinaryFormat> ShaderBinaryCache::get_supported_binary_formats() {
    std::vector<ShaderBinaryFormat> formats;
    
    if (!is_binary_caching_supported()) {
        return formats;
    }
    
    // Get number of binary formats
    GLint num_formats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num_formats);
    
    if (num_formats > 0) {
        std::vector<GLint> format_values(num_formats);
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, format_values.data());
        
        // Get vendor info
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        
        for (GLint format : format_values) {
            ShaderBinaryFormat fmt;
            fmt.format = static_cast<GLenum>(format);
            fmt.vendor = vendor ? vendor : "Unknown";
            fmt.renderer = renderer ? renderer : "Unknown";
            fmt.driver_version = version ? version : "Unknown";
            formats.push_back(fmt);
        }
    }
    
    return formats;
}

bool ShaderBinaryCache::save_to_memory_cache(const std::string& hash,
                                            std::shared_ptr<CachedShaderBinary> binary) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if we need to evict entries
    if (config_.use_lru_eviction) {
        while ((memory_cache_.size() >= config_.max_memory_entries ||
                stats_.memory_cache_size + binary->memory_size > config_.max_memory_cache_size) &&
               !lru_list_.empty()) {
            evict_lru_entries();
        }
    }
    
    // Add to cache
    memory_cache_[hash] = binary;
    stats_.memory_cache_entries = memory_cache_.size();
    stats_.memory_cache_size += binary->memory_size;
    
    // Update LRU
    if (config_.use_lru_eviction) {
        update_lru(hash);
    }
    
    return true;
}

std::shared_ptr<CachedShaderBinary> ShaderBinaryCache::load_from_memory_cache(
    const std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = memory_cache_.find(hash);
    if (it == memory_cache_.end()) {
        return nullptr;
    }
    
    // Update LRU
    if (config_.use_lru_eviction) {
        update_lru(hash);
    }
    
    return it->second;
}

bool ShaderBinaryCache::save_to_disk_cache(const std::string& hash,
                                          const CachedShaderBinary& binary) {
    std::string filename = get_cache_filename(hash);
    std::string filepath = config_.disk_cache_directory + "/" + filename;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        DX8GL_ERROR("Failed to open cache file for writing: %s", filepath.c_str());
        return false;
    }
    
    // Write header
    uint32_t magic = 0x53484442;  // "SHDB"
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Write metadata
    file.write(reinterpret_cast<const char*>(&binary.binary_format), sizeof(binary.binary_format));
    file.write(reinterpret_cast<const char*>(&binary.gl_version_hash), sizeof(binary.gl_version_hash));
    file.write(reinterpret_cast<const char*>(&binary.extension_hash), sizeof(binary.extension_hash));
    
    // Write binary data
    std::vector<uint8_t> data_to_write = binary.binary_data;
    
    if (config_.compress_disk_cache) {
        data_to_write = compress_data(binary.binary_data);
    }
    
    uint32_t data_size = data_to_write.size();
    uint8_t is_compressed = config_.compress_disk_cache ? 1 : 0;
    
    file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    file.write(reinterpret_cast<const char*>(&is_compressed), sizeof(is_compressed));
    file.write(reinterpret_cast<const char*>(data_to_write.data()), data_size);
    
    if (!file.good()) {
        DX8GL_ERROR("Failed to write cache file: %s", filepath.c_str());
        return false;
    }
    
    // Update index
    std::lock_guard<std::mutex> lock(cache_mutex_);
    disk_index_[hash] = filename;
    stats_.disk_cache_entries = disk_index_.size();
    
    return true;
}

std::shared_ptr<CachedShaderBinary> ShaderBinaryCache::load_from_disk_cache(
    const std::string& hash) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = disk_index_.find(hash);
    if (it == disk_index_.end()) {
        return nullptr;
    }
    
    std::string filepath = config_.disk_cache_directory + "/" + it->second;
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        DX8GL_WARNING("Failed to open cache file: %s", filepath.c_str());
        return nullptr;
    }
    
    // Read header
    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != 0x53484442 || version != 1) {
        DX8GL_WARNING("Invalid cache file format: %s", filepath.c_str());
        return nullptr;
    }
    
    auto binary = std::make_shared<CachedShaderBinary>();
    
    // Read metadata
    file.read(reinterpret_cast<char*>(&binary->binary_format), sizeof(binary->binary_format));
    file.read(reinterpret_cast<char*>(&binary->gl_version_hash), sizeof(binary->gl_version_hash));
    file.read(reinterpret_cast<char*>(&binary->extension_hash), sizeof(binary->extension_hash));
    
    // Read binary data
    uint32_t data_size;
    uint8_t is_compressed;
    file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
    file.read(reinterpret_cast<char*>(&is_compressed), sizeof(is_compressed));
    
    std::vector<uint8_t> file_data(data_size);
    file.read(reinterpret_cast<char*>(file_data.data()), data_size);
    
    if (!file.good()) {
        DX8GL_WARNING("Failed to read cache file: %s", filepath.c_str());
        return nullptr;
    }
    
    if (is_compressed) {
        binary->binary_data = decompress_data(file_data);
    } else {
        binary->binary_data = std::move(file_data);
    }
    
    binary->source_hash = hash;
    binary->memory_size = binary->binary_data.size();
    
    return binary;
}

void ShaderBinaryCache::evict_lru_entries() {
    if (lru_list_.empty()) return;
    
    // Remove the least recently used entry
    std::string hash = lru_list_.back();
    lru_list_.pop_back();
    
    auto it = memory_cache_.find(hash);
    if (it != memory_cache_.end()) {
        stats_.memory_cache_size -= it->second->memory_size;
        memory_cache_.erase(it);
    }
    
    lru_map_.erase(hash);
    stats_.memory_cache_entries = memory_cache_.size();
}

void ShaderBinaryCache::update_lru(const std::string& hash) {
    // Remove from current position
    auto it = lru_map_.find(hash);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
    }
    
    // Add to front
    lru_list_.push_front(hash);
    lru_map_[hash] = lru_list_.begin();
}

std::string ShaderBinaryCache::get_cache_filename(const std::string& hash) const {
    // Create subdirectory structure for better organization
    // dx8_xxxx... goes to dx8/xx/full_hash.shbin
    // other hashes go to glsl/xx/full_hash.shbin
    std::string prefix = hash.substr(0, 4);
    if (prefix == "dx8_") {
        // DirectX bytecode cache
        std::string subdir = hash.length() > 5 ? hash.substr(4, 2) : "00";
        return "dx8/" + subdir + "/" + hash + ".shbin";
    } else {
        // GLSL source cache
        std::string subdir = hash.length() > 1 ? hash.substr(0, 2) : "00";
        return "glsl/" + subdir + "/" + hash + ".shbin";
    }
}

bool ShaderBinaryCache::create_cache_directory() {
    // Create main cache directory
    if (mkdir(config_.disk_cache_directory.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    // Create subdirectories for dx8 and glsl
    std::string dx8_dir = config_.disk_cache_directory + "/dx8";
    std::string glsl_dir = config_.disk_cache_directory + "/glsl";
    
    if (mkdir(dx8_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
    }
    if (mkdir(glsl_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    // Create hex subdirectories (00-ff) for both
    for (int i = 0; i < 256; i++) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << i;
        std::string hex = ss.str();
        
        std::string dx8_subdir = dx8_dir + "/" + hex;
        std::string glsl_subdir = glsl_dir + "/" + hex;
        
        mkdir(dx8_subdir.c_str(), 0755); // Ignore errors
        mkdir(glsl_subdir.c_str(), 0755); // Ignore errors
    }
    
    return true;
}

void ShaderBinaryCache::load_disk_index() {
    std::string index_file = config_.disk_cache_directory + "/index.dat";
    std::ifstream file(index_file);
    
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find(':');
        if (sep != std::string::npos) {
            std::string hash = line.substr(0, sep);
            std::string filename = line.substr(sep + 1);
            disk_index_[hash] = filename;
        }
    }
    
    stats_.disk_cache_entries = disk_index_.size();
    DX8GL_INFO("Loaded %zu entries from disk cache index", disk_index_.size());
}

void ShaderBinaryCache::save_disk_index() {
    std::string index_file = config_.disk_cache_directory + "/index.dat";
    std::ofstream file(index_file);
    
    if (!file.is_open()) {
        DX8GL_ERROR("Failed to save disk cache index");
        return;
    }
    
    for (const auto& pair : disk_index_) {
        file << pair.first << ":" << pair.second << "\n";
    }
}

uint32_t ShaderBinaryCache::compute_gl_version_hash() const {
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    std::string combined;
    if (version) combined += version;
    if (vendor) combined += vendor;
    if (renderer) combined += renderer;
    
    // Simple hash
    uint32_t hash = 0;
    for (char c : combined) {
        hash = hash * 31 + c;
    }
    
    return hash;
}

uint32_t ShaderBinaryCache::compute_extension_hash() const {
    // Use modern OpenGL method to compute extension hash
    GLint ext_count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
    
    uint32_t hash = 0;
    
    // Hash extension count first
    hash = hash * 31 + ext_count;
    
    // Hash each extension name (limit to first 50 for performance)
    int limit = std::min(ext_count, 50);
    for (int i = 0; i < limit; i++) {
        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (ext) {
            // Hash extension name
            size_t len = strlen(ext);
            for (size_t j = 0; j < len; j++) {
                hash = hash * 31 + ext[j];
            }
        }
    }
    
    return hash;
}

bool ShaderBinaryCache::validate_binary(const CachedShaderBinary& binary) const {
    // Check version hashes
    if (binary.gl_version_hash != current_gl_version_hash_) {
        DX8GL_DEBUG("GL version hash mismatch");
        return false;
    }
    
    if (binary.extension_hash != current_extension_hash_) {
        DX8GL_DEBUG("GL extension hash mismatch");
        return false;
    }
    
    // Check if format is still supported
    GLint num_formats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num_formats);
    
    if (num_formats > 0) {
        std::vector<GLint> formats(num_formats);
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats.data());
        
        bool format_found = false;
        for (GLint fmt : formats) {
            if (static_cast<GLenum>(fmt) == binary.binary_format) {
                format_found = true;
                break;
            }
        }
        
        if (!format_found) {
            DX8GL_DEBUG("Binary format no longer supported");
            return false;
        }
    }
    
    return true;
}

std::vector<uint8_t> ShaderBinaryCache::compress_data(const std::vector<uint8_t>& data) {
    // No compression - not needed for DirectX 8 to OpenGL ES translation
    return data;
}

std::vector<uint8_t> ShaderBinaryCache::decompress_data(const std::vector<uint8_t>& compressed) {
    // No compression - not needed for DirectX 8 to OpenGL ES translation
    return compressed;
}

//////////////////////////////////////////////////////////////////////////////
// MemoryMappedShaderCache Implementation
//////////////////////////////////////////////////////////////////////////////

MemoryMappedShaderCache::MemoryMappedShaderCache(const std::string& cache_file)
    : cache_file_(cache_file) {
}

MemoryMappedShaderCache::~MemoryMappedShaderCache() {
    shutdown();
}

bool MemoryMappedShaderCache::initialize(size_t max_size) {
    // Open or create file
    file_descriptor_ = open(cache_file_.c_str(), O_RDWR | O_CREAT, 0644);
    if (file_descriptor_ < 0) {
        DX8GL_ERROR("Failed to open memory mapped cache file: %s", cache_file_.c_str());
        return false;
    }
    
    // Get file size
    struct stat file_stat;
    if (fstat(file_descriptor_, &file_stat) < 0) {
        close(file_descriptor_);
        file_descriptor_ = -1;
        return false;
    }
    
    // Resize file if needed
    if (static_cast<size_t>(file_stat.st_size) < max_size) {
        if (ftruncate(file_descriptor_, max_size) < 0) {
            close(file_descriptor_);
            file_descriptor_ = -1;
            return false;
        }
    }
    
    mapped_size_ = max_size;
    
    // Memory map the file
    mapped_memory_ = mmap(nullptr, mapped_size_, PROT_READ | PROT_WRITE,
                         MAP_SHARED, file_descriptor_, 0);
    
    if (mapped_memory_ == MAP_FAILED) {
        DX8GL_ERROR("Failed to memory map cache file");
        close(file_descriptor_);
        file_descriptor_ = -1;
        mapped_memory_ = nullptr;
        return false;
    }
    
    // Initialize header if new file
    header_ = reinterpret_cast<CacheHeader*>(mapped_memory_);
    if (header_->magic != 0x4D4D5348) {  // "MMSH"
        header_->magic = 0x4D4D5348;
        header_->version = 1;
        header_->entry_count = 0;
        header_->total_size = sizeof(CacheHeader);
    }
    
    // Build entry map
    CacheEntry* entries = reinterpret_cast<CacheEntry*>(
        static_cast<char*>(mapped_memory_) + sizeof(CacheHeader));
    
    for (uint32_t i = 0; i < header_->entry_count; i++) {
        entry_map_[std::string(entries[i].hash)] = &entries[i];
    }
    
    return true;
}

void MemoryMappedShaderCache::shutdown() {
    if (mapped_memory_) {
        munmap(mapped_memory_, mapped_size_);
        mapped_memory_ = nullptr;
    }
    
    if (file_descriptor_ >= 0) {
        close(file_descriptor_);
        file_descriptor_ = -1;
    }
}

bool MemoryMappedShaderCache::store_binary(const std::string& hash, 
                                          const void* data, size_t size) {
    if (!is_valid()) return false;
    
    // Check if we have space
    if (header_->total_size + sizeof(CacheEntry) + size > mapped_size_) {
        DX8GL_WARNING("Memory mapped cache full");
        return false;
    }
    
    // Find or create entry
    auto it = entry_map_.find(hash);
    CacheEntry* entry;
    
    if (it != entry_map_.end()) {
        entry = it->second;
        
        // Check if size has changed
        if (entry->size != size) {
            DX8GL_WARNING("Shader binary size changed for hash %s: old=%zu, new=%zu", 
                         hash.c_str(), entry->size, size);
            
            // For now, we'll refuse to update entries with different sizes
            // A more sophisticated implementation could implement defragmentation
            // or maintain a free list for reallocation
            return false;
        }
    } else {
        // Add new entry
        entry = reinterpret_cast<CacheEntry*>(
            static_cast<char*>(mapped_memory_) + sizeof(CacheHeader) +
            header_->entry_count * sizeof(CacheEntry));
        
        strncpy(entry->hash, hash.c_str(), sizeof(entry->hash) - 1);
        entry->hash[sizeof(entry->hash) - 1] = '\0';
        entry->offset = header_->total_size;
        entry->size = size;
        entry->timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        header_->entry_count++;
        entry_map_[hash] = entry;
    }
    
    // Copy data
    void* data_ptr = static_cast<char*>(mapped_memory_) + entry->offset;
    memcpy(data_ptr, data, size);
    
    header_->total_size += size;
    
    return true;
}

bool MemoryMappedShaderCache::load_binary(const std::string& hash, 
                                         std::vector<uint8_t>& data) {
    if (!is_valid()) return false;
    
    auto it = entry_map_.find(hash);
    if (it == entry_map_.end()) {
        return false;
    }
    
    CacheEntry* entry = it->second;
    
    // Copy data
    data.resize(entry->size);
    void* data_ptr = static_cast<char*>(mapped_memory_) + entry->offset;
    memcpy(data.data(), data_ptr, entry->size);
    
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////

bool initialize_shader_binary_cache(const ShaderBinaryCacheConfig& config) {
    if (g_shader_binary_cache) {
        DX8GL_WARNING("Shader binary cache already initialized");
        return true;
    }
    
    g_shader_binary_cache = std::make_unique<ShaderBinaryCache>(config);
    return g_shader_binary_cache->initialize();
}

void shutdown_shader_binary_cache() {
    if (g_shader_binary_cache) {
        g_shader_binary_cache->shutdown();
        g_shader_binary_cache.reset();
    }
}

} // namespace dx8gl