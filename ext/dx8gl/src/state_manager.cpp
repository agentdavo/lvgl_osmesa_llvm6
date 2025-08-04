#include "state_manager.h"
#include "osmesa_gl_loader.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace dx8gl {

StateManager::StateManager()
    : render_state_dirty_(true)
    , transform_state_dirty_(true)
    , texture_state_dirty_(true)
    , light_state_dirty_(true)
    , material_state_dirty_(true)
    , viewport_state_dirty_(true)
    , current_fvf_(0) {
    init_default_states();
}

StateManager::~StateManager() = default;

bool StateManager::initialize() {
    DX8GL_INFO("Initializing state manager");
    
    // Reset all states to defaults
    reset();
    
    // Apply initial OpenGL state
    apply_render_states();
    
    return true;
}

void StateManager::init_default_states() {
    // Initialize transform matrices to identity
    memset(&transform_state_.world, 0, sizeof(D3DMATRIX));
    memset(&transform_state_.view, 0, sizeof(D3DMATRIX));
    memset(&transform_state_.projection, 0, sizeof(D3DMATRIX));
    
    // Set identity matrices
    transform_state_.world._11 = transform_state_.world._22 = 
    transform_state_.world._33 = transform_state_.world._44 = 1.0f;
    
    transform_state_.view._11 = transform_state_.view._22 = 
    transform_state_.view._33 = transform_state_.view._44 = 1.0f;
    
    transform_state_.projection._11 = transform_state_.projection._22 = 
    transform_state_.projection._33 = transform_state_.projection._44 = 1.0f;
    
    // Initialize texture matrices
    for (int i = 0; i < 8; i++) {
        memset(&transform_state_.texture[i], 0, sizeof(D3DMATRIX));
        transform_state_.texture[i]._11 = transform_state_.texture[i]._22 = 
        transform_state_.texture[i]._33 = transform_state_.texture[i]._44 = 1.0f;
    }
    
    // Initialize default material
    material_state_.material.Diffuse.r = material_state_.material.Diffuse.g = 
    material_state_.material.Diffuse.b = material_state_.material.Diffuse.a = 1.0f;
    
    material_state_.material.Ambient.r = material_state_.material.Ambient.g = 
    material_state_.material.Ambient.b = material_state_.material.Ambient.a = 0.0f;
    
    material_state_.material.Specular.r = material_state_.material.Specular.g = 
    material_state_.material.Specular.b = material_state_.material.Specular.a = 0.0f;
    
    material_state_.material.Emissive.r = material_state_.material.Emissive.g = 
    material_state_.material.Emissive.b = material_state_.material.Emissive.a = 0.0f;
    
    material_state_.material.Power = 0.0f;
    
    // Initialize default lights
    for (auto& light : lights_) {
        light.properties.Type = D3DLIGHT_DIRECTIONAL;
        light.properties.Diffuse.r = light.properties.Diffuse.g = 
        light.properties.Diffuse.b = light.properties.Diffuse.a = 1.0f;
        light.properties.Specular = light.properties.Diffuse;
        light.properties.Ambient.r = light.properties.Ambient.g = 
        light.properties.Ambient.b = light.properties.Ambient.a = 0.0f;
        light.properties.Position.x = light.properties.Position.y = 
        light.properties.Position.z = 0.0f;
        light.properties.Direction.x = 0.0f;
        light.properties.Direction.y = 0.0f;
        light.properties.Direction.z = 1.0f;
        light.properties.Range = 0.0f;
        light.properties.Falloff = 0.0f;
        light.properties.Attenuation0 = 1.0f;
        light.properties.Attenuation1 = 0.0f;
        light.properties.Attenuation2 = 0.0f;
        light.properties.Theta = 0.0f;
        light.properties.Phi = 0.0f;
        light.enabled = FALSE;
    }
}

void StateManager::reset() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Reset all states to defaults
    render_state_ = RenderState();
    transform_state_ = TransformState();
    material_state_ = MaterialState();
    viewport_state_ = ViewportState();
    
    // Re-initialize defaults
    init_default_states();
    
    // Mark everything as dirty
    render_state_dirty_ = true;
    transform_state_dirty_ = true;
    texture_state_dirty_ = true;
    light_state_dirty_ = true;
    material_state_dirty_ = true;
    viewport_state_dirty_ = true;
}

