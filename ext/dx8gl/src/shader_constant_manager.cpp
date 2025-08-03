#include "shader_constant_manager.h"
#include "logger.h"
#include <chrono>
#include <cstring>
#include <algorithm>

namespace dx8gl {

// ConstantData implementation
void ShaderConstantManager::ConstantData::mark_dirty(int reg, int count) {
    int start_bit = reg;
    int end_bit = reg + count;
    
    // Ensure we have enough dirty bits
    int required_words = (end_bit + 31) / 32;
    if (dirty_bits.size() < required_words) {
        dirty_bits.resize(required_words, 0);
    }
    
    // Mark bits as dirty
    for (int i = start_bit; i < end_bit; i++) {
        int word = i / 32;
        int bit = i % 32;
        dirty_bits[word] |= (1u << bit);
    }
}

bool ShaderConstantManager::ConstantData::is_dirty(int reg) const {
    int word = reg / 32;
    int bit = reg % 32;
    
    if (word >= dirty_bits.size()) {
        return false;
    }
    
    return (dirty_bits[word] & (1u << bit)) != 0;
}

void ShaderConstantManager::ConstantData::clear_dirty() {
    std::fill(dirty_bits.begin(), dirty_bits.end(), 0);
}

// ShaderConstantManager implementation
ShaderConstantManager::ShaderConstantManager() 
    : program_(0) {
    // Pre-allocate storage for common sizes
    float_constants_.float_data.reserve(96 * 4);  // 96 float4 constants
    int_constants_.int_data.reserve(16 * 4);      // 16 int4 constants
    bool_constants_.bool_data.reserve(16);        // 16 bool constants
}

ShaderConstantManager::~ShaderConstantManager() {
}

void ShaderConstantManager::init(GLuint program) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    program_ = program;
    constants_.clear();
    register_to_name_.clear();
    
    // Query all active uniforms
    GLint uniform_count = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    
    char name_buffer[256];
    for (GLint i = 0; i < uniform_count; i++) {
        GLint size;
        GLenum type;
        glGetActiveUniform(program, i, sizeof(name_buffer), nullptr, &size, &type, name_buffer);
        
        GLint location = glGetUniformLocation(program, name_buffer);
        if (location == -1) continue;
        
        // Determine constant type and register mapping
        ConstantInfo info;
        info.name = name_buffer;
        info.location = location;
        info.dirty = true;
        
        // Parse register from name (e.g., "c0", "c1_4" for array)
        if (info.name.size() > 1 && info.name[0] == 'c') {
            size_t underscore = info.name.find('_');
            if (underscore != std::string::npos) {
                // Array constant like "c0_4"
                info.start_register = std::stoi(info.name.substr(1, underscore - 1));
                info.register_count = std::stoi(info.name.substr(underscore + 1));
            } else {
                // Single constant like "c0"
                info.start_register = std::stoi(info.name.substr(1));
                info.register_count = 1;
            }
        }
        
        // Determine type based on GL type
        switch (type) {
            case GL_FLOAT_VEC4:
                info.type = CONSTANT_FLOAT4;
                break;
            case GL_FLOAT_MAT4:
                info.type = CONSTANT_MATRIX4;
                info.register_count = 4;  // mat4 uses 4 registers
                break;
            case GL_INT_VEC4:
                info.type = CONSTANT_INT4;
                break;
            case GL_BOOL:
                info.type = CONSTANT_BOOL;
                break;
            default:
                DX8GL_WARNING("Unknown uniform type for %s", name_buffer);
                continue;
        }
        
        constants_[info.name] = info;
        
        // Map registers to names for quick lookup
        for (int r = 0; r < info.register_count; r++) {
            register_to_name_[info.start_register + r] = info.name;
        }
    }
    
    DX8GL_INFO("Initialized constant manager for program %u with %zu constants", program, constants_.size());
}

void ShaderConstantManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    program_ = 0;
    constants_.clear();
    register_to_name_.clear();
    float_constants_.clear_dirty();
    int_constants_.clear_dirty();
    bool_constants_.clear_dirty();
}

