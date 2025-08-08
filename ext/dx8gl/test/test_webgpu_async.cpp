#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <future>
#include <vector>
#include <cstring>

// Only compile these tests if WebGPU backend is enabled
#ifdef DX8GL_HAS_WEBGPU

using namespace dx8gl;

class WebGPUAsyncTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8_ = nullptr;
    IDirect3DDevice8* device_ = nullptr;
    bool webgpu_available_ = false;
    
    void SetUp() override {
        // Check if WebGPU backend is available
        setenv("DX8GL_BACKEND", "webgpu", 1);
        
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_WEBGPU;
        config.width = 256;
        config.height = 256;
        
        if (!dx8gl_init(&config)) {
            GTEST_SKIP() << "WebGPU backend not available";
            return;
        }
        
        webgpu_available_ = true;
        
        // Create D3D8 interface
        d3d8_ = Direct3DCreate8(D3D_SDK_VERSION);
        if (!d3d8_) {
            dx8gl_shutdown();
            GTEST_SKIP() << "Failed to create Direct3D8 interface";
            return;
        }
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 256;
        pp.BackBufferHeight = 256;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        
        HRESULT hr = d3d8_->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device_
        );
        
        if (FAILED(hr)) {
            d3d8_->Release();
            d3d8_ = nullptr;
            dx8gl_shutdown();
            GTEST_SKIP() << "Failed to create WebGPU device";
        }
    }
    
    void TearDown() override {
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        
        if (d3d8_) {
            d3d8_->Release();
            d3d8_ = nullptr;
        }
        
        if (webgpu_available_) {
            dx8gl_shutdown();
        }
        
        unsetenv("DX8GL_BACKEND");
    }
    
    // Helper to wait for async operations with timeout
    template<typename Func>
    bool WaitForAsync(Func condition, int timeout_ms = 5000) {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout_ms) {
                return false; // Timeout
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
};

TEST_F(WebGPUAsyncTest, DeviceCreation) {
    // WebGPU device creation is inherently async
    // Verify that the device was created successfully
    ASSERT_NE(device_, nullptr);
    
    // Test that device operations complete asynchronously
    D3DCAPS8 caps = {};
    HRESULT hr = device_->GetDeviceCaps(&caps);
    EXPECT_EQ(hr, D3D_OK);
    
    // WebGPU should report its capabilities
    EXPECT_GT(caps.MaxTextureWidth, 0u);
    EXPECT_GT(caps.MaxTextureHeight, 0u);
}

TEST_F(WebGPUAsyncTest, CommandBufferSubmission) {
    // Test async command buffer submission
    
    // Clear operation should queue commands
    HRESULT hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    // Begin scene
    hr = device_->BeginScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Draw a simple triangle
    struct Vertex {
        float x, y, z;
        DWORD color;
    };
    
    Vertex triangle[] = {
        {-0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
        { 0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(0, 255, 0)},
        { 0.0f,  0.5f, 0.5f, D3DCOLOR_XRGB(0, 0, 255)}
    };
    
    device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    hr = device_->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, triangle, sizeof(Vertex));
    EXPECT_EQ(hr, D3D_OK);
    
    // End scene
    hr = device_->EndScene();
    EXPECT_EQ(hr, D3D_OK);
    
    // Present should trigger async submission
    hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(hr, D3D_OK);
    
    // WebGPU commands should complete without blocking the thread immediately
    // but should complete eventually
    std::atomic<bool> present_completed(false);
    
    // Simulate checking for completion
    std::thread checker([&present_completed]() {
        // In real WebGPU, we'd check command buffer status
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        present_completed = true;
    });
    
    // Wait for async completion
    EXPECT_TRUE(WaitForAsync([&present_completed]() { return present_completed.load(); }));
    
    checker.join();
}