// Render state management
void StateManager::set_render_state(D3DRENDERSTATETYPE state, DWORD value) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    switch (state) {
        case D3DRS_ZENABLE:
            DX8GL_INFO("Setting D3DRS_ZENABLE to %u", value);
            render_state_.z_enable = value;
            break;
        case D3DRS_FILLMODE:
            render_state_.fill_mode = static_cast<D3DFILLMODE>(value);
            break;
        case D3DRS_SHADEMODE:
            render_state_.shade_mode = static_cast<D3DSHADEMODE>(value);
            break;
        case D3DRS_ZWRITEENABLE:
            render_state_.z_write_enable = value;
            break;
        case D3DRS_ALPHATESTENABLE:
            render_state_.alpha_test_enable = value;
            break;
        case D3DRS_SRCBLEND:
            render_state_.src_blend = static_cast<D3DBLEND>(value);
            break;
        case D3DRS_DESTBLEND:
            render_state_.dest_blend = static_cast<D3DBLEND>(value);
            break;
        case D3DRS_CULLMODE:
            DX8GL_INFO("Setting D3DRS_CULLMODE to %u", value);
            render_state_.cull_mode = static_cast<D3DCULL>(value);
            break;
        case D3DRS_ZFUNC:
            render_state_.z_func = static_cast<D3DCMPFUNC>(value);
            break;
        case D3DRS_ALPHAREF:
            render_state_.alpha_ref = value;
            break;
        case D3DRS_ALPHAFUNC:
            render_state_.alpha_func = static_cast<D3DCMPFUNC>(value);
            break;
        case D3DRS_DITHERENABLE:
            render_state_.dither_enable = value;
            break;
        case D3DRS_ALPHABLENDENABLE:
            render_state_.alpha_blend_enable = value;
            break;
        case D3DRS_FOGENABLE:
            render_state_.fog_enable = value;
            break;
        case D3DRS_SPECULARENABLE:
            render_state_.specular_enable = value;
            break;
        case D3DRS_FOGCOLOR:
            render_state_.fog_color = value;
            break;
        case D3DRS_FOGTABLEMODE:
            render_state_.fog_table_mode = static_cast<D3DFOGMODE>(value);
            break;
        case D3DRS_FOGSTART:
            render_state_.fog_start = *reinterpret_cast<const float*>(&value);
            break;
        case D3DRS_FOGEND:
            render_state_.fog_end = *reinterpret_cast<const float*>(&value);
            break;
        case D3DRS_FOGDENSITY:
            render_state_.fog_density = *reinterpret_cast<const float*>(&value);
            break;
        case D3DRS_POINTSIZE:
            render_state_.point_size = *reinterpret_cast<const float*>(&value);
            break;
        case D3DRS_STENCILENABLE:
            render_state_.stencil_enable = value;
            break;
        case D3DRS_STENCILFAIL:
            render_state_.stencil_fail = value;
            break;
        case D3DRS_STENCILZFAIL:
            render_state_.stencil_zfail = value;
            break;
        case D3DRS_STENCILPASS:
            render_state_.stencil_pass = value;
            break;
        case D3DRS_STENCILFUNC:
            render_state_.stencil_func = static_cast<D3DCMPFUNC>(value);
            break;
        case D3DRS_STENCILREF:
            render_state_.stencil_ref = value;
            break;
        case D3DRS_STENCILMASK:
            render_state_.stencil_mask = value;
            break;
        case D3DRS_STENCILWRITEMASK:
            render_state_.stencil_write_mask = value;
            break;
        case D3DRS_LIGHTING:
            render_state_.lighting = value;
            break;
        case D3DRS_AMBIENT:
            render_state_.ambient = value;
            break;
        case D3DRS_NORMALIZENORMALS:
            render_state_.normalize_normals = value;
            break;
        case D3DRS_LOCALVIEWER:
            render_state_.local_viewer = value;
            break;
        case D3DRS_SCISSORTESTENABLE:
            render_state_.scissor_test_enable = value;
            break;
        case D3DRS_BLENDOP:
            render_state_.blend_op = static_cast<D3DBLENDOP>(value);
            break;
        default:
            DX8GL_WARN("Unhandled render state: %d", state);
            break;
    }
    
    render_state_dirty_ = true;
}

