#include "wgsl_shader_cache.h"
#include "wgsl_shader_translator.h"
#include "logger.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef DX8GL_HAS_WEBGPU

namespace dx8gl {

// Hash function for shader source
static uint32_t hash_string(const std::string& str) {
    uint32_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// WGSLShaderCache implementation

WGSLShaderCache::WGSLShaderCache()
    : device_(nullptr)
    , max_cache_size_(1000)
    , enable_disk_cache_(false)
    , cache_directory_("shader_cache") {
    
    stats_ = {};
}

WGSLShaderCache::~WGSLShaderCache() {
    clear();
}

void WGSLShaderCache::initialize(WGpuDevice device) {
    device_ = device;
    DX8GL_INFO("WGSL shader cache initialized");
}

bool WGSLShaderCache::ShaderKey::operator==(const ShaderKey& other) const {
    return type == other.type && 
           source_hash == other.source_hash && 
           state_flags == other.state_flags;
}

size_t WGSLShaderCache::ShaderKeyHash::operator()(const ShaderKey& key) const {
    size_t h1 = std::hash<int>()(key.type);
    size_t h2 = std::hash<uint32_t>()(key.state_flags);
    size_t h3 = 0;
    for (uint8_t byte : key.source_hash) {
        h3 = h3 * 31 + byte;
    }
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

WGpuShaderModule WGSLShaderCache::get_or_compile_shader(
    const std::string& wgsl_source,
    ShaderKey::ShaderType type,
    uint32_t state_flags) {
    
    // Create cache key
    ShaderKey key;
    key.type = type;
    key.source_hash = compute_hash(wgsl_source);
    key.state_flags = state_flags;
    
    // Check cache
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        stats_.cache_hits++;
        update_access_time(key);
        return it->second->module;
    }
    
    // Cache miss - compile shader
    stats_.cache_misses++;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    WGpuShaderModule module = compile_shader_internal(wgsl_source, type);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto compile_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    stats_.total_compile_time_ms += compile_time;
    stats_.compilations++;
    
    if (module) {
        cache_shader(key, module, wgsl_source);
    }
    
    return module;
}

WGpuShaderModule WGSLShaderCache::get_cached_shader(const ShaderKey& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        stats_.cache_hits++;
        update_access_time(key);
        return it->second->module;
    }
    
    stats_.cache_misses++;
    return nullptr;
}

void WGSLShaderCache::cache_shader(const ShaderKey& key, WGpuShaderModule module, const std::string& source) {
    // Check cache size limit
    if (cache_.size() >= max_cache_size_) {
        evict_least_recently_used();
    }
    
    // Create cached shader entry
    auto cached = std::make_unique<CachedShader>();
    cached->module = module;
    cached->wgsl_source = source;
    cached->last_access_time = std::chrono::system_clock::now().time_since_epoch().count();
    cached->use_count = 1;
    
    // Update statistics
    stats_.total_shaders++;
    stats_.memory_usage += estimate_memory_usage(source);
    
    // Add to cache
    cache_[key] = std::move(cached);
    
    DX8GL_INFO("Cached WGSL shader (type=%d, flags=%u)", key.type, key.state_flags);
}

void WGSLShaderCache::clear() {
    for (auto& pair : cache_) {
        if (pair.second && pair.second->module) {
            wgpu_object_destroy(pair.second->module);
        }
    }
    cache_.clear();
    stats_ = {};
    DX8GL_INFO("WGSL shader cache cleared");
}

bool WGSLShaderCache::save_to_file(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        DX8GL_ERROR("Failed to open shader cache file for writing: %s", filename.c_str());
        return false;
    }
    
    // Write header
    uint32_t version = 1;
    uint32_t count = cache_.size();
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write cached shaders
    for (const auto& pair : cache_) {
        const ShaderKey& key = pair.first;
        const CachedShader& shader = *pair.second;
        
        // Write key
        uint32_t type = key.type;
        uint32_t hash_size = key.source_hash.size();
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));
        file.write(reinterpret_cast<const char*>(&key.state_flags), sizeof(key.state_flags));
        file.write(reinterpret_cast<const char*>(&hash_size), sizeof(hash_size));
        file.write(reinterpret_cast<const char*>(key.source_hash.data()), hash_size);
        
        // Write source
        uint32_t source_size = shader.wgsl_source.size();
        file.write(reinterpret_cast<const char*>(&source_size), sizeof(source_size));
        file.write(shader.wgsl_source.c_str(), source_size);
    }
    
    DX8GL_INFO("Saved %u shaders to cache file: %s", count, filename.c_str());
    return true;
}

