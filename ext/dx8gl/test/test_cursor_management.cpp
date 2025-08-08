#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include "../src/d3d8_surface.h"
#include <memory>
#include <vector>

using namespace dx8gl;

class CursorManagementTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8_ = nullptr;
    IDirect3DDevice8* device_ = nullptr;
    
    void SetUp() override {
        // Initialize dx8gl
        dx8gl_config config = {};
        config.backend_type = DX8GL_BACKEND_OSMESA;
        
        ASSERT_EQ(dx8gl_init(&config), DX8GL_SUCCESS);
        
        // Create Direct3D8 interface
        d3d8_ = Direct3DCreate8(D3D_SDK_VERSION);
        ASSERT_NE(d3d8_, nullptr);
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640;
        pp.BackBufferHeight = 480;
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
        ASSERT_EQ(hr, D3D_OK);
        ASSERT_NE(device_, nullptr);
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
        
        dx8gl_shutdown();
    }
    
    // Helper to create a test cursor surface
    IDirect3DSurface8* CreateTestCursorSurface(UINT width, UINT height) {
        IDirect3DSurface8* surface = nullptr;
        
        // Create an offscreen plain surface for the cursor
        HRESULT hr = device_->CreateImageSurface(width, height, D3DFMT_A8R8G8B8, &surface);
        if (FAILED(hr)) {
            return nullptr;
        }
        
        // Lock and fill with test pattern
        D3DLOCKED_RECT locked_rect;
        hr = surface->LockRect(&locked_rect, nullptr, 0);
        if (SUCCEEDED(hr)) {
            uint32_t* pixels = (uint32_t*)locked_rect.pBits;
            
            // Create a simple crosshair pattern
            for (UINT y = 0; y < height; y++) {
                uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * locked_rect.Pitch);
                for (UINT x = 0; x < width; x++) {
                    // Crosshair pattern with alpha
                    if (x == width / 2 || y == height / 2) {
                        row[x] = 0xFFFF0000; // Red cross
                    } else if (x == width / 2 - 1 || x == width / 2 + 1 ||
                              y == height / 2 - 1 || y == height / 2 + 1) {
                        row[x] = 0xFF000000; // Black outline
                    } else {
                        row[x] = 0x00000000; // Transparent
                    }
                }
            }
            
            surface->UnlockRect();
        }
        
        return surface;
    }
};

TEST_F(CursorManagementTest, SetCursorProperties_ValidSurface) {
    // Create a 32x32 cursor surface
    IDirect3DSurface8* cursor_surface = CreateTestCursorSurface(32, 32);
    ASSERT_NE(cursor_surface, nullptr);
    
    // Set cursor properties with hotspot at center
    HRESULT hr = device_->SetCursorProperties(16, 16, cursor_surface);
    EXPECT_EQ(hr, D3D_OK) << "SetCursorProperties should succeed with valid surface";
    
    // Clean up
    cursor_surface->Release();
}

TEST_F(CursorManagementTest, SetCursorProperties_NullSurface) {
    // Setting cursor properties with null surface should fail
    HRESULT hr = device_->SetCursorProperties(0, 0, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL) << "SetCursorProperties should fail with null surface";
}

TEST_F(CursorManagementTest, SetCursorProperties_LargeCursor) {
    // Create a 64x64 cursor surface (maximum typical size)
    IDirect3DSurface8* cursor_surface = CreateTestCursorSurface(64, 64);
    ASSERT_NE(cursor_surface, nullptr);
    
    // Should succeed but may log a warning
    HRESULT hr = device_->SetCursorProperties(32, 32, cursor_surface);
    EXPECT_EQ(hr, D3D_OK) << "SetCursorProperties should handle 64x64 cursors";
    
    cursor_surface->Release();
}

TEST_F(CursorManagementTest, SetCursorProperties_DifferentHotspots) {
    IDirect3DSurface8* cursor_surface = CreateTestCursorSurface(32, 32);
    ASSERT_NE(cursor_surface, nullptr);
    
    // Test different hotspot positions
    struct HotspotTest {
        UINT x, y;
        const char* description;
    } hotspot_tests[] = {
        {0, 0, "Top-left"},
        {31, 31, "Bottom-right"},
        {16, 16, "Center"},
        {0, 16, "Left-center"},
        {16, 0, "Top-center"},
    };
    
    for (const auto& test : hotspot_tests) {
        HRESULT hr = device_->SetCursorProperties(test.x, test.y, cursor_surface);
        EXPECT_EQ(hr, D3D_OK) << "SetCursorProperties failed for hotspot " << test.description;
    }
    
    cursor_surface->Release();
}

TEST_F(CursorManagementTest, ShowCursor_Toggle) {
    // Initially cursor should be hidden
    BOOL prev_state = device_->ShowCursor(TRUE);
    EXPECT_EQ(prev_state, FALSE) << "Initial cursor state should be hidden";
    
    // Show again should return TRUE (was shown)
    prev_state = device_->ShowCursor(TRUE);
    EXPECT_EQ(prev_state, TRUE) << "Cursor was just shown, should return TRUE";
    
    // Hide cursor
    prev_state = device_->ShowCursor(FALSE);
    EXPECT_EQ(prev_state, TRUE) << "Cursor was shown, should return TRUE when hiding";
    
    // Hide again should return FALSE (was hidden)
    prev_state = device_->ShowCursor(FALSE);
    EXPECT_EQ(prev_state, FALSE) << "Cursor was hidden, should return FALSE";
}

