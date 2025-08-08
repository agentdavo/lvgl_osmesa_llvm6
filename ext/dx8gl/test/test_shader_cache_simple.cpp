#include <gtest/gtest.h>
#include "shader_binary_cache.h"
#include "d3d8_types.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>

using namespace dx8gl;
namespace fs = std::filesystem;

class ShaderCacheSimpleTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderBinaryCache> cache_;
    std::string test_cache_dir_;
    
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
    }
    
    void TearDown() override {
        cache_->shutdown();
        cache_.reset();
        
        // Clean up test cache directory
        if (fs::exists(test_cache_dir_)) {
            fs::remove_all(test_cache_dir_);
        }
    }
    
    std::string generate_test_hash(int id) {
        return "test_shader_hash_" + std::to_string(id);
    }
};

TEST_F(ShaderCacheSimpleTest, HashComputation) {
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

TEST_F(ShaderCacheSimpleTest, CacheFileStructure) {
    // Note: Saving actual shader binaries requires OpenGL context,
    // but we can test the directory structure creation
    
    // Verify cache directory exists
    EXPECT_TRUE(fs::exists(test_cache_dir_));
    
    // Test that the cache can create subdirectories
    cache_->clear_disk_cache();
    
    // Directory should still exist after clear
    EXPECT_TRUE(fs::exists(test_cache_dir_));
}

TEST_F(ShaderCacheSimpleTest, BytecodeHashArray) {
    // Test bytecode hash with raw array
    DWORD bytecode[] = {0xFFFE0101, 0x00000001, 0x0000FFFF};
    std::string hash1 = ShaderBinaryCache::compute_bytecode_hash(bytecode, 3);
    
    DWORD bytecode2[] = {0xFFFE0101, 0x00000001, 0x0000FFFF};
    std::string hash2 = ShaderBinaryCache::compute_bytecode_hash(bytecode2, 3);
    
    EXPECT_EQ(hash1, hash2);
    
    // Different size
    std::string hash3 = ShaderBinaryCache::compute_bytecode_hash(bytecode, 2);
    EXPECT_NE(hash1, hash3);
}

TEST_F(ShaderCacheSimpleTest, CacheConfiguration) {
    auto config = cache_->get_config();
    
    EXPECT_TRUE(config.enable_memory_cache);
    EXPECT_TRUE(config.enable_disk_cache);
    EXPECT_EQ(config.disk_cache_directory, test_cache_dir_);
    EXPECT_TRUE(config.compress_disk_cache);
    EXPECT_FALSE(config.async_disk_operations);
    
    // Test configuration update
    ShaderBinaryCacheConfig new_config;
    new_config.enable_memory_cache = false;
    new_config.enable_disk_cache = true;
    new_config.disk_cache_directory = test_cache_dir_;
    
    cache_->set_config(new_config);
    auto updated_config = cache_->get_config();
    EXPECT_FALSE(updated_config.enable_memory_cache);
}

TEST_F(ShaderCacheSimpleTest, CacheStatisticsReset) {
    // Reset statistics
    cache_->reset_statistics();
    
    auto stats = cache_->get_statistics();
    EXPECT_EQ(stats.memory_cache_hits, 0u);
    EXPECT_EQ(stats.memory_cache_misses, 0u);
    EXPECT_EQ(stats.disk_cache_hits, 0u);
    EXPECT_EQ(stats.disk_cache_misses, 0u);
}

TEST_F(ShaderCacheSimpleTest, MemoryMappedCacheBasic) {
    // Test memory-mapped cache without actual OpenGL
    std::string mmap_file = test_cache_dir_ + "/mmap_cache.bin";
    
    MemoryMappedShaderCache mmap_cache(mmap_file);
    ASSERT_TRUE(mmap_cache.initialize(1 * 1024 * 1024)); // 1MB
    
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

TEST_F(ShaderCacheSimpleTest, ClearOperations) {
    // Test clear operations
    cache_->clear_memory_cache();
    cache_->clear_disk_cache();
    cache_->clear_all_caches();
    
    // Should not crash and directory should still exist
    EXPECT_TRUE(fs::exists(test_cache_dir_));
}

TEST_F(ShaderCacheSimpleTest, PreloadShaders) {
    // Test preload operations (won't actually load anything without real shaders)
    std::vector<std::string> hashes = {
        generate_test_hash(1),
        generate_test_hash(2),
        generate_test_hash(3)
    };
    
    // Should not crash
    cache_->preload_shaders(hashes);
    cache_->preload_shader(hashes[0]);
}

TEST_F(ShaderCacheSimpleTest, TrimOperations) {
    // Test trim operations
    cache_->trim_memory_cache();
    cache_->trim_disk_cache();
    
    // Should not crash
    cache_->validate_cache_entries();
}

TEST_F(ShaderCacheSimpleTest, BinaryCachingSupport) {
    // Check if binary caching is supported on this platform
    bool supported = ShaderBinaryCache::is_binary_caching_supported();
    
    // Just log the result, don't fail the test
    if (supported) {
        std::cout << "Shader binary caching is supported on this platform" << std::endl;
        
        // Get supported formats
        auto formats = ShaderBinaryCache::get_supported_binary_formats();
        std::cout << "Supported binary formats: " << formats.size() << std::endl;
        for (const auto& format : formats) {
            std::cout << "  Format: 0x" << std::hex << format.format 
                     << " Vendor: " << format.vendor 
                     << " Renderer: " << format.renderer << std::endl;
        }
    } else {
        std::cout << "Shader binary caching is not supported on this platform" << std::endl;
    }
}