DWORD StateManager::get_render_state(D3DRENDERSTATETYPE state) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    switch (state) {
        case D3DRS_ZENABLE: return render_state_.z_enable;
        case D3DRS_FILLMODE: return render_state_.fill_mode;
        case D3DRS_SHADEMODE: return render_state_.shade_mode;
        case D3DRS_ZWRITEENABLE: return render_state_.z_write_enable;
        case D3DRS_ALPHATESTENABLE: return render_state_.alpha_test_enable;
        case D3DRS_SRCBLEND: return render_state_.src_blend;
        case D3DRS_DESTBLEND: return render_state_.dest_blend;
        case D3DRS_CULLMODE: return render_state_.cull_mode;
        case D3DRS_ZFUNC: return render_state_.z_func;
        case D3DRS_ALPHAREF: return render_state_.alpha_ref;
        case D3DRS_ALPHAFUNC: return render_state_.alpha_func;
        case D3DRS_DITHERENABLE: return render_state_.dither_enable;
        case D3DRS_ALPHABLENDENABLE: return render_state_.alpha_blend_enable;
        case D3DRS_FOGENABLE: return render_state_.fog_enable;
        case D3DRS_SPECULARENABLE: return render_state_.specular_enable;
        case D3DRS_FOGCOLOR: return render_state_.fog_color;
        case D3DRS_FOGTABLEMODE: return render_state_.fog_table_mode;
        case D3DRS_FOGSTART: return *reinterpret_cast<const DWORD*>(&render_state_.fog_start);
        case D3DRS_FOGEND: return *reinterpret_cast<const DWORD*>(&render_state_.fog_end);
        case D3DRS_FOGDENSITY: return *reinterpret_cast<const DWORD*>(&render_state_.fog_density);
        case D3DRS_POINTSIZE: return *reinterpret_cast<const DWORD*>(&render_state_.point_size);
        case D3DRS_STENCILENABLE: return render_state_.stencil_enable;
        case D3DRS_STENCILFAIL: return render_state_.stencil_fail;
        case D3DRS_STENCILZFAIL: return render_state_.stencil_zfail;
        case D3DRS_STENCILPASS: return render_state_.stencil_pass;
        case D3DRS_STENCILFUNC: return render_state_.stencil_func;
        case D3DRS_STENCILREF: return render_state_.stencil_ref;
        case D3DRS_STENCILMASK: return render_state_.stencil_mask;
        case D3DRS_STENCILWRITEMASK: return render_state_.stencil_write_mask;
        case D3DRS_LIGHTING: return render_state_.lighting;
        case D3DRS_AMBIENT: return render_state_.ambient;
        case D3DRS_NORMALIZENORMALS: return render_state_.normalize_normals;
        case D3DRS_LOCALVIEWER: return render_state_.local_viewer;
        case D3DRS_SCISSORTESTENABLE: return render_state_.scissor_test_enable;
        case D3DRS_BLENDOP: return render_state_.blend_op;
        default:
            DX8GL_WARN("Unhandled render state query: %d", state);
            return 0;
    }
}

// Transform management
void StateManager::set_transform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) {
    if (!matrix) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    switch (state) {
        case D3DTS_WORLD:
            transform_state_.world = *matrix;
            transform_state_.world_view_dirty = true;
            transform_state_.world_view_projection_dirty = true;
            break;
        case D3DTS_VIEW:
            transform_state_.view = *matrix;
            transform_state_.world_view_dirty = true;
            transform_state_.world_view_projection_dirty = true;
            transform_state_.view_projection_dirty = true;
            break;
        case D3DTS_PROJECTION:
            transform_state_.projection = *matrix;
            transform_state_.world_view_projection_dirty = true;
            transform_state_.view_projection_dirty = true;
            break;
        case D3DTS_TEXTURE0:
        case D3DTS_TEXTURE1:
        case D3DTS_TEXTURE2:
        case D3DTS_TEXTURE3:
        case D3DTS_TEXTURE4:
        case D3DTS_TEXTURE5:
        case D3DTS_TEXTURE6:
        case D3DTS_TEXTURE7:
            {
                int index = state - D3DTS_TEXTURE0;
                transform_state_.texture[index] = *matrix;
                texture_state_dirty_ = true;
            }
            break;
        default:
            DX8GL_WARN("Unhandled transform state: %d", state);
            break;
    }
    
    transform_state_dirty_ = true;
}

void StateManager::get_transform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* matrix) const {
    if (!matrix) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    switch (state) {
        case D3DTS_WORLD:
            *matrix = transform_state_.world;
            break;
        case D3DTS_VIEW:
            *matrix = transform_state_.view;
            break;
        case D3DTS_PROJECTION:
            *matrix = transform_state_.projection;
            break;
        case D3DTS_TEXTURE0:
        case D3DTS_TEXTURE1:
        case D3DTS_TEXTURE2:
        case D3DTS_TEXTURE3:
        case D3DTS_TEXTURE4:
        case D3DTS_TEXTURE5:
        case D3DTS_TEXTURE6:
        case D3DTS_TEXTURE7:
            {
                int index = state - D3DTS_TEXTURE0;
                *matrix = transform_state_.texture[index];
            }
            break;
        default:
            DX8GL_WARN("Unhandled transform state query: %d", state);
            *matrix = D3DMATRIX{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            break;
    }
}

const D3DMATRIX* StateManager::get_world_view_projection_matrix() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (transform_state_.world_view_projection_dirty) {
        // First compute world_view
        if (transform_state_.world_view_dirty) {
            multiply_matrices(&transform_state_.world_view, 
                            &transform_state_.world, &transform_state_.view);
            transform_state_.world_view_dirty = false;
        }
        
        // Then compute world_view_projection
        multiply_matrices(&transform_state_.world_view_projection,
                        &transform_state_.world_view, &transform_state_.projection);
        transform_state_.world_view_projection_dirty = false;
    }
    
    return &transform_state_.world_view_projection;
}

