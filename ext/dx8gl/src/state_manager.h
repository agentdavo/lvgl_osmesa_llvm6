#ifndef DX8GL_STATE_MANAGER_H
#define DX8GL_STATE_MANAGER_H

#include "d3d8.h"
#include "gl3_headers.h"
#include <unordered_map>
#include <array>
#include <memory>
#include <mutex>
#include "logger.h"

namespace dx8gl {

// Forward declarations
class ShaderProgramManager;

// ShaderProgram struct from shader_program_manager.h
struct ShaderProgram {
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
    
    // Cached uniform locations
    std::unordered_map<std::string, GLint> uniform_locations;
    
    // Standard uniforms
    GLint u_world_matrix;
    GLint u_view_matrix;
    GLint u_projection_matrix;
    GLint u_world_view_proj_matrix;
    
    // Additional matrix uniforms for fixed-function
    GLint u_mvp_matrix;
    GLint u_normal_matrix;
    
    // Vertex shader constants (c0-c95)
    GLint u_vs_constants[96];
    
    // Pixel shader constants (c0-c7)
    GLint u_ps_constants[8];
    
    // Texture samplers
    GLint u_textures[8];
    GLint u_texture[8]; // Alias for fixed-function compatibility
    
    // Lighting uniforms for fixed-function
    GLint u_light_enabled[8];
    GLint u_light_position[8];
    GLint u_light_direction[8];
    GLint u_light_diffuse[8];
    GLint u_light_specular[8];
    GLint u_light_ambient[8];
    
    // Material uniforms for fixed-function
    GLint u_material_diffuse;
    GLint u_material_ambient;
    GLint u_material_specular;
    GLint u_material_emissive;
    GLint u_material_power;
    
    // Fog uniforms for fixed-function
    GLint u_fog_color;
    GLint u_fog_start;
    GLint u_fog_end;
    GLint u_fog_density;
    
    // Alpha test uniform
    GLint u_alpha_ref;
    
    // Texture factor uniform for D3DTA_TFACTOR
    GLint u_texture_factor;
    
    ShaderProgram() : program(0), vertex_shader(0), fragment_shader(0),
                     u_world_matrix(-1), u_view_matrix(-1),
                     u_projection_matrix(-1), u_world_view_proj_matrix(-1),
                     u_mvp_matrix(-1), u_normal_matrix(-1),
                     u_material_diffuse(-1), u_material_ambient(-1),
                     u_material_specular(-1), u_material_emissive(-1),
                     u_material_power(-1), u_fog_color(-1), u_fog_start(-1),
                     u_fog_end(-1), u_fog_density(-1), u_alpha_ref(-1),
                     u_texture_factor(-1) {
        std::fill(std::begin(u_vs_constants), std::end(u_vs_constants), -1);
        std::fill(std::begin(u_ps_constants), std::end(u_ps_constants), -1);
        std::fill(std::begin(u_textures), std::end(u_textures), -1);
        std::fill(std::begin(u_texture), std::end(u_texture), -1);
        std::fill(std::begin(u_light_enabled), std::end(u_light_enabled), -1);
        std::fill(std::begin(u_light_position), std::end(u_light_position), -1);
        std::fill(std::begin(u_light_direction), std::end(u_light_direction), -1);
        std::fill(std::begin(u_light_diffuse), std::end(u_light_diffuse), -1);
        std::fill(std::begin(u_light_specular), std::end(u_light_specular), -1);
        std::fill(std::begin(u_light_ambient), std::end(u_light_ambient), -1);
    }
};

// Render state tracking
struct RenderState {
    // Rasterizer states
    D3DFILLMODE fill_mode = D3DFILL_SOLID;
    D3DSHADEMODE shade_mode = D3DSHADE_GOURAUD;
    D3DCULL cull_mode = D3DCULL_CCW;
    float point_size = 1.0f;
    float line_width = 1.0f;
    
    // Z-buffer states
    DWORD z_enable = TRUE;
    DWORD z_write_enable = TRUE;
    D3DCMPFUNC z_func = D3DCMP_LESSEQUAL;
    DWORD z_bias = 0;  // D3DRS_ZBIAS - maps to polygon offset
    
    // Alpha blending states
    DWORD alpha_blend_enable = FALSE;
    D3DBLEND src_blend = D3DBLEND_ONE;
    D3DBLEND dest_blend = D3DBLEND_ZERO;
    D3DBLENDOP blend_op = D3DBLENDOP_ADD;
    
