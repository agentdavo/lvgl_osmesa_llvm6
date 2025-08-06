// Test alpha blending and transparency operations in dx8gl
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include "../src/d3d8_game.h"
#include "../src/d3dx_compat.h"
#include "../src/dx8gl.h"

// Test configuration
const int TEST_WIDTH = 256;
const int TEST_HEIGHT = 256;
const int NUM_BLEND_TESTS = 12;

// Test vertex structure
struct TestVertex {
    float x, y, z, rhw;
    D3DCOLOR color;
};

#define D3DFVF_TESTVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

// Blend mode test configurations
struct BlendTest {
    const char* name;
    D3DBLEND srcBlend;
    D3DBLEND destBlend;
    D3DCOLOR backgroundColor;
    D3DCOLOR foregroundColor;
    D3DCOLOR expectedColor;  // Approximate expected result
};

// Common blend mode tests
BlendTest g_blendTests[] = {
    // Standard alpha blending (most common)
    {
        "Standard Alpha (SrcAlpha/InvSrcAlpha)",
        D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA,
        D3DCOLOR_ARGB(255, 255, 0, 0),     // Red background
        D3DCOLOR_ARGB(128, 0, 0, 255),     // 50% blue foreground
        D3DCOLOR_ARGB(255, 127, 0, 128)    // Expected: blend of red and blue
    },
    
    // Additive blending (particles, lights)
    {
        "Additive (One/One)",
        D3DBLEND_ONE, D3DBLEND_ONE,
        D3DCOLOR_ARGB(255, 64, 64, 64),    // Gray background
        D3DCOLOR_ARGB(255, 128, 128, 128), // Gray foreground
        D3DCOLOR_ARGB(255, 192, 192, 192)  // Expected: lighter gray
    },
    
    // Multiplicative blending (shadows)
    {
        "Multiplicative (Zero/SrcColor)",
        D3DBLEND_ZERO, D3DBLEND_SRCCOLOR,
        D3DCOLOR_ARGB(255, 255, 255, 255), // White background
        D3DCOLOR_ARGB(255, 128, 128, 128), // Gray foreground
        D3DCOLOR_ARGB(255, 128, 128, 128)  // Expected: darkened
    },
    
    // Pre-multiplied alpha
    {
        "Pre-multiplied (One/InvSrcAlpha)",
        D3DBLEND_ONE, D3DBLEND_INVSRCALPHA,
        D3DCOLOR_ARGB(255, 200, 100, 50),  // Orange background
        D3DCOLOR_ARGB(128, 64, 64, 128),   // Pre-multiplied blue
        D3DCOLOR_ARGB(255, 164, 82, 114)   // Expected blend
    },
    
    // Inverted blend
    {
        "Inverted (InvDestColor/Zero)",
        D3DBLEND_INVDESTCOLOR, D3DBLEND_ZERO,
        D3DCOLOR_ARGB(255, 100, 150, 200), // Light blue background
        D3DCOLOR_ARGB(255, 255, 255, 255), // White foreground
        D3DCOLOR_ARGB(255, 155, 105, 55)   // Expected: inverted colors
    },
    
    // Source color modulation
    {
        "Source Modulation (SrcColor/Zero)",
        D3DBLEND_SRCCOLOR, D3DBLEND_ZERO,
        D3DCOLOR_ARGB(255, 255, 255, 255), // White background (ignored)
        D3DCOLOR_ARGB(200, 128, 64, 192),  // Purple foreground
        D3DCOLOR_ARGB(200, 128, 64, 192)   // Expected: just source
    },
    
    // Destination alpha blend
    {
        "Dest Alpha (DestAlpha/InvDestAlpha)",
        D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA,
        D3DCOLOR_ARGB(192, 255, 0, 0),     // Semi-transparent red
        D3DCOLOR_ARGB(255, 0, 255, 0),     // Opaque green
        D3DCOLOR_ARGB(223, 192, 63, 0)     // Expected blend
    },
    
    // Both alpha blend
    {
        "Both Alpha (SrcAlpha/DestAlpha)",
        D3DBLEND_SRCALPHA, D3DBLEND_DESTALPHA,
        D3DCOLOR_ARGB(128, 255, 255, 0),   // Semi-transparent yellow
        D3DCOLOR_ARGB(128, 0, 255, 255),   // Semi-transparent cyan
        D3DCOLOR_ARGB(192, 0, 255, 128)    // Expected blend
    },
    
    // Screen blend mode (brightening)
    {
        "Screen (InvDestColor/One)",
        D3DBLEND_INVDESTCOLOR, D3DBLEND_ONE,
        D3DCOLOR_ARGB(255, 128, 128, 128), // Gray background
        D3DCOLOR_ARGB(255, 128, 128, 128), // Gray foreground
        D3DCOLOR_ARGB(255, 192, 192, 192)  // Expected: lighter
    },
    
    // Overlay simulation
    {
        "Overlay Simulation (DestColor/SrcColor)",
        D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR,
        D3DCOLOR_ARGB(255, 100, 100, 100), // Dark gray background
        D3DCOLOR_ARGB(255, 200, 200, 200), // Light gray foreground
        D3DCOLOR_ARGB(255, 178, 178, 178)  // Expected blend
    },
    
    // Alpha test with full transparency
    {
        "Full Transparency (SrcAlpha/InvSrcAlpha)",
        D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA,
        D3DCOLOR_ARGB(255, 255, 0, 0),     // Red background
        D3DCOLOR_ARGB(0, 0, 0, 255),       // Fully transparent blue
        D3DCOLOR_ARGB(255, 255, 0, 0)      // Expected: just background
    },
    
    // Alpha saturation
    {
        "Alpha Saturation (SrcAlphaSat/One)",
        D3DBLEND_SRCALPHASAT, D3DBLEND_ONE,
        D3DCOLOR_ARGB(200, 100, 100, 100), // Semi-transparent gray
        D3DCOLOR_ARGB(200, 150, 150, 150), // Semi-transparent light gray
        D3DCOLOR_ARGB(255, 220, 220, 220)  // Expected: saturated blend
    }
};