bool WGSLShaderCache::load_from_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        DX8GL_INFO("Shader cache file not found: %s", filename.c_str());
        return false;
    }
    
    // Read header
    uint32_t version, count;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    if (version != 1) {
        DX8GL_WARNING("Unsupported shader cache version: %u", version);
        return false;
    }
    
    // Read cached shaders
    uint32_t loaded = 0;
    for (uint32_t i = 0; i < count; ++i) {
        // Read key
        ShaderKey key;
        uint32_t type, hash_size;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        file.read(reinterpret_cast<char*>(&key.state_flags), sizeof(key.state_flags));
        file.read(reinterpret_cast<char*>(&hash_size), sizeof(hash_size));
        
        key.type = static_cast<ShaderKey::ShaderType>(type);
        key.source_hash.resize(hash_size);
        file.read(reinterpret_cast<char*>(key.source_hash.data()), hash_size);
        
        // Read source
        uint32_t source_size;
        file.read(reinterpret_cast<char*>(&source_size), sizeof(source_size));
        
        std::string source(source_size, '\0');
        file.read(&source[0], source_size);
        
        // Recompile shader
        if (device_) {
            WGpuShaderModule module = compile_shader_internal(source, key.type);
            if (module) {
                cache_shader(key, module, source);
                loaded++;
            }
        }
    }
    
    DX8GL_INFO("Loaded %u shaders from cache file: %s", loaded, filename.c_str());
    return true;
}

void WGSLShaderCache::set_max_cache_size(size_t max_shaders) {
    max_cache_size_ = max_shaders;
    
    // Evict if necessary
    while (cache_.size() > max_cache_size_) {
        evict_least_recently_used();
    }
}

void WGSLShaderCache::evict_least_recently_used() {
    if (cache_.empty()) return;
    
    // Find least recently used shader
    auto oldest = cache_.begin();
    uint64_t oldest_time = oldest->second->last_access_time;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second->last_access_time < oldest_time) {
            oldest = it;
            oldest_time = it->second->last_access_time;
        }
    }
    
    // Destroy and remove
    if (oldest != cache_.end()) {
        if (oldest->second && oldest->second->module) {
            wgpu_object_destroy(oldest->second->module);
        }
        stats_.memory_usage -= estimate_memory_usage(oldest->second->wgsl_source);
        stats_.total_shaders--;
        cache_.erase(oldest);
    }
}

std::vector<uint8_t> WGSLShaderCache::compute_hash(const std::string& source) {
    // Simple hash for now - could use SHA256 or similar for production
    uint32_t hash = hash_string(source);
    std::vector<uint8_t> result(sizeof(hash));
    std::memcpy(result.data(), &hash, sizeof(hash));
    return result;
}

WGpuShaderModule WGSLShaderCache::compile_shader_internal(const std::string& source, ShaderKey::ShaderType type) {
    if (!device_) {
        DX8GL_ERROR("Cannot compile shader: device not initialized");
        return nullptr;
    }
    
    WGpuShaderModuleDescriptor desc = {};
    desc.label = (type == ShaderKey::VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader";
    
    WGpuShaderModuleWGSLDescriptor wgsl_desc = {};
    wgsl_desc.code = source.c_str();
    
    desc.next_in_chain = reinterpret_cast<const WGpuChainedStruct*>(&wgsl_desc);
    
    WGpuShaderModule module = wgpu_device_create_shader_module(device_, &desc);
    
    if (!module) {
        DX8GL_ERROR("Failed to compile WGSL shader");
        return nullptr;
    }
    
    return module;
}

void WGSLShaderCache::update_access_time(const ShaderKey& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second->last_access_time = std::chrono::system_clock::now().time_since_epoch().count();
        it->second->use_count++;
    }
}

