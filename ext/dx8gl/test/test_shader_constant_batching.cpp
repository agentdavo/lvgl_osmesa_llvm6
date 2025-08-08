#include <gtest/gtest.h>
#include "shader_constant_manager.h"
#include "d3d8_types.h"
#include "gl3_headers.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

using namespace dx8gl;

class ShaderConstantBatchingTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderConstantManager> manager_;
    GLuint test_program_;
    
    void SetUp() override {
        manager_ = std::make_unique<ShaderConstantManager>();
        
        // Create a test shader program with many uniforms
        test_program_ = create_test_program();
        ASSERT_NE(test_program_, 0u);
        
        // Initialize manager with the program
        manager_->init(test_program_);
        
        // Register constants for testing
        register_test_constants();
    }
    
    void TearDown() override {
        if (test_program_) {
            glDeleteProgram(test_program_);
            test_program_ = 0;
        }
        manager_.reset();
    }
    
    GLuint create_test_program() {
        // Create vertex shader with many constants
        const char* vs_source = R"(
            #version 330 core
            layout(location = 0) in vec3 a_position;
            
            // Float constants (c0-c95)
            uniform vec4 c0;
            uniform vec4 c1;
            uniform vec4 c2;
            uniform vec4 c3;
            uniform vec4 c4;
            uniform vec4 c5;
            uniform vec4 c10;
            uniform vec4 c20;
            uniform vec4 c30;
            uniform vec4 c40;
            uniform vec4 c50;
            
            // Matrix constants
            uniform mat4 c60;  // 4 registers (c60-c63)
            uniform mat4 c64;  // 4 registers (c64-c67)
            
            // Int constants
            uniform ivec4 i0;
            uniform ivec4 i1;
            
            // Bool constants
            uniform bool b0;
            uniform bool b1;
            
            void main() {
                // Use all constants to prevent optimization
                vec4 result = c0 + c1 + c2 + c3 + c4 + c5;
                result += c10 + c20 + c30 + c40 + c50;
                result += c60[0] + c64[0];
                result.x += float(i0.x + i1.x);
                result.y += b0 ? 1.0 : 0.0;
                result.z += b1 ? 1.0 : 0.0;
                
                gl_Position = vec4(a_position, 1.0) + result * 0.001;
            }
        )";
        
        const char* fs_source = R"(
            #version 330 core
            out vec4 FragColor;
            
            uniform vec4 ps_c0;
            uniform vec4 ps_c1;
            
            void main() {
                FragColor = ps_c0 + ps_c1;
            }
        )";
        
        // Compile shaders
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vs_source, nullptr);
        glCompileShader(vs);
        
        GLint vs_compiled;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &vs_compiled);
        if (!vs_compiled) {
            char log[512];
            glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
            std::cerr << "Vertex shader compilation failed: " << log << std::endl;
            glDeleteShader(vs);
            return 0;
        }
        
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fs_source, nullptr);
        glCompileShader(fs);
        
        GLint fs_compiled;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &fs_compiled);
        if (!fs_compiled) {
            char log[512];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            std::cerr << "Fragment shader compilation failed: " << log << std::endl;
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }
        
        // Link program
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            std::cerr << "Program linking failed: " << log << std::endl;
            glDeleteProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }
        
        // Clean up shader objects
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        return program;
    }
    
    void register_test_constants() {
        // Register float constants
        manager_->register_constant("c0", ShaderConstantManager::CONSTANT_FLOAT4, 0, 1);
        manager_->register_constant("c1", ShaderConstantManager::CONSTANT_FLOAT4, 1, 1);
        manager_->register_constant("c2", ShaderConstantManager::CONSTANT_FLOAT4, 2, 1);
        manager_->register_constant("c3", ShaderConstantManager::CONSTANT_FLOAT4, 3, 1);
        manager_->register_constant("c4", ShaderConstantManager::CONSTANT_FLOAT4, 4, 1);
        manager_->register_constant("c5", ShaderConstantManager::CONSTANT_FLOAT4, 5, 1);
        manager_->register_constant("c10", ShaderConstantManager::CONSTANT_FLOAT4, 10, 1);
        manager_->register_constant("c20", ShaderConstantManager::CONSTANT_FLOAT4, 20, 1);
        manager_->register_constant("c30", ShaderConstantManager::CONSTANT_FLOAT4, 30, 1);
        manager_->register_constant("c40", ShaderConstantManager::CONSTANT_FLOAT4, 40, 1);
        manager_->register_constant("c50", ShaderConstantManager::CONSTANT_FLOAT4, 50, 1);
        
        // Register matrix constants
        manager_->register_constant("c60", ShaderConstantManager::CONSTANT_MATRIX4, 60, 4);
        manager_->register_constant("c64", ShaderConstantManager::CONSTANT_MATRIX4, 64, 4);
        
        // Register int constants
        manager_->register_constant("i0", ShaderConstantManager::CONSTANT_INT4, 0, 1);
        manager_->register_constant("i1", ShaderConstantManager::CONSTANT_INT4, 1, 1);
        
        // Register bool constants
        manager_->register_constant("b0", ShaderConstantManager::CONSTANT_BOOL, 0, 1);
        manager_->register_constant("b1", ShaderConstantManager::CONSTANT_BOOL, 1, 1);
        
        // Register pixel shader constants
        manager_->register_constant("ps_c0", ShaderConstantManager::CONSTANT_FLOAT4, 0, 1);
        manager_->register_constant("ps_c1", ShaderConstantManager::CONSTANT_FLOAT4, 1, 1);
    }
};

