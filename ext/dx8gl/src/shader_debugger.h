#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <fstream>
#include <atomic>
#include <mutex>
#include "GL/gl.h"
#include "GL/glext.h"

namespace dx8gl {

// Forward declarations
class ShaderProgram;

// Shader profiling data
struct ShaderProfileData {
    size_t compile_count = 0;
    size_t use_count = 0;
    std::chrono::microseconds total_compile_time{0};
    std::chrono::microseconds total_bind_time{0};
    std::chrono::microseconds average_compile_time{0};
    std::chrono::microseconds average_bind_time{0};
    size_t vertex_shader_size = 0;
    size_t fragment_shader_size = 0;
    std::string vertex_source;
    std::string fragment_source;
    std::string compile_log;
    bool has_errors = false;
};

// Shader debugging and profiling system
class ShaderDebugger {
public:
    struct Config {
        bool enable_profiling;
        bool dump_shaders;
        bool dump_on_error;
        bool enable_hot_reload;
        bool enable_timing;
        std::string dump_directory;
        size_t max_profile_entries;
        
        Config() : enable_profiling(true), dump_shaders(false), dump_on_error(true), 
                  enable_hot_reload(false), enable_timing(true), dump_directory("shader_dumps"), 
                  max_profile_entries(10000) {}
    };
    
    ShaderDebugger();
    explicit ShaderDebugger(const Config& config);
    ~ShaderDebugger();
    
    // Initialize the debugger
    bool initialize();
    
    // Shutdown and dump final statistics
    void shutdown();
    
    // Shader compilation tracking
    void begin_shader_compile(GLuint shader, GLenum type, const std::string& source);
    void end_shader_compile(GLuint shader, bool success, const std::string& log);
    
    // Program linking tracking
    void begin_program_link(GLuint program);
    void end_program_link(GLuint program, bool success, const std::string& log);
    
    // Shader usage tracking
    void on_shader_bind(GLuint program);
    void on_shader_unbind(GLuint program);
    
    // Dump shader source to file
    void dump_shader(const std::string& name, 
                     const std::string& vertex_source,
                     const std::string& fragment_source,
                     const std::string& info = "");
    
    // Get profiling data for a shader
    const ShaderProfileData* get_profile_data(GLuint program) const;
    
    // Get all profiling data
    std::vector<std::pair<GLuint, ShaderProfileData>> get_all_profiles() const;
    
    // Generate profiling report
    std::string generate_profile_report() const;
    
    // Hot reload support
    void mark_shader_for_reload(GLuint program, const std::string& vertex_file,
                               const std::string& fragment_file);
    bool check_and_reload_shaders();
    
    // Enable/disable features at runtime
    void set_profiling_enabled(bool enabled) { config_.enable_profiling = enabled; }
    void set_dump_enabled(bool enabled) { config_.dump_shaders = enabled; }
    
    // Clear profiling data
    void clear_profile_data();
    
private:
    Config config_;
    
    // Profiling data storage
    mutable std::mutex profile_mutex_;
    std::unordered_map<GLuint, ShaderProfileData> profile_data_;
    
    // Temporary compilation tracking
    struct CompileInfo {
        GLenum type;
        std::string source;
        std::chrono::steady_clock::time_point start_time;
    };
    std::unordered_map<GLuint, CompileInfo> active_compiles_;
    
    // Hot reload tracking
    struct ReloadInfo {
        std::string vertex_file;
        std::string fragment_file;
        std::chrono::system_clock::time_point last_modified;
    };
    std::unordered_map<GLuint, ReloadInfo> reload_info_;
    
    // Statistics
    std::atomic<size_t> total_shaders_compiled_{0};
    std::atomic<size_t> total_compile_errors_{0};
    std::atomic<size_t> total_link_errors_{0};
    
    // Helper methods
    std::string get_shader_filename(const std::string& name, const std::string& type) const;
    void create_dump_directory();
    std::chrono::system_clock::time_point get_file_modification_time(const std::string& path) const;
    std::string read_file(const std::string& path) const;
};

// Shader dump formatter
class ShaderDumpFormatter {
public:
    // Format shader source with line numbers and syntax highlighting hints
    static std::string format_shader_source(const std::string& source,
                                           const std::string& error_log = "");
    
