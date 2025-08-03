#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>
#include <iomanip>

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

// Cube vertex structure
struct CUSTOMVERTEX {
    float x, y, z;     // Position
    DWORD color;       // Color
};

// Define the FVF for our vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

// Global variables
static IDirect3D8* g_pD3D = nullptr;
static IDirect3DDevice8* g_pDevice = nullptr;
static IDirect3DVertexBuffer8* g_pVB = nullptr;
static lv_obj_t* g_canvas = nullptr;
static lv_color_t* g_canvas_buf = nullptr;
static float g_rotation = 0.0f;

// Cube vertices with colors - scaled down to 0.5 units
static const CUSTOMVERTEX g_vertices[] = {
    // Front face (red)
    {-0.5f, -0.5f, -0.5f, 0xFFFF0000},
    { 0.5f, -0.5f, -0.5f, 0xFFFF0000},
    { 0.5f,  0.5f, -0.5f, 0xFFFF0000},
    {-0.5f,  0.5f, -0.5f, 0xFFFF0000},
    
    // Back face (green)
    {-0.5f, -0.5f,  0.5f, 0xFF00FF00},
    { 0.5f, -0.5f,  0.5f, 0xFF00FF00},
    { 0.5f,  0.5f,  0.5f, 0xFF00FF00},
    {-0.5f,  0.5f,  0.5f, 0xFF00FF00},
    
    // Top face (blue)
    {-0.5f,  0.5f, -0.5f, 0xFF0000FF},
    { 0.5f,  0.5f, -0.5f, 0xFF0000FF},
    { 0.5f,  0.5f,  0.5f, 0xFF0000FF},
    {-0.5f,  0.5f,  0.5f, 0xFF0000FF},
    
    // Bottom face (yellow)
    {-0.5f, -0.5f, -0.5f, 0xFFFFFF00},
    { 0.5f, -0.5f, -0.5f, 0xFFFFFF00},
    { 0.5f, -0.5f,  0.5f, 0xFFFFFF00},
    {-0.5f, -0.5f,  0.5f, 0xFFFFFF00},
    
    // Right face (magenta)
    { 0.5f, -0.5f, -0.5f, 0xFFFF00FF},
    { 0.5f, -0.5f,  0.5f, 0xFFFF00FF},
    { 0.5f,  0.5f,  0.5f, 0xFFFF00FF},
    { 0.5f,  0.5f, -0.5f, 0xFFFF00FF},
    
    // Left face (cyan)
    {-0.5f, -0.5f, -0.5f, 0xFF00FFFF},
    {-0.5f, -0.5f,  0.5f, 0xFF00FFFF},
    {-0.5f,  0.5f,  0.5f, 0xFF00FFFF},
    {-0.5f,  0.5f, -0.5f, 0xFF00FFFF},
};

// Cube indices (two triangles per face) - reversed winding for testing
static const WORD g_indices[] = {
    // Front face
    0, 2, 1,    0, 3, 2,
    // Back face  
    4, 5, 6,    4, 6, 7,
    // Top face
    8, 10, 9,   8, 11, 10,
    // Bottom face
    12, 13, 14, 12, 14, 15,
    // Right face
    16, 18, 17, 16, 19, 18,
    // Left face
    20, 21, 22, 20, 22, 23,
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
        nullptr,  // No window handle needed for OSMesa
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
    g_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);   // Enable depth testing 
    g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE); // Enable depth writing
    g_pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL); // Depth comparison
    g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);  // Enable proper culling
    g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID); // Solid fill
    g_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    g_pDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    
    // Enable alpha blending for smoother edges
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    // Create vertex buffer
    hr = g_pDevice->CreateVertexBuffer(
        sizeof(g_vertices),
        D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX,
        D3DPOOL_MANAGED,
        &g_pVB
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }
    
    // Fill vertex buffer
    void* pVertices;
    if (SUCCEEDED(g_pVB->Lock(0, sizeof(g_vertices), (BYTE**)&pVertices, 0))) {
        memcpy(pVertices, g_vertices, sizeof(g_vertices));
        g_pVB->Unlock();
    }
    
    return true;
}

static IDirect3DIndexBuffer8* g_pIB = nullptr;  // Move to global scope