TEST_F(ShaderConstantBatchingTest, BatchedFloatConstantUpdate) {
    // Set multiple contiguous float constants
    float constants[6][4] = {
        {1.0f, 2.0f, 3.0f, 4.0f},    // c0
        {5.0f, 6.0f, 7.0f, 8.0f},    // c1
        {9.0f, 10.0f, 11.0f, 12.0f}, // c2
        {13.0f, 14.0f, 15.0f, 16.0f},// c3
        {17.0f, 18.0f, 19.0f, 20.0f},// c4
        {21.0f, 22.0f, 23.0f, 24.0f} // c5
    };
    
    // Set constants individually (should be batched internally)
    for (int i = 0; i < 6; i++) {
        manager_->set_float_constant(i, constants[i], 1);
    }
    
    // Upload should batch these together
    manager_->upload_dirty_constants();
    
    // Verify metrics show batching
    auto metrics = manager_->get_metrics();
    EXPECT_GT(metrics.batched_uploads, 0u);
    EXPECT_EQ(metrics.constants_set, 6u);
    
    // Verify constants were set correctly by checking uniform values
    glUseProgram(test_program_);
    for (int i = 0; i < 6; i++) {
        char name[16];
        snprintf(name, sizeof(name), "c%d", i);
        GLint loc = glGetUniformLocation(test_program_, name);
        if (loc != -1) {
            float values[4];
            glGetUniformfv(test_program_, loc, values);
            for (int j = 0; j < 4; j++) {
                EXPECT_FLOAT_EQ(values[j], constants[i][j]);
            }
        }
    }
}