    // Alpha testing
    DWORD alpha_test_enable = FALSE;
    D3DCMPFUNC alpha_func = D3DCMP_ALWAYS;
    DWORD alpha_ref = 0;
    
    // Stencil states
    DWORD stencil_enable = FALSE;
    DWORD stencil_fail = D3DSTENCILOP_KEEP;
    DWORD stencil_zfail = D3DSTENCILOP_KEEP;
    DWORD stencil_pass = D3DSTENCILOP_KEEP;
    D3DCMPFUNC stencil_func = D3DCMP_ALWAYS;
    DWORD stencil_ref = 0;
    DWORD stencil_mask = 0xFFFFFFFF;
    DWORD stencil_write_mask = 0xFFFFFFFF;
    
    // Fog states
    DWORD fog_enable = FALSE;
    D3DCOLOR fog_color = 0;
    D3DFOGMODE fog_table_mode = D3DFOG_NONE;
    D3DFOGMODE fog_vertex_mode = D3DFOG_NONE;
    float fog_start = 0.0f;
    float fog_end = 1.0f;
    float fog_density = 1.0f;
    DWORD range_fog_enable = FALSE;  // D3DRS_RANGEFOGENABLE
    
    // Lighting states
    DWORD lighting = TRUE;
    DWORD ambient = 0;
    DWORD normalize_normals = FALSE;
    DWORD local_viewer = FALSE;
    DWORD specular_enable = FALSE;
    DWORD specular_material_source = D3DMCS_MATERIAL;  // D3DRS_SPECULARMATERIALSOURCE
    DWORD color_vertex = TRUE;  // D3DRS_COLORVERTEX
    
    // Texture states
    DWORD color_op[8] = {D3DTOP_MODULATE};
    DWORD color_arg1[8] = {D3DTA_TEXTURE};
    DWORD color_arg2[8] = {D3DTA_CURRENT};
    DWORD alpha_op[8] = {D3DTOP_SELECTARG1};
    DWORD alpha_arg1[8] = {D3DTA_TEXTURE};
    DWORD alpha_arg2[8] = {D3DTA_CURRENT};
    
    // Texture filtering
    DWORD mag_filter[8] = {D3DTEXF_POINT};
    DWORD min_filter[8] = {D3DTEXF_POINT};
    DWORD mip_filter[8] = {D3DTEXF_NONE};
    
    // Texture addressing
    DWORD address_u[8] = {D3DTADDRESS_WRAP};
    DWORD address_v[8] = {D3DTADDRESS_WRAP};
    DWORD address_w[8] = {D3DTADDRESS_WRAP};
    
    // Additional texture stage states
    float bump_env_mat[8][4] = {};  // BUMPENVMAT00, 01, 10, 11
    DWORD texcoord_index[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    DWORD border_color[8] = {0};
    float mipmap_lod_bias[8] = {0.0f};
    DWORD max_mip_level[8] = {0};
    DWORD max_anisotropy[8] = {1};
    float bump_env_lscale[8] = {0.0f};
    float bump_env_loffset[8] = {0.0f};
    DWORD texture_transform_flags[8] = {D3DTTFF_DISABLE};
    DWORD color_arg0[8] = {D3DTA_CURRENT};
    DWORD alpha_arg0[8] = {D3DTA_CURRENT};
    DWORD result_arg[8] = {D3DTA_CURRENT};
    
    // Clipping
    DWORD clipping = TRUE;
    DWORD clip_plane_enable = 0;
    
    // Misc states
    DWORD dither_enable = FALSE;
    DWORD last_pixel = TRUE;
    DWORD multisample_antialias = FALSE;
    DWORD scissor_test_enable = FALSE;
    
    // Texture factor for D3DTA_TFACTOR
    D3DCOLOR texture_factor = 0xFFFFFFFF;
};

// Transform state tracking
struct TransformState {
    D3DMATRIX world;
    D3DMATRIX view;
    D3DMATRIX projection;
    D3DMATRIX texture[8];
    
    // Combined matrices (computed on demand)
    D3DMATRIX world_view;
    D3DMATRIX world_view_projection;
    D3DMATRIX view_projection;
    
