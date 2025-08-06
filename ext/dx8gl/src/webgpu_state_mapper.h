#ifndef DX8GL_WEBGPU_STATE_MAPPER_H
#define DX8GL_WEBGPU_STATE_MAPPER_H

#include "d3d8.h"
#include "state_manager.h"

#ifdef DX8GL_HAS_WEBGPU
#include "../lib/lib_webgpu/lib_webgpu.h"
#include <memory>
#include <unordered_map>

namespace dx8gl {

/**
 * Maps DirectX 8 render states to WebGPU pipeline descriptors
 * This class handles the conversion of D3DRS_* states and texture stage states
 * to their WebGPU equivalents.
 */
class WebGPUStateMapper {
public:
    WebGPUStateMapper();
    ~WebGPUStateMapper();
    
    // Pipeline state management
    struct PipelineStateKey {
        // Blend state
        bool blend_enabled;
        WGpuBlendFactor src_blend;
        WGpuBlendFactor dst_blend;
        WGpuBlendOperation blend_op;
        WGpuBlendFactor src_alpha_blend;
        WGpuBlendFactor dst_alpha_blend;
        WGpuBlendOperation alpha_blend_op;
        
        // Depth/stencil state
        bool depth_test_enabled;
        bool depth_write_enabled;
        WGpuCompareFunction depth_compare;
        float depth_bias;
        float depth_bias_slope_scale;
        float depth_bias_clamp;
        
        bool stencil_enabled;
        WGpuStencilOperation stencil_fail_op;
        WGpuStencilOperation stencil_depth_fail_op;
        WGpuStencilOperation stencil_pass_op;
        WGpuCompareFunction stencil_compare;
        uint32_t stencil_read_mask;
        uint32_t stencil_write_mask;
        uint32_t stencil_reference;
        
        // Rasterizer state
        WGpuPrimitiveTopology topology;
        WGpuCullMode cull_mode;
        WGpuFrontFace front_face;
        bool scissor_enabled;
        
        // Vertex format (simplified for now)
        uint32_t vertex_format_hash;
        
        bool operator==(const PipelineStateKey& other) const;
    };
    
    struct PipelineStateKeyHash {
        size_t operator()(const PipelineStateKey& key) const;
    };
    
    // Convert D3D states to WebGPU pipeline descriptor
    WGpuRenderPipelineDescriptor* create_pipeline_descriptor(
        const RenderState& render_state,
        const TransformState& transform_state,
        WGpuShaderModule vertex_shader,
        WGpuShaderModule fragment_shader
    );
    
    // Create sampler from texture stage states
    WGpuSamplerDescriptor* create_sampler_descriptor(
        const RenderState& render_state,
        uint32_t stage
    );
    
    // Convert individual state mappings
    static WGpuBlendFactor d3d_to_wgpu_blend_factor(D3DBLEND blend);
    static WGpuBlendOperation d3d_to_wgpu_blend_op(D3DBLENDOP op);
    static WGpuCompareFunction d3d_to_wgpu_compare_func(D3DCMPFUNC func);
    static WGpuStencilOperation d3d_to_wgpu_stencil_op(DWORD op);
    static WGpuCullMode d3d_to_wgpu_cull_mode(D3DCULL cull);
    static WGpuAddressMode d3d_to_wgpu_address_mode(DWORD mode);
    static WGpuFilterMode d3d_to_wgpu_filter_mode(DWORD filter);
    static WGpuMipmapFilterMode d3d_to_wgpu_mipmap_filter(DWORD filter);
    
    // Pipeline caching
    WGpuRenderPipeline get_or_create_pipeline(
        WGpuDevice device,
        const PipelineStateKey& key,
        WGpuShaderModule vertex_shader,
        WGpuShaderModule fragment_shader
    );
    
    // Bind group management for textures and uniforms
    WGpuBindGroup create_texture_bind_group(
        WGpuDevice device,
        const WGpuTextureView* textures,
        const WGpuSampler* samplers,
        uint32_t count
    );
    
    WGpuBindGroup create_uniform_bind_group(
        WGpuDevice device,
        WGpuBuffer uniform_buffer,
        size_t uniform_size
    );
    
private:
    // Pipeline cache
    std::unordered_map<PipelineStateKey, WGpuRenderPipeline, PipelineStateKeyHash> pipeline_cache_;
    
    // Bind group layouts (shared across pipelines)
    WGpuBindGroupLayout texture_bind_group_layout_;
    WGpuBindGroupLayout uniform_bind_group_layout_;
    WGpuPipelineLayout default_pipeline_layout_;
    
    // Helper methods
    void create_default_layouts(WGpuDevice device);
    void cleanup_cached_pipelines();
    
    WGpuColorTargetState create_color_target_state(const RenderState& render_state);
    WGpuDepthStencilState* create_depth_stencil_state(const RenderState& render_state);
    WGpuPrimitiveState create_primitive_state(const RenderState& render_state);
    WGpuMultisampleState create_multisample_state(const RenderState& render_state);
    
    // Vertex attribute layout helpers
    WGpuVertexBufferLayout* create_vertex_buffer_layout(uint32_t fvf);
    void destroy_vertex_buffer_layout(WGpuVertexBufferLayout* layout);
    
    // Sampler cache for texture stages
    WGpuSampler samplers_[8];
    
    // Transform matrices
    float transform_matrices_[16][16]; // Up to 16 transform types
    
    // Lighting and material state
    bool lighting_enabled_;
    D3DMATERIAL8 material_;
    D3DLIGHT8 lights_[8];
    
public:
    // Transform matrix types
    enum TransformType {
        TRANSFORM_WORLD = 0,
        TRANSFORM_VIEW = 1,
        TRANSFORM_PROJECTION = 2,
        TRANSFORM_TEXTURE0 = 8,
        TRANSFORM_TEXTURE1 = 9,
        TRANSFORM_TEXTURE2 = 10,
        TRANSFORM_TEXTURE3 = 11,
        TRANSFORM_TEXTURE4 = 12,
        TRANSFORM_TEXTURE5 = 13,
        TRANSFORM_TEXTURE6 = 14,
        TRANSFORM_TEXTURE7 = 15
    };
    
    // Additional state management methods
    WGpuRenderPipeline get_or_create_pipeline(WGpuDevice device, const PipelineStateKey& key);
    void set_sampler(uint32_t stage, WGpuSampler sampler);
    void set_transform_matrix(TransformType type, const float* matrix);
    void set_lighting_enabled(bool enabled);
    void set_material(const D3DMATERIAL8& material);
    void set_light(uint32_t index, const D3DLIGHT8& light);
};

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU

#endif // DX8GL_WEBGPU_STATE_MAPPER_H