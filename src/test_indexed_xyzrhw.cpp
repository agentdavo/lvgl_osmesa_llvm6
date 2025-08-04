#include <iostream>
#include <cstring>
#include <vector>
#include "dx8gl.h"
#include "d3d8.h"

// Test application to isolate crash when mixing indexed primitives with XYZRHW vertices
// This mimics the pattern used in dx8_cube more closely

int main() {
    std::cout << "Testing indexed primitives followed by XYZRHW vertices..." << std::endl;
    
    // Initialize dx8gl
    dx8gl_init(nullptr);
    
    // Create Direct3D8
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8" << std::endl;
        return 1;
    }
    
    // Create device with minimal setup
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = 400;
    pp.BackBufferHeight = 400;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    
    IDirect3DDevice8* device = nullptr;
    HRESULT hr = d3d8->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &pp,
        &device
    );
    
    if (FAILED(hr) || !device) {
        std::cerr << "Failed to create device: " << std::hex << hr << std::endl;
        d3d8->Release();
        return 1;
    }
    
    // Enable depth testing and lighting (like dx8_cube)
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetRenderState(D3DRS_AMBIENT, 0x00202020);
    
    // Define regular vertex structure with normal (like dx8_cube)
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        DWORD color;
    };
    
    // Create vertex buffer for a simple quad
    IDirect3DVertexBuffer8* vb = nullptr;
    hr = device->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_WRITEONLY, 
                                   D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE,
                                   D3DPOOL_MANAGED, &vb);
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex buffer: " << std::hex << hr << std::endl;
        device->Release();
        d3d8->Release();
        return 1;
    }
    
    // Fill vertex buffer
    Vertex* vertices = nullptr;
    vb->Lock(0, 0, (BYTE**)&vertices, 0);
    
    // Simple quad
    vertices[0] = { -0.5f,  0.5f, 0.5f,  0.0f, 0.0f, -1.0f, 0xFFFF0000 }; // Top left - red
    vertices[1] = {  0.5f,  0.5f, 0.5f,  0.0f, 0.0f, -1.0f, 0xFF00FF00 }; // Top right - green  
    vertices[2] = {  0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -1.0f, 0xFF0000FF }; // Bottom right - blue
    vertices[3] = { -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -1.0f, 0xFFFFFF00 }; // Bottom left - yellow
    
    vb->Unlock();
    
    // Create index buffer
    IDirect3DIndexBuffer8* ib = nullptr;
    hr = device->CreateIndexBuffer(6 * sizeof(WORD), D3DUSAGE_WRITEONLY, 
                                  D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib);
    if (FAILED(hr)) {
        std::cerr << "Failed to create index buffer: " << std::hex << hr << std::endl;
        vb->Release();
        device->Release();
        d3d8->Release();
        return 1;
    }
    
    // Fill index buffer (two triangles)
    WORD* indices = nullptr;
    ib->Lock(0, 0, (BYTE**)&indices, 0);
    indices[0] = 0; indices[1] = 1; indices[2] = 2;  // First triangle
    indices[3] = 0; indices[4] = 2; indices[5] = 3;  // Second triangle
    ib->Unlock();
    
    // Clear the back buffer
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF404040, 1.0f, 0);
    
    // Begin scene
    device->BeginScene();
    
    // First draw indexed primitives (like dx8_cube)
    {
        device->SetStreamSource(0, vb, sizeof(Vertex));
        device->SetIndices(ib, 0);
        device->SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE);
        
        std::cout << "Drawing indexed quad with normals..." << std::endl;
        hr = device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
        if (FAILED(hr)) {
            std::cerr << "DrawIndexedPrimitive failed: " << std::hex << hr << std::endl;
        }
    }
    
    // Now draw XYZRHW vertices (HUD) with texture coordinates
    {
        struct XYZRHWVertex {
            float x, y, z, rhw;
            DWORD color;
            float u, v;
        };
        
        // Create HUD vertices with texture coordinates (like dx8_cube HUD)
        XYZRHWVertex hudVertices[4] = {
            { 10.0f,  10.0f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f }, // Top left
            { 110.0f, 10.0f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f }, // Top right
            { 110.0f, 60.0f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f }, // Bottom right
            { 10.0f,  60.0f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f }  // Bottom left
        };
        
        // Set vertex shader to use XYZRHW format with texture
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        
        // Disable texture for now (but keep the FVF)
        device->SetTexture(0, nullptr);
        
        // Draw the HUD quad
        std::cout << "Drawing XYZRHW quad (HUD)..." << std::endl;
        hr = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, hudVertices, sizeof(XYZRHWVertex));
        if (FAILED(hr)) {
            std::cerr << "DrawPrimitiveUP (XYZRHW) failed: " << std::hex << hr << std::endl;
        }
    }
    
    // End scene
    device->EndScene();
    
    // Present
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    std::cout << "Test completed!" << std::endl;
    
    // Cleanup
    ib->Release();
    vb->Release();
    device->Release();
    d3d8->Release();
    
    return 0;
}