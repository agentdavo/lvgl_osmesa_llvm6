#include "cube_texture_support.h"
#include "logger.h"
#include <cstring>
#include <cmath>
#include <sstream>

namespace dx8gl {

// Static member definitions
CubeTextureState::CubeTextureBinding CubeTextureState::cube_textures_[8] = {};
uint32_t CubeTextureState::active_cube_texture_mask_ = 0;
CubeTexGenMode CubeTexCoordGenerator::texgen_modes_[8] = {};

// CubeTextureSupport implementation

GLenum CubeTextureSupport::get_gl_cube_face(D3DCUBEMAP_FACES face) {
    switch (face) {
        case D3DCUBEMAP_FACE_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case D3DCUBEMAP_FACE_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case D3DCUBEMAP_FACE_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case D3DCUBEMAP_FACE_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case D3DCUBEMAP_FACE_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case D3DCUBEMAP_FACE_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }
}

std::string CubeTextureSupport::generate_cube_texcoord_glsl(int texture_unit) {
    std::stringstream ss;
    ss << "// Cube texture coordinate generation for unit " << texture_unit << "\n";
    ss << "vec3 cube_texcoord" << texture_unit << " = normalize(reflect(-view_dir, normal));\n";
    return ss.str();
}

std::string CubeTextureSupport::generate_cube_sampler_wgsl(int texture_unit) {
    std::stringstream ss;
    ss << "@group(1) @binding(" << (texture_unit * 2) << ")\n";
    ss << "var cube_texture" << texture_unit << ": texture_cube<f32>;\n";
    ss << "@group(1) @binding(" << (texture_unit * 2 + 1) << ")\n";
    ss << "var cube_sampler" << texture_unit << ": sampler;\n";
    return ss.str();
}

std::string CubeTextureSupport::generate_cube_sampling_wgsl(const std::string& sampler_name,
                                                           const std::string& coord_expr) {
    std::stringstream ss;
    ss << "textureSample(" << sampler_name << "_texture, " << sampler_name << "_sampler, " 
       << coord_expr << ")";
    return ss.str();
}

CubeTextureSupport::FaceOrientation CubeTextureSupport::get_face_orientation(D3DCUBEMAP_FACES face) {
    FaceOrientation orient = {};
    
    // DirectX cube map face orientations differ from OpenGL
    switch (face) {
        case D3DCUBEMAP_FACE_POSITIVE_X:
            orient.rotation_angle = 0.0f;
            orient.flip_horizontal = false;
            orient.flip_vertical = false;
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_X:
            orient.rotation_angle = 180.0f;
            orient.flip_horizontal = false;
            orient.flip_vertical = false;
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Y:
            orient.rotation_angle = 90.0f;
            orient.flip_horizontal = false;
            orient.flip_vertical = true;  // D3D Y+ is flipped vs GL
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Y:
            orient.rotation_angle = -90.0f;
            orient.flip_horizontal = false;
            orient.flip_vertical = true;  // D3D Y- is flipped vs GL
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Z:
            orient.rotation_angle = 0.0f;
            orient.flip_horizontal = false;
            orient.flip_vertical = false;
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Z:
            orient.rotation_angle = 180.0f;
            orient.flip_horizontal = true;
            orient.flip_vertical = false;
            break;
    }
    
    return orient;
}

std::string CubeTextureSupport::generate_reflection_vector_glsl(const std::string& normal,
                                                               const std::string& view_dir) {
    return "reflect(-" + view_dir + ", " + normal + ")";
}

std::string CubeTextureSupport::generate_reflection_vector_wgsl(const std::string& normal,
                                                               const std::string& view_dir) {
    return "reflect(-" + view_dir + ", " + normal + ")";
}

bool CubeTextureSupport::convert_face_data(const void* src_data, void* dst_data,
                                          D3DFORMAT d3d_format, GLenum gl_format,
                                          uint32_t width, uint32_t height,
                                          D3DCUBEMAP_FACES face) {
    // Get face orientation
    FaceOrientation orient = get_face_orientation(face);
    
    // For now, just copy the data
    // TODO: Implement proper orientation flipping if needed
    size_t data_size = width * height * 4;  // Assume 4 bytes per pixel for now
    std::memcpy(dst_data, src_data, data_size);
    
    return true;
}

#ifdef DX8GL_HAS_WEBGPU
WGpuTexture CubeTextureSupport::create_webgpu_cube_texture(WGpuDevice device,
                                                          uint32_t size,
                                                          uint32_t mip_levels,
                                                          WGpuTextureFormat format) {
    WGpuTextureDescriptor desc = {};
    desc.label = "Cube Texture";
    desc.size.width = size;
    desc.size.height = size;
    desc.size.depthOrArrayLayers = 6;  // 6 faces for cube
    desc.mipLevelCount = mip_levels;
    desc.sampleCount = 1;
    desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
    desc.format = format;
    desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
    desc.viewFormatCount = 0;
    desc.viewFormats = nullptr;
    
    return wgpu_device_create_texture(device, &desc);
}

WGpuTextureView CubeTextureSupport::create_cube_texture_view(WGpuTexture texture) {
    WGpuTextureViewDescriptor desc = {};
    desc.label = "Cube Texture View";
    desc.format = WGPU_TEXTURE_FORMAT_UNDEFINED;  // Use texture's format
    desc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_CUBE;
    desc.baseMipLevel = 0;
    desc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
    desc.baseArrayLayer = 0;
    desc.arrayLayerCount = 6;
    desc.aspect = WGPU_TEXTURE_ASPECT_ALL;
    
    return wgpu_texture_create_view(texture, &desc);
}

WGpuSampler CubeTextureSupport::create_cube_sampler(WGpuDevice device,
                                                   WGpuFilterMode min_filter,
                                                   WGpuFilterMode mag_filter,
                                                   WGpuMipmapFilterMode mipmap_filter) {
    WGpuSamplerDescriptor desc = {};
    desc.label = "Cube Sampler";
    desc.addressModeU = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeV = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeW = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.magFilter = mag_filter;
    desc.minFilter = min_filter;
    desc.mipmapFilter = mipmap_filter;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = WGPU_COMPARE_FUNCTION_UNDEFINED;
    desc.maxAnisotropy = 1;
    
    return wgpu_device_create_sampler(device, &desc);
}
#endif

// CubeTextureShaderGenerator implementation

std::string CubeTextureShaderGenerator::generate_glsl_declarations(int max_cube_textures) {
    std::stringstream ss;
    ss << "// Cube texture declarations\n";
    for (int i = 0; i < max_cube_textures; ++i) {
        ss << "uniform samplerCube cube_texture" << i << ";\n";
    }
    ss << "uniform int cube_texture_enabled;\n\n";
    return ss.str();
}

std::string CubeTextureShaderGenerator::generate_glsl_sampling_function() {
    return R"(
// Sample cube texture with proper coordinate system conversion
vec4 sample_cube_texture(samplerCube tex, vec3 coord) {
    // DirectX to OpenGL cube map coordinate conversion if needed
    vec3 gl_coord = coord;
    return texture(tex, gl_coord);
}
)";
}