TEST_F(CursorManagementTest, SetCursorPosition_Various) {
    struct PositionTest {
        int x, y;
        DWORD flags;
        const char* description;
    } position_tests[] = {
        {0, 0, 0, "Origin"},
        {100, 100, 0, "Positive coordinates"},
        {-50, -50, 0, "Negative coordinates"},
        {640, 480, 0, "Screen edge"},
        {1000, 1000, 0, "Beyond screen"},
        {320, 240, D3DCURSOR_IMMEDIATE_UPDATE, "Center with immediate update"},
    };
    
    for (const auto& test : position_tests) {
        // SetCursorPosition doesn't return a value, just ensure it doesn't crash
        device_->SetCursorPosition(test.x, test.y, test.flags);
        // If we get here without crashing, the test passes
        SUCCEED() << "SetCursorPosition handled " << test.description;
    }
}

TEST_F(CursorManagementTest, FullCursorWorkflow) {
    // Complete workflow: set properties, show, move, hide
    
    // 1. Create cursor surface
    IDirect3DSurface8* cursor_surface = CreateTestCursorSurface(32, 32);
    ASSERT_NE(cursor_surface, nullptr);
    
    // 2. Set cursor properties
    HRESULT hr = device_->SetCursorProperties(16, 16, cursor_surface);
    EXPECT_EQ(hr, D3D_OK);
    
    // 3. Show cursor
    BOOL prev_state = device_->ShowCursor(TRUE);
    EXPECT_EQ(prev_state, FALSE) << "Cursor should initially be hidden";
    
    // 4. Move cursor around
    device_->SetCursorPosition(100, 100, 0);
    device_->SetCursorPosition(200, 150, D3DCURSOR_IMMEDIATE_UPDATE);
    device_->SetCursorPosition(320, 240, 0);
    
    // 5. Update cursor with new surface
    IDirect3DSurface8* new_cursor = CreateTestCursorSurface(48, 48);
    ASSERT_NE(new_cursor, nullptr);
    
    hr = device_->SetCursorProperties(24, 24, new_cursor);
    EXPECT_EQ(hr, D3D_OK) << "Should be able to update cursor properties";
    
    // 6. Hide cursor
    prev_state = device_->ShowCursor(FALSE);
    EXPECT_EQ(prev_state, TRUE) << "Cursor was shown, should return TRUE";
    
    // Clean up
    cursor_surface->Release();
    new_cursor->Release();
}

TEST_F(CursorManagementTest, MultipleCursorUpdates) {
    // Test rapid cursor updates (stress test)
    const int num_updates = 10;
    
    for (int i = 0; i < num_updates; i++) {
        // Create cursor with varying sizes
        UINT size = 16 + (i * 4); // 16, 20, 24, ..., 52
        if (size > 64) size = 64;
        
        IDirect3DSurface8* cursor = CreateTestCursorSurface(size, size);
        ASSERT_NE(cursor, nullptr) << "Failed to create cursor " << i;
        
        // Set properties with varying hotspots
        HRESULT hr = device_->SetCursorProperties(size / 2, size / 2, cursor);
        EXPECT_EQ(hr, D3D_OK) << "Failed to set cursor properties for cursor " << i;
        
        // Toggle visibility
        device_->ShowCursor(i % 2 == 0 ? TRUE : FALSE);
        
        // Move cursor
        device_->SetCursorPosition(i * 10, i * 10, 0);
        
        cursor->Release();
    }
}

TEST_F(CursorManagementTest, CursorWithDifferentFormats) {
    // Test X8R8G8B8 format (no alpha)
    IDirect3DSurface8* surface = nullptr;
    HRESULT hr = device_->CreateImageSurface(32, 32, D3DFMT_X8R8G8B8, &surface);
    
    if (SUCCEEDED(hr) && surface) {
        // Fill with test data
        D3DLOCKED_RECT locked_rect;
        hr = surface->LockRect(&locked_rect, nullptr, 0);
        if (SUCCEEDED(hr)) {
            uint32_t* pixels = (uint32_t*)locked_rect.pBits;
            for (int i = 0; i < 32 * 32; i++) {
                pixels[i] = 0x00FF00FF; // Magenta (alpha ignored)
            }
            surface->UnlockRect();
        }
        
        // Should handle format without alpha
        hr = device_->SetCursorProperties(16, 16, surface);
        EXPECT_EQ(hr, D3D_OK) << "Should handle X8R8G8B8 format";
        
        surface->Release();
    }
}

// Test to verify state tracking through logs
TEST_F(CursorManagementTest, StateTrackingViaLogs) {
    // This test verifies that the cursor state changes are properly logged
    // The actual verification would be done by examining the log output
    
    std::cout << "\n=== Cursor State Tracking Test ===" << std::endl;
    std::cout << "This test will generate log messages to verify state tracking." << std::endl;
    std::cout << "Check the log output for cursor state changes." << std::endl;
    
    // Create and set cursor
    IDirect3DSurface8* cursor = CreateTestCursorSurface(32, 32);
    ASSERT_NE(cursor, nullptr);
    
    std::cout << "1. Setting cursor properties (32x32, hotspot at 16,16)" << std::endl;
    device_->SetCursorProperties(16, 16, cursor);
    
    std::cout << "2. Showing cursor" << std::endl;
    device_->ShowCursor(TRUE);
    
    std::cout << "3. Moving cursor to (100, 100)" << std::endl;
    device_->SetCursorPosition(100, 100, 0);
    
    std::cout << "4. Moving cursor to (200, 200) with immediate update" << std::endl;
    device_->SetCursorPosition(200, 200, D3DCURSOR_IMMEDIATE_UPDATE);
    
    std::cout << "5. Hiding cursor" << std::endl;
    device_->ShowCursor(FALSE);
    
    std::cout << "6. Showing cursor again" << std::endl;
    device_->ShowCursor(TRUE);
    
    cursor->Release();
    std::cout << "=== End of Cursor State Tracking Test ===" << std::endl;
}