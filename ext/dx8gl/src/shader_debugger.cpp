#include "shader_debugger.h"
#include "logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <sys/stat.h>

namespace dx8gl {

// Global shader debugger instance
std::unique_ptr<ShaderDebugger> g_shader_debugger;

//////////////////////////////////////////////////////////////////////////////
// ShaderDebugger Implementation
//////////////////////////////////////////////////////////////////////////////

ShaderDebugger::ShaderDebugger()
    : config_(Config()) {
}

ShaderDebugger::ShaderDebugger(const Config& config)
    : config_(config) {
}

ShaderDebugger::~ShaderDebugger() {
    shutdown();
}

bool ShaderDebugger::initialize() {
    DX8GL_INFO("Initializing shader debugger");
    
    if (config_.dump_shaders || config_.dump_on_error) {
        create_dump_directory();
    }
    
    return true;
}

void ShaderDebugger::shutdown() {
    DX8GL_INFO("Shutting down shader debugger");
    
    // Generate final report
    if (config_.enable_profiling) {
        std::string report = generate_profile_report();
        
        // Save report to file
        std::string report_file = config_.dump_directory + "/shader_profile_report.txt";
        std::ofstream file(report_file);
        if (file.is_open()) {
            file << report;
            DX8GL_INFO("Shader profile report saved to: %s", report_file.c_str());
        }
    }
}

void ShaderDebugger::begin_shader_compile(GLuint shader, GLenum type, const std::string& source) {
    if (!config_.enable_profiling) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    CompileInfo info;
    info.type = type;
    info.source = source;
    info.start_time = std::chrono::steady_clock::now();
    
    active_compiles_[shader] = info;
}

void ShaderDebugger::end_shader_compile(GLuint shader, bool success, const std::string& log) {
    if (!config_.enable_profiling) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    auto it = active_compiles_.find(shader);
    if (it == active_compiles_.end()) return;
    
    auto end_time = std::chrono::steady_clock::now();
    auto compile_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - it->second.start_time);
    
    total_shaders_compiled_++;
    if (!success) {
        total_compile_errors_++;
        
        // Dump on error if enabled
        if (config_.dump_on_error) {
            std::string name = "error_shader_" + std::to_string(shader);
            dump_shader(name, 
                       it->second.type == GL_VERTEX_SHADER ? it->second.source : "",
                       it->second.type == GL_FRAGMENT_SHADER ? it->second.source : "",
                       "Compilation Error:\n" + log);
        }
    }
    
    active_compiles_.erase(it);
}

void ShaderDebugger::begin_program_link(GLuint program) {
    if (!config_.enable_profiling) return;
    
    // Create profile entry if it doesn't exist
    std::lock_guard<std::mutex> lock(profile_mutex_);
    if (profile_data_.find(program) == profile_data_.end()) {
        profile_data_[program] = ShaderProfileData();
    }
}

void ShaderDebugger::end_program_link(GLuint program, bool success, const std::string& log) {
    if (!config_.enable_profiling) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    auto it = profile_data_.find(program);
    if (it != profile_data_.end()) {
        if (!success) {
            it->second.has_errors = true;
            it->second.compile_log = log;
            total_link_errors_++;
        }
    }
}

void ShaderDebugger::on_shader_bind(GLuint program) {
    if (!config_.enable_profiling) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    auto it = profile_data_.find(program);
    if (it != profile_data_.end()) {
        it->second.use_count++;
    }
}

void ShaderDebugger::on_shader_unbind(GLuint program) {
    // Currently just a placeholder for future use
}

