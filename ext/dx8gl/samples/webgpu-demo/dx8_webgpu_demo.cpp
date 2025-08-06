/**
 * DirectX 8 to WebGPU Demo using dx8gl
 * Renders 1000 spinning cubes using DirectX 8 API, translated to WebGPU
 */

#include <cstdio>
#include <cmath>
#include <vector>
#include <cstring>

// Include dx8gl headers
#include "../../src/d3d8.h"
#include "../../src/d3dx8.h"
#include "../../src/dx8gl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// Constants
const int GRID_SIZE = 10;
const int NUM_CUBES = GRID_SIZE * GRID_SIZE * GRID_SIZE;
const float CUBE_SPACING = 3.0f;

// Vertex structure matching D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1
struct CUSTOMVERTEX {
    float x, y, z;      // Position
    float nx, ny, nz;   // Normal
    float tu, tv;       // Texture coordinates
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// Global state
struct AppState {
    IDirect3D8* d3d;
    IDirect3DDevice8* device;
    IDirect3DVertexBuffer8* vertexBuffer;
    IDirect3DIndexBuffer8* indexBuffer;
    IDirect3DTexture8* texture;
    
    float time;
    int width;
    int height;
    
    std::vector<D3DXVECTOR3> cubePositions;
} g_state = {nullptr, nullptr, nullptr, nullptr, nullptr, 0.0f, 800, 600};

// Create cube vertices
void CreateCubeGeometry(std::vector<CUSTOMVERTEX>& vertices, std::vector<WORD>& indices) {
    // Define cube vertices with normals and texture coordinates
    CUSTOMVERTEX cubeVerts[] = {
        // Front face
        {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f},
        {-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f},
        // Back face
        { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f},
        {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f},
        {-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f},
        // Top face
        {-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f},
        {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f},
        // Bottom face
        {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f},
        {-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f},
        // Right face
        { 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
        { 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
        { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f},
        // Left face
        {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
        {-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
        {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
        {-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f}
    };
    
    vertices.assign(cubeVerts, cubeVerts + 24);
    
    // Define cube indices
    WORD cubeIndices[] = {
        0, 1, 2,    0, 2, 3,    // Front
        4, 5, 6,    4, 6, 7,    // Back
        8, 9, 10,   8, 10, 11,  // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };
    
    indices.assign(cubeIndices, cubeIndices + 36);
}

// Create a procedural checkerboard texture
void CreateCheckerboardTexture() {
    const int TEX_SIZE = 256;
    const int CHECKER_SIZE = 32;
    
    // Create texture
    HRESULT hr = g_state.device->CreateTexture(
        TEX_SIZE, TEX_SIZE, 1,
        0, D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &g_state.texture
    );
    
    if (FAILED(hr)) {
        printf("Failed to create texture\n");
        return;
    }
    
    // Lock texture and fill with checkerboard pattern
    D3DLOCKED_RECT lockedRect;
    hr = g_state.texture->LockRect(0, &lockedRect, NULL, 0);
    if (SUCCEEDED(hr)) {
        DWORD* pixels = (DWORD*)lockedRect.pBits;
        
        for (int y = 0; y < TEX_SIZE; y++) {
            for (int x = 0; x < TEX_SIZE; x++) {
                int checkX = x / CHECKER_SIZE;
                int checkY = y / CHECKER_SIZE;
                bool isWhite = ((checkX + checkY) % 2) == 0;
                
                DWORD color = isWhite ? 0xFFFFFFFF : 0xFF4080FF;  // White or light blue
                pixels[y * (lockedRect.Pitch / 4) + x] = color;
            }
        }
        
        g_state.texture->UnlockRect(0);
    }
}

// Initialize DirectX 8 through dx8gl
bool InitD3D() {
    printf("Initializing DirectX 8 through dx8gl...\n");
    
    // Initialize dx8gl with WebGPU backend for Emscripten
    dx8gl_config config = {};
#ifdef __EMSCRIPTEN__
    config.backend_type = DX8GL_BACKEND_WEBGPU;
#else
    config.backend_type = DX8GL_BACKEND_OSMESA;
#endif
    
    if (dx8gl_init(&config) != 0) {
        printf("Failed to initialize dx8gl\n");
        return false;
    }
    
    // Create Direct3D8 interface
    g_state.d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_state.d3d) {
        printf("Failed to create Direct3D8 interface\n");
        return false;
    }
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferWidth = g_state.width;
    pp.BackBufferHeight = g_state.height;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
#ifdef __EMSCRIPTEN__
    pp.hDeviceWindow = NULL;  // No window handle in Emscripten
#else
    pp.hDeviceWindow = GetActiveWindow();
#endif
    
    // Create device
    HRESULT hr = g_state.d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        pp.hDeviceWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &g_state.device
    );
    
    if (FAILED(hr)) {
        printf("Failed to create Direct3D device: 0x%08X\n", hr);
        return false;
    }
    
    printf("DirectX 8 initialized successfully\n");
    return true;
}

// Initialize geometry and resources
bool InitGeometry() {
    printf("Creating geometry...\n");
    
    // Create cube geometry
    std::vector<CUSTOMVERTEX> vertices;
    std::vector<WORD> indices;
    CreateCubeGeometry(vertices, indices);
    
    // Create vertex buffer
    HRESULT hr = g_state.device->CreateVertexBuffer(
        vertices.size() * sizeof(CUSTOMVERTEX),
        D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX,
        D3DPOOL_MANAGED,
        &g_state.vertexBuffer
    );
    
    if (FAILED(hr)) {
        printf("Failed to create vertex buffer\n");
        return false;
    }
    
    // Fill vertex buffer
    void* data;
    hr = g_state.vertexBuffer->Lock(0, 0, (BYTE**)&data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(data, vertices.data(), vertices.size() * sizeof(CUSTOMVERTEX));
        g_state.vertexBuffer->Unlock();
    }
    
    // Create index buffer
    hr = g_state.device->CreateIndexBuffer(
        indices.size() * sizeof(WORD),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_MANAGED,
        &g_state.indexBuffer
    );
    
    if (FAILED(hr)) {
        printf("Failed to create index buffer\n");
        return false;
    }
    
    // Fill index buffer
    hr = g_state.indexBuffer->Lock(0, 0, (BYTE**)&data, 0);
    if (SUCCEEDED(hr)) {
        memcpy(data, indices.data(), indices.size() * sizeof(WORD));
        g_state.indexBuffer->Unlock();
    }
    
    // Create texture
    CreateCheckerboardTexture();
    
    // Initialize cube positions
    g_state.cubePositions.clear();
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                float px = (x - GRID_SIZE/2.0f) * CUBE_SPACING;
                float py = (y - GRID_SIZE/2.0f) * CUBE_SPACING;
                float pz = (z - GRID_SIZE/2.0f) * CUBE_SPACING;
                g_state.cubePositions.push_back(D3DXVECTOR3(px, py, pz));
            }
        }
    }
    
    printf("Created %zu cubes\n", g_state.cubePositions.size());
    return true;
}

// Set up render states
void SetupRenderStates() {
    // Enable depth testing
    g_state.device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    g_state.device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    g_state.device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
    
    // Enable backface culling
    g_state.device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    
    // Set up lighting
    g_state.device->SetRenderState(D3DRS_LIGHTING, TRUE);
    g_state.device->SetRenderState(D3DRS_AMBIENT, 0xFF404040);
    
    // Enable one directional light
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    light.Ambient.r = 0.2f;
    light.Ambient.g = 0.2f;
    light.Ambient.b = 0.2f;
    light.Direction.x = 0.5f;
    light.Direction.y = -1.0f;
    light.Direction.z = 0.5f;
    
    g_state.device->SetLight(0, &light);
    g_state.device->LightEnable(0, TRUE);
    
    // Set texture filtering
    g_state.device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_state.device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_state.device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    
    g_state.device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    g_state.device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    
    // Set material
    D3DMATERIAL8 material = {};
    material.Diffuse.r = material.Diffuse.g = material.Diffuse.b = material.Diffuse.a = 1.0f;
    material.Ambient = material.Diffuse;
    g_state.device->SetMaterial(&material);
}

// Render frame
void Render() {
    if (!g_state.device) return;
    
    // Update time
    g_state.time += 0.016f;
    
    // Clear screen
    g_state.device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                          D3DCOLOR_XRGB(26, 26, 51), 1.0f, 0);
    
    // Begin scene
    if (SUCCEEDED(g_state.device->BeginScene())) {
        // Set up view matrix (camera orbiting around scene)
        D3DXMATRIX matView;
        D3DXVECTOR3 eye(
            30.0f * cosf(g_state.time * 0.2f),
            20.0f,
            30.0f * sinf(g_state.time * 0.2f)
        );
        D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
        D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&matView, &eye, &at, &up);
        g_state.device->SetTransform(D3DTS_VIEW, &matView);
        
        // Set up projection matrix
        D3DXMATRIX matProj;
        float aspect = (float)g_state.width / (float)g_state.height;
        D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, aspect, 0.1f, 100.0f);
        g_state.device->SetTransform(D3DTS_PROJECTION, &matProj);
        
