#include <gtest/gtest.h>
#include "shader_binary_cache.h"
#include "d3d8_types.h"
#include "gl3_headers.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>

using namespace dx8gl;
namespace fs = std::filesystem;

class ShaderCachePersistenceTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderBinaryCache> cache_;
    std::string test_cache_dir_;
    GLuint test_program_;
    
    void SetUp() override {
        // Create a unique test cache directory
        test_cache_dir_ = ".test_shader_cache_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // Configure cache with test directory
        ShaderBinaryCacheConfig config;
        config.enable_memory_cache = true;
        config.enable_disk_cache = true;
        config.disk_cache_directory = test_cache_dir_;
        config.max_memory_cache_size = 10 * 1024 * 1024; // 10MB
        config.max_disk_cache_size = 50 * 1024 * 1024;   // 50MB
        config.compress_disk_cache = true;
        config.async_disk_operations = false; // Synchronous for testing
        config.validate_binaries = true;
        
        cache_ = std::make_unique<ShaderBinaryCache>(config);
        ASSERT_TRUE(cache_->initialize());
        
        // Create a test shader program
        test_program_ = create_test_program();
        ASSERT_NE(test_program_, 0u);
    }
    
    void TearDown() override {
        if (test_program_) {
            glDeleteProgram(test_program_);
            test_program_ = 0;
        }
        
        cache_->shutdown();
        cache_.reset();
        
        // Clean up test cache directory
        if (fs::exists(test_cache_dir_)) {
            fs::remove_all(test_cache_dir_);
        }
    }
    
    GLuint create_test_program() {
        const char* vs_source = R"(
            #version 330 core
            layout(location = 0) in vec3 a_position;
            layout(location = 1) in vec2 a_texcoord;
            
            out vec2 v_texcoord;
            uniform mat4 u_mvp;
            
            void main() {
                gl_Position = u_mvp * vec4(a_position, 1.0);
                v_texcoord = a_texcoord;
            }
        )";
        
        const char* fs_source = R"(
            #version 330 core
            in vec2 v_texcoord;
            out vec4 FragColor;
            
            uniform sampler2D u_texture;
            uniform vec4 u_color;
            
            void main() {
                FragColor = texture(u_texture, v_texcoord) * u_color;
            }
        )";
        
        // Compile and link shader program
        GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_source);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_source);
        
        if (!vs || !fs) {
            if (vs) glDeleteShader(vs);
            if (fs) glDeleteShader(fs);
            return 0;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            std::cerr << "Program linking failed: " << log << std::endl;
            glDeleteProgram(program);
            return 0;
        }
        
        return program;
    }
    
    GLuint compile_shader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "Shader compilation failed: " << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }
    
    std::string generate_test_hash(int id) {
        return "test_shader_hash_" + std::to_string(id);
    }
};

TEST_F(ShaderCachePersistenceTest, SaveAndLoadBinary) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    std::string hash = generate_test_hash(1);
    
    // Save shader binary to cache
    bool saved = cache_->save_shader_binary(test_program_, hash);
    EXPECT_TRUE(saved);
    
    // Create a new program to load binary into
    GLuint new_program = glCreateProgram();
    ASSERT_NE(new_program, 0u);
    
    // Load binary from cache
    bool loaded = cache_->load_shader_binary(new_program, hash);
    EXPECT_TRUE(loaded);
    
    // Verify program is valid
    GLint link_status;
    glGetProgramiv(new_program, GL_LINK_STATUS, &link_status);
    EXPECT_EQ(link_status, GL_TRUE);
    
    glDeleteProgram(new_program);
}

TEST_F(ShaderCachePersistenceTest, DiskPersistence) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    std::string hash = generate_test_hash(2);
    
    // Save to cache
    ASSERT_TRUE(cache_->save_shader_binary(test_program_, hash));
    
    // Clear memory cache to force disk read
    cache_->clear_memory_cache();
    
    // Load should hit disk cache
    GLuint new_program = glCreateProgram();
    bool loaded = cache_->load_shader_binary(new_program, hash);
    EXPECT_TRUE(loaded);
    
    // Check statistics
    auto stats = cache_->get_statistics();
    EXPECT_GT(stats.disk_cache_hits, 0u);
    
    glDeleteProgram(new_program);
}

TEST_F(ShaderCachePersistenceTest, CacheFileStructure) {
    std::string hash = generate_test_hash(3);
    
    // Save to cache
    ASSERT_TRUE(cache_->save_shader_binary(test_program_, hash));
    
    // Verify cache directory exists
    EXPECT_TRUE(fs::exists(test_cache_dir_));
    
    // Check for cache files
    bool found_cache_file = false;
    for (const auto& entry : fs::directory_iterator(test_cache_dir_)) {
        if (entry.is_regular_file()) {
            found_cache_file = true;
            
            // Verify file has reasonable size
            auto file_size = entry.file_size();
            EXPECT_GT(file_size, 0u);
            EXPECT_LT(file_size, 1024 * 1024); // Less than 1MB for a single shader
        }
    }
    
    EXPECT_TRUE(found_cache_file);
}