static void cleanup_d3d() {
    if (g_pIB) {
        g_pIB->Release();
        g_pIB = nullptr;
    }
    if (g_pVB) {
        g_pVB->Release();
        g_pVB = nullptr;
    }
    if (g_pDevice) {
        g_pDevice->Release();
        g_pDevice = nullptr;
    }
    if (g_pD3D) {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
    dx8gl_shutdown();  // Clean up dx8gl resources
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

static void set_matrices() {
    static int matrix_debug = 0;
    bool debug_this_frame = (matrix_debug < 3);
    
    // Create rotation matrix for the cube
    float c = cos(g_rotation);
    float s = sin(g_rotation);
    
    // World matrix - scale down and rotate for better view
    glm::mat4 world = glm::mat4(1.0f);
    world = glm::scale(world, glm::vec3(0.4f, 0.4f, 0.4f)); // 40% scale
    world = glm::rotate(world, g_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Slight tilt
    
    // View matrix - simple camera back and slightly elevated
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.8f, 2.0f),  // Eye position
        glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
    );
    
    // Projection matrix - perspective view
    glm::mat4 projection = glm::perspective(
        glm::radians(50.0f),  // Moderate field of view
        1.0f,                 // Aspect ratio (400x400)
        0.1f,                 // Near plane
        10.0f                 // Far plane
    );
    
    // Convert to D3D matrices and set
    D3DMATRIX d3d_world, d3d_view, d3d_proj;
    glm_to_d3d_matrix(world, &d3d_world);
    glm_to_d3d_matrix(view, &d3d_view);
    glm_to_d3d_matrix(projection, &d3d_proj);
    
    g_pDevice->SetTransform(D3DTS_WORLD, &d3d_world);
    g_pDevice->SetTransform(D3DTS_VIEW, &d3d_view);
    g_pDevice->SetTransform(D3DTS_PROJECTION, &d3d_proj);
    
    if (debug_this_frame) {
        std::cout << "\n=== Matrix Debug Frame " << matrix_debug << " ===" << std::endl;
        std::cout << "Rotation: " << g_rotation << " radians" << std::endl;
        std::cout << "Camera at (0, 0.8, 2.0) looking at origin, cube scaled to 40%" << std::endl;
        std::cout << "Perspective projection: 50 degree FOV" << std::endl;
        matrix_debug++;
    }
}

static bool g_frame_ready = false;
static void* g_last_framebuffer = nullptr;
static int g_last_fb_width = 0;
static int g_last_fb_height = 0;

static void render_cube() {
    if (!g_pDevice) return;
    
    static int render_count = 0;
    
    if (render_count < 3) {
        std::cout << "=== render_cube called, frame " << render_count << " ===" << std::endl;
    }
    
    render_count++;
    
    // Clear the render target and depth buffer
    g_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    // Begin the scene
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        // Set up transformations
        set_matrices();
        
        // Set the vertex buffer
        g_pDevice->SetStreamSource(0, g_pVB, sizeof(CUSTOMVERTEX));
        g_pDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
        
        // Create and set index buffer for cube rendering
        if (!g_pIB) {
            HRESULT hr = g_pDevice->CreateIndexBuffer(
                sizeof(g_indices),
                D3DUSAGE_WRITEONLY,
                D3DFMT_INDEX16,
                D3DPOOL_MANAGED,
                &g_pIB
            );
            
            if (SUCCEEDED(hr)) {
                void* pIndices;
                if (SUCCEEDED(g_pIB->Lock(0, sizeof(g_indices), (BYTE**)&pIndices, 0))) {
                    memcpy(pIndices, g_indices, sizeof(g_indices));
                    g_pIB->Unlock();
                }
            }
        }
        
        if (g_pIB) {
            g_pDevice->SetIndices(g_pIB, 0);
            
            static int draw_debug = 0;
            if (draw_debug < 3) {
                std::cout << "Drawing cube: 24 vertices, 12 triangles" << std::endl;
                draw_debug++;
            }
            
            g_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 24, 0, 12);
        } else {
            // Fallback to non-indexed rendering
            for (int i = 0; i < 6; i++) {
                g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, i * 4, 2);
            }
        }
        
        // End the scene
        g_pDevice->EndScene();
    }
    
    // Present the rendered frame
    g_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Mark that a new frame is ready
    g_frame_ready = true;
}

