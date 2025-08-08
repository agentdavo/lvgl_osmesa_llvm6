#include <gtest/gtest.h>
#include "../src/dx8gl.h"
#include "../src/d3d8_interface.h"
#include "../src/d3d8_device.h"
#include <memory>

using namespace dx8gl;

class StateBlockTest : public ::testing::Test {
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

TEST_F(StateBlockTest, CreateAndDeleteStateBlock) {
    DWORD token = 0;
    
    // Create an ALL state block
    HRESULT hr = device_->CreateStateBlock(D3DSBT_ALL, &token);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(token, 0u);
    
    // Delete the state block
    hr = device_->DeleteStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
}

TEST_F(StateBlockTest, CreateMultipleStateBlocks) {
    DWORD token1 = 0, token2 = 0, token3 = 0;
    
    // Create state blocks of different types
    HRESULT hr = device_->CreateStateBlock(D3DSBT_ALL, &token1);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->CreateStateBlock(D3DSBT_PIXELSTATE, &token2);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->CreateStateBlock(D3DSBT_VERTEXSTATE, &token3);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify all tokens are unique
    EXPECT_NE(token1, token2);
    EXPECT_NE(token2, token3);
    EXPECT_NE(token1, token3);
    
    // Clean up
    device_->DeleteStateBlock(token1);
    device_->DeleteStateBlock(token2);
    device_->DeleteStateBlock(token3);
}

TEST_F(StateBlockTest, BeginAndEndStateBlock) {
    // Begin recording
    HRESULT hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Change some states while recording
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    // End recording and get token
    DWORD token = 0;
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(token, 0u);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, ApplyStateBlock) {
    // Set initial states
    device_->SetRenderState(D3DRS_ZENABLE, FALSE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    
    // Begin recording
    HRESULT hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Record different states
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_LIGHTING, TRUE);
    
    // End recording
    DWORD token = 0;
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change states to something else
    device_->SetRenderState(D3DRS_ZENABLE, FALSE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    
    // Apply the state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify states were restored
    DWORD value;
    device_->GetRenderState(D3DRS_ZENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_ALPHABLENDENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_LIGHTING, &value);
    EXPECT_EQ(value, TRUE);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, CaptureStateBlock) {
    // Create a state block with initial values
    DWORD token = 0;
    HRESULT hr = device_->CreateStateBlock(D3DSBT_ALL, &token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change states after creation
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Capture current state into the block
    hr = device_->CaptureStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change states again
    device_->SetRenderState(D3DRS_ZENABLE, FALSE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    
    // Apply the captured state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify states were restored to captured values
    DWORD value;
    device_->GetRenderState(D3DRS_ZENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_ALPHABLENDENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_CULLMODE, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DCULL_NONE));
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, TransformStateBlock) {
    // Create identity and translation matrices
    D3DMATRIX identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    D3DMATRIX translation = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        10, 20, 30, 1
    };
    
    // Set initial transform
    device_->SetTransform(D3DTS_WORLD, &identity);
    
    // Begin recording
    HRESULT hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Record transform change
    device_->SetTransform(D3DTS_WORLD, &translation);
    
    // End recording
    DWORD token = 0;
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Reset transform
    device_->SetTransform(D3DTS_WORLD, &identity);
    
    // Apply state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify transform was restored
    D3DMATRIX result;
    device_->GetTransform(D3DTS_WORLD, &result);
    
    EXPECT_FLOAT_EQ(result._41, 10.0f);
    EXPECT_FLOAT_EQ(result._42, 20.0f);
    EXPECT_FLOAT_EQ(result._43, 30.0f);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, TextureStageStateBlock) {
    // Begin recording
    HRESULT hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Record texture stage state changes
    device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device_->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADD);
    
    // End recording
    DWORD token = 0;
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change texture stage states
    device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
    device_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    device_->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    
    // Apply state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify texture stage states were restored
    DWORD value;
    device_->GetTextureStageState(0, D3DTSS_COLOROP, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DTOP_MODULATE));
    