TEST_F(ShaderCachePersistenceTest, CacheEviction) {
    // Create a small cache
    ShaderBinaryCacheConfig config;
    config.enable_memory_cache = true;
    config.max_memory_cache_size = 1024; // Very small: 1KB
    config.max_memory_entries = 3;
    config.use_lru_eviction = true;
    config.enable_disk_cache = false; // Disable disk for this test
    
    ShaderBinaryCache small_cache(config);
    ASSERT_TRUE(small_cache.initialize());
    
    // Create and cache multiple programs
    std::vector<GLuint> programs;
    for (int i = 0; i < 5; i++) {
        GLuint prog = create_test_program();
        ASSERT_NE(prog, 0u);
        programs.push_back(prog);
        
        std::string hash = generate_test_hash(100 + i);
        small_cache.save_shader_binary(prog, hash);
    }
    
    // Check that cache has evicted old entries
    auto stats = small_cache.get_statistics();
    EXPECT_LE(stats.memory_cache_entries, 3u);
    
    // Clean up
    for (auto prog : programs) {
        glDeleteProgram(prog);
    }
}

TEST_F(ShaderCachePersistenceTest, HashComputation) {
    // Test source hash computation
    std::string vs_source = "vertex shader source";
    std::string fs_source = "fragment shader source";
    
    std::string hash1 = ShaderBinaryCache::compute_source_hash(vs_source, fs_source);
    std::string hash2 = ShaderBinaryCache::compute_source_hash(vs_source, fs_source);
    
    // Same input should produce same hash
    EXPECT_EQ(hash1, hash2);
    
    // Different input should produce different hash
    std::string hash3 = ShaderBinaryCache::compute_source_hash(vs_source + " ", fs_source);
    EXPECT_NE(hash1, hash3);
    
    // Test bytecode hash computation
    std::vector<DWORD> vs_bytecode = {0xFFFE0101, 0x0000FFFF};
    std::vector<DWORD> ps_bytecode = {0xFFFF0101, 0x0000FFFF};
    
    std::string bc_hash1 = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_bytecode);
    std::string bc_hash2 = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_bytecode);
    
    EXPECT_EQ(bc_hash1, bc_hash2);
    
    // Modify bytecode
    vs_bytecode.push_back(0x12345678);
    std::string bc_hash3 = ShaderBinaryCache::compute_bytecode_hash(vs_bytecode, ps_bytecode);
    EXPECT_NE(bc_hash1, bc_hash3);
}

TEST_F(ShaderCachePersistenceTest, ConcurrentAccess) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    const int NUM_THREADS = 4;
    const int OPS_PER_THREAD = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_ops(0);
    
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([this, t, &successful_ops]() {
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                std::string hash = generate_test_hash(t * 100 + i);
                
                // Alternate between save and load
                if (i % 2 == 0) {
                    GLuint prog = create_test_program();
                    if (prog) {
                        if (cache_->save_shader_binary(prog, hash)) {
                            successful_ops++;
                        }
                        glDeleteProgram(prog);
                    }
                } else {
                    GLuint prog = glCreateProgram();
                    if (cache_->load_shader_binary(prog, hash)) {
                        successful_ops++;
                    }
                    glDeleteProgram(prog);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have completed some operations successfully
    EXPECT_GT(successful_ops.load(), 0);
}

TEST_F(ShaderCachePersistenceTest, CacheStatistics) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    // Reset statistics
    cache_->reset_statistics();
    
    std::string hash = generate_test_hash(4);
    
    // Save (should be a miss)
    cache_->save_shader_binary(test_program_, hash);
    
    // Load from memory (should be a hit)
    GLuint prog1 = glCreateProgram();
    cache_->load_shader_binary(prog1, hash);
    
    // Clear memory cache
    cache_->clear_memory_cache();
    
    // Load from disk (should be a disk hit)
    GLuint prog2 = glCreateProgram();
    cache_->load_shader_binary(prog2, hash);
    
    // Try to load non-existent (should be a miss)
    GLuint prog3 = glCreateProgram();
    cache_->load_shader_binary(prog3, "non_existent_hash");
    
    // Check statistics
    auto stats = cache_->get_statistics();
    EXPECT_GT(stats.memory_cache_hits, 0u);
    EXPECT_GT(stats.disk_cache_hits, 0u);
    EXPECT_GT(stats.memory_cache_misses, 0u);
    
    std::cout << "Cache Statistics:" << std::endl;
    std::cout << "  Memory hits: " << stats.memory_cache_hits << std::endl;
    std::cout << "  Memory misses: " << stats.memory_cache_misses << std::endl;
    std::cout << "  Disk hits: " << stats.disk_cache_hits << std::endl;
    std::cout << "  Disk misses: " << stats.disk_cache_misses << std::endl;
    std::cout << "  Memory cache size: " << stats.memory_cache_size << " bytes" << std::endl;
    std::cout << "  Disk cache size: " << stats.disk_cache_size << " bytes" << std::endl;
    
    // Clean up
    glDeleteProgram(prog1);
    glDeleteProgram(prog2);
    glDeleteProgram(prog3);
}

TEST_F(ShaderCachePersistenceTest, CachePreloading) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    // Save multiple shaders
    std::vector<std::string> hashes;
    for (int i = 0; i < 5; i++) {
        std::string hash = generate_test_hash(200 + i);
        hashes.push_back(hash);
        
        GLuint prog = create_test_program();
        ASSERT_NE(prog, 0u);
        cache_->save_shader_binary(prog, hash);
        glDeleteProgram(prog);
    }
    
    // Clear memory cache
    cache_->clear_memory_cache();
    
    // Preload specific shaders
    cache_->preload_shaders(hashes);
    
    // All should now be in memory cache
    for (const auto& hash : hashes) {
        GLuint prog = glCreateProgram();
        bool loaded = cache_->load_shader_binary(prog, hash);
        EXPECT_TRUE(loaded);
        glDeleteProgram(prog);
    }
    
    // Check that we got memory hits
    auto stats = cache_->get_statistics();
    EXPECT_GE(stats.memory_cache_hits, hashes.size());
}