size_t WGSLShaderCache::estimate_memory_usage(const std::string& source) {
    // Estimate: source size + compiled module overhead
    return source.size() + 1024; // Add 1KB overhead estimate
}

// WGSLPipelineCache implementation

WGSLPipelineCache::WGSLPipelineCache()
    : device_(nullptr) {
    stats_ = {};
}

WGSLPipelineCache::~WGSLPipelineCache() {
    clear();
}

void WGSLPipelineCache::initialize(WGpuDevice device) {
    device_ = device;
    DX8GL_INFO("WGSL pipeline cache initialized");
}

bool WGSLPipelineCache::PipelineKey::operator==(const PipelineKey& other) const {
    return vertex_shader == other.vertex_shader &&
           fragment_shader == other.fragment_shader &&
           state_hash == other.state_hash;
}

size_t WGSLPipelineCache::PipelineKeyHash::operator()(const PipelineKey& key) const {
    size_t h1 = std::hash<void*>()(key.vertex_shader);
    size_t h2 = std::hash<void*>()(key.fragment_shader);
    size_t h3 = std::hash<uint64_t>()(key.state_hash);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

WGpuRenderPipeline WGSLPipelineCache::get_or_create_render_pipeline(
    const PipelineKey& key,
    const WGpuRenderPipelineDescriptor* desc) {
    
    // Check cache
    auto it = render_cache_.find(key);
    if (it != render_cache_.end()) {
        stats_.cache_hits++;
        return it->second;
    }
    
    // Cache miss - create pipeline
    stats_.cache_misses++;
    
    if (!device_ || !desc) {
        DX8GL_ERROR("Cannot create pipeline: invalid parameters");
        return nullptr;
    }
    
    WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device_, desc);
    
    if (pipeline) {
        render_cache_[key] = pipeline;
        stats_.render_pipelines++;
        DX8GL_INFO("Created and cached render pipeline");
    }
    
    return pipeline;
}

WGpuComputePipeline WGSLPipelineCache::get_or_create_compute_pipeline(
    WGpuShaderModule compute_shader,
    const WGpuComputePipelineDescriptor* desc) {
    
    // Check cache
    auto it = compute_cache_.find(compute_shader);
    if (it != compute_cache_.end()) {
        stats_.cache_hits++;
        return it->second;
    }
    
    // Cache miss - create pipeline
    stats_.cache_misses++;
    
    if (!device_ || !desc) {
        DX8GL_ERROR("Cannot create compute pipeline: invalid parameters");
        return nullptr;
    }
    
    WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device_, desc);
    
    if (pipeline) {
        compute_cache_[compute_shader] = pipeline;
        stats_.compute_pipelines++;
        DX8GL_INFO("Created and cached compute pipeline");
    }
    
    return pipeline;
}

void WGSLPipelineCache::clear() {
    // Destroy render pipelines
    for (auto& pair : render_cache_) {
        if (pair.second) {
            wgpu_object_destroy(pair.second);
        }
    }
    render_cache_.clear();
    
    // Destroy compute pipelines
    for (auto& pair : compute_cache_) {
        if (pair.second) {
            wgpu_object_destroy(pair.second);
        }
    }
    compute_cache_.clear();
    
    stats_ = {};
    DX8GL_INFO("WGSL pipeline cache cleared");
}

// WGSLShaderManager implementation

WGSLShaderManager& WGSLShaderManager::get_instance() {
    static WGSLShaderManager instance;
    return instance;
}

WGSLShaderManager::WGSLShaderManager()
    : device_(nullptr)
    , initialized_(false) {
}

WGSLShaderManager::~WGSLShaderManager() {
    clear_all_caches();
}

void WGSLShaderManager::initialize(WGpuDevice device) {
    if (initialized_) {
        DX8GL_WARNING("WGSL shader manager already initialized");
        return;
    }
    
    device_ = device;
    shader_cache_.initialize(device);
    pipeline_cache_.initialize(device);
    
    // Try to load cached shaders
    load_caches();
    
    initialized_ = true;
    DX8GL_INFO("WGSL shader manager initialized");
}

