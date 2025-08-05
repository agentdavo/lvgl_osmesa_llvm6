#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include "../src/shader_binary_cache.h"
#include "../src/logger.h"

using namespace dx8gl;

bool test_shader_cache_resize() {
    std::cout << "=== Test: Shader Cache with Varying Binary Sizes ===\n";
    
    // Create in-memory cache for testing
    auto cache = std::make_unique<ShaderBinaryCache>();
    
    // Test 1: Store binary, then try to store same hash with different size
    std::cout << "\nTest 1: Attempting to store different sized binary with same hash\n";
    
    std::string test_hash = "test_hash_12345";
    
    // Store initial binary (small size)
    std::vector<uint8_t> small_binary(100, 0xAA);
    bool stored = cache->store_shader_binary(0, test_hash);  // Using dummy program ID
    if (stored) {
        std::cout << "Stored initial binary (size: " << small_binary.size() << ")\n";
    } else {
        std::cerr << "Failed to store initial binary\n";
        return false;
    }
    
    // Try to store larger binary with same hash
    std::vector<uint8_t> large_binary(500, 0xBB);
    stored = cache->store_binary(test_hash, large_binary.data(), large_binary.size());
    if (!stored) {
        std::cout << "PASS: Cache correctly rejected binary with different size (expected behavior)\n";
    } else {
        std::cerr << "FAIL: Cache accepted binary with different size - this should not happen!\n";
        return false;
    }
    
    // Verify original binary is still intact
    std::vector<uint8_t> retrieved;
    bool loaded = cache->load_binary(test_hash, retrieved);
    if (loaded) {
        if (retrieved.size() == small_binary.size() &&
            memcmp(retrieved.data(), small_binary.data(), small_binary.size()) == 0) {
            std::cout << "PASS: Original binary remains intact\n";
        } else {
            std::cerr << "FAIL: Retrieved binary doesn't match original\n";
            return false;
        }
    } else {
        std::cerr << "FAIL: Could not retrieve original binary\n";
        return false;
    }
    
    // Test 2: Store binaries of various sizes with different hashes
    std::cout << "\nTest 2: Storing binaries of various sizes with unique hashes\n";
    
    const size_t test_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096};
    const size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t i = 0; i < num_sizes; i++) {
        size_t size = test_sizes[i];
        std::string hash = "size_test_" + std::to_string(size);
        
        // Create binary with pattern based on size
        std::vector<uint8_t> binary(size);
        for (size_t j = 0; j < size; j++) {
            binary[j] = static_cast<uint8_t>((j + i) & 0xFF);
        }
        
        // Store binary
        if (cache->store_binary(hash, binary.data(), binary.size())) {
            std::cout << "Stored binary with size " << size << " bytes\n";
        } else {
            std::cerr << "Failed to store binary with size " << size << " bytes\n";
            return false;
        }
    }
    
    // Verify all binaries can be retrieved correctly
    std::cout << "\nVerifying all stored binaries...\n";
    for (size_t i = 0; i < num_sizes; i++) {
        size_t size = test_sizes[i];
        std::string hash = "size_test_" + std::to_string(size);
        
        std::vector<uint8_t> retrieved;
        if (cache->load_binary(hash, retrieved)) {
            if (retrieved.size() == size) {
                // Verify pattern
                bool pattern_correct = true;
                for (size_t j = 0; j < size; j++) {
                    if (retrieved[j] != static_cast<uint8_t>((j + i) & 0xFF)) {
                        pattern_correct = false;
                        break;
                    }
                }
                
                if (pattern_correct) {
                    std::cout << "Verified binary with size " << size << " bytes\n";
                } else {
                    std::cerr << "Binary with size " << size << " has corrupted data\n";
                    return false;
                }
            } else {
                std::cerr << "Binary with size " << size << " returned wrong size: " << retrieved.size() << "\n";
                return false;
            }
        } else {
            std::cerr << "Failed to retrieve binary with size " << size << "\n";
            return false;
        }
    }
    
    // Test 3: Memory-mapped cache with size changes
    std::cout << "\nTest 3: Testing memory-mapped cache with size changes\n";
    
    // Create memory-mapped cache
    cache->set_cache_directory("test_cache_resize");
    cache->enable_memory_mapped_cache(10 * 1024 * 1024); // 10MB
    
    std::string mmap_hash = "mmap_test_hash";
    
    // Store initial binary
    std::vector<uint8_t> mmap_binary1(256, 0xCC);
    stored = cache->store_binary(mmap_hash, mmap_binary1.data(), mmap_binary1.size());
    if (stored) {
        std::cout << "Stored binary in memory-mapped cache (size: " << mmap_binary1.size() << ")\n";
    } else {
        std::cerr << "Failed to store in memory-mapped cache\n";
        return false;
    }
    
    // Try to store different size with same hash
    std::vector<uint8_t> mmap_binary2(512, 0xDD);
    stored = cache->store_binary(mmap_hash, mmap_binary2.data(), mmap_binary2.size());
    if (!stored) {
        std::cout << "PASS: Memory-mapped cache correctly rejected size change\n";
    } else {
        std::cerr << "FAIL: Memory-mapped cache accepted size change\n";
        return false;
    }
    
    // Verify original is intact
    std::vector<uint8_t> mmap_retrieved;
    loaded = cache->load_binary(mmap_hash, mmap_retrieved);
    if (loaded && mmap_retrieved.size() == mmap_binary1.size() &&
        memcmp(mmap_retrieved.data(), mmap_binary1.data(), mmap_binary1.size()) == 0) {
        std::cout << "PASS: Original binary in memory-mapped cache is intact\n";
    } else {
        std::cerr << "FAIL: Memory-mapped cache binary corrupted\n";
        return false;
    }
    
    std::cout << "\nAll shader cache resize tests completed successfully!\n";
    return true;
}

int main() {
    std::cout << "Running Shader Cache Resize Tests\n";
    std::cout << "=================================\n";
    
    // Initialize dx8gl logging
    // Note: Logger is a singleton and can't be created directly
    // The logging will use default settings
    
    bool success = test_shader_cache_resize();
    
    if (success) {
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!\n";
        return 1;
    }
}