    device_->GetTextureStageState(0, D3DTSS_COLORARG1, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DTA_TEXTURE));
    
    device_->GetTextureStageState(0, D3DTSS_COLORARG2, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DTA_DIFFUSE));
    
    device_->GetTextureStageState(1, D3DTSS_COLOROP, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DTOP_ADD));
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, PixelStateBlock) {
    // Create a pixel state block
    DWORD token = 0;
    HRESULT hr = device_->CreateStateBlock(D3DSBT_PIXELSTATE, &token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change pixel-related states
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    device_->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    
    // Change a vertex-related state (should not be captured)
    device_->SetRenderState(D3DRS_LIGHTING, TRUE);
    
    // Capture the state
    hr = device_->CaptureStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change all states
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    device_->SetRenderState(D3DRS_ZENABLE, FALSE);
    device_->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    
    // Apply the pixel state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify pixel states were restored
    DWORD value;
    device_->GetRenderState(D3DRS_ALPHABLENDENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_SRCBLEND, &value);
    EXPECT_EQ(value, static_cast<DWORD>(D3DBLEND_SRCALPHA));
    
    device_->GetRenderState(D3DRS_ZENABLE, &value);
    EXPECT_EQ(value, TRUE);
    
    // Lighting should still be FALSE (not part of pixel state)
    device_->GetRenderState(D3DRS_LIGHTING, &value);
    EXPECT_EQ(value, FALSE);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, VertexStateBlock) {
    // Create a vertex state block
    DWORD token = 0;
    HRESULT hr = device_->CreateStateBlock(D3DSBT_VERTEXSTATE, &token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change vertex-related states
    device_->SetRenderState(D3DRS_LIGHTING, TRUE);
    device_->SetRenderState(D3DRS_AMBIENT, 0x00404040);
    device_->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
    
    // Set a transform (part of vertex state)
    D3DMATRIX scale = {
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 1
    };
    device_->SetTransform(D3DTS_WORLD, &scale);
    
    // Change a pixel-related state (should not be captured)
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    
    // Capture the state
    hr = device_->CaptureStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Change all states
    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    device_->SetRenderState(D3DRS_AMBIENT, 0x00FFFFFF);
    device_->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    D3DMATRIX identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    device_->SetTransform(D3DTS_WORLD, &identity);
    
    // Apply the vertex state block
    hr = device_->ApplyStateBlock(token);
    EXPECT_EQ(hr, D3D_OK);
    
    // Verify vertex states were restored
    DWORD value;
    device_->GetRenderState(D3DRS_LIGHTING, &value);
    EXPECT_EQ(value, TRUE);
    
    device_->GetRenderState(D3DRS_AMBIENT, &value);
    EXPECT_EQ(value, 0x00404040u);
    
    device_->GetRenderState(D3DRS_NORMALIZENORMALS, &value);
    EXPECT_EQ(value, TRUE);
    
    // Alpha blend should still be FALSE (not part of vertex state)
    device_->GetRenderState(D3DRS_ALPHABLENDENABLE, &value);
    EXPECT_EQ(value, FALSE);
    
    // Verify transform was restored
    D3DMATRIX result;
    device_->GetTransform(D3DTS_WORLD, &result);
    EXPECT_FLOAT_EQ(result._11, 2.0f);
    EXPECT_FLOAT_EQ(result._22, 2.0f);
    EXPECT_FLOAT_EQ(result._33, 2.0f);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, InvalidParameters) {
    DWORD token = 0;
    
    // Test null pointer for token
    HRESULT hr = device_->CreateStateBlock(D3DSBT_ALL, nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    hr = device_->EndStateBlock(nullptr);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test invalid state block type
    hr = device_->CreateStateBlock(static_cast<D3DSTATEBLOCKTYPE>(999), &token);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test EndStateBlock without BeginStateBlock
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    
    // Test operations on non-existent token
    hr = device_->ApplyStateBlock(99999);
    EXPECT_EQ(hr, D3D_OK); // Should succeed but do nothing
    
    hr = device_->CaptureStateBlock(99999);
    EXPECT_EQ(hr, D3D_OK); // Should succeed but do nothing
    
    hr = device_->DeleteStateBlock(99999);
    EXPECT_EQ(hr, D3D_OK); // Should succeed but do nothing
}

TEST_F(StateBlockTest, NestedBeginStateBlock) {
    // Begin first state block
    HRESULT hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Begin second state block (should replace the first)
    hr = device_->BeginStateBlock();
    EXPECT_EQ(hr, D3D_OK);
    
    // Change a state
    device_->SetRenderState(D3DRS_ZENABLE, TRUE);
    
    // End state block
    DWORD token = 0;
    hr = device_->EndStateBlock(&token);
    EXPECT_EQ(hr, D3D_OK);
    EXPECT_NE(token, 0u);
    
    // Clean up
    device_->DeleteStateBlock(token);
}

TEST_F(StateBlockTest, MultipleStateBlockManagement) {
    std::vector<DWORD> tokens;
    
    // Create multiple state blocks
    for (int i = 0; i < 10; i++) {
        // Begin recording
        HRESULT hr = device_->BeginStateBlock();
        EXPECT_EQ(hr, D3D_OK);
        
        // Record unique state for each block
        device_->SetRenderState(D3DRS_AMBIENT, 0x00100000 * i);
        
        // End recording
        DWORD token = 0;
        hr = device_->EndStateBlock(&token);
        EXPECT_EQ(hr, D3D_OK);
        
        tokens.push_back(token);
    }
    
    // Apply state blocks in reverse order
    for (int i = 9; i >= 0; i--) {
        HRESULT hr = device_->ApplyStateBlock(tokens[i]);
        EXPECT_EQ(hr, D3D_OK);
        
        // Verify the state
        DWORD value;
        device_->GetRenderState(D3DRS_AMBIENT, &value);
        EXPECT_EQ(value, static_cast<DWORD>(0x00100000 * i));
    }
    
    // Clean up all state blocks
    for (DWORD token : tokens) {
        device_->DeleteStateBlock(token);
    }
}