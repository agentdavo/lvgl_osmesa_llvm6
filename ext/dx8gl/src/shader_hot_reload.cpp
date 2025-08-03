#include "shader_hot_reload.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <poll.h>
#endif

namespace dx8gl {

// Global hot reload manager
std::unique_ptr<ShaderHotReloadManager> g_shader_hot_reload;

//////////////////////////////////////////////////////////////////////////////
// ShaderHotReloadManager Implementation
//////////////////////////////////////////////////////////////////////////////

ShaderHotReloadManager::ShaderHotReloadManager(const HotReloadConfig& config)
    : config_(config) {
}

ShaderHotReloadManager::~ShaderHotReloadManager() {
    stop();
}

bool ShaderHotReloadManager::start() {
    if (!config_.enabled) {
        DX8GL_INFO("Shader hot reload is disabled");
        return true;
    }
    
    if (watching_) {
        DX8GL_WARNING("Hot reload already started");
        return true;
    }
    
    DX8GL_INFO("Starting shader hot reload system");
    
    watching_ = true;
    watch_thread_ = std::make_unique<std::thread>(
        &ShaderHotReloadManager::watch_thread_function, this);
    
    return true;
}

void ShaderHotReloadManager::stop() {
    if (!watching_) return;
    
    DX8GL_INFO("Stopping shader hot reload system");
    
    watching_ = false;
    if (watch_thread_ && watch_thread_->joinable()) {
        watch_thread_->join();
    }
    watch_thread_.reset();
}

void ShaderHotReloadManager::register_shader(GLuint program,
                                            const std::string& name,
                                            const std::string& vertex_path,
                                            const std::string& fragment_path,
                                            std::function<void(GLuint)> reload_callback) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto info = std::make_unique<ShaderSourceInfo>();
    info->program_id = program;
    info->name = name;
    info->vertex_path = vertex_path;
    info->fragment_path = fragment_path;
    info->post_compile_callback = reload_callback;
    
    // Read initial source
    info->vertex_source = read_file(get_full_path(vertex_path));
    info->fragment_source = read_file(get_full_path(fragment_path));
    
    // Get initial modification times
    info->last_modified = std::max(
        get_file_modification_time(get_full_path(vertex_path)),
        get_file_modification_time(get_full_path(fragment_path))
    );
    
    shader_registry_[program] = std::move(info);
    
    if (config_.verbose_logging) {
        DX8GL_INFO("Registered shader '%s' (program %u) for hot reload", 
                   name.c_str(), program);
    }
}

void ShaderHotReloadManager::unregister_shader(GLuint program) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = shader_registry_.find(program);
    if (it != shader_registry_.end()) {
        if (config_.verbose_logging) {
            DX8GL_INFO("Unregistered shader '%s' (program %u) from hot reload", 
                       it->second->name.c_str(), program);
        }
        shader_registry_.erase(it);
    }
}

bool ShaderHotReloadManager::reload_shader(GLuint program) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = shader_registry_.find(program);
    if (it == shader_registry_.end()) {
        DX8GL_WARNING("Shader program %u not registered for hot reload", program);
        return false;
    }
    
    auto& info = *it->second;
    
    // Read new source
    std::string new_vertex_source = read_file(get_full_path(info.vertex_path));
    std::string new_fragment_source = read_file(get_full_path(info.fragment_path));
    
    if (new_vertex_source.empty() || new_fragment_source.empty()) {
        DX8GL_ERROR("Failed to read shader files for '%s'", info.name.c_str());
        if (global_reload_callback_) {
            global_reload_callback_(program, false, "Failed to read shader files");
        }
        return false;
    }
    
    // Update source
    info.vertex_source = new_vertex_source;
    info.fragment_source = new_fragment_source;
    
    // Compile and link
    bool success = compile_and_link_shader(info);
    
    if (success) {
        // Update modification time
        info.last_modified = std::max(
            get_file_modification_time(get_full_path(info.vertex_path)),
            get_file_modification_time(get_full_path(info.fragment_path))
        );
        
        DX8GL_INFO("Successfully reloaded shader '%s' (program %u)", 
                   info.name.c_str(), program);
        
        // Call post-compile callback
        if (info.post_compile_callback) {
            info.post_compile_callback(program);
        }
    } else {
        DX8GL_ERROR("Failed to reload shader '%s' (program %u)", 
                    info.name.c_str(), program);
    }
    
    // Call global callback
    if (global_reload_callback_) {
        global_reload_callback_(program, success, 
                               success ? "" : "Compilation failed");
    }
    
    return success;
}