    bool world_view_dirty = true;
    bool world_view_projection_dirty = true;
    bool view_projection_dirty = true;
};

// Light state
struct LightState {
    D3DLIGHT8 properties;
    BOOL enabled = FALSE;
};

// Material state
struct MaterialState {
    D3DMATERIAL8 material;
    bool valid = false;
};

// Viewport state
struct ViewportState {
    D3DVIEWPORT8 viewport;
    bool valid = false;
};

// Clip plane state
struct ClipPlaneState {
    float plane[4] = {0, 0, 0, 0};
    bool enabled = false;
};

// Clip status state
struct ClipStatusState {
    DWORD clip_union = 0;
    DWORD clip_intersection = 0;
    bool valid = false;
};

// State block structure for capturing/restoring device state
struct StateBlock {
    // What states to capture/apply
    D3DSTATEBLOCKTYPE type = D3DSBT_ALL;
    bool is_recording = false;
    
    // Render states
    std::unordered_map<D3DRENDERSTATETYPE, DWORD> render_states;
    
    // Transform states
    std::unordered_map<D3DTRANSFORMSTATETYPE, D3DMATRIX> transforms;
    
    // Texture stage states
    struct TextureStageState {
        std::unordered_map<D3DTEXTURESTAGESTATETYPE, DWORD> states;
    };
    std::array<TextureStageState, 8> texture_stages;
    
    // Sampler states (stored as texture stage states in DX8)
    struct SamplerState {
        std::unordered_map<D3DTEXTURESTAGESTATETYPE, DWORD> states;
    };
    std::array<SamplerState, 8> sampler_states;
    
    // Lights
    std::unordered_map<DWORD, LightState> lights;
    
    // Material
    bool has_material = false;
    MaterialState material;
    
    // Viewport
    bool has_viewport = false;
    ViewportState viewport;
    
    // Clip planes
    std::unordered_map<DWORD, ClipPlaneState> clip_planes;
    
    // Vertex shader and constants
    DWORD vertex_shader = 0;
    bool has_vertex_shader = false;
    std::unordered_map<DWORD, D3DXVECTOR4> vertex_shader_constants;
    
    // Pixel shader and constants
    DWORD pixel_shader = 0;
    bool has_pixel_shader = false;
    std::unordered_map<DWORD, D3DXVECTOR4> pixel_shader_constants;
    
    // FVF
    DWORD fvf = 0;
    bool has_fvf = false;
    
    // Textures
    std::array<IDirect3DBaseTexture8*, 8> textures = {};
    std::array<bool, 8> has_texture = {};
    
    // Stream sources
    struct StreamSource {
        IDirect3DVertexBuffer8* buffer = nullptr;
        UINT stride = 0;
        bool valid = false;
    };
    std::array<StreamSource, 16> stream_sources;
    
    // Index buffer
    IDirect3DIndexBuffer8* index_buffer = nullptr;
    UINT index_base_vertex = 0;
    bool has_index_buffer = false;
    
    // Clear the state block
    void clear();
    
    // Check if state block should capture/apply a particular state
    bool should_capture_render_state(D3DRENDERSTATETYPE state) const;
    bool should_capture_texture_stage(DWORD stage, D3DTEXTURESTAGESTATETYPE state) const;
    bool should_capture_transform(D3DTRANSFORMSTATETYPE state) const;
};

class StateManager {
public:
    StateManager();
    ~StateManager();

    // Initialize state manager
    bool initialize();

    // Render state management
    void set_render_state(D3DRENDERSTATETYPE state, DWORD value);
    DWORD get_render_state(D3DRENDERSTATETYPE state) const;
    