WGpuShaderModule WGSLShaderManager::compile_vertex_shader(const std::string& wgsl) {
    return shader_cache_.get_or_compile_shader(wgsl, WGSLShaderCache::ShaderKey::VERTEX_SHADER);
}

WGpuShaderModule WGSLShaderManager::compile_fragment_shader(const std::string& wgsl) {
    return shader_cache_.get_or_compile_shader(wgsl, WGSLShaderCache::ShaderKey::FRAGMENT_SHADER);
}

WGpuShaderModule WGSLShaderManager::compile_from_dx_bytecode(
    const std::vector<uint32_t>& bytecode,
    bool is_vertex_shader) {
    
    // Create translator
    WGSLShaderTranslator translator;
    
    // Convert bytecode to assembly string (simplified - would need proper disassembly)
    std::string error;
    if (!translator.parse_shader("", error)) {
        DX8GL_ERROR("Failed to parse shader: %s", error.c_str());
        return nullptr;
    }
    
    // Generate WGSL
    std::string wgsl = translator.generate_wgsl();
    
    // Compile WGSL
    return shader_cache_.get_or_compile_shader(
        wgsl,
        is_vertex_shader ? WGSLShaderCache::ShaderKey::VERTEX_SHADER 
                        : WGSLShaderCache::ShaderKey::FRAGMENT_SHADER
    );
}

bool WGSLShaderManager::FixedFunctionKey::operator==(const FixedFunctionKey& other) const {
    return lighting_enabled == other.lighting_enabled &&
           fog_enabled == other.fog_enabled &&
           texture_stages == other.texture_stages &&
           vertex_color == other.vertex_color &&
           transform_texcoords == other.transform_texcoords;
}

size_t WGSLShaderManager::FixedFunctionKeyHash::operator()(const FixedFunctionKey& key) const {
    size_t h1 = std::hash<bool>()(key.lighting_enabled);
    size_t h2 = std::hash<bool>()(key.fog_enabled);
    size_t h3 = std::hash<uint32_t>()(key.texture_stages);
    size_t h4 = std::hash<bool>()(key.vertex_color);
    size_t h5 = std::hash<bool>()(key.transform_texcoords);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
}

std::pair<WGpuShaderModule, WGpuShaderModule> WGSLShaderManager::get_fixed_function_shaders(
    const FixedFunctionKey& key) {
    
    // Check cache
    auto it = fixed_function_cache_.find(key);
    if (it != fixed_function_cache_.end()) {
        return it->second;
    }
    
    // Generate fixed-function shaders
    int num_textures = __builtin_popcount(key.texture_stages); // Count set bits
    
    std::string vertex_wgsl = WGSLShaderTranslator::generate_fixed_function_vertex_wgsl(
        key.lighting_enabled,
        key.fog_enabled,
        num_textures,
        key.vertex_color,
        key.transform_texcoords
    );
    
    std::string fragment_wgsl = WGSLShaderTranslator::generate_fixed_function_fragment_wgsl(
        false, // alpha_test_enabled - would come from render state
        key.fog_enabled,
        num_textures,
        key.vertex_color
    );
    
    // Compile shaders
    WGpuShaderModule vertex_module = compile_vertex_shader(vertex_wgsl);
    WGpuShaderModule fragment_module = compile_fragment_shader(fragment_wgsl);
    
    // Cache the result
    auto result = std::make_pair(vertex_module, fragment_module);
    fixed_function_cache_[key] = result;
    
    return result;
}

void WGSLShaderManager::clear_all_caches() {
    shader_cache_.clear();
    pipeline_cache_.clear();
    
    // Destroy fixed-function shaders
    for (auto& pair : fixed_function_cache_) {
        if (pair.second.first) {
            wgpu_object_destroy(pair.second.first);
        }
        if (pair.second.second) {
            wgpu_object_destroy(pair.second.second);
        }
    }
    fixed_function_cache_.clear();
    
    DX8GL_INFO("All WGSL caches cleared");
}

void WGSLShaderManager::save_caches() {
    shader_cache_.save_to_file("wgsl_shader_cache.bin");
}

void WGSLShaderManager::load_caches() {
    shader_cache_.load_from_file("wgsl_shader_cache.bin");
}

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU