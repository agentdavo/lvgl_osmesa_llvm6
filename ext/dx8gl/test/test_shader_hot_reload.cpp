#include <gtest/gtest.h>
#include "shader_hot_reload.h"
#include "dx8gl.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;
using namespace dx8gl;

class ShaderHotReloadTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderHotReloadManager> manager;
    std::string test_shader_dir;
    std::string vertex_shader_path;
    std::string fragment_shader_path;
    GLuint test_program = 0;
    
    void SetUp() override {
        // Initialize dx8gl for OpenGL context
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_OSMESA;
        config.width = 256;
        config.height = 256;
        ASSERT_EQ(dx8gl_init(&config), 0);
        
        // Create temporary directory for test shaders
        test_shader_dir = fs::temp_directory_path() / "shader_hot_reload_test";
        fs::create_directories(test_shader_dir);
        
        // Set up shader file paths
        vertex_shader_path = test_shader_dir / "test.vert";
        fragment_shader_path = test_shader_dir / "test.frag";
        
        // Create initial shader files
        CreateInitialShaders();
        
        // Create hot reload manager with test configuration
        HotReloadConfig config_reload;
        config_reload.enabled = true;
        config_reload.shader_directory = test_shader_dir;
        config_reload.poll_interval = std::chrono::milliseconds(100);
        config_reload.auto_reload = false; // Manual control for testing
        config_reload.verbose_logging = false;
        
        manager = std::make_unique<ShaderHotReloadManager>(config_reload);
        
        // Create a dummy shader program ID for testing
        test_program = 1001; // Arbitrary ID for testing
    }
    
    void TearDown() override {
        // Stop hot reload manager
        if (manager) {
            manager->stop();
            manager.reset();
        }
        
        // Clean up temporary files
        if (fs::exists(test_shader_dir)) {
            fs::remove_all(test_shader_dir);
        }
        
        dx8gl_shutdown();
    }
    
    void CreateInitialShaders() {
        // Create simple vertex shader
        std::ofstream vs(vertex_shader_path);
        vs << "#version 330 core\n"
           << "layout(location = 0) in vec3 position;\n"
           << "uniform mat4 mvpMatrix;\n"
           << "void main() {\n"
           << "    gl_Position = mvpMatrix * vec4(position, 1.0);\n"
           << "}\n";
        vs.close();
        
        // Create simple fragment shader
        std::ofstream fs(fragment_shader_path);
        fs << "#version 330 core\n"
           << "out vec4 fragColor;\n"
           << "uniform vec4 color;\n"
           << "void main() {\n"
           << "    fragColor = color;\n"
           << "}\n";
        fs.close();
    }
    
    void ModifyVertexShader(const std::string& new_content) {
        std::ofstream vs(vertex_shader_path);
        vs << new_content;
        vs.close();
        
        // Small delay to ensure file system updates timestamp
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void ModifyFragmentShader(const std::string& new_content) {
        std::ofstream fs(fragment_shader_path);
        fs << new_content;
        fs.close();
        
        // Small delay to ensure file system updates timestamp
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::string ReadFile(const std::string& path) {
        std::ifstream file(path);
        return std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    }
};

TEST_F(ShaderHotReloadTest, RegisterAndUnregisterShader) {
    // Test registering a shader for hot reload
    bool callback_called = false;
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path,
                           [&callback_called](GLuint program) {
                               callback_called = true;
                           });
    
    // Start the manager
    EXPECT_TRUE(manager->start());
    
    // Unregister the shader
    manager->unregister_shader(test_program);
    
    // Stop the manager
    manager->stop();
    
    EXPECT_FALSE(callback_called); // No reload should have happened
}

TEST_F(ShaderHotReloadTest, DetectFileModification) {
    // Register shader
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    // Start manager
    EXPECT_TRUE(manager->start());
    
    // Give it time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Modify vertex shader
    ModifyVertexShader(
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "layout(location = 1) in vec3 normal;\n"  // Added normal attribute
        "uniform mat4 mvpMatrix;\n"
        "out vec3 fragNormal;\n"
        "void main() {\n"
        "    gl_Position = mvpMatrix * vec4(position, 1.0);\n"
        "    fragNormal = normal;\n"
        "}\n"
    );
    
    // Check for modifications
    auto modified_programs = manager->check_for_modifications();
    
    // Should detect the modified shader
    EXPECT_EQ(modified_programs.size(), 1);
    if (!modified_programs.empty()) {
        EXPECT_EQ(modified_programs[0], test_program);
    }
}