        // Update light position (orbiting light)
        D3DLIGHT8 light;
        g_state.device->GetLight(0, &light);
        light.Direction.x = cosf(g_state.time);
        light.Direction.y = -0.5f;
        light.Direction.z = sinf(g_state.time);
        D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, (D3DXVECTOR3*)&light.Direction);
        g_state.device->SetLight(0, &light);
        
        // Set buffers and texture
        g_state.device->SetStreamSource(0, g_state.vertexBuffer, sizeof(CUSTOMVERTEX));
        g_state.device->SetIndices(g_state.indexBuffer, 0);
        g_state.device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
        g_state.device->SetTexture(0, g_state.texture);
        
        // Draw all cubes
        for (size_t i = 0; i < g_state.cubePositions.size(); i++) {
            const D3DXVECTOR3& pos = g_state.cubePositions[i];
            
            // Create world matrix for this cube
            D3DXMATRIX matWorld, matRotate, matTranslate;
            D3DXMatrixRotationY(&matRotate, g_state.time + i * 0.1f);
            D3DXMatrixTranslation(&matTranslate, pos.x, pos.y, pos.z);
            D3DXMatrixMultiply(&matWorld, &matRotate, &matTranslate);
            g_state.device->SetTransform(D3DTS_WORLD, &matWorld);
            
            // Draw cube
            g_state.device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 24, 0, 12);
        }
        
        g_state.device->EndScene();
    }
    
    // Present
    g_state.device->Present(NULL, NULL, NULL, NULL);
}