bool ShaderHotReloadManager::reload_all_shaders() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    bool all_success = true;
    
    for (auto& pair : shader_registry_) {
        if (!reload_shader(pair.first)) {
            all_success = false;
        }
    }
    
    return all_success;
}

std::vector<GLuint> ShaderHotReloadManager::check_for_modifications() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<GLuint> modified_programs;
    
    for (auto& pair : shader_registry_) {
        auto& info = *pair.second;
        
        auto current_time = std::max(
            get_file_modification_time(get_full_path(info.vertex_path)),
            get_file_modification_time(get_full_path(info.fragment_path))
        );
        
        if (current_time > info.last_modified) {
            modified_programs.push_back(pair.first);
            
            if (config_.auto_reload) {
                reload_shader(pair.first);
            }
        }
    }
    
    return modified_programs;
}

void ShaderHotReloadManager::set_config(const HotReloadConfig& config) {
    bool was_watching = watching_;
    
    if (was_watching) {
        stop();
    }
    
    config_ = config;
    
    if (was_watching && config_.enabled) {
        start();
    }
}

void ShaderHotReloadManager::watch_thread_function() {
    DX8GL_DEBUG("Shader watch thread started");
    
    while (watching_) {
        // Check for modifications
        check_for_modifications();
        
        // Sleep for poll interval
        std::this_thread::sleep_for(config_.poll_interval);
    }
    
    DX8GL_DEBUG("Shader watch thread stopped");
}

std::chrono::system_clock::time_point ShaderHotReloadManager::get_file_modification_time(
    const std::string& path) const {
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0) {
        return std::chrono::system_clock::from_time_t(file_stat.st_mtime);
    }
    return std::chrono::system_clock::time_point();
}