    // Generate HTML dump with syntax highlighting
    static std::string generate_html_dump(const std::string& vertex_source,
                                         const std::string& fragment_source,
                                         const ShaderProfileData& profile_data);
    
    // Generate markdown dump
    static std::string generate_markdown_dump(const std::string& vertex_source,
                                             const std::string& fragment_source,
                                             const ShaderProfileData& profile_data);
private:
    // Parse error log and extract line numbers
    static std::vector<int> parse_error_lines(const std::string& error_log);
    
    // Add line numbers to source
    static std::string add_line_numbers(const std::string& source,
                                       const std::vector<int>& error_lines = {});
};

// Shader performance analyzer
class ShaderPerformanceAnalyzer {
public:
    struct Analysis {
        // Complexity metrics
        size_t instruction_count = 0;
        size_t texture_lookups = 0;
        size_t dependent_texture_lookups = 0;
        size_t arithmetic_operations = 0;
        size_t transcendental_operations = 0; // sin, cos, exp, etc.
        
        // Register usage
        size_t temp_registers_used = 0;
        size_t uniform_slots_used = 0;
        size_t attribute_slots_used = 0;
        size_t varying_slots_used = 0;
        
        // Performance hints
        std::vector<std::string> optimization_hints;
        std::vector<std::string> potential_issues;
        
        // Estimated cycles (very rough approximation)
        size_t estimated_vertex_cycles = 0;
        size_t estimated_fragment_cycles = 0;
    };
    
    // Analyze shader source and provide performance metrics
    static Analysis analyze_shader(const std::string& vertex_source,
                                  const std::string& fragment_source);
    
    // Generate performance report
    static std::string generate_performance_report(const Analysis& analysis);
    
private:
    // Analysis helpers
    static size_t count_instructions(const std::string& source);
    static size_t count_texture_lookups(const std::string& source);
    static size_t count_arithmetic_ops(const std::string& source);
    static size_t count_transcendental_ops(const std::string& source);
    static void analyze_register_usage(const std::string& source, Analysis& analysis);
    static void generate_optimization_hints(const Analysis& analysis, 
                                          std::vector<std::string>& hints);
};

// RAII shader profiling scope
class ShaderProfileScope {
public:
    ShaderProfileScope(ShaderDebugger* debugger, GLuint program)
        : debugger_(debugger), program_(program), start_time_(std::chrono::steady_clock::now()) {
        if (debugger_) {
            debugger_->on_shader_bind(program_);
        }
    }
    
    ~ShaderProfileScope() {
        if (debugger_) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time_);
            // Could track usage time here if needed
            debugger_->on_shader_unbind(program_);
        }
    }
    
private:
    ShaderDebugger* debugger_;
    GLuint program_;
    std::chrono::steady_clock::time_point start_time_;
};

// Global shader debugger instance
extern std::unique_ptr<ShaderDebugger> g_shader_debugger;

// Initialize global shader debugger
bool initialize_shader_debugger();
bool initialize_shader_debugger(const ShaderDebugger::Config& config);

// Shutdown global shader debugger
void shutdown_shader_debugger();

// Convenience macros for shader debugging
#ifdef DEBUG_SHADERS
    #define SHADER_COMPILE_BEGIN(shader, type, source) \
        if (g_shader_debugger) g_shader_debugger->begin_shader_compile(shader, type, source)
    
    #define SHADER_COMPILE_END(shader, success, log) \
        if (g_shader_debugger) g_shader_debugger->end_shader_compile(shader, success, log)
    
    #define SHADER_BIND(program) \
        ShaderProfileScope _shader_scope(g_shader_debugger.get(), program)
    
    #define SHADER_DUMP(name, vs, fs, info) \
        if (g_shader_debugger) g_shader_debugger->dump_shader(name, vs, fs, info)
#else
    #define SHADER_COMPILE_BEGIN(shader, type, source) ((void)0)
    #define SHADER_COMPILE_END(shader, success, log) ((void)0)
    #define SHADER_BIND(program) ((void)0)
    #define SHADER_DUMP(name, vs, fs, info) ((void)0)
#endif

} // namespace dx8gl