// Cleanup
void Cleanup() {
    if (g_state.texture) {
        g_state.texture->Release();
        g_state.texture = nullptr;
    }
    if (g_state.indexBuffer) {
        g_state.indexBuffer->Release();
        g_state.indexBuffer = nullptr;
    }
    if (g_state.vertexBuffer) {
        g_state.vertexBuffer->Release();
        g_state.vertexBuffer = nullptr;
    }
    if (g_state.device) {
        g_state.device->Release();
        g_state.device = nullptr;
    }
    if (g_state.d3d) {
        g_state.d3d->Release();
        g_state.d3d = nullptr;
    }
    
    dx8gl_shutdown();
}

#ifdef __EMSCRIPTEN__
void MainLoop() {
    // Update canvas size
    emscripten_get_canvas_element_size("#canvas", &g_state.width, &g_state.height);
    
    // Render frame
    Render();
}
#endif

int main() {
    printf("=== DirectX 8 to WebGPU Demo ===\n");
    printf("Using dx8gl translation layer\n\n");
    
    // Initialize Direct3D
    if (!InitD3D()) {
        printf("Failed to initialize Direct3D\n");
        return -1;
    }
    
    // Initialize geometry
    if (!InitGeometry()) {
        printf("Failed to initialize geometry\n");
        Cleanup();
        return -1;
    }
    
    // Set up render states
    SetupRenderStates();
    
#ifdef __EMSCRIPTEN__
    // Set up main loop for Emscripten
    emscripten_set_main_loop(MainLoop, 0, 1);
#else
    // Desktop main loop
    printf("Desktop mode - would need window system integration\n");
    
    // Render a few frames for testing
    for (int i = 0; i < 100; i++) {
        Render();
    }
    
    Cleanup();
#endif
    
    return 0;
}