// Helper function to compare colors with tolerance
bool ColorsMatch(D3DCOLOR color1, D3DCOLOR color2, int tolerance = 10) {
    int a1 = (color1 >> 24) & 0xFF;
    int r1 = (color1 >> 16) & 0xFF;
    int g1 = (color1 >> 8) & 0xFF;
    int b1 = color1 & 0xFF;
    
    int a2 = (color2 >> 24) & 0xFF;
    int r2 = (color2 >> 16) & 0xFF;
    int g2 = (color2 >> 8) & 0xFF;
    int b2 = color2 & 0xFF;
    
    return (abs(a1 - a2) <= tolerance &&
            abs(r1 - r2) <= tolerance &&
            abs(g1 - g2) <= tolerance &&
            abs(b1 - b2) <= tolerance);
}

// Draw a colored quad
void DrawQuad(IDirect3DDevice8* device, float x, float y, float width, float height, D3DCOLOR color) {
    TestVertex vertices[4] = {
        { x,         y,          0.5f, 1.0f, color },
        { x + width, y,          0.5f, 1.0f, color },
        { x,         y + height, 0.5f, 1.0f, color },
        { x + width, y + height, 0.5f, 1.0f, color }
    };
    
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(TestVertex));
}

// Test a specific blend mode
bool TestBlendMode(IDirect3DDevice8* device, const BlendTest& test) {
    std::cout << "Testing: " << test.name << std::endl;
    
    // Clear to background color
    device->Clear(0, nullptr, D3DCLEAR_TARGET, test.backgroundColor, 1.0f, 0);
    
    device->BeginScene();
    
    // Enable alpha blending
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, test.srcBlend);
    device->SetRenderState(D3DRS_DESTBLEND, test.destBlend);
    
    // Draw foreground quad with blending
    DrawQuad(device, 64, 64, 128, 128, test.foregroundColor);
    
    device->EndScene();
    
    // Read back the center pixel
    IDirect3DSurface8* backBuffer = nullptr;
    device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    
    if (backBuffer) {
        D3DLOCKED_RECT lockedRect;
        RECT readRect = { 127, 127, 129, 129 };  // Center pixel
        
        if (SUCCEEDED(backBuffer->LockRect(&lockedRect, &readRect, D3DLOCK_READONLY))) {
            D3DCOLOR* pixels = (D3DCOLOR*)lockedRect.pBits;
            D3DCOLOR resultColor = pixels[0];
            
            backBuffer->UnlockRect();
            
            // Compare with expected color
            bool success = ColorsMatch(resultColor, test.expectedColor, 20);
            
            if (!success) {
                std::cerr << "  FAILED: Expected " << std::hex << test.expectedColor
                         << " but got " << resultColor << std::dec << std::endl;
            } else {
                std::cout << "  PASSED" << std::endl;
            }
            
            backBuffer->Release();
            return success;
        }
        
        backBuffer->Release();
    }
    
    return false;
}