std::string ShaderHotReloadManager::read_file(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool ShaderHotReloadManager::compile_and_link_shader(ShaderSourceInfo& info) {
    // Create new shaders
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    
    if (!vertex_shader || !fragment_shader) {
        if (vertex_shader) glDeleteShader(vertex_shader);
        if (fragment_shader) glDeleteShader(fragment_shader);
        return false;
    }
    
    // Pre-compile callback
    if (info.pre_compile_callback) {
        info.pre_compile_callback(info.vertex_source, info.fragment_source);
    }
    
    // Compile vertex shader
    const char* vs_source = info.vertex_source.c_str();
    glShaderSource(vertex_shader, 1, &vs_source, nullptr);
    glCompileShader(vertex_shader);
    
    GLint vs_status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vs_status);
    
    if (vs_status != GL_TRUE) {
        char log[1024];
        glGetShaderInfoLog(vertex_shader, sizeof(log), nullptr, log);
        DX8GL_ERROR("Vertex shader compilation failed for '%s': %s", 
                    info.name.c_str(), log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    // Compile fragment shader
    const char* fs_source = info.fragment_source.c_str();
    glShaderSource(fragment_shader, 1, &fs_source, nullptr);
    glCompileShader(fragment_shader);
    
    GLint fs_status;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fs_status);
    
    if (fs_status != GL_TRUE) {
        char log[1024];
        glGetShaderInfoLog(fragment_shader, sizeof(log), nullptr, log);
        DX8GL_ERROR("Fragment shader compilation failed for '%s': %s", 
                    info.name.c_str(), log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    // Detach old shaders from program
    GLuint attached_shaders[2];
    GLsizei count;
    glGetAttachedShaders(info.program_id, 2, &count, attached_shaders);
    
    for (GLsizei i = 0; i < count; i++) {
        glDetachShader(info.program_id, attached_shaders[i]);
        glDeleteShader(attached_shaders[i]);
    }
    
    // Attach new shaders
    glAttachShader(info.program_id, vertex_shader);
    glAttachShader(info.program_id, fragment_shader);
    
    // Link program
    glLinkProgram(info.program_id);
    
    GLint link_status;
    glGetProgramiv(info.program_id, GL_LINK_STATUS, &link_status);
    
    if (link_status != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(info.program_id, sizeof(log), nullptr, log);
        DX8GL_ERROR("Program linking failed for '%s': %s", 
                    info.name.c_str(), log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    // Mark shaders for deletion (will be deleted when program is deleted)
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

std::string ShaderHotReloadManager::get_full_path(const std::string& relative_path) const {
    if (relative_path.empty()) return "";
    
    // Check if already absolute
    if (relative_path[0] == '/') {
        return relative_path;
    }
    
    return config_.shader_directory + "/" + relative_path;
}

//////////////////////////////////////////////////////////////////////////////
// ShaderFileWatcher Implementation (Linux with inotify)
//////////////////////////////////////////////////////////////////////////////

#ifdef __linux__

ShaderFileWatcher::ShaderFileWatcher() {
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ < 0) {
        DX8GL_ERROR("Failed to initialize inotify");
    }
}

ShaderFileWatcher::~ShaderFileWatcher() {
    for (const auto& pair : watch_descriptors_) {
        inotify_rm_watch(inotify_fd_, pair.first);
    }
    
    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
    }
}

bool ShaderFileWatcher::add_watch(const std::string& path) {
    if (inotify_fd_ < 0) return false;
    
    // Check if already watching
    if (path_to_wd_.find(path) != path_to_wd_.end()) {
        return true;
    }
    
    int wd = inotify_add_watch(inotify_fd_, path.c_str(), 
                              IN_MODIFY | IN_CLOSE_WRITE);
    if (wd < 0) {
        DX8GL_ERROR("Failed to add watch for '%s'", path.c_str());
        return false;
    }
    
    watch_descriptors_[wd] = path;
    path_to_wd_[path] = wd;
    
    return true;
}

void ShaderFileWatcher::remove_watch(const std::string& path) {
    auto it = path_to_wd_.find(path);
    if (it == path_to_wd_.end()) return;
    
    int wd = it->second;
    inotify_rm_watch(inotify_fd_, wd);
    
    watch_descriptors_.erase(wd);
    path_to_wd_.erase(it);
}

std::vector<std::string> ShaderFileWatcher::check_for_changes(int timeout_ms) {
    std::vector<std::string> changed_files;
    
    if (inotify_fd_ < 0) return changed_files;
    
    struct pollfd pfd;
    pfd.fd = inotify_fd_;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) return changed_files;
    
    // Read events
    char buffer[4096];
    ssize_t len = read(inotify_fd_, buffer, sizeof(buffer));
    
    if (len <= 0) return changed_files;
    
    // Process events
    char* ptr = buffer;
    while (ptr < buffer + len) {
        struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
        
        auto it = watch_descriptors_.find(event->wd);
        if (it != watch_descriptors_.end()) {
            changed_files.push_back(it->second);
        }
        
        ptr += sizeof(struct inotify_event) + event->len;
    }
    
    // Remove duplicates
    std::sort(changed_files.begin(), changed_files.end());
    changed_files.erase(std::unique(changed_files.begin(), changed_files.end()), 
                       changed_files.end());
    
    return changed_files;
}

#endif // __linux__

//////////////////////////////////////////////////////////////////////////////
// DevelopmentShaderLoader Implementation
//////////////////////////////////////////////////////////////////////////////

DevelopmentShaderLoader::LoadResult DevelopmentShaderLoader::load_shader_with_fallback(
    const std::string& vertex_path,
    const std::string& fragment_path,
    const std::string& fallback_vertex,
    const std::string& fallback_fragment) {
    
    LoadResult result;
    result.success = false;
    result.program = 0;
    
    // Read shader sources
    std::ifstream vs_file(vertex_path);
    std::ifstream fs_file(fragment_path);
    
    std::string vertex_source;
    std::string fragment_source;
    
    if (vs_file.is_open()) {
        std::stringstream buffer;
        buffer << vs_file.rdbuf();
        vertex_source = buffer.str();
    } else {
        result.warnings.push_back("Failed to read vertex shader: " + vertex_path);
        vertex_source = fallback_vertex;
    }
    
    if (fs_file.is_open()) {
        std::stringstream buffer;
        buffer << fs_file.rdbuf();
        fragment_source = buffer.str();
    } else {
        result.warnings.push_back("Failed to read fragment shader: " + fragment_path);
        fragment_source = fallback_fragment;
    }
    
    // Validate sources
    auto vs_warnings = validate_shader_source(vertex_source, GL_VERTEX_SHADER);
    auto fs_warnings = validate_shader_source(fragment_source, GL_FRAGMENT_SHADER);
    
    result.warnings.insert(result.warnings.end(), vs_warnings.begin(), vs_warnings.end());
    result.warnings.insert(result.warnings.end(), fs_warnings.begin(), fs_warnings.end());
    
    // Create and compile shaders
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    
    const char* vs_src = vertex_source.c_str();
    const char* fs_src = fragment_source.c_str();
    
    glShaderSource(vertex_shader, 1, &vs_src, nullptr);
    glShaderSource(fragment_shader, 1, &fs_src, nullptr);
    
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);
    
    // Check compilation
    GLint vs_status, fs_status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vs_status);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fs_status);
    
    if (vs_status != GL_TRUE || fs_status != GL_TRUE) {
        char log[1024];
        
        if (vs_status != GL_TRUE) {
            glGetShaderInfoLog(vertex_shader, sizeof(log), nullptr, log);
            result.error_message += "Vertex shader error: " + std::string(log) + "\n";
        }
        
        if (fs_status != GL_TRUE) {
            glGetShaderInfoLog(fragment_shader, sizeof(log), nullptr, log);
            result.error_message += "Fragment shader error: " + std::string(log) + "\n";
        }
        
        // Try fallback
        if (!fallback_vertex.empty() && !fallback_fragment.empty()) {
            DX8GL_WARNING("Shader compilation failed, using fallback");
            
            // Generate error shader
            auto error_shader = generate_error_shader();
            vs_src = error_shader.first.c_str();
            fs_src = error_shader.second.c_str();
            
            glShaderSource(vertex_shader, 1, &vs_src, nullptr);
            glShaderSource(fragment_shader, 1, &fs_src, nullptr);
            
            glCompileShader(vertex_shader);
            glCompileShader(fragment_shader);
        }
    }
    
    // Create and link program
    result.program = glCreateProgram();
    glAttachShader(result.program, vertex_shader);
    glAttachShader(result.program, fragment_shader);
    glLinkProgram(result.program);
    
    GLint link_status;
    glGetProgramiv(result.program, GL_LINK_STATUS, &link_status);
    
    if (link_status == GL_TRUE) {
        result.success = true;
    } else {
        char log[1024];
        glGetProgramInfoLog(result.program, sizeof(log), nullptr, log);
        result.error_message += "Link error: " + std::string(log);
        
        glDeleteProgram(result.program);
        result.program = 0;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return result;
}

std::vector<std::string> DevelopmentShaderLoader::validate_shader_source(
    const std::string& source,
    GLenum shader_type) {
    
    std::vector<std::string> warnings;
    
    // Check for common issues
    if (source.empty()) {
        warnings.push_back("Shader source is empty");
        return warnings;
    }
    
    // Check for precision qualifier (required in ES)
    if (source.find("precision") == std::string::npos) {
        warnings.push_back("Missing precision qualifier (required for OpenGL ES)");
    }
    
    // Check for main function
    if (source.find("void main") == std::string::npos) {
        warnings.push_back("Missing main() function");
    }
    
    // Check vertex shader specifics
    if (shader_type == GL_VERTEX_SHADER) {
        if (source.find("gl_Position") == std::string::npos) {
            warnings.push_back("Vertex shader doesn't write to gl_Position");
        }
    }
    
    // Check fragment shader specifics
    if (shader_type == GL_FRAGMENT_SHADER) {
        if (source.find("gl_FragColor") == std::string::npos && 
            source.find("gl_FragData") == std::string::npos) {
            warnings.push_back("Fragment shader doesn't write to gl_FragColor or gl_FragData");
        }
    }
    
    return warnings;
}

std::pair<std::string, std::string> DevelopmentShaderLoader::generate_error_shader(
    const std::string& error_color) {
    
    std::string vertex_shader = R"(
precision highp float;
attribute vec3 a_position;
uniform mat4 u_mvp_matrix;
void main() {
    gl_Position = u_mvp_matrix * vec4(a_position, 1.0);
}
)";
    
    std::string fragment_shader = R"(
precision highp float;
void main() {
    gl_FragColor = vec4()" + error_color + R"();
}
)";
    
    return {vertex_shader, fragment_shader};
}

