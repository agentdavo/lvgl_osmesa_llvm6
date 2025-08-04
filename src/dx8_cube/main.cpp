#include <iostream>
#include <cmath>
#include <cstring>
// #include <chrono>  // Not needed anymore, using frame count instead of timeout
#include <iomanip>

// LVGL headers
#include "lvgl/lvgl.h"
#include "lvgl_platform.h"

// DirectX 8 headers from dx8gl
#include "d3d8.h"
#include "dx8gl.h"
#include "../ext/dx8gl/src/d3dx_compat.h"

// Canvas dimensions
#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

// Cube vertex structure with normals for lighting
struct CUSTOMVERTEX {
    float x, y, z;     // Position
    float nx, ny, nz;  // Normal
    DWORD color;       // Color
};

// Define the FVF for our vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE)

// Floor vertex structure with texture coordinates and normals
struct CUSTOMVERTEX_TEX {
    float x, y, z;     // Position
    float nx, ny, nz;  // Normal
    DWORD color;       // Color
    float tu, tv;      // Texture coordinates
};

// Define the FVF for textured vertex structure
#define D3DFVF_CUSTOMVERTEX_TEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Global variables
static IDirect3D8* g_pD3D = nullptr;
static IDirect3DDevice8* g_pDevice = nullptr;
static IDirect3DVertexBuffer8* g_pVB = nullptr;
static IDirect3DVertexBuffer8* g_pFloorVB = nullptr;
static IDirect3DIndexBuffer8* g_pIB = nullptr;
static IDirect3DIndexBuffer8* g_pFloorIB = nullptr;
static IDirect3DTexture8* g_pFloorTexture = nullptr;
static lv_obj_t* g_canvas = nullptr;
static lv_color_t* g_canvas_buf = nullptr;
static float g_rotation = 0.0f;

// Cube vertices with normals and colors - scaled down to 0.5 units
static const CUSTOMVERTEX g_cube_vertices[] = {
    // Front face (red) - Normal pointing towards -Z
    {-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0xFFFF0000},
    { 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0xFFFF0000},
    { 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0xFFFF0000},
    {-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0xFFFF0000},
    
    // Back face (green) - Normal pointing towards +Z
    {-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0xFF00FF00},
    { 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0xFF00FF00},
    { 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0xFF00FF00},
    {-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0xFF00FF00},
    
    // Top face (blue) - Normal pointing towards +Y
    {-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0xFF0000FF},
    { 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0xFF0000FF},
    { 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0xFF0000FF},
    {-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0xFF0000FF},
    
    // Bottom face (yellow) - Normal pointing towards -Y
    {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0xFFFFFF00},
    { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0xFFFFFF00},
    { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0xFFFFFF00},
    {-0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0xFFFFFF00},
    
    // Right face (magenta) - Normal pointing towards +X
    { 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 0xFFFF00FF},
    { 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0xFFFF00FF},
    { 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0xFFFF00FF},
    { 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 0xFFFF00FF},
    
    // Left face (cyan) - Normal pointing towards -X
    {-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0xFF00FFFF},
    {-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 0xFF00FFFF},
    {-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 0xFF00FFFF},
    {-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0xFF00FFFF},
};

// Floor vertices - a large plane below the cube with texture coordinates
static const CUSTOMVERTEX_TEX g_floor_vertices[] = {
    // Floor at Y = -1.5 (below the cube)
    {-5.0f, -1.5f, -5.0f,  0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f},  // Top-left
    { 5.0f, -1.5f, -5.0f,  0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 5.0f, 0.0f},  // Top-right
    { 5.0f, -1.5f,  5.0f,  0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 5.0f, 5.0f},  // Bottom-right
    {-5.0f, -1.5f,  5.0f,  0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 0.0f, 5.0f},  // Bottom-left
};

// Cube indices (two triangles per face)
static const WORD g_indices[] = {
    // Front face (vertices 0-3)
    0, 1, 2,    0, 2, 3,
    // Back face (vertices 4-7)
    4, 6, 5,    4, 7, 6,
    // Top face (vertices 8-11)
    8, 9, 10,   8, 10, 11,
    // Bottom face (vertices 12-15)
    12, 14, 13, 12, 15, 14,
    // Right face (vertices 16-19)
    16, 17, 18, 16, 18, 19,
    // Left face (vertices 20-23)
    20, 22, 21, 20, 23, 22,
};