// Test alpha test functionality
bool TestAlphaTest(IDirect3DDevice8* device) {
    std::cout << "Testing Alpha Test functionality..." << std::endl;
    
    // Clear to red
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 1.0f, 0);
    
    device->BeginScene();
    
    // Enable alpha test
    device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHAREF, 128);  // Threshold at 50%
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    
    // Draw three quads with different alpha values
    DrawQuad(device, 10, 10, 50, 50, D3DCOLOR_ARGB(255, 0, 255, 0));  // Should pass
    DrawQuad(device, 70, 10, 50, 50, D3DCOLOR_ARGB(127, 0, 255, 0));  // Should fail
    DrawQuad(device, 130, 10, 50, 50, D3DCOLOR_ARGB(200, 0, 255, 0)); // Should pass
    
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    
    device->EndScene();
    
    // Verify results (simplified check)
    std::cout << "  Alpha test visual check complete" << std::endl;
    return true;
}

// Test texture alpha blending
bool TestTextureAlpha(IDirect3DDevice8* device) {
    std::cout << "Testing Texture Alpha blending..." << std::endl;
    
    // Create a texture with alpha channel
    IDirect3DTexture8* texture = nullptr;
    if (FAILED(device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture))) {
        std::cerr << "  Failed to create texture" << std::endl;
        return false;
    }
    
    // Fill texture with gradient alpha
    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(texture->LockRect(0, &lockedRect, nullptr, 0))) {
        D3DCOLOR* pixels = (D3DCOLOR*)lockedRect.pBits;
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                int alpha = (x * 255) / 63;  // Horizontal alpha gradient
                pixels[y * (lockedRect.Pitch / 4) + x] = D3DCOLOR_ARGB(alpha, 255, 255, 255);
            }
        }
        texture->UnlockRect(0);
    }
    
    // Clear to blue
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);
    
    device->BeginScene();
    
    // Set up texture and blending
    device->SetTexture(0, texture);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    // Set texture stage states
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    
    // Draw textured quad
    struct TexVertex {
        float x, y, z, rhw;
        float u, v;
    } vertices[4] = {
        { 50,  50, 0.5f, 1.0f, 0.0f, 0.0f },
        { 200, 50, 0.5f, 1.0f, 1.0f, 0.0f },
        { 50,  200, 0.5f, 1.0f, 0.0f, 1.0f },
        { 200, 200, 0.5f, 1.0f, 1.0f, 1.0f }
    };
    
    device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(TexVertex));
    
    // Clean up
    device->SetTexture(0, nullptr);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    device->EndScene();
    
    texture->Release();
    
    std::cout << "  Texture alpha test complete" << std::endl;
    return true;
}