std::string CubeTextureShaderGenerator::generate_wgsl_declarations(int max_cube_textures) {
    std::stringstream ss;
    ss << "// Cube texture declarations\n";
    for (int i = 0; i < max_cube_textures; ++i) {
        ss << "@group(1) @binding(" << (i * 2) << ")\n";
        ss << "var cube_texture" << i << ": texture_cube<f32>;\n";
        ss << "@group(1) @binding(" << (i * 2 + 1) << ")\n";
        ss << "var cube_sampler" << i << ": sampler;\n\n";
    }
    return ss.str();
}

std::string CubeTextureShaderGenerator::generate_wgsl_sampling_function() {
    return R"(
// Sample cube texture
fn sample_cube_texture(tex: texture_cube<f32>, samp: sampler, coord: vec3<f32>) -> vec4<f32> {
    return textureSample(tex, samp, coord);
}
)";
}

std::string CubeTextureShaderGenerator::generate_environment_mapping_glsl(
    const EnvironmentMapConfig& config) {
    std::stringstream ss;
    
    ss << "// Environment mapping\n";
    ss << "vec3 env_coord;\n";
    
    if (config.use_reflection) {
        ss << "vec3 view_dir = normalize(camera_pos - world_pos);\n";
        ss << "env_coord = reflect(-view_dir, world_normal);\n";
    }
    
    if (config.use_refraction) {
        ss << "float eta = 1.0 / " << config.refraction_index << ";\n";
        ss << "vec3 refract_coord = refract(-view_dir, world_normal, eta);\n";
        if (config.use_reflection) {
            ss << "// Mix reflection and refraction\n";
        } else {
            ss << "env_coord = refract_coord;\n";
        }
    }
    
    if (config.use_fresnel) {
        ss << "float fresnel = pow(1.0 - dot(view_dir, world_normal), " 
           << config.fresnel_power << ");\n";
        if (config.use_reflection && config.use_refraction) {
            ss << "env_coord = mix(refract_coord, env_coord, fresnel);\n";
        }
    }
    
    ss << "vec4 env_color = texture(cube_texture0, env_coord);\n";
    
    return ss.str();
}

