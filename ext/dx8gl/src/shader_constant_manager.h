#ifndef DX8GL_SHADER_CONSTANT_MANAGER_H
#define DX8GL_SHADER_CONSTANT_MANAGER_H

#include <vector>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <string>
#include "d3d8_types.h"
#include "gl3_headers.h"

namespace dx8gl {

// Manages shader constants (uniforms) with efficient batching
class ShaderConstantManager {
public:
    // Constant types
    enum ConstantType {
        CONSTANT_FLOAT4,    // vec4 uniforms
        CONSTANT_MATRIX4,   // mat4 uniforms
        CONSTANT_INT4,      // ivec4 uniforms
        CONSTANT_BOOL       // bool uniforms
    };
    
    // Constant metadata
    struct ConstantInfo {
        std::string name;
        ConstantType type;
        GLint location;
        int start_register;
        int register_count;
        bool dirty;
    };
    
    // Batch update entry
    struct BatchEntry {
        GLint location;
        ConstantType type;
        int count;
        const void* data;
    };
    
    ShaderConstantManager();
    ~ShaderConstantManager();
    
    // Initialize with shader program
    void init(GLuint program);
    void reset();
    
    // Register constants from shader
    void register_constant(const std::string& name, ConstantType type, 
                          int start_register, int register_count);
    
    // Set constant values (marks as dirty but doesn't upload)
    void set_float_constant(int reg, const float* values, int count = 1);
    void set_int_constant(int reg, const int* values, int count = 1);
    void set_bool_constant(int reg, const BOOL* values, int count = 1);
    void set_matrix_constant(int reg, const float* matrix, bool transpose = false);
    
    // Batch set multiple constants
    void set_float_constants(int start_reg, const float* values, int count);
    void set_matrix_constants(int start_reg, const float* matrices, int count, bool transpose = false);
    
    // Upload all dirty constants to GPU (batched)
    void upload_dirty_constants();
    
    // Force upload all constants (after shader change)
    void upload_all_constants();
    
    // Query constant info
    bool has_constant(const std::string& name) const;
    const ConstantInfo* get_constant_info(const std::string& name) const;
    
    // Performance metrics
    struct Metrics {
        size_t total_uploads;
        size_t batched_uploads;
        size_t constants_set;
        size_t bytes_uploaded;
        double upload_time_ms;
    };
    
    const Metrics& get_metrics() const { return metrics_; }
    void reset_metrics() { metrics_ = {}; }
    
private:
    // Constant storage
    struct ConstantData {
        std::vector<float> float_data;      // For FLOAT4 constants
        std::vector<int> int_data;          // For INT4 constants
        std::vector<BOOL> bool_data;        // For BOOL constants
        std::vector<uint32_t> dirty_bits;   // Bit per register
        
        void mark_dirty(int reg, int count);
        bool is_dirty(int reg) const;
        void clear_dirty();
    };
    
    // Upload helpers
    void batch_upload_floats();
    void batch_upload_ints();
    void batch_upload_bools();
    void upload_single_constant(const ConstantInfo& info);
    
    // Find contiguous dirty ranges for batching
    struct DirtyRange {
        int start;
        int count;
    };
    std::vector<DirtyRange> find_dirty_ranges(const std::vector<uint32_t>& dirty_bits, int max_reg);
    
    // Member variables
    GLuint program_;
    std::unordered_map<std::string, ConstantInfo> constants_;
    std::unordered_map<int, std::string> register_to_name_;
    
    // Constant data storage
    ConstantData float_constants_;
    ConstantData int_constants_;
    ConstantData bool_constants_;
    
    // Batching
    std::vector<BatchEntry> batch_queue_;
    static constexpr size_t MAX_BATCH_SIZE = 64;
    
    // Metrics
    mutable Metrics metrics_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

// Global constant cache for sharing common constants between shaders
class GlobalConstantCache {
public:
    static GlobalConstantCache& instance();
    
    // Register global constants (view/projection matrices, etc.)
    void register_global(const std::string& name, ShaderConstantManager::ConstantType type);
    
    // Set global constant values
    void set_global_float(const std::string& name, const float* values, int count = 1);
    void set_global_matrix(const std::string& name, const float* matrix, bool transpose = false);
    
    // Apply globals to a shader constant manager
    void apply_to_manager(ShaderConstantManager& manager);
    
private:
    GlobalConstantCache() = default;
    
    struct GlobalConstant {
        ShaderConstantManager::ConstantType type;
        std::vector<float> data;
        bool dirty;
    };
    
    std::unordered_map<std::string, GlobalConstant> globals_;
    mutable std::mutex mutex_;
};

} // namespace dx8gl

#endif // DX8GL_SHADER_CONSTANT_MANAGER_H