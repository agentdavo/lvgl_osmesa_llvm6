#include <gtest/gtest.h>
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/dx8gl.h"
#include "../src/offscreen_framebuffer.h"
#include <vector>
#include <cmath>
#include <memory>
#include <fstream>

using namespace dx8gl;

/**
 * Cross-backend rendering test suite
 * 
 * This test suite renders canonical scenes with each backend and compares
 * the pixel outputs to ensure consistency across OSMesa, EGL, and WebGPU.
 */

class CrossBackendRenderingTest : public ::testing::Test {
protected:
    struct BackendTestContext {
        DX8BackendType backend_type;
        std::string backend_name;
        IDirect3D8* d3d8 = nullptr;
        IDirect3DDevice8* device = nullptr;
        std::vector<uint32_t> framebuffer_data;
        bool initialized = false;
    };
    
    void SetUp() override {
        // Test dimensions
        test_width_ = 256;
        test_height_ = 256;
        
        // Initialize backends to test
        backends_to_test_ = {
            {DX8GL_BACKEND_OSMESA, "OSMesa"},
#ifdef DX8GL_HAS_EGL
            {DX8GL_BACKEND_EGL, "EGL"},
#endif
#ifdef DX8GL_HAS_WEBGPU
            {DX8GL_BACKEND_WEBGPU, "WebGPU"},
#endif
        };
    }
    
    void TearDown() override {
        // Clean up all backend contexts
        for (auto& ctx : backends_to_test_) {
            cleanup_backend(ctx);
        }
    }
    
    bool init_backend(BackendTestContext& ctx) {
        // Initialize dx8gl with specific backend
        dx8gl_config config = {};
        config.backend_type = ctx.backend_type;
        config.width = test_width_;
        config.height = test_height_;
        
        if (dx8gl_init(&config) != DX8GL_SUCCESS) {
            return false;
        }
        
        // Create Direct3D8 interface
        ctx.d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        if (!ctx.d3d8) {
            dx8gl_shutdown();
            return false;
        }
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = test_width_;
        pp.BackBufferHeight = test_height_;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = D3DFMT_D24S8;
        
        HRESULT hr = ctx.d3d8->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &ctx.device
        );
        
        if (FAILED(hr)) {
            ctx.d3d8->Release();
            ctx.d3d8 = nullptr;
            dx8gl_shutdown();
            return false;
        }
        
