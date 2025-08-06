#include "webgpu_async_handler.h"
#include "logger.h"
#include <cstring>

#ifdef DX8GL_HAS_WEBGPU

namespace dx8gl {

// WebGPUAsyncHandler implementation

WebGPUAsyncHandler::AdapterFuture WebGPUAsyncHandler::request_adapter_async(
    const WGpuRequestAdapterOptions* options) {
    
    auto context = std::make_unique<AdapterContext>();
    auto future = context->promise.get_future();
    
    // Request adapter with our callback
    wgpu_instance_request_adapter(nullptr, options, adapter_callback, context.release());
    
    return future;
}

WebGPUAsyncHandler::DeviceFuture WebGPUAsyncHandler::request_device_async(
    WGpuAdapter adapter, const WGpuDeviceDescriptor* desc) {
    
    auto context = std::make_unique<DeviceContext>();
    auto future = context->promise.get_future();
    
    // Request device with our callback
    wgpu_adapter_request_device(adapter, desc, device_callback, context.release());
    
    return future;
}

WebGPUAsyncHandler::BufferMapFuture WebGPUAsyncHandler::map_buffer_async(
    WGpuBuffer buffer, WGpuMapModeFlags mode, size_t offset, size_t size) {
    
    auto context = std::make_unique<BufferMapContext>();
    context->buffer = buffer;
    context->offset = offset;
    context->size = size;
    auto future = context->promise.get_future();
    
    // Map buffer with our callback
    wgpu_buffer_map_async(buffer, mode, offset, size, buffer_map_callback, context.release());
    
    return future;
}

void WebGPUAsyncHandler::adapter_callback(WGpuRequestAdapterStatus status, WGpuAdapter adapter,
                                         const char* message, void* user_data) {
    auto context = std::unique_ptr<AdapterContext>(static_cast<AdapterContext*>(user_data));
    if (!context) return;
    
    std::lock_guard<std::mutex> lock(context->mutex);
    if (context->completed) return;  // Prevent double completion
    context->completed = true;
    
    AdapterResult result;
    result.success = (status == WGPU_REQUEST_ADAPTER_STATUS_SUCCESS);
    result.adapter = adapter;
    result.error_message = message ? message : "";
    
    if (result.success) {
        DX8GL_INFO("WebGPU adapter created successfully");
    } else {
        DX8GL_ERROR("Failed to create WebGPU adapter: %s", result.error_message.c_str());
    }
    
    context->promise.set_value(std::move(result));
}

void WebGPUAsyncHandler::device_callback(WGpuRequestDeviceStatus status, WGpuDevice device,
                                        const char* message, void* user_data) {
    auto context = std::unique_ptr<DeviceContext>(static_cast<DeviceContext*>(user_data));
    if (!context) return;
    
    std::lock_guard<std::mutex> lock(context->mutex);
    if (context->completed) return;  // Prevent double completion
    context->completed = true;
    
    DeviceResult result;
    result.success = (status == WGPU_REQUEST_DEVICE_STATUS_SUCCESS);
    result.device = device;
    result.error_message = message ? message : "";
    
    if (result.success) {
        DX8GL_INFO("WebGPU device created successfully");
    } else {
        DX8GL_ERROR("Failed to create WebGPU device: %s", result.error_message.c_str());
    }
    
    context->promise.set_value(std::move(result));
}

void WebGPUAsyncHandler::buffer_map_callback(WGpuBufferMapAsyncStatus status, void* user_data) {
    auto context = std::unique_ptr<BufferMapContext>(static_cast<BufferMapContext*>(user_data));
    if (!context) return;
    
    std::lock_guard<std::mutex> lock(context->mutex);
    if (context->completed) return;  // Prevent double completion
    context->completed = true;
    
    BufferMapResult result;
    result.success = (status == WGPU_BUFFER_MAP_ASYNC_STATUS_SUCCESS);
    
    if (result.success) {
        result.mapped_data = wgpu_buffer_get_mapped_range(context->buffer, context->offset, context->size);
        result.mapped_size = context->size;
        DX8GL_TRACE("WebGPU buffer mapped successfully");
    } else {
        result.mapped_data = nullptr;
        result.mapped_size = 0;
        result.error_message = "Buffer mapping failed";
        DX8GL_ERROR("Failed to map WebGPU buffer: status=%d", status);
    }
    
    context->promise.set_value(std::move(result));
}

// WebGPUAsyncResource implementation

WebGPUAsyncResource::WebGPUAsyncResource(WebGPUAsyncResource&& other) noexcept
    : adapter_(other.adapter_)
    , device_(other.device_)
    , queue_(other.queue_)
    , error_message_(std::move(other.error_message_)) {
    other.adapter_ = nullptr;
    other.device_ = nullptr;
    other.queue_ = nullptr;
}

WebGPUAsyncResource& WebGPUAsyncResource::operator=(WebGPUAsyncResource&& other) noexcept {
    if (this != &other) {
        release();
        adapter_ = other.adapter_;
        device_ = other.device_;
        queue_ = other.queue_;
        error_message_ = std::move(other.error_message_);
        other.adapter_ = nullptr;
        other.device_ = nullptr;
        other.queue_ = nullptr;
    }
    return *this;
}

bool WebGPUAsyncResource::init_adapter(const WGpuRequestAdapterOptions* options,
                                      std::chrono::milliseconds timeout) {
    release();
    
    auto future = WebGPUAsyncHandler::request_adapter_async(options);
    
    if (!WebGPUAsyncHandler::wait_for_future(future, timeout)) {
        error_message_ = "Adapter request timed out";
        DX8GL_ERROR("%s", error_message_.c_str());
        return false;
    }
    
    auto result = future.get();
    if (!result.success) {
        error_message_ = result.error_message;
        return false;
    }
    
    adapter_ = result.adapter;
    return true;
}

bool WebGPUAsyncResource::init_device(const WGpuDeviceDescriptor* desc,
                                     std::chrono::milliseconds timeout) {
    if (!adapter_) {
        error_message_ = "No adapter available";
        return false;
    }
    
    auto future = WebGPUAsyncHandler::request_device_async(adapter_, desc);
    
    if (!WebGPUAsyncHandler::wait_for_future(future, timeout)) {
        error_message_ = "Device request timed out";
        DX8GL_ERROR("%s", error_message_.c_str());
        return false;
    }
    
    auto result = future.get();
    if (!result.success) {
        error_message_ = result.error_message;
        return false;
    }
    
    device_ = result.device;
    queue_ = wgpu_device_get_queue(device_);
    return true;
}

void WebGPUAsyncResource::release() {
    if (queue_) {
        wgpu_object_destroy(queue_);
        queue_ = nullptr;
    }
    if (device_) {
        wgpu_object_destroy(device_);
        device_ = nullptr;
    }
    if (adapter_) {
        wgpu_object_destroy(adapter_);
        adapter_ = nullptr;
    }
}

// WebGPUAsyncCommand implementation

WebGPUAsyncCommand::WebGPUAsyncCommand(WGpuDevice device, WGpuQueue queue)
    : device_(device)
    , queue_(queue) {
}

WebGPUAsyncCommand::~WebGPUAsyncCommand() {
    // Wait for any remaining commands
    wait_all(std::chrono::seconds(30));
}

void WebGPUAsyncCommand::submit_async(WGpuCommandBuffer command_buffer, CompletionCallback callback) {
    std::lock_guard<std::mutex> lock(commands_mutex_);
    
    // Submit the command buffer
    wgpu_queue_submit(queue_, 1, &command_buffer);
    
    // Track the submission
    PendingCommand pending;
    pending.submission_id = next_submission_id_++;
    pending.callback = callback;
    pending.submit_time = std::chrono::steady_clock::now();
    
    // Register callback for completion
    wgpu_queue_on_submitted_work_done(queue_, on_submitted_work_done, 
                                      new std::pair<WebGPUAsyncCommand*, uint64_t>(this, pending.submission_id));
    
    pending_commands_.push_back(std::move(pending));
}

void WebGPUAsyncCommand::wait_all(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(commands_mutex_);
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (!pending_commands_.empty()) {
        auto wait_time = deadline - std::chrono::steady_clock::now();
        if (wait_time <= std::chrono::milliseconds::zero()) {
            DX8GL_ERROR("Timeout waiting for WebGPU commands to complete");
            break;
        }
        
        commands_cv_.wait_for(lock, wait_time, [this] {
            return pending_commands_.empty();
        });
    }
}

bool WebGPUAsyncCommand::has_pending_commands() const {
    std::lock_guard<std::mutex> lock(commands_mutex_);
    return !pending_commands_.empty();
}

void WebGPUAsyncCommand::on_submitted_work_done(WGpuQueueWorkDoneStatus status, void* user_data) {
    auto* pair = static_cast<std::pair<WebGPUAsyncCommand*, uint64_t>*>(user_data);
    if (!pair) return;
    
    auto* command = pair->first;
    auto submission_id = pair->second;
    delete pair;
    
    CompletionCallback callback;
    {
        std::lock_guard<std::mutex> lock(command->commands_mutex_);
        
        // Find and remove the pending command
        auto it = std::find_if(command->pending_commands_.begin(), command->pending_commands_.end(),
                               [submission_id](const PendingCommand& cmd) {
                                   return cmd.submission_id == submission_id;
                               });
        
        if (it != command->pending_commands_.end()) {
            callback = it->callback;
            command->pending_commands_.erase(it);
        }
    }
    
    // Notify waiters
    command->commands_cv_.notify_all();
    
    // Call callback outside of lock
    if (callback) {
        bool success = (status == WGPU_QUEUE_WORK_DONE_STATUS_SUCCESS);
        callback(success);
    }
}

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU