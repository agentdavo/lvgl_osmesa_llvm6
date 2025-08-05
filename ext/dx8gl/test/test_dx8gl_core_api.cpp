#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/logger.h"

using namespace std;

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            cout << "[PASS] " << message << endl; \
        } else { \
            cout << "[FAIL] " << message << endl; \
            return false; \
        } \
    } while(0)

// Custom log callback for testing
static vector<string> log_messages;
static void test_log_callback(const char* message) {
    log_messages.push_back(string(message));
}

bool test_initialization_and_shutdown() {
    cout << "\n=== Test: Initialization and Shutdown ===" << endl;
    
    // Test basic initialization
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Basic initialization should succeed");
    
    // Test double initialization
    result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_ERROR_ALREADY_INITIALIZED, "Double initialization should fail with ALREADY_INITIALIZED");
    
    // Test shutdown
    dx8gl_shutdown();
    
    // Test initialization with custom config
    log_messages.clear();
    dx8gl_config config = {};
    config.enable_logging = true;
    config.log_callback = test_log_callback;
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    result = dx8gl_init(&config);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization with config should succeed");
    TEST_ASSERT(!log_messages.empty(), "Custom log callback should receive messages");
    
    dx8gl_shutdown();
    return true;
}

bool test_device_management() {
    cout << "\n=== Test: Device Management ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Test device creation with null parameter
    result = dx8gl_create_device(nullptr);
    TEST_ASSERT(result == DX8GL_ERROR_INVALID_PARAMETER, "Device creation with null parameter should fail");
    
    // Test device creation
    dx8gl_device* device = nullptr;
    result = dx8gl_create_device(&device);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Device creation should succeed");
    TEST_ASSERT(device != nullptr, "Device pointer should not be null");
    
    // Test device capabilities
    dx8gl_caps caps = {};
    result = dx8gl_get_caps(device, &caps);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Getting device capabilities should succeed");
    TEST_ASSERT(caps.max_texture_size > 0, "Max texture size should be positive");
    TEST_ASSERT(caps.max_texture_units > 0, "Max texture units should be positive");
    TEST_ASSERT(caps.max_vertex_shader_version == 0x0101, "Should support vs_1_1");
    TEST_ASSERT(caps.max_pixel_shader_version == 0x0104, "Should support ps_1_4");
    
    // Test statistics
    dx8gl_stats stats = {};
    result = dx8gl_get_stats(device, &stats);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Getting device statistics should succeed");
    
    // Test statistics reset
    dx8gl_reset_stats(device);
    
    // Test device destruction
    dx8gl_destroy_device(device);
    
    // Test null device destruction (should not crash)
    dx8gl_destroy_device(nullptr);
    
    dx8gl_shutdown();
    return true;
}

bool test_context_management() {
    cout << "\n=== Test: Context Management ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Test context creation
    dx8gl_context* context1 = dx8gl_context_create();
    TEST_ASSERT(context1 != nullptr, "Context creation should succeed");
    
    dx8gl_context* context2 = dx8gl_context_create_with_size(640, 480);
    TEST_ASSERT(context2 != nullptr, "Context creation with size should succeed");
    
    // Test current context tracking
    dx8gl_context* current = dx8gl_context_get_current();
    TEST_ASSERT(current == nullptr, "Initially no context should be current");
    
    bool success = dx8gl_context_make_current(context1);
    TEST_ASSERT(success, "Making context current should succeed");
    
    current = dx8gl_context_get_current();
    TEST_ASSERT(current == context1, "Current context should match the one set");
    
    success = dx8gl_context_make_current(context2);
    TEST_ASSERT(success, "Switching context should succeed");
    
    current = dx8gl_context_get_current();
    TEST_ASSERT(current == context2, "Current context should be updated");
    
    // Test setting current to null
    success = dx8gl_context_make_current(nullptr);
    TEST_ASSERT(success, "Setting context to null should succeed");
    
    current = dx8gl_context_get_current();
    TEST_ASSERT(current == nullptr, "Current context should be null");
    
    // Test context size query
    uint32_t width, height;
    dx8gl_context_get_size(context2, &width, &height);
    TEST_ASSERT(width == 640 && height == 480, "Context should have correct size");
    
    // Test context destruction
    dx8gl_context_destroy(context1);
    dx8gl_context_destroy(context2);
    dx8gl_context_destroy(nullptr); // Should not crash
    
    dx8gl_shutdown();
    return true;
}