std::string CubeTextureShaderGenerator::generate_environment_mapping_wgsl(
    const EnvironmentMapConfig& config) {
    std::stringstream ss;
    
    ss << "// Environment mapping\n";
    ss << "var env_coord: vec3<f32>;\n";
    
    if (config.use_reflection) {
        ss << "let view_dir = normalize(camera_pos - world_pos);\n";
        ss << "env_coord = reflect(-view_dir, world_normal);\n";
    }
    
    if (config.use_refraction) {
        ss << "let eta = 1.0 / " << config.refraction_index << ";\n";
        ss << "let refract_coord = refract(-view_dir, world_normal, eta);\n";
        if (config.use_reflection) {
            ss << "// Mix reflection and refraction\n";
        } else {
            ss << "env_coord = refract_coord;\n";
        }
    }
    
    if (config.use_fresnel) {
        ss << "let fresnel = pow(1.0 - dot(view_dir, world_normal), " 
           << config.fresnel_power << ");\n";
        if (config.use_reflection && config.use_refraction) {
            ss << "env_coord = mix(refract_coord, env_coord, fresnel);\n";
        }
    }
    
    ss << "let env_color = textureSample(cube_texture0, cube_sampler0, env_coord);\n";
    
    return ss.str();
}

// CubeTextureState implementation

void CubeTextureState::set_cube_texture(uint32_t stage, const CubeTextureBinding& binding) {
    if (stage >= 8) return;
    
    cube_textures_[stage] = binding;
    if (binding.is_cube_map) {
        active_cube_texture_mask_ |= (1u << stage);
    } else {
        active_cube_texture_mask_ &= ~(1u << stage);
    }
    
    DX8GL_TRACE("Set cube texture stage %u: texture=%u, is_cube=%d",
                stage, binding.texture_id, binding.is_cube_map);
}

const CubeTextureState::CubeTextureBinding* CubeTextureState::get_cube_texture(uint32_t stage) {
    if (stage >= 8) return nullptr;
    return &cube_textures_[stage];
}

void CubeTextureState::clear_cube_texture(uint32_t stage) {
    if (stage >= 8) return;
    
    cube_textures_[stage] = {};
    active_cube_texture_mask_ &= ~(1u << stage);
    
    DX8GL_TRACE("Cleared cube texture stage %u", stage);
}

bool CubeTextureState::has_cube_texture(uint32_t stage) {
    if (stage >= 8) return false;
    return (active_cube_texture_mask_ & (1u << stage)) != 0;
}

std::string CubeTextureState::generate_shader_defines() {
    std::stringstream ss;
    
    for (uint32_t i = 0; i < 8; ++i) {
        if (has_cube_texture(i)) {
            ss << "#define CUBE_TEXTURE_" << i << "_ENABLED 1\n";
        }
    }
    
    if (active_cube_texture_mask_ != 0) {
        ss << "#define HAS_CUBE_TEXTURES 1\n";
        ss << "#define ACTIVE_CUBE_TEXTURE_MASK " << active_cube_texture_mask_ << "u\n";
    }
    
    return ss.str();
}

// CubeTexCoordGenerator implementation

void CubeTexCoordGenerator::set_texgen_mode(uint32_t stage, CubeTexGenMode mode) {
    if (stage >= 8) return;
    texgen_modes_[stage] = mode;
    
    DX8GL_TRACE("Set cube texgen mode for stage %u: %d", stage, mode);
}