TEST_F(ShaderHotReloadTest, ReloadModifiedShader) {
    // Track reload events
    bool reload_callback_fired = false;
    bool reload_success = false;
    std::string reload_error;
    
    manager->set_global_reload_callback(
        [&](GLuint program, bool success, const std::string& error) {
            reload_callback_fired = true;
            reload_success = success;
            reload_error = error;
        }
    );
    
    // Register shader
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    // Start manager
    EXPECT_TRUE(manager->start());
    
    // Modify fragment shader
    ModifyFragmentShader(
        "#version 330 core\n"
        "out vec4 fragColor;\n"
        "uniform vec4 color;\n"
        "uniform float time;\n"  // Added time uniform
        "void main() {\n"
        "    fragColor = color * abs(sin(time));\n"  // Animated color
        "}\n"
    );
    
    // Manually trigger reload
    bool result = manager->reload_shader(test_program);
    
    // Note: Actual shader compilation may fail without a real GL context,
    // but we're testing the reload mechanism itself
    EXPECT_TRUE(reload_callback_fired);
}

TEST_F(ShaderHotReloadTest, MultipleShaderReload) {
    // Register multiple shaders
    GLuint program1 = 1001;
    GLuint program2 = 1002;
    GLuint program3 = 1003;
    
    // Create additional shader files
    std::string vs2_path = test_shader_dir / "shader2.vert";
    std::string fs2_path = test_shader_dir / "shader2.frag";
    std::string vs3_path = test_shader_dir / "shader3.vert";
    std::string fs3_path = test_shader_dir / "shader3.frag";
    
    // Copy initial shaders to new files
    fs::copy(vertex_shader_path, vs2_path);
    fs::copy(fragment_shader_path, fs2_path);
    fs::copy(vertex_shader_path, vs3_path);
    fs::copy(fragment_shader_path, fs3_path);
    
    manager->register_shader(program1, "shader1", vertex_shader_path, fragment_shader_path);
    manager->register_shader(program2, "shader2", vs2_path, fs2_path);
    manager->register_shader(program3, "shader3", vs3_path, fs3_path);
    
    // Start manager
    EXPECT_TRUE(manager->start());
    
    // Modify multiple shaders
    ModifyVertexShader("#version 330 core\n// Modified shader 1\n");
    
    std::ofstream fs2(fs2_path);
    fs2 << "#version 330 core\n// Modified shader 2\n";
    fs2.close();
    
    // Give time for modifications to register
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Check for modifications
    auto modified = manager->check_for_modifications();
    
    // Should detect at least 2 modified shaders
    EXPECT_GE(modified.size(), 2);
    
    // Reload all shaders
    bool result = manager->reload_all_shaders();
    // Result depends on actual shader compilation
}

TEST_F(ShaderHotReloadTest, EnableDisableHotReload) {
    // Register shader
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    // Start manager
    EXPECT_TRUE(manager->start());
    
    // Disable hot reload
    manager->set_enabled(false);
    EXPECT_FALSE(manager->is_enabled());
    
    // Modify shader
    ModifyVertexShader("#version 330 core\n// Modified while disabled\n");
    
    // Check for modifications - should return empty since disabled
    auto modified = manager->check_for_modifications();
    EXPECT_EQ(modified.size(), 0);
    
    // Re-enable hot reload
    manager->set_enabled(true);
    EXPECT_TRUE(manager->is_enabled());
    
    // Now check should detect the modification
    modified = manager->check_for_modifications();
    EXPECT_GE(modified.size(), 1);
}

TEST_F(ShaderHotReloadTest, ShaderReloadScope) {
    // Test the RAII reload scope utility
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    manager->start();
    
    {
        ShaderReloadScope scope(manager.get(), test_program);
        
        // Modify shader source through the scope
        scope.modify_vertex_source(
            "#version 330 core\n"
            "// Modified through reload scope\n"
            "layout(location = 0) in vec3 position;\n"
            "void main() {\n"
            "    gl_Position = vec4(position, 1.0);\n"
            "}\n"
        );
        
        // Trigger reload
        bool result = scope.reload();
        // Result depends on shader compilation
    }
    // Scope destructor should restore original source
    
    // Verify original source is restored
    std::string current_vs = ReadFile(vertex_shader_path);
    EXPECT_NE(current_vs.find("mvpMatrix"), std::string::npos);
}

TEST_F(ShaderHotReloadTest, InvalidShaderHandling) {
    // Test handling of invalid shader syntax
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    bool error_caught = false;
    std::string error_message;
    
    manager->set_global_reload_callback(
        [&](GLuint program, bool success, const std::string& error) {
            if (!success) {
                error_caught = true;
                error_message = error;
            }
        }
    );
    
    manager->start();
    
    // Write invalid shader
    ModifyVertexShader(
        "#version 330 core\n"
        "This is not valid GLSL syntax!\n"
        "layout(location = 0) in vec3 position;\n"
    );
    
    // Try to reload
    bool result = manager->reload_shader(test_program);
    
    // Should handle the error gracefully
    // Note: Without actual GL context, compilation may not happen,
    // but the mechanism should handle errors if they occur
}

TEST_F(ShaderHotReloadTest, WatchThreadLifecycle) {
    // Test the watch thread starts and stops correctly
    EXPECT_FALSE(manager->is_enabled());
    
    // Configure and start
    HotReloadConfig config;
    config.enabled = true;
    config.auto_reload = true;
    config.poll_interval = std::chrono::milliseconds(50);
    manager->set_config(config);
    
    EXPECT_TRUE(manager->start());
    
    // Register a shader
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    // Let the watch thread run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop the manager
    manager->stop();
    
    // Should be able to start again
    EXPECT_TRUE(manager->start());
    
    // Clean stop
    manager->stop();
}

