#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <memory>

using namespace dx8gl;

// Define D3DCS_* clip status flags if not already defined
#ifndef D3DCS_LEFT
#define D3DCS_LEFT        0x00000001L
#define D3DCS_RIGHT       0x00000002L
#define D3DCS_TOP         0x00000004L
#define D3DCS_BOTTOM      0x00000008L
#define D3DCS_FRONT       0x00000010L
#define D3DCS_BACK        0x00000020L
#define D3DCS_PLANE0      0x00000040L
#define D3DCS_PLANE1      0x00000080L
#define D3DCS_PLANE2      0x00000100L
#define D3DCS_PLANE3      0x00000200L
#define D3DCS_PLANE4      0x00000400L
#define D3DCS_PLANE5      0x00000800L
#define D3DCS_ALL         0x00000FFFL
#endif

class ClipStatusTest : public ::testing::Test {
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
};

TEST_F(ClipStatusTest, SetAndGetClipStatus) {
    D3DCLIPSTATUS8 clipStatus = {};
    clipStatus.ClipUnion = D3DCS_LEFT | D3DCS_RIGHT;
    clipStatus.ClipIntersection = D3DCS_TOP | D3DCS_BOTTOM;
    
    // Set clip status
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get clip status
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify values match
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus.ClipIntersection);
}

TEST_F(ClipStatusTest, SetClipStatusWithAllFlags) {
    D3DCLIPSTATUS8 clipStatus = {};
    clipStatus.ClipUnion = D3DCS_ALL;
    clipStatus.ClipIntersection = 0;
    
    // Set clip status with all flags
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get and verify
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    EXPECT_EQ(retrievedStatus.ClipUnion, D3DCS_ALL);
    EXPECT_EQ(retrievedStatus.ClipIntersection, 0u);
}

TEST_F(ClipStatusTest, SetClipStatusWithPlanes) {
    D3DCLIPSTATUS8 clipStatus = {};
    clipStatus.ClipUnion = D3DCS_PLANE0 | D3DCS_PLANE1 | D3DCS_PLANE2;
    clipStatus.ClipIntersection = D3DCS_PLANE3 | D3DCS_PLANE4 | D3DCS_PLANE5;
    
    // Set clip status with plane flags
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get and verify
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus.ClipIntersection);
}

TEST_F(ClipStatusTest, SetClipStatusWithFrustumPlanes) {
    D3DCLIPSTATUS8 clipStatus = {};
    clipStatus.ClipUnion = D3DCS_LEFT | D3DCS_RIGHT | D3DCS_TOP | D3DCS_BOTTOM | D3DCS_FRONT | D3DCS_BACK;
    clipStatus.ClipIntersection = D3DCS_FRONT | D3DCS_BACK;
    
    // Set clip status with frustum plane flags
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get and verify
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus.ClipIntersection);
}

TEST_F(ClipStatusTest, MultipleSetClipStatus) {
    // Set first clip status
    D3DCLIPSTATUS8 clipStatus1 = {};
    clipStatus1.ClipUnion = D3DCS_LEFT;
    clipStatus1.ClipIntersection = D3DCS_RIGHT;
    
    HRESULT hr = device_->SetClipStatus(&clipStatus1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify first status
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_EQ(retrievedStatus.ClipUnion, D3DCS_LEFT);
    EXPECT_EQ(retrievedStatus.ClipIntersection, D3DCS_RIGHT);
    
    // Set second clip status
    D3DCLIPSTATUS8 clipStatus2 = {};
    clipStatus2.ClipUnion = D3DCS_TOP | D3DCS_BOTTOM;
    clipStatus2.ClipIntersection = D3DCS_FRONT | D3DCS_BACK;
    
    hr = device_->SetClipStatus(&clipStatus2);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify second status overwrote first
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus2.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus2.ClipIntersection);
}

TEST_F(ClipStatusTest, GetClipStatusInitialState) {
    // Get clip status without setting it first
    D3DCLIPSTATUS8 retrievedStatus = {};
    retrievedStatus.ClipUnion = 0xDEADBEEF;     // Set to non-zero to test initialization
    retrievedStatus.ClipIntersection = 0xBADCAFE;
    
    HRESULT hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Should return zeros (default initialized state)
    EXPECT_EQ(retrievedStatus.ClipUnion, 0u);
    EXPECT_EQ(retrievedStatus.ClipIntersection, 0u);
}

