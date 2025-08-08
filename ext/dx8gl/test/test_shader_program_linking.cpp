#include <gtest/gtest.h>
#include "d3d8_interface.h"
#include "d3d8_device.h"
#include "shader_program_manager.h"
#include "vertex_shader_manager.h"
#include "pixel_shader_manager.h"
#include "shader_constant_manager.h"
#include "d3d8_types.h"
#include <memory>
#include <vector>

using namespace dx8gl;

class ShaderProgramLinkingTest : public ::testing::Test {
protected:
    IDirect3D8* d3d8_;
    IDirect3DDevice8* device_;
    std::unique_ptr<ShaderProgramManager> program_manager_;
    std::unique_ptr<VertexShaderManager> vertex_manager_;
    std::unique_ptr<PixelShaderManager> pixel_manager_;
    std::unique_ptr<ShaderConstantManager> constant_manager_;
    
    void SetUp() override {
        // Initialize D3D8
        d3d8_ = Direct3DCreate8(D3D_SDK_VERSION);
        ASSERT_NE(d3d8_, nullptr);
        
        // Create device
        D3DPRESENT_PARAMETERS pp = {};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferWidth = 640;
        pp.BackBufferHeight = 480;
        
        HRESULT hr = d3d8_->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &pp,
            &device_
        );
        ASSERT_EQ(hr, D3D_OK);
        
        // Create managers
        vertex_manager_ = std::make_unique<VertexShaderManager>();
        pixel_manager_ = std::make_unique<PixelShaderManager>();
        constant_manager_ = std::make_unique<ShaderConstantManager>();
        program_manager_ = std::make_unique<ShaderProgramManager>();
        
        // Initialize program manager with other managers
        bool init_ok = program_manager_->initialize(
            vertex_manager_.get(),
            pixel_manager_.get(),
            constant_manager_.get()
        );
        ASSERT_TRUE(init_ok);
    }
    
    void TearDown() override {
        program_manager_.reset();
        constant_manager_.reset();
        pixel_manager_.reset();
        vertex_manager_.reset();
        
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        if (d3d8_) {
            d3d8_->Release();
            d3d8_ = nullptr;
        }
    }
    
    // Helper to create a simple vertex shader
    DWORD CreateVertexShader(const std::vector<DWORD>& bytecode) {
        DWORD handle = 0;
        HRESULT hr = device_->CreateVertexShader(
            nullptr,  // Declaration
            bytecode.data(),
            &handle,
            0  // Usage
        );
        EXPECT_EQ(hr, D3D_OK);
        return handle;
    }
    
    // Helper to create a simple pixel shader
    DWORD CreatePixelShader(const std::vector<DWORD>& bytecode) {
        DWORD handle = 0;
        HRESULT hr = device_->CreatePixelShader(bytecode.data(), &handle);
        EXPECT_EQ(hr, D3D_OK);
        return handle;
    }
};

TEST_F(ShaderProgramLinkingTest, LinkBasicShaders) {
    // Simple vertex shader bytecode (vs.1.1)
    std::vector<DWORD> vs_bytecode = {
        0xFFFE0101,  // vs_1_1
        0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
        0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
        0x0000FFFF  // end
    };
    
    // Simple pixel shader bytecode (ps.1.1)
    std::vector<DWORD> ps_bytecode = {
        0xFFFF0101,  // ps_1_1
        0x00000051, 0xA00F0000, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,  // def c0, 1, 1, 1, 1
        0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
        0x0000FFFF  // end
    };
    
    // Create shaders
    DWORD vs_handle = CreateVertexShader(vs_bytecode);
    DWORD ps_handle = CreatePixelShader(ps_bytecode);
    
    ASSERT_NE(vs_handle, 0u);
    ASSERT_NE(ps_handle, 0u);
    
    // Set shaders on device
    HRESULT hr = device_->SetVertexShader(vs_handle);
    EXPECT_EQ(hr, D3D_OK);
    
    hr = device_->SetPixelShader(ps_handle);
    EXPECT_EQ(hr, D3D_OK);
    
    // Get linked program
    GLuint program = program_manager_->get_current_program();
    EXPECT_NE(program, 0u);
    
    // Apply shader state
    program_manager_->apply_shader_state();
    
    // Verify program is valid
    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    EXPECT_EQ(link_status, GL_TRUE);
    
    // Clean up
    device_->DeleteVertexShader(vs_handle);
    device_->DeletePixelShader(ps_handle);
}