void ShaderConstantManager::register_constant(const std::string& name, ConstantType type,
                                            int start_register, int register_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (program_ == 0) return;
    
    GLint location = glGetUniformLocation(program_, name.c_str());
    if (location == -1) {
        DX8GL_WARNING("Uniform %s not found in program", name.c_str());
        return;
    }
    
    ConstantInfo info;
    info.name = name;
    info.type = type;
    info.location = location;
    info.start_register = start_register;
    info.register_count = register_count;
    info.dirty = true;
    
    constants_[name] = info;
    
    // Map registers to names
    for (int r = 0; r < register_count; r++) {
        register_to_name_[start_register + r] = name;
    }
}

void ShaderConstantManager::set_float_constant(int reg, const float* values, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Ensure storage
    size_t required_size = (reg + count) * 4;
    if (float_constants_.float_data.size() < required_size) {
        float_constants_.float_data.resize(required_size);
    }
    
    // Copy data
    memcpy(&float_constants_.float_data[reg * 4], values, count * 4 * sizeof(float));
    
    // Mark dirty
    float_constants_.mark_dirty(reg, count);
    
    metrics_.constants_set++;
}

void ShaderConstantManager::set_int_constant(int reg, const int* values, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t required_size = (reg + count) * 4;
    if (int_constants_.int_data.size() < required_size) {
        int_constants_.int_data.resize(required_size);
    }
    
    memcpy(&int_constants_.int_data[reg * 4], values, count * 4 * sizeof(int));
    int_constants_.mark_dirty(reg, count);
    
    metrics_.constants_set++;
}

void ShaderConstantManager::set_bool_constant(int reg, const BOOL* values, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t required_size = reg + count;
    if (bool_constants_.bool_data.size() < required_size) {
        bool_constants_.bool_data.resize(required_size);
    }
    
    memcpy(&bool_constants_.bool_data[reg], values, count * sizeof(BOOL));
    bool_constants_.mark_dirty(reg, count);
    
    metrics_.constants_set++;
}

void ShaderConstantManager::set_matrix_constant(int reg, const float* matrix, bool transpose) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // A 4x4 matrix uses 4 consecutive float4 registers
    size_t required_size = (reg + 4) * 4;
    if (float_constants_.float_data.size() < required_size) {
        float_constants_.float_data.resize(required_size);
    }
    
    if (transpose) {
        // Transpose while copying
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                float_constants_.float_data[(reg + row) * 4 + col] = matrix[col * 4 + row];
            }
        }
    } else {
        // Direct copy
        memcpy(&float_constants_.float_data[reg * 4], matrix, 16 * sizeof(float));
    }
    
    float_constants_.mark_dirty(reg, 4);
    metrics_.constants_set++;
}

void ShaderConstantManager::set_float_constants(int start_reg, const float* values, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t required_size = (start_reg + count) * 4;
    if (float_constants_.float_data.size() < required_size) {
        float_constants_.float_data.resize(required_size);
    }
    
    memcpy(&float_constants_.float_data[start_reg * 4], values, count * 4 * sizeof(float));
    float_constants_.mark_dirty(start_reg, count);
    
    metrics_.constants_set += count;
}

void ShaderConstantManager::set_matrix_constants(int start_reg, const float* matrices, 
                                                int count, bool transpose) {
    for (int i = 0; i < count; i++) {
        set_matrix_constant(start_reg + i * 4, matrices + i * 16, transpose);
    }
}

std::vector<ShaderConstantManager::DirtyRange> 
ShaderConstantManager::find_dirty_ranges(const std::vector<uint32_t>& dirty_bits, int max_reg) {
    std::vector<DirtyRange> ranges;
    
    int current_start = -1;
    int current_count = 0;
    
    for (int reg = 0; reg < max_reg; reg++) {
        int word = reg / 32;
        int bit = reg % 32;
        
        bool is_dirty = (word < dirty_bits.size()) && (dirty_bits[word] & (1u << bit));
        
        if (is_dirty) {
            if (current_start == -1) {
                current_start = reg;
                current_count = 1;
            } else {
                current_count++;
            }
        } else {
            if (current_start != -1) {
                ranges.push_back({current_start, current_count});
                current_start = -1;
                current_count = 0;
            }
        }
    }
    
    // Handle last range
    if (current_start != -1) {
        ranges.push_back({current_start, current_count});
    }
    
    return ranges;
}

