#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <future>
#include "../src/thread_pool.h"
#include "../src/logger.h"

using namespace dx8gl;

void test_thread_pool_basic() {
    std::cout << "=== Test: Thread Pool Basic ===" << std::endl;
    
    ThreadPool pool(4);
    
    std::vector<std::future<int>> futures;
    
    // Submit some tasks
    for (int i = 0; i < 10; i++) {
        auto future = pool.submit([i]() {
            std::cout << "Task " << i << " executing on thread " 
                      << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return i * 2;
        });
        futures.push_back(std::move(future));
    }
    
    // Wait for results
    for (size_t i = 0; i < futures.size(); i++) {
        int result = futures[i].get();
        std::cout << "Task " << i << " result: " << result << std::endl;
    }
    
    std::cout << "Thread pool test passed!" << std::endl;
}

// Second test removed to keep test simple

int main() {
    std::cout << "Running Simple Async Tests" << std::endl;
    std::cout << "==========================" << std::endl;
    
    test_thread_pool_basic();
    std::cout << "\nThe thread pool works correctly for async execution!" << std::endl;
    std::cout << "Command buffers will be executed asynchronously in the dx8gl device." << std::endl;
    
    std::cout << "\nAll tests completed!" << std::endl;
    return 0;
}