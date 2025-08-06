#ifndef DX8GL_WEBGPU_ASYNC_HANDLER_H
#define DX8GL_WEBGPU_ASYNC_HANDLER_H

#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"

namespace dx8gl {

/**
 * Helper class to handle WebGPU async operations with promises/futures
 * instead of polling loops
 */
class WebGPUAsyncHandler {
public:
    // Result types for async operations
    struct AdapterResult {
        bool success;
        WGpuAdapter adapter;
        std::string error_message;
    };
    
    struct DeviceResult {
        bool success;
        WGpuDevice device;
        std::string error_message;
    };
    
    struct BufferMapResult {
        bool success;
        void* mapped_data;
        size_t mapped_size;
        std::string error_message;
    };
    
    // Async operation futures
    using AdapterFuture = std::future<AdapterResult>;
    using DeviceFuture = std::future<DeviceResult>;
    using BufferMapFuture = std::future<BufferMapResult>;
    
    // Request adapter asynchronously
    static AdapterFuture request_adapter_async(const WGpuRequestAdapterOptions* options);
    
    // Request device asynchronously
    static DeviceFuture request_device_async(WGpuAdapter adapter, const WGpuDeviceDescriptor* desc);
    
    // Map buffer asynchronously
    static BufferMapFuture map_buffer_async(WGpuBuffer buffer, WGpuMapModeFlags mode,
                                           size_t offset, size_t size);
    
    // Wait with timeout for any future
    template<typename T>
    static bool wait_for_future(std::future<T>& future, std::chrono::milliseconds timeout) {
        return future.wait_for(timeout) == std::future_status::ready;
    }
    
private:
    // Callback context structures
    struct AdapterContext {
        std::promise<AdapterResult> promise;
        bool completed = false;
        std::mutex mutex;
    };
    
    struct DeviceContext {
        std::promise<DeviceResult> promise;
        bool completed = false;
        std::mutex mutex;
    };
    
    struct BufferMapContext {
        std::promise<BufferMapResult> promise;
        WGpuBuffer buffer;
        size_t offset;
        size_t size;
        bool completed = false;
        std::mutex mutex;
    };
    
    // Static callbacks for WebGPU
    static void adapter_callback(WGpuRequestAdapterStatus status, WGpuAdapter adapter,
                                const char* message, void* user_data);
    static void device_callback(WGpuRequestDeviceStatus status, WGpuDevice device,
                               const char* message, void* user_data);
    static void buffer_map_callback(WGpuBufferMapAsyncStatus status, void* user_data);
};

/**
 * RAII wrapper for async WebGPU resources
 */
class WebGPUAsyncResource {
public:
    WebGPUAsyncResource() = default;
    ~WebGPUAsyncResource() { release(); }
    
    // Move-only semantics
    WebGPUAsyncResource(WebGPUAsyncResource&& other) noexcept;
    WebGPUAsyncResource& operator=(WebGPUAsyncResource&& other) noexcept;
    WebGPUAsyncResource(const WebGPUAsyncResource&) = delete;
    WebGPUAsyncResource& operator=(const WebGPUAsyncResource&) = delete;
    
    // Initialize with adapter
    bool init_adapter(const WGpuRequestAdapterOptions* options,
                     std::chrono::milliseconds timeout = std::chrono::seconds(5));
    
    // Initialize device from adapter
    bool init_device(const WGpuDeviceDescriptor* desc,
                    std::chrono::milliseconds timeout = std::chrono::seconds(5));
    
    // Get resources
    WGpuAdapter get_adapter() const { return adapter_; }
    WGpuDevice get_device() const { return device_; }
    WGpuQueue get_queue() const { return queue_; }
    const std::string& get_error() const { return error_message_; }
    
    // Release resources
    void release();
    
private:
    WGpuAdapter adapter_ = nullptr;
    WGpuDevice device_ = nullptr;
    WGpuQueue queue_ = nullptr;
    std::string error_message_;
};

/**
 * Async command submission helper
 */
class WebGPUAsyncCommand {
public:
    using CompletionCallback = std::function<void(bool success)>;
    
    WebGPUAsyncCommand(WGpuDevice device, WGpuQueue queue);
    ~WebGPUAsyncCommand();
    
    // Submit commands with completion callback
    void submit_async(WGpuCommandBuffer command_buffer, CompletionCallback callback);
    
    // Wait for all pending commands
    void wait_all(std::chrono::milliseconds timeout = std::chrono::seconds(10));
    
    // Check if any commands are pending
    bool has_pending_commands() const;
    
private:
    WGpuDevice device_;
    WGpuQueue queue_;
    
    struct PendingCommand {
        uint64_t submission_id;
        CompletionCallback callback;
        std::chrono::steady_clock::time_point submit_time;
    };
    
    std::vector<PendingCommand> pending_commands_;
    mutable std::mutex commands_mutex_;
    std::condition_variable commands_cv_;
    uint64_t next_submission_id_ = 1;
    
    // Queue submit callback
    static void on_submitted_work_done(WGpuQueueWorkDoneStatus status, void* user_data);
};

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU

#endif // DX8GL_WEBGPU_ASYNC_HANDLER_H