void ShaderConstantManager::upload_dirty_constants() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (program_ == 0) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Find dirty ranges for batching
    auto float_ranges = find_dirty_ranges(float_constants_.dirty_bits, 
                                         float_constants_.float_data.size() / 4);
    
    // Upload float constants in batches
    for (const auto& range : float_ranges) {
        // Find if this range corresponds to a uniform
        auto it = register_to_name_.find(range.start);
        if (it != register_to_name_.end()) {
            auto& info = constants_[it->second];
            if (info.type == CONSTANT_FLOAT4) {
                glUniform4fv(info.location, range.count, 
                           &float_constants_.float_data[range.start * 4]);
                metrics_.batched_uploads++;
                metrics_.bytes_uploaded += range.count * 16;
            } else if (info.type == CONSTANT_MATRIX4 && range.count >= 4) {
                // Upload matrices
                int matrix_count = range.count / 4;
                glUniformMatrix4fv(info.location, matrix_count, GL_FALSE,
                                 &float_constants_.float_data[range.start * 4]);
                metrics_.batched_uploads++;
                metrics_.bytes_uploaded += matrix_count * 64;
            }
        }
    }
    
    // Clear dirty flags
    float_constants_.clear_dirty();
    int_constants_.clear_dirty();
    bool_constants_.clear_dirty();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    metrics_.upload_time_ms += duration.count() / 1000.0;
    metrics_.total_uploads++;
}

void ShaderConstantManager::upload_all_constants() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (program_ == 0) return;
    
    // Mark all constants as dirty
    for (auto& pair : constants_) {
        pair.second.dirty = true;
    }
    
    // Mark all data as dirty
    if (!float_constants_.float_data.empty()) {
        float_constants_.mark_dirty(0, float_constants_.float_data.size() / 4);
    }
    if (!int_constants_.int_data.empty()) {
        int_constants_.mark_dirty(0, int_constants_.int_data.size() / 4);
    }
    if (!bool_constants_.bool_data.empty()) {
        bool_constants_.mark_dirty(0, bool_constants_.bool_data.size());
    }
    
    // Upload everything
    upload_dirty_constants();
}

bool ShaderConstantManager::has_constant(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return constants_.find(name) != constants_.end();
}

const ShaderConstantManager::ConstantInfo* 
ShaderConstantManager::get_constant_info(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = constants_.find(name);
    if (it != constants_.end()) {
        return &it->second;
    }
    return nullptr;
}

// GlobalConstantCache implementation
GlobalConstantCache& GlobalConstantCache::instance() {
    static GlobalConstantCache instance;
    return instance;
}

void GlobalConstantCache::register_global(const std::string& name, 
                                        ShaderConstantManager::ConstantType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GlobalConstant& global = globals_[name];
    global.type = type;
    global.dirty = true;
    
    // Pre-allocate storage
    switch (type) {
        case ShaderConstantManager::CONSTANT_FLOAT4:
            global.data.resize(4);
            break;
        case ShaderConstantManager::CONSTANT_MATRIX4:
            global.data.resize(16);
            break;
        default:
            break;
    }
}

void GlobalConstantCache::set_global_float(const std::string& name, const float* values, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = globals_.find(name);
    if (it == globals_.end()) return;
    
    it->second.data.resize(count * 4);
    memcpy(it->second.data.data(), values, count * 4 * sizeof(float));
    it->second.dirty = true;
}

void GlobalConstantCache::set_global_matrix(const std::string& name, const float* matrix, bool transpose) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = globals_.find(name);
    if (it == globals_.end()) return;
    
    it->second.data.resize(16);
    
    if (transpose) {
        // Transpose while copying
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                it->second.data[row * 4 + col] = matrix[col * 4 + row];
            }
        }
    } else {
        memcpy(it->second.data.data(), matrix, 16 * sizeof(float));
    }
    
    it->second.dirty = true;
}

void GlobalConstantCache::apply_to_manager(ShaderConstantManager& manager) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [name, global] : globals_) {
        if (!global.dirty) continue;
        
        if (manager.has_constant(name)) {
            switch (global.type) {
                case ShaderConstantManager::CONSTANT_FLOAT4:
                    // Assuming global constants start at high registers (e.g., c90+)
                    manager.set_float_constant(90, global.data.data(), global.data.size() / 4);
                    break;
                case ShaderConstantManager::CONSTANT_MATRIX4:
                    manager.set_matrix_constant(90, global.data.data(), false);
                    break;
                default:
                    break;
            }
        }
    }
}

} // namespace dx8gl