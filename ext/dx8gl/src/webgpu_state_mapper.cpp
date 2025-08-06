#include "webgpu_state_mapper.h"
#include "logger.h"
#include <cstring>
#include <functional>

#ifdef DX8GL_HAS_WEBGPU

namespace dx8gl {

WebGPUStateMapper::WebGPUStateMapper()
    : texture_bind_group_layout_(nullptr)
    , uniform_bind_group_layout_(nullptr)
    , default_pipeline_layout_(nullptr)
    , lighting_enabled_(false) {
    
    // Initialize samplers to nullptr
    for (uint32_t i = 0; i < 8; ++i) {
        samplers_[i] = nullptr;
    }
    
    // Initialize transform matrices to identity
    for (uint32_t i = 0; i < 16; ++i) {
        std::memset(transform_matrices_[i], 0, sizeof(float) * 16);
        transform_matrices_[i][0] = 1.0f;
        transform_matrices_[i][5] = 1.0f;
        transform_matrices_[i][10] = 1.0f;
        transform_matrices_[i][15] = 1.0f;
    }
    
    // Initialize material to defaults
    std::memset(&material_, 0, sizeof(material_));
    material_.Diffuse.r = material_.Diffuse.g = material_.Diffuse.b = material_.Diffuse.a = 1.0f;
    material_.Ambient.r = material_.Ambient.g = material_.Ambient.b = material_.Ambient.a = 1.0f;
    material_.Power = 1.0f;
    
    // Initialize lights
    for (uint32_t i = 0; i < 8; ++i) {
        std::memset(&lights_[i], 0, sizeof(D3DLIGHT8));
        lights_[i].Type = D3DLIGHT_DIRECTIONAL;
        lights_[i].Diffuse.r = lights_[i].Diffuse.g = lights_[i].Diffuse.b = lights_[i].Diffuse.a = 1.0f;
        lights_[i].Direction.z = -1.0f; // Default pointing down -Z
    }
}

WebGPUStateMapper::~WebGPUStateMapper() {
    cleanup_cached_pipelines();
    
    // Clean up samplers
    for (uint32_t i = 0; i < 8; ++i) {
        if (samplers_[i]) {
            wgpu_object_destroy(samplers_[i]);
            samplers_[i] = nullptr;
        }
    }
    
    if (default_pipeline_layout_) {
        wgpu_object_destroy(default_pipeline_layout_);
    }
    if (uniform_bind_group_layout_) {
        wgpu_object_destroy(uniform_bind_group_layout_);
    }
    if (texture_bind_group_layout_) {
        wgpu_object_destroy(texture_bind_group_layout_);
    }
}

// D3D to WebGPU blend factor conversion
WGpuBlendFactor WebGPUStateMapper::d3d_to_wgpu_blend_factor(D3DBLEND blend) {
    switch (blend) {
        case D3DBLEND_ZERO:           return WGPU_BLEND_FACTOR_ZERO;
        case D3DBLEND_ONE:            return WGPU_BLEND_FACTOR_ONE;
        case D3DBLEND_SRCCOLOR:       return WGPU_BLEND_FACTOR_SRC;
        case D3DBLEND_INVSRCCOLOR:    return WGPU_BLEND_FACTOR_ONE_MINUS_SRC;
        case D3DBLEND_SRCALPHA:       return WGPU_BLEND_FACTOR_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA:    return WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA:      return WGPU_BLEND_FACTOR_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA:   return WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR:      return WGPU_BLEND_FACTOR_DST;
        case D3DBLEND_INVDESTCOLOR:   return WGPU_BLEND_FACTOR_ONE_MINUS_DST;
        case D3DBLEND_SRCALPHASAT:    return WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED;
        default:
            DX8GL_WARNING("Unknown D3DBLEND value: %d, defaulting to ONE", blend);
            return WGPU_BLEND_FACTOR_ONE;
    }
}

// D3D to WebGPU blend operation conversion
WGpuBlendOperation WebGPUStateMapper::d3d_to_wgpu_blend_op(D3DBLENDOP op) {
    switch (op) {
        case D3DBLENDOP_ADD:          return WGPU_BLEND_OPERATION_ADD;
        case D3DBLENDOP_SUBTRACT:     return WGPU_BLEND_OPERATION_SUBTRACT;
        case D3DBLENDOP_REVSUBTRACT:  return WGPU_BLEND_OPERATION_REVERSE_SUBTRACT;
        case D3DBLENDOP_MIN:          return WGPU_BLEND_OPERATION_MIN;
        case D3DBLENDOP_MAX:          return WGPU_BLEND_OPERATION_MAX;
        default:
            DX8GL_WARNING("Unknown D3DBLENDOP value: %d, defaulting to ADD", op);
            return WGPU_BLEND_OPERATION_ADD;
    }
}

// D3D to WebGPU comparison function conversion
WGpuCompareFunction WebGPUStateMapper::d3d_to_wgpu_compare_func(D3DCMPFUNC func) {
    switch (func) {
        case D3DCMP_NEVER:        return WGPU_COMPARE_FUNCTION_NEVER;
        case D3DCMP_LESS:         return WGPU_COMPARE_FUNCTION_LESS;
        case D3DCMP_EQUAL:        return WGPU_COMPARE_FUNCTION_EQUAL;
        case D3DCMP_LESSEQUAL:    return WGPU_COMPARE_FUNCTION_LESS_EQUAL;
        case D3DCMP_GREATER:      return WGPU_COMPARE_FUNCTION_GREATER;
        case D3DCMP_NOTEQUAL:     return WGPU_COMPARE_FUNCTION_NOT_EQUAL;
        case D3DCMP_GREATEREQUAL: return WGPU_COMPARE_FUNCTION_GREATER_EQUAL;
        case D3DCMP_ALWAYS:       return WGPU_COMPARE_FUNCTION_ALWAYS;
        default:
            DX8GL_WARNING("Unknown D3DCMPFUNC value: %d, defaulting to ALWAYS", func);
            return WGPU_COMPARE_FUNCTION_ALWAYS;
    }
}

// D3D to WebGPU stencil operation conversion
WGpuStencilOperation WebGPUStateMapper::d3d_to_wgpu_stencil_op(DWORD op) {
    switch (op) {
        case D3DSTENCILOP_KEEP:     return WGPU_STENCIL_OPERATION_KEEP;
        case D3DSTENCILOP_ZERO:     return WGPU_STENCIL_OPERATION_ZERO;
        case D3DSTENCILOP_REPLACE:  return WGPU_STENCIL_OPERATION_REPLACE;
        case D3DSTENCILOP_INCRSAT:  return WGPU_STENCIL_OPERATION_INCREMENT_CLAMP;
        case D3DSTENCILOP_DECRSAT:  return WGPU_STENCIL_OPERATION_DECREMENT_CLAMP;
        case D3DSTENCILOP_INVERT:   return WGPU_STENCIL_OPERATION_INVERT;
        case D3DSTENCILOP_INCR:     return WGPU_STENCIL_OPERATION_INCREMENT_WRAP;
        case D3DSTENCILOP_DECR:     return WGPU_STENCIL_OPERATION_DECREMENT_WRAP;
        default:
            DX8GL_WARNING("Unknown D3DSTENCILOP value: %d, defaulting to KEEP", op);
            return WGPU_STENCIL_OPERATION_KEEP;
    }
}

// D3D to WebGPU cull mode conversion
WGpuCullMode WebGPUStateMapper::d3d_to_wgpu_cull_mode(D3DCULL cull) {
    switch (cull) {
        case D3DCULL_NONE: return WGPU_CULL_MODE_NONE;
        case D3DCULL_CW:   return WGPU_CULL_MODE_BACK;  // D3D CW = back face
        case D3DCULL_CCW:  return WGPU_CULL_MODE_FRONT; // D3D CCW = front face
        default:
            DX8GL_WARNING("Unknown D3DCULL value: %d, defaulting to NONE", cull);
            return WGPU_CULL_MODE_NONE;
    }
}

// D3D to WebGPU texture address mode conversion
WGpuAddressMode WebGPUStateMapper::d3d_to_wgpu_address_mode(DWORD mode) {
    switch (mode) {
        case D3DTADDRESS_WRAP:       return WGPU_ADDRESS_MODE_REPEAT;
        case D3DTADDRESS_MIRROR:     return WGPU_ADDRESS_MODE_MIRROR_REPEAT;
        case D3DTADDRESS_CLAMP:      return WGPU_ADDRESS_MODE_CLAMP_TO_EDGE;
        case D3DTADDRESS_BORDER:     return WGPU_ADDRESS_MODE_CLAMP_TO_EDGE; // WebGPU doesn't have border
        case D3DTADDRESS_MIRRORONCE: return WGPU_ADDRESS_MODE_MIRROR_REPEAT;
        default:
            DX8GL_WARNING("Unknown D3DTADDRESS mode: %d, defaulting to REPEAT", mode);
            return WGPU_ADDRESS_MODE_REPEAT;
    }
}

// D3D to WebGPU filter mode conversion
WGpuFilterMode WebGPUStateMapper::d3d_to_wgpu_filter_mode(DWORD filter) {
    switch (filter) {
        case D3DTEXF_NONE:
        case D3DTEXF_POINT:     return WGPU_FILTER_MODE_NEAREST;
        case D3DTEXF_LINEAR:    return WGPU_FILTER_MODE_LINEAR;
        case D3DTEXF_ANISOTROPIC: return WGPU_FILTER_MODE_LINEAR; // WebGPU handles anisotropy separately
        default:
            DX8GL_WARNING("Unknown D3DTEXF filter: %d, defaulting to LINEAR", filter);
            return WGPU_FILTER_MODE_LINEAR;
    }
}

// D3D to WebGPU mipmap filter conversion
WGpuMipmapFilterMode WebGPUStateMapper::d3d_to_wgpu_mipmap_filter(DWORD filter) {
    switch (filter) {
        case D3DTEXF_NONE:
        case D3DTEXF_POINT:     return WGPU_MIPMAP_FILTER_MODE_NEAREST;
        case D3DTEXF_LINEAR:    return WGPU_MIPMAP_FILTER_MODE_LINEAR;
        default:
            return WGPU_MIPMAP_FILTER_MODE_LINEAR;
    }
}

// Create color target state from render state
WGpuColorTargetState WebGPUStateMapper::create_color_target_state(const RenderState& render_state) {
    WGpuColorTargetState color_target = {};
    color_target.format = WGPU_TEXTURE_FORMAT_BGRA8_UNORM; // Default format
    
    // Set up blending
    if (render_state.alpha_blend_enable) {
        WGpuBlendState* blend = new WGpuBlendState();
        
        // Color blending
        blend->color.src_factor = d3d_to_wgpu_blend_factor(render_state.src_blend);
        blend->color.dst_factor = d3d_to_wgpu_blend_factor(render_state.dest_blend);
        blend->color.operation = d3d_to_wgpu_blend_op(render_state.blend_op);
        
        // Alpha blending (D3D8 doesn't have separate alpha blend states, so we use the same)
        blend->alpha.src_factor = d3d_to_wgpu_blend_factor(render_state.src_blend);
        blend->alpha.dst_factor = d3d_to_wgpu_blend_factor(render_state.dest_blend);
        blend->alpha.operation = d3d_to_wgpu_blend_op(render_state.blend_op);
        
        color_target.blend = blend;
    } else {
        color_target.blend = nullptr;
    }
    
    // Write mask (D3D8 doesn't have per-channel write masks, so we enable all)
    color_target.write_mask = WGPU_COLOR_WRITE_MASK_ALL;
    
    return color_target;
}

// Create depth stencil state from render state
WGpuDepthStencilState* WebGPUStateMapper::create_depth_stencil_state(const RenderState& render_state) {
    if (!render_state.z_enable && !render_state.stencil_enable) {
        return nullptr;
    }
    
    WGpuDepthStencilState* depth_stencil = new WGpuDepthStencilState();
    
    // Depth state
    depth_stencil->format = WGPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    depth_stencil->depth_write_enabled = render_state.z_write_enable;
    depth_stencil->depth_compare = render_state.z_enable ? 
        d3d_to_wgpu_compare_func(render_state.z_func) : 
        WGPU_COMPARE_FUNCTION_ALWAYS;
    
    // Stencil state
    if (render_state.stencil_enable) {
        WGpuStencilFaceState stencil_front = {};
        stencil_front.compare = d3d_to_wgpu_compare_func(render_state.stencil_func);
        stencil_front.fail_op = d3d_to_wgpu_stencil_op(render_state.stencil_fail);
        stencil_front.depth_fail_op = d3d_to_wgpu_stencil_op(render_state.stencil_zfail);
        stencil_front.pass_op = d3d_to_wgpu_stencil_op(render_state.stencil_pass);
        
        // D3D8 doesn't have separate front/back stencil, so we use the same for both
        depth_stencil->stencil_front = stencil_front;
        depth_stencil->stencil_back = stencil_front;
        
        depth_stencil->stencil_read_mask = render_state.stencil_mask;
        depth_stencil->stencil_write_mask = render_state.stencil_write_mask;
    } else {
        // Disable stencil
        WGpuStencilFaceState stencil_disabled = {};
        stencil_disabled.compare = WGPU_COMPARE_FUNCTION_ALWAYS;
        stencil_disabled.fail_op = WGPU_STENCIL_OPERATION_KEEP;
        stencil_disabled.depth_fail_op = WGPU_STENCIL_OPERATION_KEEP;
        stencil_disabled.pass_op = WGPU_STENCIL_OPERATION_KEEP;
        
        depth_stencil->stencil_front = stencil_disabled;
        depth_stencil->stencil_back = stencil_disabled;
        depth_stencil->stencil_read_mask = 0;
        depth_stencil->stencil_write_mask = 0;
    }
    
    // Depth bias (polygon offset)
    depth_stencil->depth_bias = static_cast<int32_t>(render_state.z_bias);
    depth_stencil->depth_bias_slope_scale = 0.0f; // D3D8 doesn't have slope scale
    depth_stencil->depth_bias_clamp = 0.0f; // D3D8 doesn't have bias clamp
    
    return depth_stencil;
}

// Create primitive state from render state
WGpuPrimitiveState WebGPUStateMapper::create_primitive_state(const RenderState& render_state) {
    WGpuPrimitiveState primitive = {};
    
    // Topology is set per draw call, not in the pipeline
    primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    // Strip index format (not used in D3D8)
    primitive.strip_index_format = WGPU_INDEX_FORMAT_UNDEFINED;
    
    // Front face and culling
    primitive.front_face = WGPU_FRONT_FACE_CCW; // D3D uses CCW as front face
    primitive.cull_mode = d3d_to_wgpu_cull_mode(render_state.cull_mode);
    
    // Polygon mode (D3D fill mode)
    switch (render_state.fill_mode) {
        case D3DFILL_POINT:
            primitive.polygon_mode = WGPU_POLYGON_MODE_POINT;
            break;
        case D3DFILL_WIREFRAME:
            primitive.polygon_mode = WGPU_POLYGON_MODE_LINE;
            break;
        case D3DFILL_SOLID:
        default:
            primitive.polygon_mode = WGPU_POLYGON_MODE_FILL;
            break;
    }
    
    // Unclipped depth (not in D3D8)
    primitive.unclipped_depth = false;
    
    return primitive;
}

// Create multisample state from render state
WGpuMultisampleState WebGPUStateMapper::create_multisample_state(const RenderState& render_state) {
    WGpuMultisampleState multisample = {};
    
    // D3D8 has limited multisample support
    multisample.count = render_state.multisample_antialias ? 4 : 1;
    multisample.mask = 0xFFFFFFFF;
    multisample.alpha_to_coverage_enabled = false; // Not in D3D8
    
    return multisample;
}

// Create sampler descriptor from texture stage states
WGpuSamplerDescriptor* WebGPUStateMapper::create_sampler_descriptor(
    const RenderState& render_state,
    uint32_t stage) {
    
    if (stage >= 8) {
        DX8GL_ERROR("Invalid texture stage: %u", stage);
        return nullptr;
    }
    
    WGpuSamplerDescriptor* sampler = new WGpuSamplerDescriptor();
    memset(sampler, 0, sizeof(WGpuSamplerDescriptor));
    
    // Address modes
    sampler->address_mode_u = d3d_to_wgpu_address_mode(render_state.address_u[stage]);
    sampler->address_mode_v = d3d_to_wgpu_address_mode(render_state.address_v[stage]);
    sampler->address_mode_w = d3d_to_wgpu_address_mode(render_state.address_w[stage]);
    
    // Filter modes
    sampler->mag_filter = d3d_to_wgpu_filter_mode(render_state.mag_filter[stage]);
    sampler->min_filter = d3d_to_wgpu_filter_mode(render_state.min_filter[stage]);
    sampler->mipmap_filter = d3d_to_wgpu_mipmap_filter(render_state.mip_filter[stage]);
    
    // LOD settings
    sampler->lod_min_clamp = 0.0f;
    sampler->lod_max_clamp = static_cast<float>(render_state.max_mip_level[stage]);
    
    // Anisotropy
    uint32_t max_anisotropy = render_state.max_anisotropy[stage];
    if (max_anisotropy > 1) {
        sampler->max_anisotropy = static_cast<uint16_t>(max_anisotropy);
    } else {
        sampler->max_anisotropy = 1;
    }
    
    // Comparison (not used in D3D8 texture sampling)
    sampler->compare = WGPU_COMPARE_FUNCTION_UNDEFINED;
    
    // Border color (WebGPU doesn't support custom border colors)
    // We could approximate based on render_state.border_color[stage]
    
    return sampler;
}

// Create pipeline descriptor from D3D states
WGpuRenderPipelineDescriptor* WebGPUStateMapper::create_pipeline_descriptor(
    const RenderState& render_state,
    const TransformState& transform_state,
    WGpuShaderModule vertex_shader,
    WGpuShaderModule fragment_shader) {
    
    WGpuRenderPipelineDescriptor* desc = new WGpuRenderPipelineDescriptor();
    memset(desc, 0, sizeof(WGpuRenderPipelineDescriptor));
    
    // Vertex stage
    desc->vertex.module = vertex_shader;
    desc->vertex.entry_point = "main";
    // Vertex buffers layout would be set based on FVF or vertex declaration
    desc->vertex.buffer_count = 0; // To be filled based on vertex format
    desc->vertex.buffers = nullptr; // To be filled based on vertex format
    
    // Fragment stage
    WGpuFragmentState* fragment = new WGpuFragmentState();
    fragment->module = fragment_shader;
    fragment->entry_point = "main";
    
    // Color targets
    fragment->target_count = 1;
    WGpuColorTargetState* targets = new WGpuColorTargetState[1];
    targets[0] = create_color_target_state(render_state);
    fragment->targets = targets;
    
    desc->fragment = fragment;
    
    // Primitive state
    desc->primitive = create_primitive_state(render_state);
    
    // Depth stencil state
    desc->depth_stencil = create_depth_stencil_state(render_state);
    
    // Multisample state
    desc->multisample = create_multisample_state(render_state);
    
    // Pipeline layout (would be created separately)
    desc->layout = default_pipeline_layout_;
    
    return desc;
}

// Pipeline state key comparison
bool WebGPUStateMapper::PipelineStateKey::operator==(const PipelineStateKey& other) const {
    return blend_enabled == other.blend_enabled &&
           src_blend == other.src_blend &&
           dst_blend == other.dst_blend &&
           blend_op == other.blend_op &&
           src_alpha_blend == other.src_alpha_blend &&
           dst_alpha_blend == other.dst_alpha_blend &&
           alpha_blend_op == other.alpha_blend_op &&
           depth_test_enabled == other.depth_test_enabled &&
           depth_write_enabled == other.depth_write_enabled &&
           depth_compare == other.depth_compare &&
           depth_bias == other.depth_bias &&
           depth_bias_slope_scale == other.depth_bias_slope_scale &&
           depth_bias_clamp == other.depth_bias_clamp &&
           stencil_enabled == other.stencil_enabled &&
           stencil_fail_op == other.stencil_fail_op &&
           stencil_depth_fail_op == other.stencil_depth_fail_op &&
           stencil_pass_op == other.stencil_pass_op &&
           stencil_compare == other.stencil_compare &&
           stencil_read_mask == other.stencil_read_mask &&
           stencil_write_mask == other.stencil_write_mask &&
           stencil_reference == other.stencil_reference &&
           topology == other.topology &&
           cull_mode == other.cull_mode &&
           front_face == other.front_face &&
           scissor_enabled == other.scissor_enabled &&
           vertex_format_hash == other.vertex_format_hash;
}

// Pipeline state key hash
size_t WebGPUStateMapper::PipelineStateKeyHash::operator()(const PipelineStateKey& key) const {
    size_t hash = 0;
    
    // Simple hash combination using XOR and bit shifting
    auto hash_combine = [&hash](size_t value) {
        hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    };
    
    hash_combine(std::hash<bool>()(key.blend_enabled));
    hash_combine(static_cast<size_t>(key.src_blend));
    hash_combine(static_cast<size_t>(key.dst_blend));
    hash_combine(static_cast<size_t>(key.blend_op));
    hash_combine(static_cast<size_t>(key.depth_compare));
    hash_combine(std::hash<bool>()(key.depth_test_enabled));
    hash_combine(std::hash<bool>()(key.depth_write_enabled));
    hash_combine(std::hash<float>()(key.depth_bias));
    hash_combine(std::hash<bool>()(key.stencil_enabled));
    hash_combine(static_cast<size_t>(key.stencil_fail_op));
    hash_combine(static_cast<size_t>(key.stencil_compare));
    hash_combine(static_cast<size_t>(key.cull_mode));
    hash_combine(static_cast<size_t>(key.topology));
    hash_combine(key.vertex_format_hash);
    
    return hash;
}

// Get or create pipeline from cache
WGpuRenderPipeline WebGPUStateMapper::get_or_create_pipeline(
    WGpuDevice device,
    const PipelineStateKey& key,
    WGpuShaderModule vertex_shader,
    WGpuShaderModule fragment_shader) {
    
    // Check cache first
    auto it = pipeline_cache_.find(key);
    if (it != pipeline_cache_.end()) {
        return it->second;
    }
    
    // Create new pipeline
    // This would use create_pipeline_descriptor internally
    DX8GL_INFO("Creating new WebGPU pipeline for state key");
    
    // For now, return null (would be implemented with actual WebGPU calls)
    return nullptr;
}

// Create bind group for textures
WGpuBindGroup WebGPUStateMapper::create_texture_bind_group(
    WGpuDevice device,
    const WGpuTextureView* textures,
    const WGpuSampler* samplers,
    uint32_t count) {
    
    if (!texture_bind_group_layout_) {
        DX8GL_ERROR("Texture bind group layout not initialized");
        return nullptr;
    }
    
    // Create bind group descriptor
    WGpuBindGroupDescriptor desc = {};
    desc.layout = texture_bind_group_layout_;
    desc.entry_count = count * 2; // Each texture has a sampler
    
    WGpuBindGroupEntry* entries = new WGpuBindGroupEntry[desc.entry_count];
    
    for (uint32_t i = 0; i < count; ++i) {
        // Texture binding
        entries[i * 2].binding = i * 2;
        entries[i * 2].texture_view = textures[i];
        
        // Sampler binding
        entries[i * 2 + 1].binding = i * 2 + 1;
        entries[i * 2 + 1].sampler = samplers[i];
    }
    
    desc.entries = entries;
    
    WGpuBindGroup bind_group = wgpu_device_create_bind_group(device, &desc);
    
    delete[] entries;
    
    return bind_group;
}

// Create bind group for uniforms
WGpuBindGroup WebGPUStateMapper::create_uniform_bind_group(
    WGpuDevice device,
    WGpuBuffer uniform_buffer,
    size_t uniform_size) {
    
    if (!uniform_bind_group_layout_) {
        DX8GL_ERROR("Uniform bind group layout not initialized");
        return nullptr;
    }
    
    WGpuBindGroupDescriptor desc = {};
    desc.layout = uniform_bind_group_layout_;
    desc.entry_count = 1;
    
    WGpuBindGroupEntry entry = {};
    entry.binding = 0;
    entry.buffer = uniform_buffer;
    entry.offset = 0;
    entry.size = uniform_size;
    
    desc.entries = &entry;
    
    return wgpu_device_create_bind_group(device, &desc);
}

// Create default bind group layouts
void WebGPUStateMapper::create_default_layouts(WGpuDevice device) {
    // Create texture bind group layout
    {
        WGpuBindGroupLayoutDescriptor desc = {};
        
        // Support up to 8 texture units (D3D8 limit)
        const uint32_t max_textures = 8;
        WGpuBindGroupLayoutEntry* entries = new WGpuBindGroupLayoutEntry[max_textures * 2];
        
        for (uint32_t i = 0; i < max_textures; ++i) {
            // Texture entry
            entries[i * 2].binding = i * 2;
            entries[i * 2].visibility = WGPU_SHADER_STAGE_FRAGMENT;
            entries[i * 2].texture.sample_type = WGPU_TEXTURE_SAMPLE_TYPE_FLOAT;
            entries[i * 2].texture.view_dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
            entries[i * 2].texture.multisampled = false;
            
            // Sampler entry
            entries[i * 2 + 1].binding = i * 2 + 1;
            entries[i * 2 + 1].visibility = WGPU_SHADER_STAGE_FRAGMENT;
            entries[i * 2 + 1].sampler.type = WGPU_SAMPLER_BINDING_TYPE_FILTERING;
        }
        
        desc.entry_count = max_textures * 2;
        desc.entries = entries;
        
        texture_bind_group_layout_ = wgpu_device_create_bind_group_layout(device, &desc);
        
        delete[] entries;
    }
    
    // Create uniform bind group layout
    {
        WGpuBindGroupLayoutDescriptor desc = {};
        
        WGpuBindGroupLayoutEntry entry = {};
        entry.binding = 0;
        entry.visibility = WGPU_SHADER_STAGE_VERTEX | WGPU_SHADER_STAGE_FRAGMENT;
        entry.buffer.type = WGPU_BUFFER_BINDING_TYPE_UNIFORM;
        entry.buffer.has_dynamic_offset = false;
        entry.buffer.min_binding_size = 0; // No minimum size
        
        desc.entry_count = 1;
        desc.entries = &entry;
        
        uniform_bind_group_layout_ = wgpu_device_create_bind_group_layout(device, &desc);
    }
    
    // Create default pipeline layout
    {
        WGpuPipelineLayoutDescriptor desc = {};
        
        WGpuBindGroupLayout layouts[] = {
            uniform_bind_group_layout_,
            texture_bind_group_layout_
        };
        
        desc.bind_group_layout_count = 2;
        desc.bind_group_layouts = layouts;
        
        default_pipeline_layout_ = wgpu_device_create_pipeline_layout(device, &desc);
    }
}

// Cleanup cached pipelines
void WebGPUStateMapper::cleanup_cached_pipelines() {
    for (auto& pair : pipeline_cache_) {
        if (pair.second) {
            wgpu_object_destroy(pair.second);
        }
    }
    pipeline_cache_.clear();
}

// Get or create pipeline with caching
WGpuRenderPipeline WebGPUStateMapper::get_or_create_pipeline(WGpuDevice device, const PipelineStateKey& key) {
    // Check cache first
    auto it = pipeline_cache_.find(key);
    if (it != pipeline_cache_.end()) {
        return it->second;
    }
    
    // Create new pipeline
    // In a real implementation, you would need the shader modules here
    // For now, return nullptr as a placeholder
    WGpuRenderPipeline pipeline = nullptr;
    
    // TODO: Create actual pipeline using the key and shader modules
    // This would involve calling create_pipeline_descriptor with the key's states
    // and then wgpu_device_create_render_pipeline
    
    // Cache the pipeline
    pipeline_cache_[key] = pipeline;
    
    return pipeline;
}

// Set sampler for a texture stage
void WebGPUStateMapper::set_sampler(uint32_t stage, WGpuSampler sampler) {
    if (stage < 8) {
        // Clean up old sampler if exists
        if (samplers_[stage]) {
            wgpu_object_destroy(samplers_[stage]);
        }
        samplers_[stage] = sampler;
    }
}

// Set transform matrix
void WebGPUStateMapper::set_transform_matrix(TransformType type, const float* matrix) {
    if (type < 16 && matrix) {
        std::memcpy(transform_matrices_[type], matrix, sizeof(float) * 16);
    }
}

// Set lighting enabled state
void WebGPUStateMapper::set_lighting_enabled(bool enabled) {
    lighting_enabled_ = enabled;
}

// Set material properties
void WebGPUStateMapper::set_material(const D3DMATERIAL8& material) {
    material_ = material;
}

// Set light properties
void WebGPUStateMapper::set_light(uint32_t index, const D3DLIGHT8& light) {
    if (index < 8) {
        lights_[index] = light;
    }
}

} // namespace dx8gl

#endif // DX8GL_HAS_WEBGPU