const D3DMATRIX* StateManager::get_world_view_matrix() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (transform_state_.world_view_dirty) {
        multiply_matrices(&transform_state_.world_view,
                        &transform_state_.world, &transform_state_.view);
        transform_state_.world_view_dirty = false;
    }
    
    return &transform_state_.world_view;
}

const D3DMATRIX* StateManager::get_view_projection_matrix() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (transform_state_.view_projection_dirty) {
        multiply_matrices(&transform_state_.view_projection,
                        &transform_state_.view, &transform_state_.projection);
        transform_state_.view_projection_dirty = false;
    }
    
    return &transform_state_.view_projection;
}

// Apply render states to OpenGL
void StateManager::apply_render_states() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!render_state_dirty_) {
        DX8GL_INFO("Render states not dirty, skipping apply");
        return;
    }
    
    DX8GL_INFO("Applying render states, z_enable=%u", render_state_.z_enable);
    
    // Depth test - always check actual OpenGL state
    GLboolean actual_depth_test = GL_FALSE;
    glGetBooleanv(GL_DEPTH_TEST, &actual_depth_test);
    bool should_enable = render_state_.z_enable != 0;
    
    if (should_enable != (actual_depth_test == GL_TRUE)) {
        if (should_enable) {
            glEnable(GL_DEPTH_TEST);
            DX8GL_INFO("Enabled GL_DEPTH_TEST (was %s)", actual_depth_test ? "enabled" : "disabled");
        } else {
            glDisable(GL_DEPTH_TEST);
            DX8GL_INFO("Disabled GL_DEPTH_TEST (was %s)", actual_depth_test ? "enabled" : "disabled");
        }
        gl_cache_.depth_test_enabled = render_state_.z_enable;
    } else {
        DX8GL_INFO("GL_DEPTH_TEST already in correct state: %s", should_enable ? "enabled" : "disabled");
    }
    
    // Depth write
    if (render_state_.z_write_enable != gl_cache_.depth_write_enabled) {
        glDepthMask(render_state_.z_write_enable ? GL_TRUE : GL_FALSE);
        gl_cache_.depth_write_enabled = render_state_.z_write_enable;
    }
    
    // Depth function
    GLenum depth_func = convert_cmp_func(render_state_.z_func);
    if (depth_func != gl_cache_.depth_func) {
        glDepthFunc(depth_func);
        gl_cache_.depth_func = depth_func;
    }
    
    // Blending
    if (render_state_.alpha_blend_enable != gl_cache_.blend_enabled) {
        if (render_state_.alpha_blend_enable) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        gl_cache_.blend_enabled = render_state_.alpha_blend_enable;
    }
    
    // Blend function
    GLenum src_blend = convert_blend_factor(render_state_.src_blend);
    GLenum dst_blend = convert_blend_factor(render_state_.dest_blend);
    if (src_blend != gl_cache_.src_blend || dst_blend != gl_cache_.dst_blend) {
        glBlendFunc(src_blend, dst_blend);
        gl_cache_.src_blend = src_blend;
        gl_cache_.dst_blend = dst_blend;
    }
    
    // Culling
    bool cull_enable = (render_state_.cull_mode != D3DCULL_NONE);
    if (cull_enable != gl_cache_.cull_face_enabled) {
        if (cull_enable) {
            glEnable(GL_CULL_FACE);
            DX8GL_INFO("Enabled GL_CULL_FACE");
        } else {
            glDisable(GL_CULL_FACE);
            DX8GL_INFO("Disabled GL_CULL_FACE");
        }
        gl_cache_.cull_face_enabled = cull_enable;
    } else {
        DX8GL_INFO("GL_CULL_FACE already in correct state: %s", cull_enable ? "enabled" : "disabled");
    }
    
    // Cull mode
    if (cull_enable) {
        GLenum cull_mode = convert_cull_mode(render_state_.cull_mode);
        if (cull_mode != gl_cache_.cull_mode) {
            glCullFace(cull_mode);
            gl_cache_.cull_mode = cull_mode;
        }
    }
    
    // Scissor test
    if (render_state_.scissor_test_enable != gl_cache_.scissor_enabled) {
        if (render_state_.scissor_test_enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        gl_cache_.scissor_enabled = render_state_.scissor_test_enable;
    }
    
    // Stencil test
    if (render_state_.stencil_enable != gl_cache_.stencil_enabled) {
        if (render_state_.stencil_enable) {
            glEnable(GL_STENCIL_TEST);
        } else {
            glDisable(GL_STENCIL_TEST);
        }
        gl_cache_.stencil_enabled = render_state_.stencil_enable;
    }
    
    // Stencil function and operations
    if (render_state_.stencil_enable) {
        glStencilFunc(convert_cmp_func(render_state_.stencil_func),
                     render_state_.stencil_ref, render_state_.stencil_mask);
        glStencilOp(convert_stencil_op(render_state_.stencil_fail),
                   convert_stencil_op(render_state_.stencil_zfail),
                   convert_stencil_op(render_state_.stencil_pass));
        glStencilMask(render_state_.stencil_write_mask);
    }
    
    // Line width
    if (render_state_.fill_mode == D3DFILL_WIREFRAME) {
        glLineWidth(render_state_.line_width);
    }
    
    render_state_dirty_ = false;
}

