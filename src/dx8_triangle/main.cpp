#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>

// GLM headers for matrix operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// LVGL headers
#include "lvgl/lvgl.h"
#include "lvgl_platform.h"

// DirectX 8 headers from dx8gl
#include "d3d8.h"
#include "dx8gl.h"

// Canvas dimensions
#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

// Triangle vertex structure
struct CUSTOMVERTEX {
    float x, y, z;     // Position
    DWORD color;       // Color
};

// Define the FVF for our vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

// Global variables
static IDirect3D8* g_pD3D = nullptr;
static IDirect3DDevice8* g_pDevice = nullptr;
static lv_obj_t* g_canvas = nullptr;
static lv_color_t* g_canvas_buf = nullptr;

// Simple triangle vertices
static const CUSTOMVERTEX g_vertices[] = {
    {  0.0f,  1.0f, 0.0f, 0xFFFF0000 },  // Top (red)
    { -1.0f, -1.0f, 0.0f, 0xFF00FF00 },  // Bottom left (green)
    {  1.0f, -1.0f, 0.0f, 0xFF0000FF },  // Bottom right (blue)
};

static bool init_d3d() {
    // Initialize dx8gl
    if (dx8gl_init(nullptr) != 0) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return false;
    }
    
    // Create Direct3D8 object
    g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_pD3D) {
        std::cerr << "Failed to create Direct3D8 object" << std::endl;
        return false;
    }
    
    // Set up the present parameters for offscreen rendering
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = CANVAS_WIDTH;
    d3dpp.BackBufferHeight = CANVAS_HEIGHT;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    // Create the Direct3D device
    HRESULT hr = g_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_pDevice
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create Direct3D device: " << hr << std::endl;
        return false;
    }
    
    // Set up render states
    g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    return true;
}

static void cleanup_d3d() {
    if (g_pDevice) {
        g_pDevice->Release();
        g_pDevice = nullptr;
    }
    if (g_pD3D) {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
    dx8gl_shutdown();
}

// Helper function to convert glm::mat4 to D3DMATRIX
static void glm_to_d3d_matrix(const glm::mat4& src, D3DMATRIX* dst) {
    const float* ptr = glm::value_ptr(src);
    // DirectX uses row-major matrices, GLM uses column-major
    // So we need to transpose
    dst->_11 = ptr[0];  dst->_12 = ptr[4];  dst->_13 = ptr[8];   dst->_14 = ptr[12];
    dst->_21 = ptr[1];  dst->_22 = ptr[5];  dst->_23 = ptr[9];   dst->_24 = ptr[13];
    dst->_31 = ptr[2];  dst->_32 = ptr[6];  dst->_33 = ptr[10];  dst->_34 = ptr[14];
    dst->_41 = ptr[3];  dst->_42 = ptr[7];  dst->_43 = ptr[11];  dst->_44 = ptr[15];
}

static void render_triangle() {
    if (!g_pDevice) return;
    
    // Clear the render target
    g_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    // Begin the scene
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        // Set identity transforms for simple test
        D3DMATRIX identity;
        memset(&identity, 0, sizeof(identity));
        identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
        
        g_pDevice->SetTransform(D3DTS_WORLD, &identity);
        g_pDevice->SetTransform(D3DTS_VIEW, &identity);
        
        // Simple orthographic projection for 2D-like rendering
        glm::mat4 proj = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
        D3DMATRIX d3dProj;
        glm_to_d3d_matrix(proj, &d3dProj);
        g_pDevice->SetTransform(D3DTS_PROJECTION, &d3dProj);
        
        // Set vertex format
        g_pDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
        
        // Draw the triangle
        g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, g_vertices, sizeof(CUSTOMVERTEX));
        
        // End the scene
        g_pDevice->EndScene();
    }
    
    // Present the rendered frame
    g_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Get the rendered image from dx8gl
    int fb_width, fb_height, frame_number;
    bool updated;
    void* framebuffer = dx8gl_get_shared_framebuffer(&fb_width, &fb_height, &frame_number, &updated);
    
    std::cout << "Frame " << frame_number << ": fb=" << framebuffer 
              << ", updated=" << updated << ", size=" << fb_width << "x" << fb_height << std::endl;
    
    // Save first frame as PPM for debugging
    static bool saved_ppm = false;
    if (framebuffer && !saved_ppm) {
        FILE* fp = fopen("dx8_triangle_test.ppm", "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", fb_width, fb_height);
            uint8_t* src = (uint8_t*)framebuffer;
            for (int y = 0; y < fb_height; y++) {
                for (int x = 0; x < fb_width; x++) {
                    int idx = (y * fb_width + x) * 4;
                    uint8_t r = src[idx + 0];
                    uint8_t g = src[idx + 1];
                    uint8_t b = src[idx + 2];
                    fwrite(&r, 1, 1, fp);
                    fwrite(&g, 1, 1, fp);
                    fwrite(&b, 1, 1, fp);
                }
            }
            fclose(fp);
            std::cout << "Saved dx8_triangle_test.ppm" << std::endl;
        }
        saved_ppm = true;
    }
    
    if (framebuffer && g_canvas_buf) {
        // Convert from RGBA to LVGL format
        uint8_t* src = (uint8_t*)framebuffer;
        lv_color_t* dst = g_canvas_buf;
        
        // OSMesa renders upside down, so we need to flip vertically
        for (int y = 0; y < CANVAS_HEIGHT; y++) {
            for (int x = 0; x < CANVAS_WIDTH; x++) {
                int src_y = CANVAS_HEIGHT - 1 - y;
                int src_idx = (src_y * CANVAS_WIDTH + x) * 4;
                int dst_idx = y * CANVAS_WIDTH + x;
                
                uint8_t r = src[src_idx + 0];
                uint8_t g = src[src_idx + 1];
                uint8_t b = src[src_idx + 2];
                
                dst[dst_idx] = lv_color_make(r, g, b);
            }
        }
        
        // Invalidate the canvas to trigger redraw
        lv_obj_invalidate(g_canvas);
    }
}