void ShaderDebugger::dump_shader(const std::string& name, 
                                const std::string& vertex_source,
                                const std::string& fragment_source,
                                const std::string& info) {
    // Create timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp;
    timestamp << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    std::string base_name = config_.dump_directory + "/" + name + "_" + timestamp.str();
    
    // Dump vertex shader
    if (!vertex_source.empty()) {
        std::string vs_file = base_name + ".vert";
        std::ofstream file(vs_file);
        if (file.is_open()) {
            file << "// Vertex Shader: " << name << "\n";
            file << "// Generated: " << timestamp.str() << "\n";
            if (!info.empty()) {
                file << "// Info: " << info << "\n";
            }
            file << "\n" << vertex_source;
        }
    }
    
    // Dump fragment shader
    if (!fragment_source.empty()) {
        std::string fs_file = base_name + ".frag";
        std::ofstream file(fs_file);
        if (file.is_open()) {
            file << "// Fragment Shader: " << name << "\n";
            file << "// Generated: " << timestamp.str() << "\n";
            if (!info.empty()) {
                file << "// Info: " << info << "\n";
            }
            file << "\n" << fragment_source;
        }
    }
    
    // Generate HTML dump with both shaders
    if (!vertex_source.empty() && !fragment_source.empty()) {
        ShaderProfileData dummy_profile;
        dummy_profile.vertex_source = vertex_source;
        dummy_profile.fragment_source = fragment_source;
        
        std::string html_content = ShaderDumpFormatter::generate_html_dump(
            vertex_source, fragment_source, dummy_profile);
        
        std::string html_file = base_name + ".html";
        std::ofstream file(html_file);
        if (file.is_open()) {
            file << html_content;
        }
    }
}

const ShaderProfileData* ShaderDebugger::get_profile_data(GLuint program) const {
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    auto it = profile_data_.find(program);
    if (it != profile_data_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::pair<GLuint, ShaderProfileData>> ShaderDebugger::get_all_profiles() const {
    std::lock_guard<std::mutex> lock(profile_mutex_);
    
    std::vector<std::pair<GLuint, ShaderProfileData>> result;
    for (const auto& pair : profile_data_) {
        result.push_back(pair);
    }
    
    // Sort by use count (most used first)
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            return a.second.use_count > b.second.use_count;
        });
    
    return result;
}

std::string ShaderDebugger::generate_profile_report() const {
    std::ostringstream report;
    
    report << "=== Shader Profiling Report ===\n\n";
    report << "Total shaders compiled: " << total_shaders_compiled_ << "\n";
    report << "Compilation errors: " << total_compile_errors_ << "\n";
    report << "Link errors: " << total_link_errors_ << "\n";
    report << "\n";
    
    auto profiles = get_all_profiles();
    
    report << "Top 10 Most Used Shaders:\n";
    report << std::left << std::setw(10) << "Program" 
           << std::setw(15) << "Use Count"
           << std::setw(20) << "Avg Compile Time"
           << std::setw(15) << "VS Size"
           << std::setw(15) << "FS Size"
           << "\n";
    report << std::string(75, '-') << "\n";
    
    int count = 0;
    for (const auto& pair : profiles) {
        if (count++ >= 10) break;
        
        const auto& profile = pair.second;
        report << std::left << std::setw(10) << pair.first
               << std::setw(15) << profile.use_count
               << std::setw(20) << profile.average_compile_time.count() << " μs"
               << std::setw(15) << profile.vertex_shader_size
               << std::setw(15) << profile.fragment_shader_size
               << "\n";
    }
    
    // Performance analysis for top shaders
    report << "\n\nPerformance Analysis:\n";
    count = 0;
    for (const auto& pair : profiles) {
        if (count++ >= 5) break;
        
        const auto& profile = pair.second;
        if (!profile.vertex_source.empty() && !profile.fragment_source.empty()) {
            auto analysis = ShaderPerformanceAnalyzer::analyze_shader(
                profile.vertex_source, profile.fragment_source);
            
            report << "\nProgram " << pair.first << ":\n";
            report << ShaderPerformanceAnalyzer::generate_performance_report(analysis);
        }
    }
    
    return report.str();
}

void ShaderDebugger::mark_shader_for_reload(GLuint program, const std::string& vertex_file,
                                           const std::string& fragment_file) {
    if (!config_.enable_hot_reload) return;
    
    ReloadInfo info;
    info.vertex_file = vertex_file;
    info.fragment_file = fragment_file;
    info.last_modified = std::max(
        get_file_modification_time(vertex_file),
        get_file_modification_time(fragment_file)
    );
    
    reload_info_[program] = info;
}