std::string DevelopmentShaderLoader::inject_debug_code(
    const std::string& source,
    GLenum shader_type,
    const std::string& debug_info) {
    
    std::stringstream result;
    
    // Add debug info as comment
    result << "// Debug info: " << debug_info << "\n";
    result << "// Injected at: " << __DATE__ << " " << __TIME__ << "\n\n";
    
    // Add debug defines
    result << "#define DEBUG_MODE 1\n";
    result << "#define DEBUG_COLOR vec4(1.0, 0.0, 1.0, 1.0)\n\n";
    
    // Insert original source
    result << source;
    
    // Add debug visualization for fragment shaders
    if (shader_type == GL_FRAGMENT_SHADER) {
        // Find main function and inject debug code
        size_t main_pos = source.find("void main");
        if (main_pos != std::string::npos) {
            size_t brace_pos = source.find("{", main_pos);
            if (brace_pos != std::string::npos) {
                result << "\n#ifdef DEBUG_MODE\n";
                result << "    // Debug visualization\n";
                result << "    if (gl_FragCoord.x < 10.0 && gl_FragCoord.y < 10.0) {\n";
                result << "        gl_FragColor = DEBUG_COLOR;\n";
                result << "        return;\n";
                result << "    }\n";
                result << "#endif\n";
            }
        }
    }
    
    return result.str();
}