// Floor indices - two triangles
static const WORD g_floor_indices[] = {
    0, 1, 2,    0, 2, 3,
};

static bool init_d3d() {
    // Create Direct3D8 object - this will initialize dx8gl automatically
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
    g_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);   // Enable lighting
    g_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);   // Enable depth testing 
    g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE); // Enable depth writing
    g_pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL); // Depth comparison
    g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);  // Enable proper culling
    g_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID); // Solid fill
    g_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    g_pDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    g_pDevice->SetRenderState(D3DRS_AMBIENT, 0xFF404040); // Ambient light (dark gray)
    g_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE); // Normalize normals after scaling
    
    // Enable alpha blending for smoother edges
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    // Set up lighting
    D3DLIGHT8 light;
    memset(&light, 0, sizeof(D3DLIGHT8));
    
    // Light 0: Directional light (sun-like)
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 0.8f;
    light.Diffuse.g = 0.8f;
    light.Diffuse.b = 0.8f;
    light.Diffuse.a = 1.0f;
    light.Specular.r = 0.5f;
    light.Specular.g = 0.5f;
    light.Specular.b = 0.5f;
    light.Specular.a = 1.0f;
    light.Direction.x = -0.5f;
    light.Direction.y = -1.0f;
    light.Direction.z = -0.5f;
    g_pDevice->SetLight(0, &light);
    g_pDevice->LightEnable(0, TRUE);
    
    // Light 1: Spotlight
    memset(&light, 0, sizeof(D3DLIGHT8));
    light.Type = D3DLIGHT_SPOT;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 0.9f;
    light.Diffuse.b = 0.7f;
    light.Diffuse.a = 1.0f;
    light.Position.x = 2.0f;
    light.Position.y = 3.0f;
    light.Position.z = 2.0f;
    light.Direction.x = -2.0f;
    light.Direction.y = -3.0f;
    light.Direction.z = -2.0f;
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, (D3DXVECTOR3*)&light.Direction);
    light.Range = 10.0f;
    light.Falloff = 1.0f;
    light.Attenuation0 = 0.0f;
    light.Attenuation1 = 0.1f;
    light.Attenuation2 = 0.0f;
    light.Theta = 0.5f;   // Inner cone angle
    light.Phi = 1.0f;     // Outer cone angle
    g_pDevice->SetLight(1, &light);
    g_pDevice->LightEnable(1, TRUE);
    
    // Set material properties
    D3DMATERIAL8 material;
    memset(&material, 0, sizeof(D3DMATERIAL8));
    material.Diffuse.r = 1.0f;
    material.Diffuse.g = 1.0f;
    material.Diffuse.b = 1.0f;
    material.Diffuse.a = 1.0f;
    material.Ambient.r = 1.0f;
    material.Ambient.g = 1.0f;
    material.Ambient.b = 1.0f;
    material.Ambient.a = 1.0f;
    material.Specular.r = 0.5f;
    material.Specular.g = 0.5f;
    material.Specular.b = 0.5f;
    material.Specular.a = 1.0f;
    material.Power = 20.0f;
    g_pDevice->SetMaterial(&material);
    
    // Create vertex buffer for cube
    hr = g_pDevice->CreateVertexBuffer(
        sizeof(g_cube_vertices),
        D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX,
        D3DPOOL_MANAGED,
        &g_pVB
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create cube vertex buffer" << std::endl;
        return false;
    }
    
    // Fill cube vertex buffer
    void* pVertices;
    if (SUCCEEDED(g_pVB->Lock(0, sizeof(g_cube_vertices), (BYTE**)&pVertices, 0))) {
        memcpy(pVertices, g_cube_vertices, sizeof(g_cube_vertices));
        g_pVB->Unlock();
    }
    
    // Create vertex buffer for floor
    hr = g_pDevice->CreateVertexBuffer(
        sizeof(g_floor_vertices),
        D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX_TEX,
        D3DPOOL_MANAGED,
        &g_pFloorVB
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create floor vertex buffer" << std::endl;
        return false;
    }
    
    // Fill floor vertex buffer
    if (SUCCEEDED(g_pFloorVB->Lock(0, sizeof(g_floor_vertices), (BYTE**)&pVertices, 0))) {
        memcpy(pVertices, g_floor_vertices, sizeof(g_floor_vertices));
        g_pFloorVB->Unlock();
    }
    
    // Load floor texture
    const char* texturePath = "wall01.tga";
    hr = D3DXCreateTextureFromFile(g_pDevice, texturePath, &g_pFloorTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to load floor texture from " << texturePath << ": " << hr << std::endl;
        // Continue without texture - not fatal
    } else {
        std::cout << "Successfully loaded floor texture" << std::endl;
    }
    
    return true;
}

