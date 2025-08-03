#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>
#include <mutex>
#include "GL/gl.h"
#include "GL/glext.h"

namespace dx8gl {

// Forward declarations
class ShaderProgram;

// Hot reload configuration
struct HotReloadConfig {
    bool enabled = false;
    std::string shader_directory = "shaders";
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds(1000);
    bool auto_reload = true;
    bool verbose_logging = true;
    
    HotReloadConfig() = default;
};

// Shader source information
struct ShaderSourceInfo {
    std::string vertex_path;
    std::string fragment_path;
    std::string vertex_source;
    std::string fragment_source;
    std::chrono::system_clock::time_point last_modified;
    GLuint program_id;
    std::string name;
    
    // Optional callbacks
    std::function<void(const std::string&, const std::string&)> pre_compile_callback;
    std::function<void(GLuint)> post_compile_callback;
};

// Hot reload manager for shader development
class ShaderHotReloadManager {
public:
    explicit ShaderHotReloadManager(const HotReloadConfig& config = HotReloadConfig());
    ~ShaderHotReloadManager();
    
    // Start/stop the hot reload system
    bool start();
    void stop();
    
    // Register a shader for hot reloading
    void register_shader(GLuint program,
                        const std::string& name,
                        const std::string& vertex_path,
                        const std::string& fragment_path,
                        std::function<void(GLuint)> reload_callback = nullptr);
    
    // Unregister a shader
    void unregister_shader(GLuint program);
    
    // Manual reload
    bool reload_shader(GLuint program);
    bool reload_all_shaders();
    
    // Check for modifications (called automatically if auto_reload is enabled)
    std::vector<GLuint> check_for_modifications();
    
    // Get/set configuration
    const HotReloadConfig& get_config() const { return config_; }
    void set_config(const HotReloadConfig& config);
    
    // Enable/disable hot reload at runtime
    void set_enabled(bool enabled) { config_.enabled = enabled; }
    bool is_enabled() const { return config_.enabled; }
    
    // Callbacks for global events
    using ReloadCallback = std::function<void(GLuint, bool success, const std::string& error)>;
    void set_global_reload_callback(ReloadCallback callback) { global_reload_callback_ = callback; }
    
private:
    HotReloadConfig config_;
    std::unordered_map<GLuint, std::unique_ptr<ShaderSourceInfo>> shader_registry_;
    
    // File watching thread
    std::unique_ptr<std::thread> watch_thread_;
    std::atomic<bool> watching_{false};
    std::mutex registry_mutex_;
    
    // Callbacks
    ReloadCallback global_reload_callback_;
    
    // File watching thread function
    void watch_thread_function();
    
    // Helper functions
    std::chrono::system_clock::time_point get_file_modification_time(const std::string& path) const;
    std::string read_file(const std::string& path) const;
    bool compile_and_link_shader(ShaderSourceInfo& info);
    std::string get_full_path(const std::string& relative_path) const;
};

// Shader file watcher with inotify support (Linux)
#ifdef __linux__
class ShaderFileWatcher {
public:
    ShaderFileWatcher();
    ~ShaderFileWatcher();
    
    // Add/remove file to watch
    bool add_watch(const std::string& path);
    void remove_watch(const std::string& path);
    
    // Check for file changes (blocking with timeout)
    std::vector<std::string> check_for_changes(int timeout_ms = 100);
    
private:
    int inotify_fd_ = -1;
    std::unordered_map<int, std::string> watch_descriptors_;
    std::unordered_map<std::string, int> path_to_wd_;
};
#endif

// Development shader loader with error recovery
class DevelopmentShaderLoader {
public:
    struct LoadResult {
        bool success;
        GLuint program;
        std::string error_message;
        std::vector<std::string> warnings;
    };
    
    // Load shader with fallback support
    static LoadResult load_shader_with_fallback(
        const std::string& vertex_path,
        const std::string& fragment_path,
        const std::string& fallback_vertex = "",
        const std::string& fallback_fragment = "");
    
    // Validate shader before compilation
    static std::vector<std::string> validate_shader_source(
        const std::string& source,
        GLenum shader_type);
    
    // Generate error shader (solid color for debugging)
    static std::pair<std::string, std::string> generate_error_shader(
        const std::string& error_color = "1.0, 0.0, 1.0, 1.0");
    
    // Inject debug code into shader
    static std::string inject_debug_code(
        const std::string& source,
        GLenum shader_type,
        const std::string& debug_info);
};

// RAII shader reload scope for testing
class ShaderReloadScope {
public:
    ShaderReloadScope(ShaderHotReloadManager* manager, GLuint program)
        : manager_(manager), program_(program), original_vertex_source_(), original_fragment_source_() {
        // Save original source for restoration
        save_original_source();
    }
    
    ~ShaderReloadScope() {
        // Restore original source if modified
        if (modified_) {
            restore_original_source();
        }
    }
    
    // Temporarily modify shader source for testing
    void modify_vertex_source(const std::string& new_source);
    void modify_fragment_source(const std::string& new_source);
    
    // Trigger reload
    bool reload() {
        return manager_->reload_shader(program_);
    }
    
private:
    ShaderHotReloadManager* manager_;
    GLuint program_;
    std::string original_vertex_source_;
    std::string original_fragment_source_;
    bool modified_ = false;
    
    void save_original_source();
    void restore_original_source();
};

// Global hot reload manager
extern std::unique_ptr<ShaderHotReloadManager> g_shader_hot_reload;

// Initialize global hot reload manager
bool initialize_shader_hot_reload(const HotReloadConfig& config = HotReloadConfig());

// Shutdown global hot reload manager
void shutdown_shader_hot_reload();

// Convenience macros for development builds
#ifdef ENABLE_HOT_RELOAD
    #define REGISTER_SHADER_FOR_RELOAD(program, name, vs_path, fs_path) \
        if (g_shader_hot_reload) g_shader_hot_reload->register_shader(program, name, vs_path, fs_path)
    
    #define UNREGISTER_SHADER_FOR_RELOAD(program) \
        if (g_shader_hot_reload) g_shader_hot_reload->unregister_shader(program)
    
    #define CHECK_SHADER_RELOAD() \
        if (g_shader_hot_reload && g_shader_hot_reload->is_enabled()) \
            g_shader_hot_reload->check_for_modifications()
#else
    #define REGISTER_SHADER_FOR_RELOAD(program, name, vs_path, fs_path) ((void)0)
    #define UNREGISTER_SHADER_FOR_RELOAD(program) ((void)0)
    #define CHECK_SHADER_RELOAD() ((void)0)
#endif

} // namespace dx8gl