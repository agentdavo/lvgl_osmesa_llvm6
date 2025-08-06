/**
 * Test file for WebGPU state mapping validation
 * 
 * This test validates that D3DRS_* states and texture stage states
 * are correctly converted to WebGPU pipeline descriptors.
 */

#include <gtest/gtest.h>
#include "../src/webgpu_state_mapper.h"
#include "../src/state_manager.h"
#include "../src/d3d8.h"

#ifdef DX8GL_HAS_WEBGPU

using namespace dx8gl;

class WebGPUStateMappingTest : public ::testing::Test {
protected:
    void SetUp() override {
        mapper = std::make_unique<WebGPUStateMapper>();
    }
    
    void TearDown() override {
        mapper.reset();
    }
    
    std::unique_ptr<WebGPUStateMapper> mapper;
};

// Test blend factor conversions
TEST_F(WebGPUStateMappingTest, BlendFactorConversion) {
    // Test all D3D blend factors
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_ZERO), 
              WGPU_BLEND_FACTOR_ZERO);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_ONE), 
              WGPU_BLEND_FACTOR_ONE);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_SRCCOLOR), 
              WGPU_BLEND_FACTOR_SRC);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_INVSRCCOLOR), 
              WGPU_BLEND_FACTOR_ONE_MINUS_SRC);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_SRCALPHA), 
              WGPU_BLEND_FACTOR_SRC_ALPHA);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_INVSRCALPHA), 
              WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_DESTALPHA), 
              WGPU_BLEND_FACTOR_DST_ALPHA);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_INVDESTALPHA), 
              WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_DESTCOLOR), 
              WGPU_BLEND_FACTOR_DST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_INVDESTCOLOR), 
              WGPU_BLEND_FACTOR_ONE_MINUS_DST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND_SRCALPHASAT), 
              WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED);
}

// Test blend operation conversions
TEST_F(WebGPUStateMappingTest, BlendOpConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP_ADD), 
              WGPU_BLEND_OPERATION_ADD);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP_SUBTRACT), 
              WGPU_BLEND_OPERATION_SUBTRACT);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP_REVSUBTRACT), 
              WGPU_BLEND_OPERATION_REVERSE_SUBTRACT);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP_MIN), 
              WGPU_BLEND_OPERATION_MIN);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP_MAX), 
              WGPU_BLEND_OPERATION_MAX);
}

// Test comparison function conversions
TEST_F(WebGPUStateMappingTest, CompareFunctionConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_NEVER), 
              WGPU_COMPARE_FUNCTION_NEVER);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_LESS), 
              WGPU_COMPARE_FUNCTION_LESS);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_EQUAL), 
              WGPU_COMPARE_FUNCTION_EQUAL);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_LESSEQUAL), 
              WGPU_COMPARE_FUNCTION_LESS_EQUAL);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_GREATER), 
              WGPU_COMPARE_FUNCTION_GREATER);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_NOTEQUAL), 
              WGPU_COMPARE_FUNCTION_NOT_EQUAL);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_GREATEREQUAL), 
              WGPU_COMPARE_FUNCTION_GREATER_EQUAL);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMP_ALWAYS), 
              WGPU_COMPARE_FUNCTION_ALWAYS);
}

// Test stencil operation conversions
TEST_F(WebGPUStateMappingTest, StencilOpConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_KEEP), 
              WGPU_STENCIL_OPERATION_KEEP);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_ZERO), 
              WGPU_STENCIL_OPERATION_ZERO);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_REPLACE), 
              WGPU_STENCIL_OPERATION_REPLACE);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_INCRSAT), 
              WGPU_STENCIL_OPERATION_INCREMENT_CLAMP);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_DECRSAT), 
              WGPU_STENCIL_OPERATION_DECREMENT_CLAMP);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_INVERT), 
              WGPU_STENCIL_OPERATION_INVERT);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_INCR), 
              WGPU_STENCIL_OPERATION_INCREMENT_WRAP);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_stencil_op(D3DSTENCILOP_DECR), 
              WGPU_STENCIL_OPERATION_DECREMENT_WRAP);
}