TEST_F(ShaderProgramLinkingTest, LinkMultiplePrograms) {
    // Create multiple shader pairs
    struct ShaderPair {
        DWORD vs_handle;
        DWORD ps_handle;
        GLuint expected_program;
    };
    
    std::vector<ShaderPair> shader_pairs;
    
    // Create 3 different shader combinations
    for (int i = 0; i < 3; ++i) {
        // Vertex shader with different constant
        std::vector<DWORD> vs_bytecode = {
            0xFFFE0101,  // vs_1_1
            0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
            0x00000051, 0xA00F0000 + i,  // def c[i]
            0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,  // 1, 1, 1, 1
            0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
            0x0000FFFF  // end
        };
        
        // Pixel shader with different color
        std::vector<DWORD> ps_bytecode = {
            0xFFFF0101,  // ps_1_1
            0x00000051, 0xA00F0000,  // def c0
            static_cast<DWORD>(0x3F800000),  // R
            static_cast<DWORD>(i == 1 ? 0x3F800000 : 0),  // G
            static_cast<DWORD>(i == 2 ? 0x3F800000 : 0),  // B
            0x3F800000,  // A
            0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
            0x0000FFFF  // end
        };
        
        ShaderPair pair;
        pair.vs_handle = CreateVertexShader(vs_bytecode);
        pair.ps_handle = CreatePixelShader(ps_bytecode);
        
        // Set and get program
        device_->SetVertexShader(pair.vs_handle);
        device_->SetPixelShader(pair.ps_handle);
        pair.expected_program = program_manager_->get_current_program();
        
        shader_pairs.push_back(pair);
    }
    
    // Verify each program is unique
    EXPECT_NE(shader_pairs[0].expected_program, shader_pairs[1].expected_program);
    EXPECT_NE(shader_pairs[1].expected_program, shader_pairs[2].expected_program);
    EXPECT_NE(shader_pairs[0].expected_program, shader_pairs[2].expected_program);
    
    // Switch between programs and verify caching
    for (int iteration = 0; iteration < 2; ++iteration) {
        for (const auto& pair : shader_pairs) {
            device_->SetVertexShader(pair.vs_handle);
            device_->SetPixelShader(pair.ps_handle);
            
            GLuint program = program_manager_->get_current_program();
            EXPECT_EQ(program, pair.expected_program);
            
            // Verify program is still valid
            GLint link_status = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &link_status);
            EXPECT_EQ(link_status, GL_TRUE);
        }
    }
    
    // Clean up
    for (const auto& pair : shader_pairs) {
        device_->DeleteVertexShader(pair.vs_handle);
        device_->DeletePixelShader(pair.ps_handle);
    }
}

TEST_F(ShaderProgramLinkingTest, VertexOnlyProgram) {
    // Create vertex shader only
    std::vector<DWORD> vs_bytecode = {
        0xFFFE0101,  // vs_1_1
        0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
        0x0000001F, 0x80000005, 0x900F0001,  // dcl_diffuse v1
        0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
        0x00000001, 0xD00F0000, 0x90E40001,  // mov oD0, v1
        0x0000FFFF  // end
    };
    
    DWORD vs_handle = CreateVertexShader(vs_bytecode);
    ASSERT_NE(vs_handle, 0u);
    
    // Set vertex shader only (no pixel shader)
    device_->SetVertexShader(vs_handle);
    device_->SetPixelShader(0);
    
    // Should create program with default pixel shader
    GLuint program = program_manager_->get_current_program();
    EXPECT_NE(program, 0u);
    
    // Apply and verify
    program_manager_->apply_shader_state();
    
    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    EXPECT_EQ(link_status, GL_TRUE);
    
    // Clean up
    device_->DeleteVertexShader(vs_handle);
}