bool test_error_handling() {
    cout << "\n=== Test: Error Handling ===" << endl;
    
    // Test operations before initialization
    dx8gl_device* device = nullptr;
    dx8gl_error result = dx8gl_create_device(&device);
    TEST_ASSERT(result == DX8GL_ERROR_NOT_INITIALIZED, "Device creation before init should fail");
    
    // Initialize for other tests
    result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Test invalid parameters
    dx8gl_caps caps = {};
    result = dx8gl_get_caps(nullptr, &caps);
    TEST_ASSERT(result == DX8GL_ERROR_INVALID_PARAMETER, "Getting caps with null device should fail");
    
    result = dx8gl_create_device(&device);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Device creation should succeed");
    
    result = dx8gl_get_caps(device, nullptr);
    TEST_ASSERT(result == DX8GL_ERROR_INVALID_PARAMETER, "Getting caps with null caps should fail");
    
    // Test error string function
    const char* error_str = dx8gl_get_error_string();
    TEST_ASSERT(error_str != nullptr, "Error string should not be null");
    
    dx8gl_destroy_device(device);
    dx8gl_shutdown();
    return true;
}

bool test_version_and_utility_functions() {
    cout << "\n=== Test: Version and Utility Functions ===" << endl;
    
    const char* version = dx8gl_get_version_string();
    TEST_ASSERT(version != nullptr, "Version string should not be null");
    TEST_ASSERT(strlen(version) > 0, "Version string should not be empty");
    TEST_ASSERT(strcmp(version, DX8GL_VERSION_STRING) == 0, "Version string should match constant");
    
    // Test plugin management (should return not supported)
    dx8gl_error result = dx8gl_load_plugin("test.so");
    TEST_ASSERT(result == DX8GL_ERROR_NOT_SUPPORTED, "Plugin loading should return not supported");
    
    result = dx8gl_unload_plugin("test");
    TEST_ASSERT(result == DX8GL_ERROR_NOT_SUPPORTED, "Plugin unloading should return not supported");
    
    size_t count = 999;
    result = dx8gl_list_plugins(nullptr, &count);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Plugin listing should succeed");
    TEST_ASSERT(count == 0, "Plugin count should be zero");
    
    return true;
}

bool test_directx_compatibility() {
    cout << "\n=== Test: DirectX 8 Compatibility ===" << endl;
    
    // Initialize dx8gl
    dx8gl_error result = dx8gl_init(nullptr);
    TEST_ASSERT(result == DX8GL_SUCCESS, "Initialization should succeed");
    
    // Test DirectX 8 interface creation
    IDirect3D8* d3d8 = Direct3DCreate8(120); // D3D_SDK_VERSION
    TEST_ASSERT(d3d8 != nullptr, "Direct3DCreate8 should succeed");
    
    // Test invalid SDK version
    IDirect3D8* d3d8_invalid = Direct3DCreate8(0x12345678);
    TEST_ASSERT(d3d8_invalid == nullptr, "Direct3DCreate8 with invalid SDK should fail");
    
    // Test framebuffer functions (these may return null but shouldn't crash)
    int width, height;
    void* framebuffer = dx8gl_get_framebuffer(nullptr, &width, &height);
    // Note: framebuffer may be null in some configurations, that's OK
    
    bool updated = false;
    int frame_number = 0;
    void* shared_fb = dx8gl_get_shared_framebuffer(&width, &height, &frame_number, &updated);
    // Note: shared framebuffer may be null without a device, that's OK
    
    // Clean up D3D8 interface
    if (d3d8) {
        // Cast to the actual implementation for cleanup
        delete static_cast<dx8gl::Direct3D8*>(d3d8);
    }
    
    dx8gl_shutdown();
    return true;
}

bool run_all_tests() {
    cout << "Running dx8gl Core API Tests" << endl;
    cout << "=============================" << endl;
    
    bool all_passed = true;
    
    all_passed &= test_initialization_and_shutdown();
    all_passed &= test_device_management();
    all_passed &= test_context_management();
    all_passed &= test_error_handling();
    all_passed &= test_version_and_utility_functions();
    all_passed &= test_directx_compatibility();
    
    return all_passed;
}

int main() {
    bool success = run_all_tests();
    
    cout << "\n=============================" << endl;
    cout << "Test Results: " << tests_passed << "/" << tests_run << " passed" << endl;
    
    if (success && tests_passed == tests_run) {
        cout << "All tests PASSED!" << endl;
        return 0;
    } else {
        cout << "Some tests FAILED!" << endl;
        return 1;
    }
}