    // Transform management
    void set_transform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix);
    void get_transform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* matrix) const;
    void multiply_transform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix);
    const D3DMATRIX* get_world_view_projection_matrix();
    const D3DMATRIX* get_world_view_matrix();
    const D3DMATRIX* get_view_projection_matrix();
    
    // Texture stage state management
    void set_texture_stage_state(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    DWORD get_texture_stage_state(DWORD stage, D3DTEXTURESTAGESTATETYPE type) const;
    
    // Light management
    void set_light(DWORD index, const D3DLIGHT8* light);
    void get_light(DWORD index, D3DLIGHT8* light) const;
    void light_enable(DWORD index, BOOL enable);
    BOOL is_light_enabled(DWORD index) const;
    
    // Material management
    void set_material(const D3DMATERIAL8* material);
    void get_material(D3DMATERIAL8* material) const;
    
    // Viewport management
    void set_viewport(const D3DVIEWPORT8* viewport);
    void get_viewport(D3DVIEWPORT8* viewport) const;
    
    // Scissor rect management
    void set_scissor_rect(const RECT& rect, BOOL enable);
    
    // Clear operations
    void clear(DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil);
    
    // Clip plane management
    void set_clip_plane(DWORD index, const float* plane);
    void get_clip_plane(DWORD index, float* plane) const;
    
    // Clip status management
    void set_clip_status(DWORD clip_union, DWORD clip_intersection);
    void get_clip_status(DWORD* clip_union, DWORD* clip_intersection) const;
    
    // Apply current state to OpenGL
    void apply_render_states();
    void apply_transform_states(ShaderProgram* shader);
    void apply_texture_states();
    void apply_light_states(ShaderProgram* shader);
    void apply_material_state(ShaderProgram* shader);
    void apply_fog_state(ShaderProgram* shader);
    
    // State validation
    bool validate_state() const;
    
    // Reset to default state
    void reset();
    
    // Invalidate cached states (forces full state reapplication)
    void invalidate_cached_render_states();
    
    // FVF tracking
    void set_current_fvf(DWORD fvf) { current_fvf_ = fvf; }
    DWORD get_current_fvf() const { return current_fvf_; }
    
    // Texture state tracking
    void set_texture_enabled(DWORD stage, bool enabled);
    bool is_texture_enabled(DWORD stage) const;
    
    // Apply shader state (for immediate mode)
    void apply_shader_state();
    
    // State block management
    DWORD create_state_block(D3DSTATEBLOCKTYPE type);
    void delete_state_block(DWORD token);
    void begin_state_block();
    DWORD end_state_block();
    void apply_state_block(DWORD token);
    void capture_state_block(DWORD token);
    
    // Get current recording state block (if any)
    StateBlock* get_recording_state_block() { return recording_state_block_; }

private:
    // Helper methods
    static GLenum convert_blend_factor(D3DBLEND blend);
    static GLenum convert_blend_op(D3DBLENDOP op);
    static GLenum convert_cmp_func(D3DCMPFUNC func);
    static GLenum convert_stencil_op(DWORD op);
    static GLenum convert_cull_mode(D3DCULL mode);
    static void multiply_matrices(D3DMATRIX* out, const D3DMATRIX* a, const D3DMATRIX* b);
    static void transpose_matrix(D3DMATRIX* out, const D3DMATRIX* in);
    
    // State validation methods
    bool validate_render_states() const;
    bool validate_texture_states() const;
    bool validate_transform_states() const;
    bool validate_light_states() const;
    
    // Initialize default states
    void init_default_states();
    
    mutable std::mutex state_mutex_;
    
    // Current states
    RenderState render_state_;
    TransformState transform_state_;
    MaterialState material_state_;
    ViewportState viewport_state_;
    
    // Light and clip plane arrays
    static constexpr DWORD MAX_LIGHTS = 8;
    static constexpr DWORD MAX_CLIP_PLANES = 6;
    std::array<LightState, MAX_LIGHTS> lights_;
    std::array<ClipPlaneState, MAX_CLIP_PLANES> clip_planes_;
    
    // Clip status state
    ClipStatusState clip_status_;
    
    // State dirty flags
    bool render_state_dirty_;
    bool transform_state_dirty_;
    bool texture_state_dirty_;
    bool light_state_dirty_;
    bool material_state_dirty_;
    bool viewport_state_dirty_;
    
    // OpenGL state cache (to avoid redundant state changes)
    struct GLStateCache {
        bool blend_enabled = false;
        GLenum src_blend = GL_ONE;
        GLenum dst_blend = GL_ZERO;
        bool depth_test_enabled = true;
        bool depth_write_enabled = true;
        GLenum depth_func = GL_LESS;
        bool cull_face_enabled = true;
        GLenum cull_mode = GL_BACK;
        bool scissor_enabled = false;
        bool stencil_enabled = false;
    } gl_cache_;
    
    // Current FVF for immediate mode
    DWORD current_fvf_;
    
    // State block management
    std::unordered_map<DWORD, std::unique_ptr<StateBlock>> state_blocks_;
    StateBlock* recording_state_block_ = nullptr;
    DWORD next_state_block_token_ = 1;
};

} // namespace dx8gl

#endif // DX8GL_STATE_MANAGER_H