        ctx.initialized = true;
        ctx.framebuffer_data.resize(test_width_ * test_height_);
        return true;
    }
    
    void cleanup_backend(BackendTestContext& ctx) {
        if (ctx.device) {
            ctx.device->Release();
            ctx.device = nullptr;
        }
        if (ctx.d3d8) {
            ctx.d3d8->Release();
            ctx.d3d8 = nullptr;
        }
        if (ctx.initialized) {
            dx8gl_shutdown();
            ctx.initialized = false;
        }
    }
    
    // Render a solid color triangle
    void render_solid_triangle(IDirect3DDevice8* device, uint32_t color) {
        struct Vertex {
            float x, y, z, rhw;
            DWORD color;
        };
        
        Vertex vertices[] = {
            { 128.0f,  50.0f, 0.5f, 1.0f, color },  // Top
            { 206.0f, 206.0f, 0.5f, 1.0f, color },  // Bottom right
            {  50.0f, 206.0f, 0.5f, 1.0f, color },  // Bottom left
        };
        
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
        device->BeginScene();
        
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        
        device->EndScene();
        device->Present(nullptr, nullptr, nullptr, nullptr);
    }
    
    // Render a textured quad
    void render_textured_quad(IDirect3DDevice8* device) {
        struct Vertex {
            float x, y, z, rhw;
            float u, v;
        };
        
        Vertex vertices[] = {
            {  50.0f,  50.0f, 0.5f, 1.0f, 0.0f, 0.0f },  // Top left
            { 206.0f,  50.0f, 0.5f, 1.0f, 1.0f, 0.0f },  // Top right
            {  50.0f, 206.0f, 0.5f, 1.0f, 0.0f, 1.0f },  // Bottom left
            { 206.0f, 206.0f, 0.5f, 1.0f, 1.0f, 1.0f },  // Bottom right
        };
        
        // Create a simple checkerboard texture
        IDirect3DTexture8* texture = nullptr;
        HRESULT hr = device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
        if (SUCCEEDED(hr)) {
            D3DLOCKED_RECT locked_rect;
            hr = texture->LockRect(0, &locked_rect, nullptr, 0);
            if (SUCCEEDED(hr)) {
                uint32_t* pixels = static_cast<uint32_t*>(locked_rect.pBits);
                for (int y = 0; y < 64; y++) {
                    for (int x = 0; x < 64; x++) {
                        bool checker = ((x / 8) + (y / 8)) % 2;
                        pixels[y * locked_rect.Pitch / 4 + x] = checker ? 0xFFFFFFFF : 0xFF000000;
                    }
                }
                texture->UnlockRect(0);
            }
        }
        
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF404040, 1.0f, 0);
        device->BeginScene();
        
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
        
        if (texture) {
            device->SetTexture(0, texture);
            device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        }
        
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
        
        device->EndScene();
        device->Present(nullptr, nullptr, nullptr, nullptr);
        
        if (texture) {
            texture->Release();
        }
    }
    
    // Render with alpha blending
    void render_alpha_blended_quads(IDirect3DDevice8* device) {
        struct Vertex {
            float x, y, z, rhw;
            DWORD color;
        };
        
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);
        device->BeginScene();
        
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        
        // Red quad with 50% alpha
        Vertex red_quad[] = {
            {  30.0f,  30.0f, 0.5f, 1.0f, 0x80FF0000 },
            { 150.0f,  30.0f, 0.5f, 1.0f, 0x80FF0000 },
            {  30.0f, 150.0f, 0.5f, 1.0f, 0x80FF0000 },
            { 150.0f, 150.0f, 0.5f, 1.0f, 0x80FF0000 },
        };
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, red_quad, sizeof(Vertex));
        
        // Blue quad with 50% alpha, overlapping
        Vertex blue_quad[] = {
            { 106.0f, 106.0f, 0.5f, 1.0f, 0x800000FF },
            { 226.0f, 106.0f, 0.5f, 1.0f, 0x800000FF },
            { 106.0f, 226.0f, 0.5f, 1.0f, 0x800000FF },
            { 226.0f, 226.0f, 0.5f, 1.0f, 0x800000FF },
        };
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blue_quad, sizeof(Vertex));
        
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        
        device->EndScene();
        device->Present(nullptr, nullptr, nullptr, nullptr);
    }
    
    // Capture framebuffer from backend
    bool capture_framebuffer(BackendTestContext& ctx) {
        // Get the back buffer
        IDirect3DSurface8* back_buffer = nullptr;
        HRESULT hr = ctx.device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        if (FAILED(hr)) {
            return false;
        }
        
        // Lock and read the surface
        D3DLOCKED_RECT locked_rect;
        hr = back_buffer->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);
        if (SUCCEEDED(hr)) {
            uint32_t* src = static_cast<uint32_t*>(locked_rect.pBits);
            for (uint32_t y = 0; y < test_height_; y++) {
                for (uint32_t x = 0; x < test_width_; x++) {
                    ctx.framebuffer_data[y * test_width_ + x] = 
                        src[y * locked_rect.Pitch / 4 + x];
                }
            }
            back_buffer->UnlockRect();
        }
        
        back_buffer->Release();
        return SUCCEEDED(hr);
    }
    
    // Calculate pixel difference
    double calculate_pixel_difference(uint32_t pixel1, uint32_t pixel2) {
        int r1 = (pixel1 >> 16) & 0xFF;
        int g1 = (pixel1 >> 8) & 0xFF;
        int b1 = pixel1 & 0xFF;
        
        int r2 = (pixel2 >> 16) & 0xFF;
        int g2 = (pixel2 >> 8) & 0xFF;
        int b2 = pixel2 & 0xFF;
        
        double dr = r1 - r2;
        double dg = g1 - g2;
        double db = b1 - b2;
        
        return std::sqrt(dr * dr + dg * dg + db * db);
    }
    
    // Compare two framebuffers
    double compare_framebuffers(const std::vector<uint32_t>& fb1,
                               const std::vector<uint32_t>& fb2,
                               double tolerance = 5.0) {
        if (fb1.size() != fb2.size()) {
            return 1000000.0;  // Maximum difference
        }
        
        double total_diff = 0.0;
        double max_diff = 0.0;
        int diff_pixels = 0;
        
        for (size_t i = 0; i < fb1.size(); i++) {
            double diff = calculate_pixel_difference(fb1[i], fb2[i]);
            total_diff += diff;
            max_diff = std::max(max_diff, diff);
            
            if (diff > tolerance) {
                diff_pixels++;
            }
        }
        
        double avg_diff = total_diff / fb1.size();
        
        // Log statistics
        std::cout << "  Average pixel difference: " << avg_diff << std::endl;
        std::cout << "  Maximum pixel difference: " << max_diff << std::endl;
        std::cout << "  Pixels over tolerance: " << diff_pixels 
                  << " (" << (100.0 * diff_pixels / fb1.size()) << "%)" << std::endl;
        
        return avg_diff;
    }
    
    // Save framebuffer to PPM file for debugging
    void save_framebuffer_ppm(const std::string& filename,
                              const std::vector<uint32_t>& framebuffer) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return;
        
        file << "P6\n" << test_width_ << " " << test_height_ << "\n255\n";
        
        for (uint32_t pixel : framebuffer) {
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            file.write(reinterpret_cast<char*>(&r), 1);
            file.write(reinterpret_cast<char*>(&g), 1);
            file.write(reinterpret_cast<char*>(&b), 1);
        }
    }
    