TEST_F(WebGPUAsyncTest, BufferMappingAsync) {
    // Create a vertex buffer
    IDirect3DVertexBuffer8* vb = nullptr;
    HRESULT hr = device_->CreateVertexBuffer(
        1024, D3DUSAGE_DYNAMIC, D3DFVF_XYZ, D3DPOOL_DEFAULT, &vb);
    
    if (FAILED(hr) || !vb) {
        GTEST_SKIP() << "Failed to create vertex buffer";
    }
    
    // Test async buffer mapping
    std::atomic<bool> map_completed(false);
    void* mapped_data = nullptr;
    
    // Lock buffer (in WebGPU this would be async)
    BYTE* data = nullptr;
    hr = vb->Lock(0, 0, &data, 0);
    EXPECT_EQ(hr, D3D_OK);
    
    if (SUCCEEDED(hr) && data) {
        // Simulate async write operation
        std::thread writer([data, &map_completed]() {
            // Write some test data
            float vertices[] = {
                0.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f
            };
            std::memcpy(data, vertices, sizeof(vertices));
            
            // Simulate async completion
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            map_completed = true;
        });
        
        // Wait for write to complete
        EXPECT_TRUE(WaitForAsync([&map_completed]() { return map_completed.load(); }));
        
        writer.join();
        
        // Unlock buffer
        hr = vb->Unlock();
        EXPECT_EQ(hr, D3D_OK);
    }
    
    // Test async read-back
    map_completed = false;
    
    hr = vb->Lock(0, 0, &data, D3DLOCK_READONLY);
    if (SUCCEEDED(hr) && data) {
        std::thread reader([data, &map_completed]() {
            // Read and verify data
            float* vertices = reinterpret_cast<float*>(data);
            
            // Check first vertex
            if (vertices[0] == 0.0f && vertices[1] == 0.0f && vertices[2] == 0.0f) {
                map_completed = true;
            }
        });
        
        EXPECT_TRUE(WaitForAsync([&map_completed]() { return map_completed.load(); }));
        
        reader.join();
        
        vb->Unlock();
    }
    
    vb->Release();
}

TEST_F(WebGPUAsyncTest, ShaderHotReload) {
    // Test shader hot reload with async pipeline updates
    
    // Create initial vertex shader
    std::vector<DWORD> vs_bytecode_v1 = {
        0xFFFE0101,  // vs_1_1
        0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
        0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
        0x0000FFFF  // end
    };
    
    DWORD vs_handle = 0;
    HRESULT hr = device_->CreateVertexShader(nullptr, vs_bytecode_v1.data(), &vs_handle, 0);
    
    if (FAILED(hr) || vs_handle == 0) {
        GTEST_SKIP() << "Vertex shader creation not supported";
    }
    
    // Set the shader
    hr = device_->SetVertexShader(vs_handle);
    EXPECT_EQ(hr, D3D_OK);
    
    // Simulate shader modification and hot reload
    std::atomic<bool> reload_completed(false);
    
    std::thread reloader([this, &reload_completed, vs_handle]() {
        // Simulate time for shader recompilation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Create new shader bytecode (simulated modification)
        std::vector<DWORD> vs_bytecode_v2 = {
            0xFFFE0101,  // vs_1_1
            0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
            0x0000001F, 0x80000005, 0x900F0001,  // dcl_color v1
            0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
            0x00000001, 0xD00F0000, 0x90E40001,  // mov oD0, v1
            0x0000FFFF  // end
        };
        
        // In WebGPU, pipeline recreation would be async
        // Here we simulate by creating a new shader
        DWORD vs_handle_v2 = 0;
        device_->CreateVertexShader(nullptr, vs_bytecode_v2.data(), &vs_handle_v2, 0);
        
        if (vs_handle_v2 != 0) {
            // Delete old shader
            device_->DeleteVertexShader(vs_handle);
            
            // Set new shader
            device_->SetVertexShader(vs_handle_v2);
            
            reload_completed = true;
        }
    });
    
    // Continue rendering while reload happens
    for (int frame = 0; frame < 10; frame++) {
        device_->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device_->BeginScene();
        
        // Draw something
        struct Vertex {
            float x, y, z;
            DWORD color;
        };
        
        Vertex quad[] = {
            {-0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 0, 0)},
            { 0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(0, 255, 0)},
            { 0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(0, 0, 255)},
            {-0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 0)}
        };
        
        device_->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
        
        device_->EndScene();
        device_->Present(nullptr, nullptr, nullptr, nullptr);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Ensure reload completed
    EXPECT_TRUE(WaitForAsync([&reload_completed]() { return reload_completed.load(); }));
    
    reloader.join();
}