TEST_F(ShaderConstantBatchingTest, BatchedMatrixConstantUpdate) {
    // Create identity and rotation matrices
    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    float rotation[16] = {
        0.7071f, -0.7071f, 0, 0,
        0.7071f,  0.7071f, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    // Set matrix constants
    manager_->set_matrix_constant(60, identity, false);
    manager_->set_matrix_constant(64, rotation, false);
    
    // Upload batched
    manager_->upload_dirty_constants();
    
    // Verify batching occurred
    auto metrics = manager_->get_metrics();
    EXPECT_GT(metrics.batched_uploads, 0u);
    EXPECT_EQ(metrics.constants_set, 2u);
}

TEST_F(ShaderConstantBatchingTest, MixedConstantTypes) {
    // Set different types of constants
    float float_const[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    int int_const[4] = {10, 20, 30, 40};
    BOOL bool_const[2] = {TRUE, FALSE};
    
    manager_->set_float_constant(0, float_const, 1);
    manager_->set_int_constant(0, int_const, 1);
    manager_->set_bool_constant(0, bool_const, 2);
    
    // Upload all types
    manager_->upload_dirty_constants();
    
    // Should have separate batches for each type
    auto metrics = manager_->get_metrics();
    EXPECT_GE(metrics.batched_uploads, 3u); // At least one batch per type
}

TEST_F(ShaderConstantBatchingTest, NonContiguousConstants) {
    // Set non-contiguous constants (should not batch)
    float const0[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float const10[4] = {10.0f, 11.0f, 12.0f, 13.0f};
    float const20[4] = {20.0f, 21.0f, 22.0f, 23.0f};
    
    manager_->set_float_constant(0, const0, 1);
    manager_->set_float_constant(10, const10, 1);
    manager_->set_float_constant(20, const20, 1);
    
    manager_->upload_dirty_constants();
    
    // Should have separate uploads for non-contiguous ranges
    auto metrics = manager_->get_metrics();
    EXPECT_EQ(metrics.constants_set, 3u);
    EXPECT_GE(metrics.total_uploads, 3u);
}

TEST_F(ShaderConstantBatchingTest, LargeBatchPerformance) {
    // Measure performance of batched vs individual updates
    const int NUM_CONSTANTS = 50;
    std::vector<float> constants(NUM_CONSTANTS * 4);
    
    // Fill with random values
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    for (auto& val : constants) {
        val = dis(gen);
    }
    
    // Time batched update
    auto start = std::chrono::high_resolution_clock::now();
    manager_->set_float_constants(0, constants.data(), NUM_CONSTANTS);
    manager_->upload_dirty_constants();
    auto end = std::chrono::high_resolution_clock::now();
    auto batched_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // Reset metrics
    manager_->reset_metrics();
    
    // Time individual updates
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_CONSTANTS; i++) {
        manager_->set_float_constant(i, &constants[i * 4], 1);
    }
    manager_->upload_dirty_constants();
    end = std::chrono::high_resolution_clock::now();
    auto individual_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // Batched should be faster (or at least not significantly slower)
    std::cout << "Batched time: " << batched_time << " µs" << std::endl;
    std::cout << "Individual time: " << individual_time << " µs" << std::endl;
    
    // Verify batching occurred
    auto metrics = manager_->get_metrics();
    EXPECT_GT(metrics.batched_uploads, 0u);
}

TEST_F(ShaderConstantBatchingTest, DirtyFlagManagement) {
    float const0[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    // Set constant
    manager_->set_float_constant(0, const0, 1);
    
    // First upload
    manager_->upload_dirty_constants();
    auto metrics1 = manager_->get_metrics();
    EXPECT_EQ(metrics1.constants_set, 1u);
    
    // Upload again without changes (should do nothing)
    manager_->upload_dirty_constants();
    auto metrics2 = manager_->get_metrics();
    EXPECT_EQ(metrics2.constants_set, metrics1.constants_set); // No new constants set
    EXPECT_EQ(metrics2.total_uploads, metrics1.total_uploads); // No new uploads
    
    // Change constant value
    const0[0] = 5.0f;
    manager_->set_float_constant(0, const0, 1);
    
    // Should upload the change
    manager_->upload_dirty_constants();
    auto metrics3 = manager_->get_metrics();
    EXPECT_EQ(metrics3.constants_set, 2u); // One more constant set
    EXPECT_GT(metrics3.total_uploads, metrics2.total_uploads); // New upload occurred
}

TEST_F(ShaderConstantBatchingTest, ForceUploadAll) {
    // Set some constants
    float constants[3][4] = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {5.0f, 6.0f, 7.0f, 8.0f},
        {9.0f, 10.0f, 11.0f, 12.0f}
    };
    
    for (int i = 0; i < 3; i++) {
        manager_->set_float_constant(i, constants[i], 1);
    }
    
    // Normal upload
    manager_->upload_dirty_constants();
    auto metrics1 = manager_->get_metrics();
    
    // Force upload all (even if not dirty)
    manager_->upload_all_constants();
    auto metrics2 = manager_->get_metrics();
    
    // Should have more uploads after force
    EXPECT_GT(metrics2.total_uploads, metrics1.total_uploads);
}

TEST_F(ShaderConstantBatchingTest, GlobalConstantCache) {
    // Test global constant cache integration
    auto& global_cache = GlobalConstantCache::instance();
    
    // Register global constants
    global_cache.register_global("u_view_matrix", ShaderConstantManager::CONSTANT_MATRIX4);
    global_cache.register_global("u_projection_matrix", ShaderConstantManager::CONSTANT_MATRIX4);
    global_cache.register_global("u_time", ShaderConstantManager::CONSTANT_FLOAT4);
    
    // Set global values
    float view_matrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, -10, 1
    };
    
    float proj_matrix[16] = {
        1.5f, 0, 0, 0,
        0, 2.0f, 0, 0,
        0, 0, -1.0f, -1.0f,
        0, 0, -2.0f, 0
    };
    
    float time[4] = {1.234f, 0, 0, 0};
    
    global_cache.set_global_matrix("u_view_matrix", view_matrix, false);
    global_cache.set_global_matrix("u_projection_matrix", proj_matrix, false);
    global_cache.set_global_float("u_time", time, 1);
    
    // Apply to our manager
    global_cache.apply_to_manager(*manager_);
    
    // Verify globals are applied when uploading
    manager_->upload_dirty_constants();
    
    // Check that constants were set
    auto metrics = manager_->get_metrics();
    EXPECT_GT(metrics.constants_set, 0u);
}

TEST_F(ShaderConstantBatchingTest, MemoryUsageTracking) {
    // Set a large number of constants
    const int NUM_CONSTANTS = 96; // Max vertex shader constants
    std::vector<float> constants(NUM_CONSTANTS * 4, 1.0f);
    
    for (int i = 0; i < NUM_CONSTANTS; i++) {
        manager_->set_float_constant(i, &constants[i * 4], 1);
    }
    
    manager_->upload_dirty_constants();
    
    // Check memory usage
    auto metrics = manager_->get_metrics();
    size_t expected_bytes = NUM_CONSTANTS * 4 * sizeof(float);
    EXPECT_GE(metrics.bytes_uploaded, expected_bytes);
    
    std::cout << "Memory uploaded: " << metrics.bytes_uploaded << " bytes" << std::endl;
    std::cout << "Upload time: " << metrics.upload_time_ms << " ms" << std::endl;
}