// Viewport management
void StateManager::set_viewport(const D3DVIEWPORT8* viewport) {
    if (!viewport) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    viewport_state_.viewport = *viewport;
    viewport_state_.valid = true;
    viewport_state_dirty_ = true;
    
    // Apply immediately
    DX8GL_INFO("Setting viewport: %dx%d at (%d,%d), depth [%f,%f]", 
               viewport->Width, viewport->Height, viewport->X, viewport->Y,
               viewport->MinZ, viewport->MaxZ);
    glViewport(viewport->X, viewport->Y, viewport->Width, viewport->Height);
    glDepthRangef(viewport->MinZ, viewport->MaxZ);
}

void StateManager::get_viewport(D3DVIEWPORT8* viewport) const {
    if (!viewport) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (viewport_state_.valid) {
        *viewport = viewport_state_.viewport;
    } else {
        // Return default viewport
        viewport->X = 0;
        viewport->Y = 0;
        viewport->Width = 640;
        viewport->Height = 480;
        viewport->MinZ = 0.0f;
        viewport->MaxZ = 1.0f;
    }
}

// Helper methods
GLenum StateManager::convert_blend_factor(D3DBLEND blend) {
    switch (blend) {
        case D3DBLEND_ZERO: return GL_ZERO;
        case D3DBLEND_ONE: return GL_ONE;
        case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
        case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
        case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
        default: return GL_ONE;
    }
}

GLenum StateManager::convert_blend_op(D3DBLENDOP op) {
    switch (op) {
        case D3DBLENDOP_ADD: return GL_FUNC_ADD;
        case D3DBLENDOP_SUBTRACT: return GL_FUNC_SUBTRACT;
        case D3DBLENDOP_REVSUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        // MIN/MAX not supported in ES 2.0
        default: return GL_FUNC_ADD;
    }
}

GLenum StateManager::convert_cmp_func(D3DCMPFUNC func) {
    switch (func) {
        case D3DCMP_NEVER: return GL_NEVER;
        case D3DCMP_LESS: return GL_LESS;
        case D3DCMP_EQUAL: return GL_EQUAL;
        case D3DCMP_LESSEQUAL: return GL_LEQUAL;
        case D3DCMP_GREATER: return GL_GREATER;
        case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
        case D3DCMP_ALWAYS: return GL_ALWAYS;
        default: return GL_ALWAYS;
    }
}

GLenum StateManager::convert_stencil_op(DWORD op) {
    switch (op) {
        case D3DSTENCILOP_KEEP: return GL_KEEP;
        case D3DSTENCILOP_ZERO: return GL_ZERO;
        case D3DSTENCILOP_REPLACE: return GL_REPLACE;
        case D3DSTENCILOP_INCRSAT: return GL_INCR;
        case D3DSTENCILOP_DECRSAT: return GL_DECR;
        case D3DSTENCILOP_INVERT: return GL_INVERT;
        case D3DSTENCILOP_INCR: return GL_INCR_WRAP;
        case D3DSTENCILOP_DECR: return GL_DECR_WRAP;
        default: return GL_KEEP;
    }
}

GLenum StateManager::convert_cull_mode(D3DCULL mode) {
    switch (mode) {
        case D3DCULL_CW: return GL_FRONT;
        case D3DCULL_CCW: return GL_BACK;
        default: return GL_BACK;
    }
}

void StateManager::multiply_matrices(D3DMATRIX* out, const D3DMATRIX* a, const D3DMATRIX* b) {
    D3DMATRIX temp;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m(i, k) * b->m(k, j);
            }
            temp.m(i, j) = sum;
        }
    }
    
    *out = temp;
}

void StateManager::transpose_matrix(D3DMATRIX* out, const D3DMATRIX* in) {
    D3DMATRIX temp;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp.m(i, j) = in->m(j, i);
        }
    }
    
    *out = temp;
}

// Other state management methods (texture, light, material) would be implemented similarly...
// For brevity, I'm including placeholders for the remaining methods

