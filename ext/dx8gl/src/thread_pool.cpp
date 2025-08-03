#include "thread_pool.h"
#include <algorithm>

namespace dx8gl {

ThreadPool::ThreadPool(size_t num_threads) 
    : stop_(false)
    , active_threads_(0)
    , total_tasks_processed_(0) {
    
#ifdef __EMSCRIPTEN__
    // Emscripten without pthread support - use single-threaded mode
    num_threads = 0;
    DX8GL_INFO("Creating thread pool in single-threaded mode (Emscripten)");
#else
    // If num_threads is 0, use hardware concurrency
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4;  // Default to 4 threads
        }
    }
    
    DX8GL_INFO("Creating thread pool with %zu threads", num_threads);
    
    threads_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&ThreadPool::worker_thread, this, i);
    }
#endif
}

ThreadPool::~ThreadPool() {
#ifndef __EMSCRIPTEN__
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
#endif
    
    DX8GL_INFO("Thread pool destroyed. Total tasks processed: %zu", 
               total_tasks_processed_.load());
}

void ThreadPool::wait_all() {
#ifdef __EMSCRIPTEN__
    // In single-threaded mode, all tasks are executed immediately
    return;
#else
    std::unique_lock<std::mutex> lock(queue_mutex_);
    finished_condition_.wait(lock, [this] {
        return tasks_.empty() && active_threads_ == 0;
    });
#endif
}

size_t ThreadPool::pending_tasks() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

bool ThreadPool::is_idle() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.empty() && active_threads_ == 0;
}

void ThreadPool::worker_thread(size_t thread_id) {
    DX8GL_DEBUG("Worker thread %zu started", thread_id);
    
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) {
                break;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
                active_threads_++;
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                DX8GL_ERROR("Exception in worker thread %zu: %s", thread_id, e.what());
            } catch (...) {
                DX8GL_ERROR("Unknown exception in worker thread %zu", thread_id);
            }
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                active_threads_--;
                total_tasks_processed_++;
            }
            
            finished_condition_.notify_all();
        }
    }
    
    DX8GL_DEBUG("Worker thread %zu stopped", thread_id);
}

// Global thread pool instance
ThreadPool& get_global_thread_pool() {
    static ThreadPool pool;
    return pool;
}

} // namespace dx8gl