TEST_F(ClipStatusTest, SetClipStatusNullPointer) {
    // Test with null pointer
    HRESULT hr = device_->SetClipStatus(nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(ClipStatusTest, GetClipStatusNullPointer) {
    // Test with null pointer
    HRESULT hr = device_->GetClipStatus(nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
}

TEST_F(ClipStatusTest, SetClipStatusWithInvalidFlags) {
    D3DCLIPSTATUS8 clipStatus = {};
    // Set flags beyond D3DCS_ALL (which is 0xFFF)
    clipStatus.ClipUnion = 0xF000;  // Invalid flags
    clipStatus.ClipIntersection = 0x8000;  // Invalid flags
    
    // Should still succeed but might log a warning
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get and verify - invalid bits might be masked off or preserved
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // The implementation should handle invalid flags gracefully
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus.ClipIntersection);
}

TEST_F(ClipStatusTest, ClipStatusPersistence) {
    // Set clip status
    D3DCLIPSTATUS8 clipStatus = {};
    clipStatus.ClipUnion = D3DCS_LEFT | D3DCS_TOP;
    clipStatus.ClipIntersection = D3DCS_RIGHT | D3DCS_BOTTOM;
    
    HRESULT hr = device_->SetClipStatus(&clipStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    // Perform some other operations
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    D3DMATRIX identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    device_->SetTransform(D3DTS_WORLD, &identity);
    
    // Verify clip status is still the same
    D3DCLIPSTATUS8 retrievedStatus = {};
    hr = device_->GetClipStatus(&retrievedStatus);
    EXPECT_EQ(hr, D3D_OK);
    
    EXPECT_EQ(retrievedStatus.ClipUnion, clipStatus.ClipUnion);
    EXPECT_EQ(retrievedStatus.ClipIntersection, clipStatus.ClipIntersection);
}

TEST_F(ClipStatusTest, ClipStatusCombinations) {
    struct TestCase {
        DWORD union_flags;
        DWORD intersection_flags;
        const char* description;
    };
    
    TestCase testCases[] = {
        {0, 0, "Empty clip status"},
        {D3DCS_LEFT, 0, "Only union, no intersection"},
        {0, D3DCS_RIGHT, "Only intersection, no union"},
        {D3DCS_LEFT | D3DCS_RIGHT, D3DCS_TOP | D3DCS_BOTTOM, "Mixed planes"},
        {D3DCS_PLANE0 | D3DCS_PLANE1, D3DCS_PLANE2 | D3DCS_PLANE3, "User clip planes"},
        {D3DCS_ALL, D3DCS_ALL, "All flags in both"},
        {D3DCS_FRONT | D3DCS_BACK, D3DCS_FRONT | D3DCS_BACK, "Same flags in both"},
    };
    
    for (const auto& test : testCases) {
        D3DCLIPSTATUS8 clipStatus = {};
        clipStatus.ClipUnion = test.union_flags;
        clipStatus.ClipIntersection = test.intersection_flags;
        
        // Set clip status
        HRESULT hr = device_->SetClipStatus(&clipStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to set: " << test.description;
        
        // Get and verify
        D3DCLIPSTATUS8 retrievedStatus = {};
        hr = device_->GetClipStatus(&retrievedStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get: " << test.description;
        
        EXPECT_EQ(retrievedStatus.ClipUnion, test.union_flags) 
            << "Union mismatch for: " << test.description;
        EXPECT_EQ(retrievedStatus.ClipIntersection, test.intersection_flags)
            << "Intersection mismatch for: " << test.description;
    }
}

TEST_F(ClipStatusTest, ClipStatusBitOperations) {
    // Test setting individual bits
    for (int bit = 0; bit < 12; bit++) {
        DWORD flag = 1 << bit;
        
        D3DCLIPSTATUS8 clipStatus = {};
        clipStatus.ClipUnion = flag;
        clipStatus.ClipIntersection = 0;
        
        HRESULT hr = device_->SetClipStatus(&clipStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to set bit " << bit;
        
        D3DCLIPSTATUS8 retrievedStatus = {};
        hr = device_->GetClipStatus(&retrievedStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get bit " << bit;
        
        EXPECT_EQ(retrievedStatus.ClipUnion, flag) << "Bit " << bit << " not preserved in union";
        EXPECT_EQ(retrievedStatus.ClipIntersection, 0u) << "Intersection not zero for bit " << bit;
        
        // Now test with intersection
        clipStatus.ClipUnion = 0;
        clipStatus.ClipIntersection = flag;
        
        hr = device_->SetClipStatus(&clipStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to set intersection bit " << bit;
        
        hr = device_->GetClipStatus(&retrievedStatus);
        EXPECT_EQ(hr, D3D_OK) << "Failed to get intersection bit " << bit;
        
        EXPECT_EQ(retrievedStatus.ClipUnion, 0u) << "Union not zero for bit " << bit;
        EXPECT_EQ(retrievedStatus.ClipIntersection, flag) << "Bit " << bit << " not preserved in intersection";
    }
}