void StateManager::set_texture_stage_state(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (stage >= 8) return;
    
    switch (type) {
        case D3DTSS_COLOROP:
            render_state_.color_op[stage] = value;
            break;
        case D3DTSS_COLORARG1:
            render_state_.color_arg1[stage] = value;
            break;
        case D3DTSS_COLORARG2:
            render_state_.color_arg2[stage] = value;
            break;
        case D3DTSS_ALPHAOP:
            render_state_.alpha_op[stage] = value;
            break;
        case D3DTSS_ALPHAARG1:
            render_state_.alpha_arg1[stage] = value;
            break;
        case D3DTSS_ALPHAARG2:
            render_state_.alpha_arg2[stage] = value;
            break;
        case D3DTSS_MAGFILTER:
            // Texture magnification filter - handled when texture is bound
            render_state_.mag_filter[stage] = value;
            break;
        case D3DTSS_MINFILTER:
            // Texture minification filter - handled when texture is bound
            render_state_.min_filter[stage] = value;
            break;
        case D3DTSS_MIPFILTER:
            // Mipmap filter - handled when texture is bound
            render_state_.mip_filter[stage] = value;
            break;
        case D3DTSS_ADDRESSU:
            // Texture U address mode
            render_state_.address_u[stage] = value;
            break;
        case D3DTSS_ADDRESSV:
            // Texture V address mode
            render_state_.address_v[stage] = value;
            break;
        case D3DTSS_ADDRESSW:
            // Texture W address mode (for 3D textures)
            render_state_.address_w[stage] = value;
            break;
        case D3DTSS_BUMPENVMAT00:
            render_state_.bump_env_mat[stage][0] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_BUMPENVMAT01:
            render_state_.bump_env_mat[stage][1] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_BUMPENVMAT10:
            render_state_.bump_env_mat[stage][2] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_BUMPENVMAT11:
            render_state_.bump_env_mat[stage][3] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_TEXCOORDINDEX:
            render_state_.texcoord_index[stage] = value;
            break;
        case D3DTSS_BORDERCOLOR:
            render_state_.border_color[stage] = value;
            break;
        case D3DTSS_MIPMAPLODBIAS:
            render_state_.mipmap_lod_bias[stage] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_MAXMIPLEVEL:
            render_state_.max_mip_level[stage] = value;
            break;
        case D3DTSS_MAXANISOTROPY:
            render_state_.max_anisotropy[stage] = value;
            break;
        case D3DTSS_BUMPENVLSCALE:
            render_state_.bump_env_lscale[stage] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_BUMPENVLOFFSET:
            render_state_.bump_env_loffset[stage] = *reinterpret_cast<const float*>(&value);
            break;
        case D3DTSS_TEXTURETRANSFORMFLAGS:
            render_state_.texture_transform_flags[stage] = value;
            break;
        case D3DTSS_COLORARG0:
            render_state_.color_arg0[stage] = value;
            break;
        case D3DTSS_ALPHAARG0:
            render_state_.alpha_arg0[stage] = value;
            break;
        case D3DTSS_RESULTARG:
            render_state_.result_arg[stage] = value;
            break;
        default:
            DX8GL_WARN("Unhandled texture stage state: %d", type);
            break;
    }
    
    texture_state_dirty_ = true;
}

DWORD StateManager::get_texture_stage_state(DWORD stage, D3DTEXTURESTAGESTATETYPE type) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (stage >= 8) return 0;
    
    switch (type) {
        case D3DTSS_COLOROP: return render_state_.color_op[stage];
        case D3DTSS_COLORARG1: return render_state_.color_arg1[stage];
        case D3DTSS_COLORARG2: return render_state_.color_arg2[stage];
        case D3DTSS_ALPHAOP: return render_state_.alpha_op[stage];
        case D3DTSS_ALPHAARG1: return render_state_.alpha_arg1[stage];
        case D3DTSS_ALPHAARG2: return render_state_.alpha_arg2[stage];
        case D3DTSS_MAGFILTER: return render_state_.mag_filter[stage];
        case D3DTSS_MINFILTER: return render_state_.min_filter[stage];
        case D3DTSS_MIPFILTER: return render_state_.mip_filter[stage];
        case D3DTSS_ADDRESSU: return render_state_.address_u[stage];
        case D3DTSS_ADDRESSV: return render_state_.address_v[stage];
        case D3DTSS_ADDRESSW: return render_state_.address_w[stage];
        case D3DTSS_BUMPENVMAT00: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_mat[stage][0]);
        case D3DTSS_BUMPENVMAT01: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_mat[stage][1]);
        case D3DTSS_BUMPENVMAT10: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_mat[stage][2]);
        case D3DTSS_BUMPENVMAT11: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_mat[stage][3]);
        case D3DTSS_TEXCOORDINDEX: return render_state_.texcoord_index[stage];
        case D3DTSS_BORDERCOLOR: return render_state_.border_color[stage];
        case D3DTSS_MIPMAPLODBIAS: return *reinterpret_cast<const DWORD*>(&render_state_.mipmap_lod_bias[stage]);
        case D3DTSS_MAXMIPLEVEL: return render_state_.max_mip_level[stage];
        case D3DTSS_MAXANISOTROPY: return render_state_.max_anisotropy[stage];
        case D3DTSS_BUMPENVLSCALE: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_lscale[stage]);
        case D3DTSS_BUMPENVLOFFSET: return *reinterpret_cast<const DWORD*>(&render_state_.bump_env_loffset[stage]);
        case D3DTSS_TEXTURETRANSFORMFLAGS: return render_state_.texture_transform_flags[stage];
        case D3DTSS_COLORARG0: return render_state_.color_arg0[stage];
        case D3DTSS_ALPHAARG0: return render_state_.alpha_arg0[stage];
        case D3DTSS_RESULTARG: return render_state_.result_arg[stage];
        default:
            DX8GL_WARN("Unhandled texture stage state query: %d", type);
            return 0;
    }
}