static void animation_timer_cb(lv_timer_t* timer) {
    (void)timer;
    render_triangle();
}

static void create_ui() {
    // Get the active screen
    lv_obj_t* scr = lv_scr_act();
    
    // Create title label
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "DirectX 8 Simple Triangle Test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create the canvas
    g_canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(g_canvas, g_canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_NATIVE);
    lv_obj_center(g_canvas);
    
    // Add a border around the canvas
    lv_obj_set_style_border_width(g_canvas, 2, 0);
    lv_obj_set_style_border_color(g_canvas, lv_color_hex(0x4080ff), 0);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // Initialize LVGL platform
    lv_display_t* disp = LvglPlatform::create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdl");
    if (!disp) {
        std::cerr << "Failed to initialize LVGL platform" << std::endl;
        return 1;
    }
    
    // Allocate canvas buffer
    g_canvas_buf = (lv_color_t*)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t));
    if (!g_canvas_buf) {
        std::cerr << "Failed to allocate canvas buffer" << std::endl;
        return 1;
    }
    
    // Initialize Direct3D
    if (!init_d3d()) {
        std::cerr << "Failed to initialize Direct3D" << std::endl;
        free(g_canvas_buf);
        return 1;
    }
    
    // Create the UI
    create_ui();
    
    // Create animation timer (30 FPS)
    lv_timer_create(animation_timer_cb, 33, nullptr);
    
    // Initial render
    render_triangle();
    
    // Run the main loop with 5 second timeout
    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        if (elapsed >= 5) {
            std::cout << "5 second timeout reached, exiting..." << std::endl;
            break;
        }
        
        LvglPlatform::poll_events();
        lv_timer_handler();
    }
    
    // Cleanup
    cleanup_d3d();
    free(g_canvas_buf);
    
    return 0;
}