//////////////////////////////////////////////////////////////////////////////
// ShaderReloadScope Implementation
//////////////////////////////////////////////////////////////////////////////

void ShaderReloadScope::save_original_source() {
    // This would need access to the shader registry to save original source
    // For now, this is a placeholder
}

void ShaderReloadScope::restore_original_source() {
    // This would restore the original source files
    // For now, this is a placeholder
}

void ShaderReloadScope::modify_vertex_source(const std::string& new_source) {
    // This would temporarily modify the vertex shader file
    // For now, this is a placeholder
    modified_ = true;
}

void ShaderReloadScope::modify_fragment_source(const std::string& new_source) {
    // This would temporarily modify the fragment shader file
    // For now, this is a placeholder
    modified_ = true;
}

//////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////

bool initialize_shader_hot_reload(const HotReloadConfig& config) {
    if (g_shader_hot_reload) {
        DX8GL_WARNING("Shader hot reload already initialized");
        return true;
    }
    
    g_shader_hot_reload = std::make_unique<ShaderHotReloadManager>(config);
    
    if (config.enabled) {
        return g_shader_hot_reload->start();
    }
    
    return true;
}

void shutdown_shader_hot_reload() {
    if (g_shader_hot_reload) {
        g_shader_hot_reload->stop();
        g_shader_hot_reload.reset();
    }
}

} // namespace dx8gl