protected:
    uint32_t test_width_;
    uint32_t test_height_;
    std::vector<BackendTestContext> backends_to_test_;
};

TEST_F(CrossBackendRenderingTest, SolidTriangle) {
    std::cout << "Testing solid triangle rendering across backends..." << std::endl;
    
    std::vector<BackendTestContext> rendered_backends;
    
    // Render with each backend
    for (auto& ctx : backends_to_test_) {
        std::cout << "Rendering with " << ctx.backend_name << " backend..." << std::endl;
        
        if (!init_backend(ctx)) {
            std::cout << "  Skipping " << ctx.backend_name << " (initialization failed)" << std::endl;
            continue;
        }
        
        // Render red triangle
        render_solid_triangle(ctx.device, 0xFFFF0000);
        
        // Capture framebuffer
        if (!capture_framebuffer(ctx)) {
            std::cout << "  Failed to capture framebuffer" << std::endl;
            cleanup_backend(ctx);
            continue;
        }
        
        // Save for debugging
        save_framebuffer_ppm("triangle_" + ctx.backend_name + ".ppm", ctx.framebuffer_data);
        
        rendered_backends.push_back(ctx);
        cleanup_backend(ctx);
    }
    
    // Compare all rendered backends
    if (rendered_backends.size() >= 2) {
        std::cout << "\nComparing backend outputs..." << std::endl;
        
        for (size_t i = 1; i < rendered_backends.size(); i++) {
            std::cout << "Comparing " << rendered_backends[0].backend_name 
                      << " vs " << rendered_backends[i].backend_name << ":" << std::endl;
            
            double diff = compare_framebuffers(rendered_backends[0].framebuffer_data,
                                              rendered_backends[i].framebuffer_data);
            
            // Allow small differences due to rasterization differences
            EXPECT_LT(diff, 10.0) << "Backends differ too much: " 
                                   << rendered_backends[0].backend_name << " vs "
                                   << rendered_backends[i].backend_name;
        }
    } else {
        std::cout << "Not enough backends available for comparison" << std::endl;
    }
}