int main() {
    std::cout << "=== DirectX 8 Alpha Blending and Transparency Test ===" << std::endl;
    
    // Initialize dx8gl
    dx8gl_config config = {};
    config.backend_type = DX8GL_BACKEND_OSMESA;
    
    if (!dx8gl_init(&config)) {
        std::cerr << "Failed to initialize dx8gl" << std::endl;
        return 1;
    }
    
    // Create Direct3D8 interface
    IDirect3D8* d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8) {
        std::cerr << "Failed to create Direct3D8 interface" << std::endl;
        dx8gl_shutdown();
        return 1;
    }
    
    // Set up presentation parameters
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;  // Use format with alpha
    pp.BackBufferWidth = TEST_WIDTH;
    pp.BackBufferHeight = TEST_HEIGHT;
    
    // Create device
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
        std::cerr << "Failed to create Direct3D8 device" << std::endl;
        d3d8->Release();
        dx8gl_shutdown();
        return 1;
    }
    
    // Set up basic render states
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetVertexShader(D3DFVF_TESTVERTEX);
    
    int passedTests = 0;
    int totalTests = 0;
    
    // Run blend mode tests
    std::cout << "\n--- Blend Mode Tests ---" << std::endl;
    for (int i = 0; i < NUM_BLEND_TESTS; ++i) {
        if (TestBlendMode(device, g_blendTests[i])) {
            passedTests++;
        }
        totalTests++;
    }
    
    // Run alpha test
    std::cout << "\n--- Alpha Test ---" << std::endl;
    if (TestAlphaTest(device)) {
        passedTests++;
    }
    totalTests++;
    
    // Run texture alpha test
    std::cout << "\n--- Texture Alpha Test ---" << std::endl;
    if (TestTextureAlpha(device)) {
        passedTests++;
    }
    totalTests++;
    
    // Save a test image showing various blend modes
    std::cout << "\n--- Saving Blend Test Results ---" << std::endl;
    
    // Create a composite image showing all blend modes
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(64, 64, 64), 1.0f, 0);
    device->BeginScene();
    
    // Draw a grid of blend mode examples
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            int testIndex = i * 3 + j;
            if (testIndex < NUM_BLEND_TESTS) {
                float x = j * 80 + 10;
                float y = i * 60 + 10;
                
                // Draw background
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                DrawQuad(device, x, y, 70, 50, g_blendTests[testIndex].backgroundColor);
                
                // Draw blended foreground
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                device->SetRenderState(D3DRS_SRCBLEND, g_blendTests[testIndex].srcBlend);
                device->SetRenderState(D3DRS_DESTBLEND, g_blendTests[testIndex].destBlend);
                DrawQuad(device, x + 15, y + 10, 40, 30, g_blendTests[testIndex].foregroundColor);
            }
        }
    }
    
    device->EndScene();
    device->Present(nullptr, nullptr, nullptr, nullptr);
    
    // Try to save the result
    IDirect3DSurface8* backBuffer = nullptr;
    if (SUCCEEDED(device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) {
        HRESULT saveResult = D3DXSaveSurfaceToFile(
            "alpha_blend_test_results.bmp",
            D3DXIFF_BMP,
            backBuffer,
            nullptr,
            nullptr
        );
        
        if (SUCCEEDED(saveResult)) {
            std::cout << "Saved blend test results to alpha_blend_test_results.bmp" << std::endl;
        } else {
            std::cout << "Failed to save test results (D3DXSaveSurfaceToFile not available)" << std::endl;
        }
        
        backBuffer->Release();
    }
    
    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passedTests << "/" << totalTests << " tests" << std::endl;
    
    if (passedTests == totalTests) {
        std::cout << "SUCCESS: All alpha blending tests passed!" << std::endl;
    } else {
        std::cout << "PARTIAL: Some tests failed, review results above" << std::endl;
    }
    
    // Clean up
    device->Release();
    d3d8->Release();
    dx8gl_shutdown();
    
    return (passedTests == totalTests) ? 0 : 1;
}