void StateManager::set_light(DWORD index, const D3DLIGHT8* light) {
    if (!light || index >= MAX_LIGHTS) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    lights_[index].properties = *light;
    light_state_dirty_ = true;
}

void StateManager::get_light(DWORD index, D3DLIGHT8* light) const {
    if (!light || index >= MAX_LIGHTS) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    *light = lights_[index].properties;
}

void StateManager::light_enable(DWORD index, BOOL enable) {
    if (index >= MAX_LIGHTS) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    lights_[index].enabled = enable;
    light_state_dirty_ = true;
}

BOOL StateManager::is_light_enabled(DWORD index) const {
    if (index >= MAX_LIGHTS) return FALSE;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    return lights_[index].enabled;
}

void StateManager::set_material(const D3DMATERIAL8* material) {
    if (!material) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    material_state_.material = *material;
    material_state_.valid = true;
    material_state_dirty_ = true;
}

void StateManager::get_material(D3DMATERIAL8* material) const {
    if (!material) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    *material = material_state_.material;
}

void StateManager::set_clip_plane(DWORD index, const float* plane) {
    if (!plane || index >= MAX_CLIP_PLANES) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    memcpy(clip_planes_[index].plane, plane, sizeof(float) * 4);
}

void StateManager::get_clip_plane(DWORD index, float* plane) const {
    if (!plane || index >= MAX_CLIP_PLANES) return;
    
    std::lock_guard<std::mutex> lock(state_mutex_);
    memcpy(plane, clip_planes_[index].plane, sizeof(float) * 4);
}

bool StateManager::validate_state() const {
    // TODO: Implement state validation
    return true;
}

// Placeholder implementations for shader-dependent methods
void StateManager::apply_transform_states(ShaderProgram* shader) {
    // Will be implemented when ShaderProgram is created
}

void StateManager::apply_texture_states() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!texture_state_dirty_) return;
    
    // Apply texture states for each active texture unit
    for (DWORD stage = 0; stage < 8; stage++) {
        // Check if this stage is active
        if (render_state_.color_op[stage] == D3DTOP_DISABLE) {
            continue;
        }
        
        // Activate texture unit
        glActiveTexture(GL_TEXTURE0 + stage);
        
        // Check if a texture is actually bound to this unit
        GLint current_texture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
        
        // Only apply texture parameters if a texture is bound
        if (current_texture == 0) {
            continue;
        }
        
        // Apply texture filtering
        // Convert D3D filter modes to GL
        GLenum min_filter = GL_NEAREST;
        GLenum mag_filter = GL_NEAREST;
        
        // Magnification filter
        switch (render_state_.mag_filter[stage]) {
            case D3DTEXF_POINT:
                mag_filter = GL_NEAREST;
                break;
            case D3DTEXF_LINEAR:
                mag_filter = GL_LINEAR;
                break;
            default:
                mag_filter = GL_LINEAR;
                break;
        }
        
        // Minification filter (with mipmap consideration)
        bool has_mipmap = (render_state_.mip_filter[stage] != D3DTEXF_NONE);
        
        if (!has_mipmap) {
            switch (render_state_.min_filter[stage]) {
                case D3DTEXF_POINT:
                    min_filter = GL_NEAREST;
                    break;
                case D3DTEXF_LINEAR:
                    min_filter = GL_LINEAR;
                    break;
                default:
                    min_filter = GL_LINEAR;
                    break;
            }
        } else {
            // With mipmaps
            switch (render_state_.min_filter[stage]) {
                case D3DTEXF_POINT:
                    switch (render_state_.mip_filter[stage]) {
                        case D3DTEXF_POINT:
                            min_filter = GL_NEAREST_MIPMAP_NEAREST;
                            break;
                        case D3DTEXF_LINEAR:
                            min_filter = GL_NEAREST_MIPMAP_LINEAR;
                            break;
                    }
                    break;
                case D3DTEXF_LINEAR:
                    switch (render_state_.mip_filter[stage]) {
                        case D3DTEXF_POINT:
                            min_filter = GL_LINEAR_MIPMAP_NEAREST;
                            break;
                        case D3DTEXF_LINEAR:
                            min_filter = GL_LINEAR_MIPMAP_LINEAR;
                            break;
                    }
                    break;
            }
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
        
        // Apply texture addressing modes
        GLenum wrap_s = GL_REPEAT;
        GLenum wrap_t = GL_REPEAT;
        
        // Convert D3D addressing modes to GL
        auto convert_address_mode = [](DWORD mode) -> GLenum {
            switch (mode) {
                case D3DTADDRESS_WRAP:
                    return GL_REPEAT;
                case D3DTADDRESS_CLAMP:
                    return GL_CLAMP_TO_EDGE;
                case D3DTADDRESS_MIRROR:
                    return GL_MIRRORED_REPEAT;
                case D3DTADDRESS_BORDER:
                    // GL ES 2.0 doesn't support GL_CLAMP_TO_BORDER
                    // Fall back to clamp to edge
                    return GL_CLAMP_TO_EDGE;
                default:
                    return GL_REPEAT;
            }
        };
        
        wrap_s = convert_address_mode(render_state_.address_u[stage]);
        wrap_t = convert_address_mode(render_state_.address_v[stage]);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
        
        // Apply anisotropic filtering if supported
        if (render_state_.max_anisotropy[stage] > 1) {
            // Check for anisotropic filtering extension using modern method
            if (has_extension("GL_EXT_texture_filter_anisotropic")) {
                GLfloat max_aniso = 1.0f;
                glGetFloatv(0x84FF /* GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT */, &max_aniso);
                GLfloat aniso = std::min((float)render_state_.max_anisotropy[stage], max_aniso);
                glTexParameterf(GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY_EXT */, aniso);
            }
        }
        
        // Apply LOD bias if supported (not in core GL ES 2.0)
        // This would require an extension like GL_EXT_texture_lod_bias
    }
    
    // Reset to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    
    texture_state_dirty_ = false;
}