// Separate function to update the canvas from the last rendered frame
static void update_canvas() {
    if (!g_frame_ready || !g_canvas_buf) return;
    
    // Get the rendered image from dx8gl
    int fb_width, fb_height, frame_number;
    bool updated;
    void* framebuffer = dx8gl_get_shared_framebuffer(&fb_width, &fb_height, &frame_number, &updated);
    
    if (!framebuffer) return;
    
    static int debug_count = 0;
    if (debug_count < 5) {
        std::cout << "Update canvas - Frame " << frame_number << ": size=" << fb_width << "x" << fb_height << std::endl;
        
        // Sample a few pixels
        uint8_t* fb = (uint8_t*)framebuffer;
        std::cout << "  First pixel RGBA: " 
                  << (int)fb[0] << "," << (int)fb[1] << "," << (int)fb[2] << "," << (int)fb[3] << std::endl;
        std::cout << "  Center pixel RGBA: " 
                  << (int)fb[200*400*4 + 200*4] << "," 
                  << (int)fb[200*400*4 + 200*4 + 1] << "," 
                  << (int)fb[200*400*4 + 200*4 + 2] << "," 
                  << (int)fb[200*400*4 + 200*4 + 3] << std::endl;
        
        // Save PPM for debugging
        char filename[256];
        snprintf(filename, sizeof(filename), "dx8_cube_frame_%02d.ppm", debug_count);
        FILE* fp = fopen(filename, "wb");
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
            std::cout << "Saved " << filename << std::endl;
        }
        
        debug_count++;
    }
    
    // Convert from RGBA to LVGL native format (XRGB8888 = BGRX in memory)
    uint8_t* src = (uint8_t*)framebuffer;
    uint8_t* dst = (uint8_t*)g_canvas_buf;
    
    // OSMesa renders upside down, so flip vertically
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int src_y = CANVAS_HEIGHT - 1 - y;
            int src_idx = (src_y * CANVAS_WIDTH + x) * 4;
            int dst_idx = (y * CANVAS_WIDTH + x) * 4;
            
            // OSMesa: RGBA -> LVGL XRGB8888: BGRX
            // Shader now outputs correct RGBA after swizzling from BGRA
            dst[dst_idx + 0] = src[src_idx + 2];  // B
            dst[dst_idx + 1] = src[src_idx + 1];  // G
            dst[dst_idx + 2] = src[src_idx + 0];  // R
            dst[dst_idx + 3] = 0xFF;               // X (unused alpha)
        }
    }
    
    // Invalidate the canvas to trigger redraw
    lv_obj_invalidate(g_canvas);
    
    // Reset frame ready flag
    g_frame_ready = false;
}

static void animation_timer_cb(lv_timer_t* timer) {
    (void)timer;
    
    // Update rotation
    g_rotation += 0.02f;
    if (g_rotation > 2.0f * 3.14159f) {
        g_rotation -= 2.0f * 3.14159f;
    }
    
    // Render the cube
    render_cube();
}

static void display_timer_cb(lv_timer_t* timer) {
    (void)timer;
    
    // Update the canvas with the last rendered frame
    update_canvas();
}

static void create_ui() {
    // Get the active screen
    lv_obj_t* scr = lv_scr_act();
    
    // Set screen background color
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a1a), 0);
    
    // Create a container for centering
    lv_obj_t* cont = lv_obj_create(scr);
    lv_obj_set_size(cont, WINDOW_WIDTH, WINDOW_HEIGHT);
    lv_obj_center(cont);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    
    // Create title label
    lv_obj_t* title = lv_label_create(cont);
    lv_label_set_text(title, "DirectX 8 Spinning Cube Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create the canvas
    g_canvas = lv_canvas_create(cont);
    lv_canvas_set_buffer(g_canvas, g_canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_NATIVE);
    lv_obj_center(g_canvas);
    
    // Add a border around the canvas
    lv_obj_set_style_border_width(g_canvas, 2, 0);
    lv_obj_set_style_border_color(g_canvas, lv_color_hex(0x4080ff), 0);
    
    // Create info label
    lv_obj_t* info = lv_label_create(cont);
    lv_label_set_text(info, "Rendered with dx8gl -> OSMesa -> LVGL Canvas");
    lv_obj_set_style_text_color(info, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
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
    
    // Create animation timer (30 FPS) - renders to framebuffer
    lv_timer_create(animation_timer_cb, 33, nullptr);
    
    // Create display timer (60 FPS) - updates canvas from framebuffer
    lv_timer_create(display_timer_cb, 16, nullptr);
    
    // Initial render
    render_cube();
    
    // Run the main loop with 5 second timeout
    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        // Check for 5 second timeout
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        if (elapsed >= 20) {
            std::cout << "20 second timeout reached, exiting..." << std::endl;
            break;
        }
        
        LvglPlatform::poll_events();
        lv_timer_handler();  // Process LVGL timers and redraw
    }
    
    // Cleanup
    cleanup_d3d();
    free(g_canvas_buf);
    // Platform cleanup is automatic
    
    return 0;
}