bool ShaderDebugger::check_and_reload_shaders() {
    if (!config_.enable_hot_reload) return false;
    
    bool any_reloaded = false;
    
    for (auto& pair : reload_info_) {
        GLuint program = pair.first;
        ReloadInfo& info = pair.second;
        
        auto current_time = std::max(
            get_file_modification_time(info.vertex_file),
            get_file_modification_time(info.fragment_file)
        );
        
        if (current_time > info.last_modified) {
            // File has been modified, reload needed
            DX8GL_INFO("Hot reloading shader program %u", program);
            
            // Note: Actual reloading would need to be implemented
            // by the shader system, this just detects changes
            
            info.last_modified = current_time;
            any_reloaded = true;
        }
    }
    
    return any_reloaded;
}

void ShaderDebugger::clear_profile_data() {
    std::lock_guard<std::mutex> lock(profile_mutex_);
    profile_data_.clear();
}

void ShaderDebugger::create_dump_directory() {
    mkdir(config_.dump_directory.c_str(), 0755);
}

std::chrono::system_clock::time_point ShaderDebugger::get_file_modification_time(
    const std::string& path) const {
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0) {
        return std::chrono::system_clock::from_time_t(file_stat.st_mtime);
    }
    return std::chrono::system_clock::time_point();
}