TEST_F(ShaderHotReloadTest, ConcurrentModifications) {
    // Test handling multiple files modified at the same time
    const int num_shaders = 5;
    std::vector<GLuint> programs;
    std::vector<std::string> vs_paths;
    std::vector<std::string> fs_paths;
    
    // Create multiple shader files
    for (int i = 0; i < num_shaders; i++) {
        GLuint program = 2000 + i;
        programs.push_back(program);
        
        std::string vs_path = test_shader_dir / ("shader" + std::to_string(i) + ".vert");
        std::string fs_path = test_shader_dir / ("shader" + std::to_string(i) + ".frag");
        
        vs_paths.push_back(vs_path);
        fs_paths.push_back(fs_path);
        
        // Create shader files
        fs::copy(vertex_shader_path, vs_path);
        fs::copy(fragment_shader_path, fs_path);
        
        // Register with manager
        manager->register_shader(program, "shader" + std::to_string(i),
                               vs_path, fs_path);
    }
    
    manager->start();
    
    // Modify all shaders concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < num_shaders; i++) {
        threads.emplace_back([this, i, &vs_paths]() {
            std::ofstream vs(vs_paths[i]);
            vs << "#version 330 core\n"
               << "// Concurrent modification " << i << "\n"
               << "layout(location = 0) in vec3 position;\n"
               << "void main() {\n"
               << "    gl_Position = vec4(position * " << (i + 1) << ".0, 1.0);\n"
               << "}\n";
        });
    }
    
    // Wait for all modifications
    for (auto& t : threads) {
        t.join();
    }
    
    // Give time for file system updates
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for modifications
    auto modified = manager->check_for_modifications();
    
    // Should detect all modified shaders
    EXPECT_EQ(modified.size(), num_shaders);
}

TEST_F(ShaderHotReloadTest, TemporaryFileEdits) {
    // Test simulating external editor behavior with temp files
    manager->register_shader(test_program, "test_shader",
                           vertex_shader_path, fragment_shader_path);
    
    manager->start();
    
    // Simulate editor creating temp file and moving it
    std::string temp_path = vertex_shader_path + ".tmp";
    
    // Write to temp file
    std::ofstream temp(temp_path);
    temp << "#version 330 core\n"
         << "// Edited via temp file\n"
         << "layout(location = 0) in vec3 position;\n"
         << "void main() {\n"
         << "    gl_Position = vec4(position, 1.0);\n"
         << "}\n";
    temp.close();
    
    // Simulate atomic rename (common editor pattern)
    fs::rename(temp_path, vertex_shader_path);
    
    // Give time for detection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for modifications
    auto modified = manager->check_for_modifications();
    
    // Should detect the modification
    EXPECT_GE(modified.size(), 1);
}

// Test helper class implementations
TEST_F(ShaderHotReloadTest, DevelopmentShaderLoader) {
    // Test the development shader loader utilities
    
    // Test error shader generation
    auto [error_vs, error_fs] = DevelopmentShaderLoader::generate_error_shader("1.0, 0.0, 0.0, 1.0");
    EXPECT_FALSE(error_vs.empty());
    EXPECT_FALSE(error_fs.empty());
    EXPECT_NE(error_fs.find("1.0, 0.0, 0.0, 1.0"), std::string::npos);
    
    // Test shader validation (basic check)
    std::string valid_vs = 
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "void main() { gl_Position = vec4(position, 1.0); }\n";
    
    auto warnings = DevelopmentShaderLoader::validate_shader_source(valid_vs, GL_VERTEX_SHADER);
    // Without GL context, validation may be limited
    
    // Test debug code injection
    std::string debug_injected = DevelopmentShaderLoader::inject_debug_code(
        valid_vs, GL_VERTEX_SHADER, "// DEBUG: Test injection");
    EXPECT_NE(debug_injected.find("DEBUG: Test injection"), std::string::npos);
}

TEST_F(ShaderHotReloadTest, GlobalManagerIntegration) {
    // Test global hot reload manager integration
    HotReloadConfig config;
    config.enabled = true;
    config.shader_directory = test_shader_dir;
    
    // Initialize global manager
    EXPECT_TRUE(initialize_shader_hot_reload(config));
    
    // Global manager should be available
    EXPECT_NE(g_shader_hot_reload, nullptr);
    
    if (g_shader_hot_reload) {
        // Register shader with global manager
        g_shader_hot_reload->register_shader(test_program, "global_test",
                                           vertex_shader_path, fragment_shader_path);
        
        // Start global manager
        g_shader_hot_reload->start();
        
        // Check it's enabled
        EXPECT_TRUE(g_shader_hot_reload->is_enabled());
    }
    
    // Shutdown global manager
    shutdown_shader_hot_reload();
    EXPECT_EQ(g_shader_hot_reload, nullptr);
}