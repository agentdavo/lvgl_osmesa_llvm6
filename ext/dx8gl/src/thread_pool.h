#ifndef DX8GL_THREAD_POOL_H
#define DX8GL_THREAD_POOL_H

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <type_traits>
#include "logger.h"

namespace dx8gl {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 0);
    ~ThreadPool();

    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a task to the thread pool
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        
#ifdef __EMSCRIPTEN__
        // Execute immediately in single-threaded mode
        (*task)();
        total_tasks_processed_++;
#else
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            
            tasks_.emplace([task](){ (*task)(); });
        }
        
        condition_.notify_one();
#endif
        return res;
    }

    // Submit a batch of tasks
    template<typename F>
    void submit_batch(const std::vector<F>& tasks) {
#ifdef __EMSCRIPTEN__
        // Execute all tasks immediately in single-threaded mode
        for (const auto& task : tasks) {
            task();
            total_tasks_processed_++;
        }
#else
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (stop_) {
            throw std::runtime_error("submit_batch on stopped ThreadPool");
        }
        
        for (const auto& task : tasks) {
            tasks_.emplace(task);
        }
        
        condition_.notify_all();
#endif
    }

    // Wait for all tasks to complete
    void wait_all();
    
    // Get number of threads
    size_t thread_count() const { 
#ifdef __EMSCRIPTEN__
        return 0;  // Single-threaded mode
#else
        return threads_.size(); 
#endif
    }
    
    // Get number of pending tasks
    size_t pending_tasks() const;
    
    // Check if pool is idle
    bool is_idle() const;

private:
    void worker_thread(size_t thread_id);

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable finished_condition_;
    
    std::atomic<bool> stop_;
    std::atomic<size_t> active_threads_;
    std::atomic<size_t> total_tasks_processed_;
};

// RAII helper for parallel execution with automatic synchronization
template<typename F>
class ParallelExecutor {
public:
    ParallelExecutor(ThreadPool& pool, size_t num_tasks)
        : pool_(pool), futures_() {
        futures_.reserve(num_tasks);
    }
    
    ~ParallelExecutor() {
        wait();
    }
    
    template<typename... Args>
    void submit(F&& f, Args&&... args) {
        futures_.push_back(pool_.submit(std::forward<F>(f), std::forward<Args>(args)...));
    }
    
    void wait() {
        for (auto& future : futures_) {
            future.wait();
        }
        futures_.clear();
    }
    
private:
    ThreadPool& pool_;
    std::vector<std::future<void>> futures_;
};

// Global thread pool instance
ThreadPool& get_global_thread_pool();

} // namespace dx8gl

#endif // DX8GL_THREAD_POOL_H