TEST_F(CrossBackendRenderingTest, TexturedQuad) {
    std::cout << "Testing textured quad rendering across backends..." << std::endl;
    
    std::vector<BackendTestContext> rendered_backends;
    
    // Render with each backend
    for (auto& ctx : backends_to_test_) {
        std::cout << "Rendering with " << ctx.backend_name << " backend..." << std::endl;
        
        if (!init_backend(ctx)) {
            std::cout << "  Skipping " << ctx.backend_name << " (initialization failed)" << std::endl;
            continue;
        }
        
        // Render textured quad
        render_textured_quad(ctx.device);
        
        // Capture framebuffer
        if (!capture_framebuffer(ctx)) {
            std::cout << "  Failed to capture framebuffer" << std::endl;
            cleanup_backend(ctx);
            continue;
        }
        
        // Save for debugging
        save_framebuffer_ppm("textured_" + ctx.backend_name + ".ppm", ctx.framebuffer_data);
        
        rendered_backends.push_back(ctx);
        cleanup_backend(ctx);
    }
    
    // Compare all rendered backends
    if (rendered_backends.size() >= 2) {
        std::cout << "\nComparing backend outputs..." << std::endl;
        
        for (size_t i = 1; i < rendered_backends.size(); i++) {
            std::cout << "Comparing " << rendered_backends[0].backend_name 
                      << " vs " << rendered_backends[i].backend_name << ":" << std::endl;
            
            double diff = compare_framebuffers(rendered_backends[0].framebuffer_data,
                                              rendered_backends[i].framebuffer_data);
            
            // Texture filtering might cause slightly more differences
            EXPECT_LT(diff, 15.0) << "Backends differ too much: " 
                                   << rendered_backends[0].backend_name << " vs "
                                   << rendered_backends[i].backend_name;
        }
    }
}

TEST_F(CrossBackendRenderingTest, AlphaBlending) {
    std::cout << "Testing alpha blending across backends..." << std::endl;
    
    std::vector<BackendTestContext> rendered_backends;
    
    // Render with each backend
    for (auto& ctx : backends_to_test_) {
        std::cout << "Rendering with " << ctx.backend_name << " backend..." << std::endl;
        
        if (!init_backend(ctx)) {
            std::cout << "  Skipping " << ctx.backend_name << " (initialization failed)" << std::endl;
            continue;
        }
        
        // Render alpha blended quads
        render_alpha_blended_quads(ctx.device);
        
        // Capture framebuffer
        if (!capture_framebuffer(ctx)) {
            std::cout << "  Failed to capture framebuffer" << std::endl;
            cleanup_backend(ctx);
            continue;
        }
        
        // Save for debugging
        save_framebuffer_ppm("alpha_" + ctx.backend_name + ".ppm", ctx.framebuffer_data);
        
        rendered_backends.push_back(ctx);
        cleanup_backend(ctx);
    }
    
    // Compare all rendered backends
    if (rendered_backends.size() >= 2) {
        std::cout << "\nComparing backend outputs..." << std::endl;
        
        for (size_t i = 1; i < rendered_backends.size(); i++) {
            std::cout << "Comparing " << rendered_backends[0].backend_name 
                      << " vs " << rendered_backends[i].backend_name << ":" << std::endl;
            
            double diff = compare_framebuffers(rendered_backends[0].framebuffer_data,
                                              rendered_backends[i].framebuffer_data);
            
            // Alpha blending precision might vary slightly
            EXPECT_LT(diff, 12.0) << "Backends differ too much: " 
                                   << rendered_backends[0].backend_name << " vs "
                                   << rendered_backends[i].backend_name;
        }
    }
}