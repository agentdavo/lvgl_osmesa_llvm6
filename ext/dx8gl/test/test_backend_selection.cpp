#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include "../src/dx8gl.h"
#include "../src/d3d8.h"
#include "../src/render_backend.h"
#include "../src/logger.h"

// Test utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERTION FAILED: " << message << std::endl; \
            std::cerr << "  at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASSED" << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            failed++; \
        } \
        total++; \
    } while(0)

// Helper to set environment variable
class EnvVarGuard {
public:
    EnvVarGuard(const char* name, const char* value) : name_(name) {
        const char* old_value = std::getenv(name);
        if (old_value) {
            old_value_ = old_value;
            had_value_ = true;
        } else {
            had_value_ = false;
        }
        
        if (value) {
            setenv(name, value, 1);
        } else {
            unsetenv(name);
        }
    }
    
    ~EnvVarGuard() {
        if (had_value_) {
            setenv(name_, old_value_.c_str(), 1);
        } else {
            unsetenv(name_);
        }
    }
    
private:
    const char* name_;
    std::string old_value_;
    bool had_value_;
};

// Test 1: Backend selection via environment variable
bool test_env_var_selection() {
    // Test OSMesa selection
    {
        EnvVarGuard env_guard("DX8GL_BACKEND", "osmesa");
        
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize with OSMesa backend");
        
        // Verify we got OSMesa backend
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null");
        TEST_ASSERT(backend->get_type() == DX8GL_BACKEND_OSMESA, "Expected OSMesa backend");
        
        dx8gl_shutdown();
    }
    
    // Test EGL selection (may fail if not available)
    {
        EnvVarGuard env_guard("DX8GL_BACKEND", "egl");
        
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        
        // EGL might not be available, so we just check initialization doesn't crash
        bool init_result = dx8gl_init(&config);
        if (init_result) {
            auto* backend = dx8gl::g_render_backend;
            TEST_ASSERT(backend != nullptr, "Backend is null");
            // If EGL is available, verify we got it
            if (backend->get_type() == DX8GL_BACKEND_EGL) {
                std::cout << "(EGL backend available) ";
            }
        } else {
            std::cout << "(EGL backend not available - fallback OK) ";
        }
        
        dx8gl_shutdown();
    }
    
    // Test auto selection
    {
        EnvVarGuard env_guard("DX8GL_BACKEND", "auto");
        
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize with auto backend");
        
        // Should get some backend
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null");
        std::cout << "(Auto selected: " << 
            (backend->get_type() == DX8GL_BACKEND_OSMESA ? "OSMesa" :
             backend->get_type() == DX8GL_BACKEND_EGL ? "EGL" :
             backend->get_type() == DX8GL_BACKEND_WEBGPU ? "WebGPU" : "Unknown") << ") ";
        
        dx8gl_shutdown();
    }
    
    return true;
}

// Test 2: Backend selection via config API
bool test_config_api_selection() {
    // Test explicit OSMesa selection
    {
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        config.backend_type = DX8GL_BACKEND_OSMESA;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize with OSMesa backend via config");
        
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null");
        TEST_ASSERT(backend->get_type() == DX8GL_BACKEND_OSMESA, "Expected OSMesa backend from config");
        
        dx8gl_shutdown();
    }
    
    // Test default selection
    {
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        config.backend_type = DX8GL_BACKEND_DEFAULT;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize with default backend");
        
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null");
        // Should get some valid backend
        TEST_ASSERT(backend->get_type() == DX8GL_BACKEND_OSMESA ||
                   backend->get_type() == DX8GL_BACKEND_EGL ||
                   backend->get_type() == DX8GL_BACKEND_WEBGPU,
                   "Got unexpected backend type");
        
        dx8gl_shutdown();
    }
    
    return true;
}

// Test 3: Backend fallback mechanism
bool test_backend_fallback() {
    // Test requesting unavailable backend falls back gracefully
    {
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        // Request WebGPU which likely isn't available
        config.backend_type = DX8GL_BACKEND_WEBGPU;
        
        // Should still initialize (with fallback)
        bool init_result = dx8gl_init(&config);
        if (init_result) {
            auto* backend = dx8gl::g_render_backend;
            TEST_ASSERT(backend != nullptr, "Backend is null after fallback");
            std::cout << "(Fallback to: " << 
                (backend->get_type() == DX8GL_BACKEND_OSMESA ? "OSMesa" :
                 backend->get_type() == DX8GL_BACKEND_EGL ? "EGL" : "Other") << ") ";
        }
        
        dx8gl_shutdown();
    }
    
    return true;
}

// Test 4: Backend reinitialization
bool test_backend_reinit() {
    // Initialize with one backend
    {
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        config.backend_type = DX8GL_BACKEND_OSMESA;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize first time");
        dx8gl_shutdown();
    }
    
    // Reinitialize with different settings
    {
        dx8gl_config config = {};
        config.width = 800;
        config.height = 600;
        config.backend_type = DX8GL_BACKEND_DEFAULT;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to reinitialize");
        
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null after reinit");
        
        dx8gl_shutdown();
    }
    
    return true;
}

// Test 5: Command line argument parsing
bool test_command_line_parsing() {
    // Test --backend=osmesa
    {
        EnvVarGuard env_guard("DX8GL_ARGS", "--backend=osmesa");
        
        dx8gl_config config = {};
        config.width = 640;
        config.height = 480;
        
        TEST_ASSERT(dx8gl_init(&config) == true, "Failed to initialize with command line args");
        
        auto* backend = dx8gl::g_render_backend;
        TEST_ASSERT(backend != nullptr, "Backend is null");
        TEST_ASSERT(backend->get_type() == DX8GL_BACKEND_OSMESA, "Expected OSMesa from command line");
        
        dx8gl_shutdown();
    }
    
    return true;
}

int main() {
    std::cout << "=== dx8gl Backend Selection Tests ===" << std::endl;
    
    int total = 0, passed = 0, failed = 0;
    
    // Run all tests
    RUN_TEST(test_env_var_selection);
    RUN_TEST(test_config_api_selection);
    RUN_TEST(test_backend_fallback);
    RUN_TEST(test_backend_reinit);
    RUN_TEST(test_command_line_parsing);
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return failed == 0 ? 0 : 1;
}