CubeTexGenMode CubeTexCoordGenerator::get_texgen_mode(uint32_t stage) {
    if (stage >= 8) return CUBE_TEXGEN_NONE;
    return texgen_modes_[stage];
}

std::string CubeTexCoordGenerator::generate_texgen_glsl(uint32_t stage,
                                                       const std::string& position,
                                                       const std::string& normal,
                                                       const std::string& view_matrix) {
    CubeTexGenMode mode = get_texgen_mode(stage);
    std::stringstream ss;
    
    ss << "// Cube texture coordinate generation for stage " << stage << "\n";
    
    switch (mode) {
        case CUBE_TEXGEN_REFLECTION_MAP:
            ss << "vec3 view_pos = (" << view_matrix << " * vec4(" << position << ", 1.0)).xyz;\n";
            ss << "vec3 view_normal = normalize(mat3(" << view_matrix << ") * " << normal << ");\n";
            ss << "vec3 view_dir = normalize(-view_pos);\n";
            ss << "vec3 cube_coord" << stage << " = reflect(-view_dir, view_normal);\n";
            break;
            
        case CUBE_TEXGEN_NORMAL_MAP:
            ss << "vec3 cube_coord" << stage << " = normalize(" << normal << ");\n";
            break;
            
        case CUBE_TEXGEN_SPHERE_MAP:
            ss << "vec3 view_normal = normalize(mat3(" << view_matrix << ") * " << normal << ");\n";
            ss << "vec3 cube_coord" << stage << " = view_normal;\n";
            break;
            
        case CUBE_TEXGEN_CAMERA_SPACE:
            ss << "vec3 view_pos = (" << view_matrix << " * vec4(" << position << ", 1.0)).xyz;\n";
            ss << "vec3 cube_coord" << stage << " = normalize(view_pos);\n";
            break;
            
        case CUBE_TEXGEN_OBJECT_SPACE:
            ss << "vec3 cube_coord" << stage << " = normalize(" << position << ");\n";
            break;
            
        default:
            ss << "vec3 cube_coord" << stage << " = vec3(0.0, 0.0, 1.0);\n";
            break;
    }
    
    return ss.str();
}

std::string CubeTexCoordGenerator::generate_texgen_wgsl(uint32_t stage,
                                                       const std::string& position,
                                                       const std::string& normal,
                                                       const std::string& view_matrix) {
    CubeTexGenMode mode = get_texgen_mode(stage);
    std::stringstream ss;
    
    ss << "// Cube texture coordinate generation for stage " << stage << "\n";
    
    switch (mode) {
        case CUBE_TEXGEN_REFLECTION_MAP:
            ss << "let view_pos = (" << view_matrix << " * vec4<f32>(" << position << ", 1.0)).xyz;\n";
            ss << "let view_normal = normalize(mat3x3<f32>(" << view_matrix << "[0].xyz, " 
               << view_matrix << "[1].xyz, " << view_matrix << "[2].xyz) * " << normal << ");\n";
            ss << "let view_dir = normalize(-view_pos);\n";
            ss << "let cube_coord" << stage << " = reflect(-view_dir, view_normal);\n";
            break;
            
        case CUBE_TEXGEN_NORMAL_MAP:
            ss << "let cube_coord" << stage << " = normalize(" << normal << ");\n";
            break;
            
        default:
            ss << "let cube_coord" << stage << " = vec3<f32>(0.0, 0.0, 1.0);\n";
            break;
    }
    
    return ss.str();
}

// DynamicCubeTexture implementation

GLuint DynamicCubeTexture::create_cube_face_framebuffer(GLuint cube_texture,
                                                        D3DCUBEMAP_FACES face,
                                                        uint32_t mip_level) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    GLenum target = CubeTextureSupport::get_gl_cube_face(face);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          target, cube_texture, mip_level);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DX8GL_ERROR("Cube face framebuffer incomplete: 0x%04x", status);
        glDeleteFramebuffers(1, &fbo);
        return 0;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

void DynamicCubeTexture::setup_cube_face_viewport(uint32_t size, uint32_t mip_level) {
    uint32_t mip_size = size >> mip_level;
    if (mip_size == 0) mip_size = 1;
    glViewport(0, 0, mip_size, mip_size);
}