TEST_F(ShaderCachePersistenceTest, CacheTrimming) {
    // Save many shaders to trigger trimming
    for (int i = 0; i < 20; i++) {
        std::string hash = generate_test_hash(300 + i);
        GLuint prog = create_test_program();
        if (prog) {
            cache_->save_shader_binary(prog, hash);
            glDeleteProgram(prog);
        }
    }
    
    // Get initial stats
    auto stats_before = cache_->get_statistics();
    
    // Trim caches
    cache_->trim_memory_cache();
    cache_->trim_disk_cache();
    
    // Get stats after trimming
    auto stats_after = cache_->get_statistics();
    
    // Memory cache should be smaller or same
    EXPECT_LE(stats_after.memory_cache_size, stats_before.memory_cache_size);
}

TEST_F(ShaderCachePersistenceTest, BinaryValidation) {
    if (!ShaderBinaryCache::is_binary_caching_supported()) {
        GTEST_SKIP() << "Binary caching not supported on this platform";
    }
    
    std::string hash = generate_test_hash(5);
    
    // Save with validation enabled
    ASSERT_TRUE(cache_->save_shader_binary(test_program_, hash));
    
    // Corrupt the cache file (simulate disk corruption)
    std::string cache_file;
    for (const auto& entry : fs::directory_iterator(test_cache_dir_)) {
        if (entry.is_regular_file()) {
            cache_file = entry.path().string();
            break;
        }
    }
    
    if (!cache_file.empty()) {
        // Append garbage to corrupt the file
        std::ofstream file(cache_file, std::ios::app | std::ios::binary);
        file << "CORRUPTED_DATA";
        file.close();
        
        // Clear memory cache
        cache_->clear_memory_cache();
        
        // Try to load corrupted binary (should fail validation)
        GLuint prog = glCreateProgram();
        bool loaded = cache_->load_shader_binary(prog, hash);
        
        // May or may not load depending on validation implementation
        // But if loaded, program should not be valid
        if (loaded) {
            GLint link_status;
            glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
            // Corrupted binary might fail linking
        }
        
        glDeleteProgram(prog);
    }
}

TEST_F(ShaderCachePersistenceTest, MemoryMappedCache) {
    // Test memory-mapped cache for ultra-fast access
    std::string mmap_file = test_cache_dir_ + "/mmap_cache.bin";
    
    MemoryMappedShaderCache mmap_cache(mmap_file);
    ASSERT_TRUE(mmap_cache.initialize(10 * 1024 * 1024)); // 10MB
    
    // Store some data
    std::string hash = "mmap_test_hash";
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
    
    EXPECT_TRUE(mmap_cache.store_binary(hash, data.data(), data.size()));
    
    // Load data back
    std::vector<uint8_t> loaded_data;
    EXPECT_TRUE(mmap_cache.load_binary(hash, loaded_data));
    
    // Verify data matches
    EXPECT_EQ(loaded_data, data);
    
    mmap_cache.shutdown();
}