// Test cull mode conversions
TEST_F(WebGPUStateMappingTest, CullModeConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_cull_mode(D3DCULL_NONE), 
              WGPU_CULL_MODE_NONE);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_cull_mode(D3DCULL_CW), 
              WGPU_CULL_MODE_BACK);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_cull_mode(D3DCULL_CCW), 
              WGPU_CULL_MODE_FRONT);
}

// Test texture address mode conversions
TEST_F(WebGPUStateMappingTest, AddressModeConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_address_mode(D3DTADDRESS_WRAP), 
              WGPU_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_address_mode(D3DTADDRESS_MIRROR), 
              WGPU_ADDRESS_MODE_MIRROR_REPEAT);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_address_mode(D3DTADDRESS_CLAMP), 
              WGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_address_mode(D3DTADDRESS_BORDER), 
              WGPU_ADDRESS_MODE_CLAMP_TO_EDGE); // WebGPU doesn't have border
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_address_mode(D3DTADDRESS_MIRRORONCE), 
              WGPU_ADDRESS_MODE_MIRROR_REPEAT);
}

// Test filter mode conversions
TEST_F(WebGPUStateMappingTest, FilterModeConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_filter_mode(D3DTEXF_NONE), 
              WGPU_FILTER_MODE_NEAREST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_filter_mode(D3DTEXF_POINT), 
              WGPU_FILTER_MODE_NEAREST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_filter_mode(D3DTEXF_LINEAR), 
              WGPU_FILTER_MODE_LINEAR);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_filter_mode(D3DTEXF_ANISOTROPIC), 
              WGPU_FILTER_MODE_LINEAR); // Anisotropy handled separately
}

// Test mipmap filter conversions
TEST_F(WebGPUStateMappingTest, MipmapFilterConversion) {
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_mipmap_filter(D3DTEXF_NONE), 
              WGPU_MIPMAP_FILTER_MODE_NEAREST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_mipmap_filter(D3DTEXF_POINT), 
              WGPU_MIPMAP_FILTER_MODE_NEAREST);
    EXPECT_EQ(WebGPUStateMapper::d3d_to_wgpu_mipmap_filter(D3DTEXF_LINEAR), 
              WGPU_MIPMAP_FILTER_MODE_LINEAR);
}