void StateManager::apply_light_states(ShaderProgram* shader) {
    // Will be implemented when ShaderProgram is created
}

void StateManager::apply_material_state(ShaderProgram* shader) {
    // Will be implemented when ShaderProgram is created
}

void StateManager::apply_fog_state(ShaderProgram* shader) {
    // Will be implemented when ShaderProgram is created
}

void StateManager::set_scissor_rect(const RECT& rect, BOOL enable) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (enable) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        gl_cache_.scissor_enabled = true;
    } else {
        glDisable(GL_SCISSOR_TEST);
        gl_cache_.scissor_enabled = false;
    }
}

void StateManager::clear(DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {
    // This implementation is a fallback - normally would be handled by Direct3DDevice8::Clear
    
    GLbitfield clear_mask = 0;
    
    if (flags & D3DCLEAR_TARGET) {
        // Extract color components
        float r = ((color >> 16) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = (color & 0xFF) / 255.0f;
        float a = ((color >> 24) & 0xFF) / 255.0f;
        
        glClearColor(r, g, b, a);
        clear_mask |= GL_COLOR_BUFFER_BIT;
    }
    
    if (flags & D3DCLEAR_ZBUFFER) {
        glClearDepthf(z);
        clear_mask |= GL_DEPTH_BUFFER_BIT;
    }
    
    if (flags & D3DCLEAR_STENCIL) {
        glClearStencil(stencil);
        clear_mask |= GL_STENCIL_BUFFER_BIT;
    }
    
    if (count == 0 || rects == nullptr) {
        // Clear entire viewport
        if (clear_mask) {
            glClear(clear_mask);
            // Force synchronization to prevent Mesa fence crash
            glFinish();
        }
    } else {
        // Clear specific rectangles using scissor test
        GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
        if (!scissor_was_enabled) {
            glEnable(GL_SCISSOR_TEST);
        }
        
        for (DWORD i = 0; i < count; i++) {
            const D3DRECT& rect = rects[i];
            glScissor(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1);
            
            if (clear_mask) {
                glClear(clear_mask);
            }
        }
        
        if (!scissor_was_enabled) {
            glDisable(GL_SCISSOR_TEST);
        }
        
        // Force synchronization to prevent Mesa fence crash
        if (clear_mask) {
            glFinish();
        }
    }
}

void StateManager::set_texture_enabled(DWORD stage, bool enabled) {
    if (stage >= 8) return;
    
    // Track texture enabled state per stage
    // This is used to determine which stages are active for shader generation
    if (enabled) {
        render_state_.color_op[stage] = D3DTOP_MODULATE;  // Default when texture is bound
    } else {
        render_state_.color_op[stage] = D3DTOP_DISABLE;   // Disable stage when no texture
    }
    texture_state_dirty_ = true;
}

bool StateManager::is_texture_enabled(DWORD stage) const {
    if (stage >= 8) return false;
    return render_state_.color_op[stage] != D3DTOP_DISABLE;
}

void StateManager::apply_shader_state() {
    // Apply all state changes needed before drawing
    apply_render_states();
    apply_transform_states(nullptr);  // TODO: Pass actual shader program
    apply_texture_states();
}

} // namespace dx8gl