static void cleanup_d3d() {
    std::cout << "cleanup_d3d: Starting cleanup..." << std::endl;
    
    if (g_pFloorTexture) {
        std::cout << "cleanup_d3d: Releasing floor texture..." << std::endl;
        g_pFloorTexture->Release();
        g_pFloorTexture = nullptr;
    }
    if (g_pFloorIB) {
        std::cout << "cleanup_d3d: Releasing floor index buffer..." << std::endl;
        g_pFloorIB->Release();
        g_pFloorIB = nullptr;
    }
    if (g_pIB) {
        std::cout << "cleanup_d3d: Releasing cube index buffer..." << std::endl;
        g_pIB->Release();
        g_pIB = nullptr;
    }
    if (g_pFloorVB) {
        std::cout << "cleanup_d3d: Releasing floor vertex buffer..." << std::endl;
        g_pFloorVB->Release();
        g_pFloorVB = nullptr;
    }
    if (g_pVB) {
        std::cout << "cleanup_d3d: Releasing cube vertex buffer..." << std::endl;
        g_pVB->Release();
        g_pVB = nullptr;
    }
    if (g_pDevice) {
        std::cout << "cleanup_d3d: Releasing device..." << std::endl;
        g_pDevice->Release();
        g_pDevice = nullptr;
    }
    if (g_pD3D) {
        std::cout << "cleanup_d3d: Releasing Direct3D..." << std::endl;
        g_pD3D->Release();
        g_pD3D = nullptr;
    }
    
    std::cout << "cleanup_d3d: Calling dx8gl_shutdown..." << std::endl;
    dx8gl_shutdown();  // Clean up dx8gl resources
    std::cout << "cleanup_d3d: Cleanup complete." << std::endl;
}


static void set_matrices_for_cube() {
    static int matrix_debug = 0;
    bool debug_this_frame = (matrix_debug < 3);
    
    // Build world matrix - combine scaling and Y rotation
    D3DMATRIX matScale, matRotY, matWorld;
    
    // Scale matrix (40% size)
    D3DXMatrixScaling(&matScale, 0.4f, 0.4f, 0.4f);
    
    // Rotation around Y axis (spin)
    D3DXMatrixRotationY(&matRotY, g_rotation);
    
    // No X-axis tilt for now - just Y rotation
    
    // Combine transformations: Scale * RotY
    D3DXMatrixMultiply(&matWorld, &matScale, &matRotY);
    
    // Build view matrix using D3DXMatrixLookAtLH
    D3DXVECTOR3 vEye(3.0f, 3.0f, 3.0f);    // Camera position - higher and further
    D3DXVECTOR3 vAt(0.0f, -0.5f, 0.0f);   // Look slightly below origin
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);     // Up vector
    
    D3DMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    
    // Build projection matrix
    D3DMATRIX matProj;
    float fov = 45.0f * 3.14159f / 180.0f;    // Convert degrees to radians
    D3DXMatrixPerspectiveFovLH(&matProj, fov, 1.0f, 0.5f, 10.0f);
    
    // Set the matrices
    g_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pDevice->SetTransform(D3DTS_VIEW, &matView);
    g_pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
    
    if (debug_this_frame) {
        std::cout << "\n=== Matrix Debug Frame " << matrix_debug << " ===" << std::endl;
        std::cout << "Rotation: " << g_rotation << " radians" << std::endl;
        std::cout << "Camera at (1.5, 1.2, 1.5) looking at origin, cube scaled to 40%" << std::endl;
        std::cout << "Perspective projection: 45 degree FOV, near=0.5, far=10.0" << std::endl;
        
        // Print projection matrix values
        std::cout << "Projection matrix:" << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "  [";
            for (int j = 0; j < 4; j++) {
                std::cout << std::fixed << std::setprecision(2) << matProj.m(i,j);
                if (j < 3) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
        matrix_debug++;
    }
}