std::string ShaderDebugger::read_file(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//////////////////////////////////////////////////////////////////////////////
// ShaderDumpFormatter Implementation
//////////////////////////////////////////////////////////////////////////////

std::string ShaderDumpFormatter::format_shader_source(const std::string& source,
                                                     const std::string& error_log) {
    auto error_lines = parse_error_lines(error_log);
    return add_line_numbers(source, error_lines);
}

std::string ShaderDumpFormatter::generate_html_dump(const std::string& vertex_source,
                                                   const std::string& fragment_source,
                                                   const ShaderProfileData& profile_data) {
    std::ostringstream html;
    
    html << R"(<!DOCTYPE html>
<html>
<head>
    <title>Shader Dump</title>
    <style>
        body { font-family: monospace; background: #1e1e1e; color: #d4d4d4; }
        .container { display: flex; gap: 20px; }
        .shader-panel { flex: 1; }
        .shader-header { 
            background: #2d2d2d; 
            padding: 10px; 
            border-bottom: 2px solid #0e7490;
            font-weight: bold;
        }
        .shader-source { 
            background: #1e1e1e; 
            padding: 10px; 
            overflow-x: auto;
            border: 1px solid #2d2d2d;
        }
        .line-number { 
            color: #858585; 
            margin-right: 10px; 
            display: inline-block;
            width: 40px;
            text-align: right;
        }
        .error-line { background: #562626; }
        .keyword { color: #569cd6; }
        .type { color: #4ec9b0; }
        .number { color: #b5cea8; }
        .function { color: #dcdcaa; }
        .comment { color: #6a9955; }
        .stats { 
            background: #2d2d2d; 
            padding: 10px; 
            margin-bottom: 20px;
            border: 1px solid #444;
        }
    </style>
</head>
<body>
)";
    
    // Add statistics if available
    if (profile_data.use_count > 0) {
        html << R"(<div class="stats">)";
        html << "<h3>Shader Statistics</h3>";
        html << "<p>Use Count: " << profile_data.use_count << "</p>";
        html << "<p>Compile Time: " << profile_data.average_compile_time.count() << " μs</p>";
        html << "<p>Vertex Shader Size: " << profile_data.vertex_shader_size << " bytes</p>";
        html << "<p>Fragment Shader Size: " << profile_data.fragment_shader_size << " bytes</p>";
        html << "</div>";
    }
    
    html << R"(<div class="container">)";
    
    // Vertex shader panel
    html << R"(<div class="shader-panel">)";
    html << R"(<div class="shader-header">Vertex Shader</div>)";
    html << R"(<div class="shader-source">)";
    
    // Add syntax highlighted vertex shader
    std::istringstream vs_stream(vertex_source);
    std::string line;
    int line_num = 1;
    while (std::getline(vs_stream, line)) {
        html << "<div>";
        html << "<span class=\"line-number\">" << line_num++ << "</span>";
        
        // Basic syntax highlighting
        line = std::regex_replace(line, std::regex(R"(\b(attribute|uniform|varying|void|float|vec2|vec3|vec4|mat2|mat3|mat4|sampler2D|if|else|for|while|return)\b)"), 
                                 "<span class=\"keyword\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(\b(gl_Position|gl_FragColor|gl_FragData)\b)"), 
                                 "<span class=\"type\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(\b(\d+\.?\d*)\b)"), 
                                 "<span class=\"number\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(//.*$)"), 
                                 "<span class=\"comment\">$0</span>");
        
        html << line;
        html << "</div>";
    }
    
    html << "</div></div>";
    
    // Fragment shader panel
    html << R"(<div class="shader-panel">)";
    html << R"(<div class="shader-header">Fragment Shader</div>)";
    html << R"(<div class="shader-source">)";
    
    std::istringstream fs_stream(fragment_source);
    line_num = 1;
    while (std::getline(fs_stream, line)) {
        html << "<div>";
        html << "<span class=\"line-number\">" << line_num++ << "</span>";
        
        // Basic syntax highlighting (same as vertex shader)
        line = std::regex_replace(line, std::regex(R"(\b(attribute|uniform|varying|void|float|vec2|vec3|vec4|mat2|mat3|mat4|sampler2D|if|else|for|while|return)\b)"), 
                                 "<span class=\"keyword\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(\b(gl_Position|gl_FragColor|gl_FragData)\b)"), 
                                 "<span class=\"type\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(\b(\d+\.?\d*)\b)"), 
                                 "<span class=\"number\">$1</span>");
        line = std::regex_replace(line, std::regex(R"(//.*$)"), 
                                 "<span class=\"comment\">$0</span>");
        
        html << line;
        html << "</div>";
    }
    
    html << "</div></div>";
    html << "</div></body></html>";
    
    return html.str();
}

std::string ShaderDumpFormatter::generate_markdown_dump(const std::string& vertex_source,
                                                       const std::string& fragment_source,
                                                       const ShaderProfileData& profile_data) {
    std::ostringstream md;
    
    md << "# Shader Dump\n\n";
    
    if (profile_data.use_count > 0) {
        md << "## Statistics\n\n";
        md << "- **Use Count**: " << profile_data.use_count << "\n";
        md << "- **Compile Time**: " << profile_data.average_compile_time.count() << " μs\n";
        md << "- **Vertex Shader Size**: " << profile_data.vertex_shader_size << " bytes\n";
        md << "- **Fragment Shader Size**: " << profile_data.fragment_shader_size << " bytes\n\n";
    }
    
    md << "## Vertex Shader\n\n";
    md << "```glsl\n" << vertex_source << "\n```\n\n";
    
    md << "## Fragment Shader\n\n";
    md << "```glsl\n" << fragment_source << "\n```\n";
    
    return md.str();
}

std::vector<int> ShaderDumpFormatter::parse_error_lines(const std::string& error_log) {
    std::vector<int> error_lines;
    
    // Parse common error formats
    // Example: "0:15(2): error: syntax error"
    std::regex error_regex(R"((\d+):(\d+)(?:\((\d+)\))?: (?:error|ERROR))");
    
    std::sregex_iterator it(error_log.begin(), error_log.end(), error_regex);
    std::sregex_iterator end;
    
    while (it != end) {
        int line = std::stoi((*it)[2]);
        error_lines.push_back(line);
        ++it;
    }
    
    // Remove duplicates
    std::sort(error_lines.begin(), error_lines.end());
    error_lines.erase(std::unique(error_lines.begin(), error_lines.end()), error_lines.end());
    
    return error_lines;
}

std::string ShaderDumpFormatter::add_line_numbers(const std::string& source,
                                                 const std::vector<int>& error_lines) {
    std::ostringstream result;
    std::istringstream stream(source);
    std::string line;
    int line_num = 1;
    
    while (std::getline(stream, line)) {
        bool is_error_line = std::find(error_lines.begin(), error_lines.end(), line_num) 
                            != error_lines.end();
        
        if (is_error_line) {
            result << ">>> ";
        } else {
            result << "    ";
        }
        
        result << std::setw(4) << line_num << ": " << line << "\n";
        line_num++;
    }
    
    return result.str();
}

//////////////////////////////////////////////////////////////////////////////
// ShaderPerformanceAnalyzer Implementation
//////////////////////////////////////////////////////////////////////////////

ShaderPerformanceAnalyzer::Analysis ShaderPerformanceAnalyzer::analyze_shader(
    const std::string& vertex_source,
    const std::string& fragment_source) {
    
    Analysis analysis;
    
    // Analyze vertex shader
    analysis.instruction_count += count_instructions(vertex_source);
    analysis.arithmetic_operations += count_arithmetic_ops(vertex_source);
    analysis.transcendental_operations += count_transcendental_ops(vertex_source);
    analyze_register_usage(vertex_source, analysis);
    
    // Analyze fragment shader
    analysis.instruction_count += count_instructions(fragment_source);
    analysis.texture_lookups = count_texture_lookups(fragment_source);
    analysis.arithmetic_operations += count_arithmetic_ops(fragment_source);
    analysis.transcendental_operations += count_transcendental_ops(fragment_source);
    analyze_register_usage(fragment_source, analysis);
    
    // Estimate cycles (very rough approximation)
    analysis.estimated_vertex_cycles = analysis.arithmetic_operations * 1 + 
                                      analysis.transcendental_operations * 4;
    analysis.estimated_fragment_cycles = analysis.arithmetic_operations * 1 + 
                                        analysis.texture_lookups * 4 +
                                        analysis.transcendental_operations * 8;
    
    // Generate optimization hints
    generate_optimization_hints(analysis, analysis.optimization_hints);
    
    return analysis;
}

std::string ShaderPerformanceAnalyzer::generate_performance_report(const Analysis& analysis) {
    std::ostringstream report;
    
    report << "Shader Complexity:\n";
    report << "  Instructions: " << analysis.instruction_count << "\n";
    report << "  Arithmetic Ops: " << analysis.arithmetic_operations << "\n";
    report << "  Transcendental Ops: " << analysis.transcendental_operations << "\n";
    report << "  Texture Lookups: " << analysis.texture_lookups << "\n";
    
    report << "\nRegister Usage:\n";
    report << "  Temp Registers: " << analysis.temp_registers_used << "\n";
    report << "  Uniforms: " << analysis.uniform_slots_used << "\n";
    report << "  Attributes: " << analysis.attribute_slots_used << "\n";
    report << "  Varyings: " << analysis.varying_slots_used << "\n";
    
    report << "\nEstimated Cycles:\n";
    report << "  Vertex: ~" << analysis.estimated_vertex_cycles << "\n";
    report << "  Fragment: ~" << analysis.estimated_fragment_cycles << "\n";
    
    if (!analysis.optimization_hints.empty()) {
        report << "\nOptimization Hints:\n";
        for (const auto& hint : analysis.optimization_hints) {
            report << "  - " << hint << "\n";
        }
    }
    
    if (!analysis.potential_issues.empty()) {
        report << "\nPotential Issues:\n";
        for (const auto& issue : analysis.potential_issues) {
            report << "  ! " << issue << "\n";
        }
    }
    
    return report.str();
}

size_t ShaderPerformanceAnalyzer::count_instructions(const std::string& source) {
    // Count semicolons as a rough approximation of instructions
    return std::count(source.begin(), source.end(), ';');
}

size_t ShaderPerformanceAnalyzer::count_texture_lookups(const std::string& source) {
    size_t count = 0;
    std::regex texture_regex(R"(texture2D\s*\()");
    
    std::sregex_iterator it(source.begin(), source.end(), texture_regex);
    std::sregex_iterator end;
    
    while (it != end) {
        count++;
        ++it;
    }
    
    return count;
}

size_t ShaderPerformanceAnalyzer::count_arithmetic_ops(const std::string& source) {
    size_t count = 0;
    
    // Count basic arithmetic operations
    count += std::count(source.begin(), source.end(), '+');
    count += std::count(source.begin(), source.end(), '-');
    count += std::count(source.begin(), source.end(), '*');
    count += std::count(source.begin(), source.end(), '/');
    
    // Count vector operations
    std::regex vec_ops(R"(\b(dot|cross|normalize|length|distance)\s*\()");
    std::sregex_iterator it(source.begin(), source.end(), vec_ops);
    std::sregex_iterator end;
    
    while (it != end) {
        count += 3; // Approximate cost
        ++it;
    }
    
    return count;
}

size_t ShaderPerformanceAnalyzer::count_transcendental_ops(const std::string& source) {
    size_t count = 0;
    
    std::regex trans_ops(R"(\b(sin|cos|tan|asin|acos|atan|exp|log|pow|sqrt)\s*\()");
    std::sregex_iterator it(source.begin(), source.end(), trans_ops);
    std::sregex_iterator end;
    
    while (it != end) {
        count++;
        ++it;
    }
    
    return count;
}

void ShaderPerformanceAnalyzer::analyze_register_usage(const std::string& source, 
                                                       Analysis& analysis) {
    // Count uniform declarations
    std::regex uniform_regex(R"(uniform\s+\w+\s+(\w+))");
    std::sregex_iterator uniform_it(source.begin(), source.end(), uniform_regex);
    std::sregex_iterator end;
    
    while (uniform_it != end) {
        analysis.uniform_slots_used++;
        ++uniform_it;
    }
    
    // Count attribute declarations
    std::regex attribute_regex(R"(attribute\s+\w+\s+(\w+))");
    std::sregex_iterator attr_it(source.begin(), source.end(), attribute_regex);
    
    while (attr_it != end) {
        analysis.attribute_slots_used++;
        ++attr_it;
    }
    
    // Count varying declarations
    std::regex varying_regex(R"(varying\s+\w+\s+(\w+))");
    std::sregex_iterator vary_it(source.begin(), source.end(), varying_regex);
    
    while (vary_it != end) {
        analysis.varying_slots_used++;
        ++vary_it;
    }
    
    // Rough estimate of temp registers (local variables)
    std::regex temp_regex(R"(\b(float|vec2|vec3|vec4|mat2|mat3|mat4)\s+(\w+)\s*[=;])");
    std::sregex_iterator temp_it(source.begin(), source.end(), temp_regex);
    
    while (temp_it != end) {
        analysis.temp_registers_used++;
        ++temp_it;
    }
}

void ShaderPerformanceAnalyzer::generate_optimization_hints(const Analysis& analysis,
                                                           std::vector<std::string>& hints) {
    if (analysis.texture_lookups > 4) {
        hints.push_back("High number of texture lookups (" + 
                       std::to_string(analysis.texture_lookups) + 
                       "). Consider texture atlasing or reducing samples.");
    }
    
    if (analysis.transcendental_operations > 4) {
        hints.push_back("Many transcendental operations. Consider using approximations or lookup tables.");
    }
    
    if (analysis.varying_slots_used > 8) {
        hints.push_back("High varying usage. Consider packing multiple values into vec4s.");
    }
    
    if (analysis.dependent_texture_lookups > 0) {
        hints.push_back("Dependent texture reads detected. These can cause pipeline stalls.");
    }
    
    if (analysis.estimated_fragment_cycles > 50) {
        hints.push_back("Complex fragment shader. Consider moving calculations to vertex shader.");
    }
}

//////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////

bool initialize_shader_debugger() {
    return initialize_shader_debugger(ShaderDebugger::Config());
}

bool initialize_shader_debugger(const ShaderDebugger::Config& config) {
    if (g_shader_debugger) {
        DX8GL_WARNING("Shader debugger already initialized");
        return true;
    }
    
    g_shader_debugger = std::make_unique<ShaderDebugger>(config);
    return g_shader_debugger->initialize();
}

void shutdown_shader_debugger() {
    if (g_shader_debugger) {
        g_shader_debugger->shutdown();
        g_shader_debugger.reset();
    }
}

} // namespace dx8gl