// Test sampler descriptor creation
TEST_F(WebGPUStateMappingTest, SamplerDescriptorCreation) {
    RenderState render_state = {};
    
    // Set up texture stage 0 states
    render_state.address_u[0] = D3DTADDRESS_WRAP;
    render_state.address_v[0] = D3DTADDRESS_CLAMP;
    render_state.address_w[0] = D3DTADDRESS_MIRROR;
    render_state.mag_filter[0] = D3DTEXF_LINEAR;
    render_state.min_filter[0] = D3DTEXF_LINEAR;
    render_state.mip_filter[0] = D3DTEXF_POINT;
    render_state.max_anisotropy[0] = 4;
    render_state.max_mip_level[0] = 10;
    
    auto sampler_desc = mapper->create_sampler_descriptor(render_state, 0);
    ASSERT_NE(sampler_desc, nullptr);
    
    // Verify sampler settings
    EXPECT_EQ(sampler_desc->address_mode_u, WGPU_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(sampler_desc->address_mode_v, WGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(sampler_desc->address_mode_w, WGPU_ADDRESS_MODE_MIRROR_REPEAT);
    EXPECT_EQ(sampler_desc->mag_filter, WGPU_FILTER_MODE_LINEAR);
    EXPECT_EQ(sampler_desc->min_filter, WGPU_FILTER_MODE_LINEAR);
    EXPECT_EQ(sampler_desc->mipmap_filter, WGPU_MIPMAP_FILTER_MODE_NEAREST);
    EXPECT_EQ(sampler_desc->max_anisotropy, 4);
    EXPECT_EQ(sampler_desc->lod_max_clamp, 10.0f);
    
    delete sampler_desc;
}

// Test pipeline state key equality
TEST_F(WebGPUStateMappingTest, PipelineStateKeyEquality) {
    WebGPUStateMapper::PipelineStateKey key1 = {};
    WebGPUStateMapper::PipelineStateKey key2 = {};
    
    // Initially equal
    EXPECT_TRUE(key1 == key2);
    
    // Change blend state
    key1.blend_enabled = true;
    EXPECT_FALSE(key1 == key2);
    key2.blend_enabled = true;
    EXPECT_TRUE(key1 == key2);
    
    // Change depth state
    key1.depth_test_enabled = true;
    key1.depth_compare = WGPU_COMPARE_FUNCTION_LESS;
    EXPECT_FALSE(key1 == key2);
    key2.depth_test_enabled = true;
    key2.depth_compare = WGPU_COMPARE_FUNCTION_LESS;
    EXPECT_TRUE(key1 == key2);
    
    // Change stencil state
    key1.stencil_enabled = true;
    key1.stencil_compare = WGPU_COMPARE_FUNCTION_EQUAL;
    EXPECT_FALSE(key1 == key2);
    key2.stencil_enabled = true;
    key2.stencil_compare = WGPU_COMPARE_FUNCTION_EQUAL;
    EXPECT_TRUE(key1 == key2);
    
    // Change rasterizer state
    key1.cull_mode = WGPU_CULL_MODE_BACK;
    EXPECT_FALSE(key1 == key2);
    key2.cull_mode = WGPU_CULL_MODE_BACK;
    EXPECT_TRUE(key1 == key2);
}

// Test pipeline state key hashing
TEST_F(WebGPUStateMappingTest, PipelineStateKeyHashing) {
    WebGPUStateMapper::PipelineStateKeyHash hasher;
    
    WebGPUStateMapper::PipelineStateKey key1 = {};
    WebGPUStateMapper::PipelineStateKey key2 = {};
    
    // Same keys should have same hash
    EXPECT_EQ(hasher(key1), hasher(key2));
    
    // Different keys should (likely) have different hashes
    key1.blend_enabled = true;
    key1.src_blend = WGPU_BLEND_FACTOR_SRC_ALPHA;
    key1.dst_blend = WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    
    key2.depth_test_enabled = true;
    key2.depth_compare = WGPU_COMPARE_FUNCTION_LESS_EQUAL;
    
    // Hashes should be different (though collisions are possible)
    EXPECT_NE(hasher(key1), hasher(key2));
}

// Test complete render state to pipeline descriptor conversion
TEST_F(WebGPUStateMappingTest, CompletePipelineDescriptorCreation) {
    RenderState render_state = {};
    TransformState transform_state = {};
    
    // Set up a typical render state
    render_state.alpha_blend_enable = TRUE;
    render_state.src_blend = D3DBLEND_SRCALPHA;
    render_state.dest_blend = D3DBLEND_INVSRCALPHA;
    render_state.blend_op = D3DBLENDOP_ADD;
    
    render_state.z_enable = TRUE;
    render_state.z_write_enable = TRUE;
    render_state.z_func = D3DCMP_LESSEQUAL;
    render_state.z_bias = 1;
    
    render_state.stencil_enable = FALSE;
    
    render_state.cull_mode = D3DCULL_CCW;
    render_state.fill_mode = D3DFILL_SOLID;
    
    render_state.multisample_antialias = FALSE;
    
    // Create mock shader modules (would be actual WebGPU shader modules)
    WGpuShaderModule vertex_shader = reinterpret_cast<WGpuShaderModule>(0x1234);
    WGpuShaderModule fragment_shader = reinterpret_cast<WGpuShaderModule>(0x5678);
    
    auto pipeline_desc = mapper->create_pipeline_descriptor(
        render_state, transform_state, vertex_shader, fragment_shader);
    
    ASSERT_NE(pipeline_desc, nullptr);
    
    // Verify vertex stage
    EXPECT_EQ(pipeline_desc->vertex.module, vertex_shader);
    EXPECT_STREQ(pipeline_desc->vertex.entry_point, "main");
    
    // Verify fragment stage
    ASSERT_NE(pipeline_desc->fragment, nullptr);
    EXPECT_EQ(pipeline_desc->fragment->module, fragment_shader);
    EXPECT_STREQ(pipeline_desc->fragment->entry_point, "main");
    EXPECT_EQ(pipeline_desc->fragment->target_count, 1);
    
    // Verify primitive state
    EXPECT_EQ(pipeline_desc->primitive.cull_mode, WGPU_CULL_MODE_FRONT); // D3D CCW = front face culling
    EXPECT_EQ(pipeline_desc->primitive.polygon_mode, WGPU_POLYGON_MODE_FILL);
    EXPECT_EQ(pipeline_desc->primitive.front_face, WGPU_FRONT_FACE_CCW);
    
    // Verify depth stencil state
    ASSERT_NE(pipeline_desc->depth_stencil, nullptr);
    EXPECT_EQ(pipeline_desc->depth_stencil->depth_write_enabled, true);
    EXPECT_EQ(pipeline_desc->depth_stencil->depth_compare, WGPU_COMPARE_FUNCTION_LESS_EQUAL);
    EXPECT_EQ(pipeline_desc->depth_stencil->depth_bias, 1);
    
    // Verify multisample state
    EXPECT_EQ(pipeline_desc->multisample.count, 1); // No multisampling
    
    // Clean up
    delete pipeline_desc->fragment;
    delete pipeline_desc->depth_stencil;
    delete pipeline_desc;
}

// Test texture stage state combinations
TEST_F(WebGPUStateMappingTest, TextureStageStateCombinations) {
    RenderState render_state = {};
    
    // Test all 8 texture stages
    for (uint32_t i = 0; i < 8; ++i) {
        // Set different states for each stage
        render_state.address_u[i] = static_cast<DWORD>(D3DTADDRESS_WRAP + (i % 5));
        render_state.address_v[i] = static_cast<DWORD>(D3DTADDRESS_WRAP + ((i + 1) % 5));
        render_state.address_w[i] = static_cast<DWORD>(D3DTADDRESS_WRAP + ((i + 2) % 5));
        
        render_state.mag_filter[i] = (i % 2) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        render_state.min_filter[i] = (i % 2) ? D3DTEXF_POINT : D3DTEXF_LINEAR;
        render_state.mip_filter[i] = (i % 3) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        
        render_state.max_anisotropy[i] = 1 << i; // Powers of 2
        render_state.max_mip_level[i] = i * 2;
        
        auto sampler_desc = mapper->create_sampler_descriptor(render_state, i);
        ASSERT_NE(sampler_desc, nullptr);
        
        // Verify unique configuration for each stage
        if (i > 0) {
            auto prev_sampler_desc = mapper->create_sampler_descriptor(render_state, i - 1);
            ASSERT_NE(prev_sampler_desc, nullptr);
            
            // At least one setting should be different
            bool different = 
                sampler_desc->address_mode_u != prev_sampler_desc->address_mode_u ||
                sampler_desc->address_mode_v != prev_sampler_desc->address_mode_v ||
                sampler_desc->address_mode_w != prev_sampler_desc->address_mode_w ||
                sampler_desc->mag_filter != prev_sampler_desc->mag_filter ||
                sampler_desc->min_filter != prev_sampler_desc->min_filter ||
                sampler_desc->mipmap_filter != prev_sampler_desc->mipmap_filter ||
                sampler_desc->max_anisotropy != prev_sampler_desc->max_anisotropy ||
                sampler_desc->lod_max_clamp != prev_sampler_desc->lod_max_clamp;
            
            EXPECT_TRUE(different);
            
            delete prev_sampler_desc;
        }
        
        delete sampler_desc;
    }
}

// Test edge cases and boundary conditions
TEST_F(WebGPUStateMappingTest, EdgeCases) {
    // Test invalid texture stage
    RenderState render_state = {};
    auto sampler_desc = mapper->create_sampler_descriptor(render_state, 8); // Out of bounds
    EXPECT_EQ(sampler_desc, nullptr);
    
    // Test with all states at maximum values
    render_state.max_anisotropy[0] = 0xFFFFFFFF;
    render_state.max_mip_level[0] = 0xFFFFFFFF;
    render_state.stencil_mask = 0xFFFFFFFF;
    render_state.stencil_write_mask = 0xFFFFFFFF;
    
    sampler_desc = mapper->create_sampler_descriptor(render_state, 0);
    ASSERT_NE(sampler_desc, nullptr);
    
    // WebGPU has limits on these values
    EXPECT_LE(sampler_desc->max_anisotropy, 16); // Typical WebGPU limit
    EXPECT_LE(sampler_desc->lod_max_clamp, 1000.0f); // Reasonable limit
    
    delete sampler_desc;
}

#endif // DX8GL_HAS_WEBGPU

// Main test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}