static void set_matrices_for_floor() {
    // Build world matrix for floor - NO rotation, just identity
    D3DMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    
    // Build view matrix using D3DXMatrixLookAtLH - same camera as cube
    D3DXVECTOR3 vEye(3.0f, 3.0f, 3.0f);    // Camera position - higher and further (matching cube)
    D3DXVECTOR3 vAt(0.0f, -0.5f, 0.0f);   // Look slightly below origin (matching cube)
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);     // Up vector
    
    D3DMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    
    // Build projection matrix - same as cube
    D3DMATRIX matProj;
    float fov = 45.0f * 3.14159f / 180.0f;    // Convert degrees to radians
    D3DXMatrixPerspectiveFovLH(&matProj, fov, 1.0f, 0.5f, 10.0f);
    
    // Set the matrices
    g_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pDevice->SetTransform(D3DTS_VIEW, &matView);
    g_pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

static bool g_frame_ready = false;
static void* g_last_framebuffer = nullptr;
static int g_last_fb_width = 0;
static int g_last_fb_height = 0;

static int g_render_count = 0;  // Global frame counter

static void render_cube() {
    if (!g_pDevice) return;
    
    if (g_render_count < 3) {
        std::cout << "=== render_cube called, frame " << g_render_count << " ===" << std::endl;
    }
    
    g_render_count++;
    
    // Clear the render target and depth buffer
    g_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
    
    // Begin the scene
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        // First, render the floor with static matrices
        set_matrices_for_floor();
        g_pDevice->SetStreamSource(0, g_pFloorVB, sizeof(CUSTOMVERTEX_TEX));
        g_pDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX_TEX);
        
        // Set floor texture
        if (g_pFloorTexture) {
            g_pDevice->SetTexture(0, g_pFloorTexture);
            g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            g_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            g_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            g_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        }
        
        // Create and set index buffer for floor rendering
        if (!g_pFloorIB) {
            HRESULT hr = g_pDevice->CreateIndexBuffer(
                sizeof(g_floor_indices),
                D3DUSAGE_WRITEONLY,
                D3DFMT_INDEX16,
                D3DPOOL_MANAGED,
                &g_pFloorIB
            );
            
            if (SUCCEEDED(hr)) {
                void* pIndices;
                if (SUCCEEDED(g_pFloorIB->Lock(0, sizeof(g_floor_indices), (BYTE**)&pIndices, 0))) {
                    memcpy(pIndices, g_floor_indices, sizeof(g_floor_indices));
                    g_pFloorIB->Unlock();
                }
            }
        }
        
        if (g_pFloorIB) {
            g_pDevice->SetIndices(g_pFloorIB, 0);
            g_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
        }
        
        // Now render the cube with rotating matrices
        set_matrices_for_cube();
        g_pDevice->SetStreamSource(0, g_pVB, sizeof(CUSTOMVERTEX));
        g_pDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
        
        // Disable texture for cube
        g_pDevice->SetTexture(0, nullptr);
        
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
            // Flip vertically when saving PPM since OpenGL has Y=0 at bottom
            for (int y = 0; y < fb_height; y++) {
                for (int x = 0; x < fb_width; x++) {
                    // Read from bottom row going up
                    int src_y = fb_height - 1 - y;
                    int idx = (src_y * fb_width + x) * 4;
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
    
    // Update rotation - slower for debugging
    g_rotation += 0.05f;
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
    
    // Run the main loop for 100 frames
    const int MAX_FRAMES = 100;
    std::cout << "Running for " << MAX_FRAMES << " frames..." << std::endl;
    
    while (g_render_count < MAX_FRAMES) {
        LvglPlatform::poll_events();
        lv_timer_handler();  // Process LVGL timers and redraw
    }
    
    std::cout << "Rendered " << g_render_count << " frames, exiting gracefully." << std::endl;
    
    // Cleanup
    cleanup_d3d();
    free(g_canvas_buf);
    // Platform cleanup is automatic
    
    return 0;
}