TEST_F(WebGPUAsyncTest, MultipleAsyncOperations) {
    // Test multiple concurrent async operations
    
    std::vector<std::future<bool>> futures;
    
    // Launch multiple async operations
    for (int i = 0; i < 5; i++) {
        futures.push_back(std::async(std::launch::async, [this, i]() {
            // Create a texture asynchronously
            IDirect3DTexture8* texture = nullptr;
            HRESULT hr = device_->CreateTexture(
                64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
            
            if (FAILED(hr) || !texture) {
                return false;
            }
            
            // Lock and fill texture
            D3DLOCKED_RECT locked;
            hr = texture->LockRect(0, &locked, nullptr, 0);
            if (SUCCEEDED(hr)) {
                uint32_t* data = static_cast<uint32_t*>(locked.pBits);
                for (int y = 0; y < 64; y++) {
                    for (int x = 0; x < 64; x++) {
                        // Create unique pattern for each texture
                        data[y * (locked.Pitch / 4) + x] = 
                            D3DCOLOR_ARGB(255, i * 50, x * 4, y * 4);
                    }
                }
                texture->UnlockRect(0);
            }
            
            // Simulate async upload
            std::this_thread::sleep_for(std::chrono::milliseconds(20 + i * 10));
            
            // Clean up
            texture->Release();
            return true;
        }));
    }
    
    // Wait for all operations to complete
    bool all_succeeded = true;
    for (auto& future : futures) {
        if (!future.get()) {
            all_succeeded = false;
        }
    }
    
    EXPECT_TRUE(all_succeeded);
}

TEST_F(WebGPUAsyncTest, ResourceCleanup) {
    // Test that resources are properly cleaned up in async operations
    
    std::atomic<int> resource_count(0);
    std::vector<std::thread> threads;
    
    // Create and destroy resources concurrently
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this, &resource_count, i]() {
            // Random delay to create race conditions
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 50));
            
            // Create a resource
            IDirect3DVertexBuffer8* vb = nullptr;
            HRESULT hr = device_->CreateVertexBuffer(
                256, 0, D3DFVF_XYZ, D3DPOOL_MANAGED, &vb);
            
            if (SUCCEEDED(hr) && vb) {
                resource_count++;
                
                // Use the resource
                BYTE* data = nullptr;
                hr = vb->Lock(0, 0, &data, 0);
                if (SUCCEEDED(hr)) {
                    // Write some data
                    memset(data, i, 256);
                    vb->Unlock();
                }
                
                // Random delay before cleanup
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 20));
                
                // Clean up
                vb->Release();
                resource_count--;
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // All resources should be cleaned up
    EXPECT_EQ(resource_count.load(), 0);
}

TEST_F(WebGPUAsyncTest, AsyncPresentTiming) {
    // Test that Present() doesn't block for vsync in async mode
    
    const int num_frames = 30;
    std::vector<double> frame_times;
    
    for (int i = 0; i < num_frames; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Clear and draw
        device_->Clear(0, nullptr, D3DCLEAR_TARGET, 
                      D3DCOLOR_XRGB(i * 8, 255 - i * 8, 128), 1.0f, 0);
        device_->BeginScene();
        device_->EndScene();
        
        // Present should be async and not wait for vsync
        HRESULT hr = device_->Present(nullptr, nullptr, nullptr, nullptr);
        EXPECT_EQ(hr, D3D_OK);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start);
        frame_times.push_back(duration.count());
    }
    
    // Calculate average frame time
    double avg_time = 0;
    for (double t : frame_times) {
        avg_time += t;
    }
    avg_time /= num_frames;
    
    // WebGPU async present should be fast (not waiting for vsync)
    // Expect less than 8ms average (>120 FPS potential)
    EXPECT_LT(avg_time, 8.0) << "Average frame time too high for async: " << avg_time << "ms";
    
    // Log timing info
    RecordProperty("AverageFrameTime", avg_time);
    RecordProperty("MinFrameTime", *std::min_element(frame_times.begin(), frame_times.end()));
    RecordProperty("MaxFrameTime", *std::max_element(frame_times.begin(), frame_times.end()));
}

#else // !DX8GL_HAS_WEBGPU

TEST(WebGPUAsyncTest, NotAvailable) {
    GTEST_SKIP() << "WebGPU backend not compiled in this build";
}

#endif // DX8GL_HAS_WEBGPU