TEST_F(ShaderProgramLinkingTest, ProgramInvalidation) {
    // Create initial shaders
    std::vector<DWORD> vs_bytecode = {
        0xFFFE0101,  // vs_1_1
        0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
        0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
        0x0000FFFF  // end
    };
    
    std::vector<DWORD> ps_bytecode = {
        0xFFFF0101,  // ps_1_1
        0x00000051, 0xA00F0000, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,  // def c0, 1, 1, 1, 1
        0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
        0x0000FFFF  // end
    };
    
    DWORD vs_handle = CreateVertexShader(vs_bytecode);
    DWORD ps_handle = CreatePixelShader(ps_bytecode);
    
    // Set and get initial program
    device_->SetVertexShader(vs_handle);
    device_->SetPixelShader(ps_handle);
    GLuint program1 = program_manager_->get_current_program();
    
    // Invalidate current program
    program_manager_->invalidate_current_program();
    
    // Getting program again should return same one (from cache)
    GLuint program2 = program_manager_->get_current_program();
    EXPECT_EQ(program1, program2);
    
    // Create new pixel shader
    std::vector<DWORD> ps_bytecode2 = {
        0xFFFF0101,  // ps_1_1
        0x00000051, 0xA00F0000, 0, 0, 0, 0x3F800000,  // def c0, 0, 0, 0, 1
        0x00000001, 0x800F0000, 0xA0E40000,  // mov r0, c0
        0x0000FFFF  // end
    };
    
    DWORD ps_handle2 = CreatePixelShader(ps_bytecode2);
    
    // Set new pixel shader
    device_->SetPixelShader(ps_handle2);
    
    // Should get different program
    GLuint program3 = program_manager_->get_current_program();
    EXPECT_NE(program3, program1);
    
    // Clean up
    device_->DeleteVertexShader(vs_handle);
    device_->DeletePixelShader(ps_handle);
    device_->DeletePixelShader(ps_handle2);
}

TEST_F(ShaderProgramLinkingTest, DrawWithLinkedProgram) {
    // Create shaders with texture sampling
    std::vector<DWORD> vs_bytecode = {
        0xFFFE0101,  // vs_1_1
        0x0000001F, 0x80000000, 0x900F0000,  // dcl_position v0
        0x0000001F, 0x80000005, 0x900F0001,  // dcl_texcoord0 v1
        0x00000001, 0xC00F0000, 0x90E40000,  // mov oPos, v0
        0x00000001, 0xE00F0000, 0x90E40001,  // mov oT0, v1
        0x0000FFFF  // end
    };
    
    std::vector<DWORD> ps_bytecode = {
        0xFFFF0101,  // ps_1_1
        0x00000042, 0xB00F0000,  // tex t0
        0x00000001, 0x800F0000, 0xB0E40000,  // mov r0, t0
        0x0000FFFF  // end
    };
    
    DWORD vs_handle = CreateVertexShader(vs_bytecode);
    DWORD ps_handle = CreatePixelShader(ps_bytecode);
    
    // Set shaders
    device_->SetVertexShader(vs_handle);
    device_->SetPixelShader(ps_handle);
    
    // Get and apply program
    GLuint program = program_manager_->get_current_program();
    program_manager_->apply_shader_state();
    
    // Create a simple triangle vertex buffer
    struct Vertex {
        float x, y, z;
        float u, v;
    };
    
    Vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
        {  0.0f,  1.0f, 0.0f, 0.5f, 0.0f }
    };
    
    IDirect3DVertexBuffer8* vb = nullptr;
    HRESULT hr = device_->CreateVertexBuffer(
        sizeof(vertices),
        D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ | D3DFVF_TEX1,
        D3DPOOL_MANAGED,
        &vb
    );
    ASSERT_EQ(hr, D3D_OK);
    
    // Fill vertex buffer
    void* data = nullptr;
    hr = vb->Lock(0, sizeof(vertices), (BYTE**)&data, 0);
    ASSERT_EQ(hr, D3D_OK);
    memcpy(data, vertices, sizeof(vertices));
    vb->Unlock();
    
    // Set vertex buffer and FVF
    device_->SetStreamSource(0, vb, sizeof(Vertex));
    device_->SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX1);
    
    // Draw the triangle (this should use the linked program)
    hr = device_->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    EXPECT_EQ(hr, D3D_OK);
    
    // Clean up
    vb->Release();
    device_->DeleteVertexShader(vs_handle);
    device_->DeletePixelShader(ps_handle);
}