void DynamicCubeTexture::setup_cube_face_view_matrix(D3DCUBEMAP_FACES face,
                                                    const float* cube_center,
                                                    float* view_matrix) {
    // Define look vectors for each cube face
    struct LookVectors {
        float target[3];
        float up[3];
    };
    
    static const LookVectors face_vectors[6] = {
        // POSITIVE_X
        {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        // NEGATIVE_X
        {{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        // POSITIVE_Y
        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        // NEGATIVE_Y
        {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        // POSITIVE_Z
        {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        // NEGATIVE_Z
        {{0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}
    };
    
    int face_index = static_cast<int>(face);
    const LookVectors& vecs = face_vectors[face_index];
    
    // Build look-at matrix
    // This is a simplified version - would need proper matrix math library
    float forward[3] = {vecs.target[0], vecs.target[1], vecs.target[2]};
    float up[3] = {vecs.up[0], vecs.up[1], vecs.up[2]};
    
    // Calculate right vector
    float right[3];
    right[0] = up[1] * forward[2] - up[2] * forward[1];
    right[1] = up[2] * forward[0] - up[0] * forward[2];
    right[2] = up[0] * forward[1] - up[1] * forward[0];
    
    // Normalize right
    float len = std::sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
    right[0] /= len; right[1] /= len; right[2] /= len;
    
    // Recalculate up
    up[0] = forward[1] * right[2] - forward[2] * right[1];
    up[1] = forward[2] * right[0] - forward[0] * right[2];
    up[2] = forward[0] * right[1] - forward[1] * right[0];
    
    // Build view matrix
    view_matrix[0] = right[0];
    view_matrix[1] = up[0];
    view_matrix[2] = -forward[0];
    view_matrix[3] = 0.0f;
    
    view_matrix[4] = right[1];
    view_matrix[5] = up[1];
    view_matrix[6] = -forward[1];
    view_matrix[7] = 0.0f;
    
    view_matrix[8] = right[2];
    view_matrix[9] = up[2];
    view_matrix[10] = -forward[2];
    view_matrix[11] = 0.0f;
    
    view_matrix[12] = -(right[0] * cube_center[0] + right[1] * cube_center[1] + right[2] * cube_center[2]);
    view_matrix[13] = -(up[0] * cube_center[0] + up[1] * cube_center[1] + up[2] * cube_center[2]);
    view_matrix[14] = forward[0] * cube_center[0] + forward[1] * cube_center[1] + forward[2] * cube_center[2];
    view_matrix[15] = 1.0f;
}

void DynamicCubeTexture::setup_cube_projection_matrix(float near_plane,
                                                     float far_plane,
                                                     float* proj_matrix) {
    // 90 degree FOV perspective projection for cube map
    float aspect = 1.0f;  // Square aspect ratio for cube faces
    float fov = 90.0f * 3.14159265f / 180.0f;  // 90 degrees in radians
    float f = 1.0f / std::tan(fov / 2.0f);
    
    std::memset(proj_matrix, 0, sizeof(float) * 16);
    
    proj_matrix[0] = f / aspect;
    proj_matrix[5] = f;
    proj_matrix[10] = (far_plane + near_plane) / (near_plane - far_plane);
    proj_matrix[11] = -1.0f;
    proj_matrix[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
}

#ifdef DX8GL_HAS_WEBGPU
WGpuRenderPassEncoder DynamicCubeTexture::begin_cube_face_render_pass(
    WGpuCommandEncoder encoder,
    WGpuTextureView cube_face_view,
    const WGpuColor* clear_color) {
    
    WGpuRenderPassColorAttachment color_attachment = {};
    color_attachment.view = cube_face_view;
    color_attachment.loadOp = WGPU_LOAD_OP_CLEAR;
    color_attachment.storeOp = WGPU_STORE_OP_STORE;
    if (clear_color) {
        color_attachment.clearValue = *clear_color;
    } else {
        color_attachment.clearValue.r = 0.0;
        color_attachment.clearValue.g = 0.0;
        color_attachment.clearValue.b = 0.0;
        color_attachment.clearValue.a = 1.0;
    }
    
    WGpuRenderPassDescriptor desc = {};
    desc.label = "Cube Face Render Pass";
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &color_attachment;
    desc.depthStencilAttachment = nullptr;
    
    return wgpu_command_encoder_begin_render_pass(encoder, &desc);
}
#endif

} // namespace dx8gl