#include "command_buffer.h"
#include "state_manager.h"
#include "d3d8_vertexbuffer.h"
#include "d3d8_indexbuffer.h"
#include "d3d8_texture.h"
#include "fixed_function_shader.h"
#include "fvf_utils.h"
#include "vertex_shader_manager.h"
#include "pixel_shader_manager.h"
#include "shader_program_manager.h"
#include "vao_manager.h"
#include "osmesa_gl_loader.h"
#include "gl_error_check.h"
#include "blue_screen.h"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Include OpenGL headers - order is important!
#ifdef DX8GL_HAS_OSMESA
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/osmesa.h>
// MUST include osmesa_gl_loader.h AFTER GL headers to override function definitions
#include "osmesa_gl_loader.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace dx8gl {

// Helper function to populate FixedFunctionState with bump mapping information
static void populate_fixed_function_state(FixedFunctionState& ff_state, const StateManager& state_manager, DWORD vertex_format) {
    ff_state.lighting_enabled = state_manager.get_render_state(D3DRS_LIGHTING) != 0;
    ff_state.alpha_test_enabled = state_manager.get_render_state(D3DRS_ALPHATESTENABLE) != 0;
    ff_state.fog_enabled = state_manager.get_render_state(D3DRS_FOGENABLE) != 0;
    ff_state.vertex_format = vertex_format;
    
    // Count active lights for shader generation
    ff_state.num_active_lights = 0;
    if (ff_state.lighting_enabled) {
        for (int i = 0; i < 8; i++) {
            if (state_manager.is_light_enabled(i)) {
                ff_state.num_active_lights++;
            }
        }
    }
    
    // Initialize texture operations and bump mapping state
    for (int i = 0; i < 8; i++) {
        ff_state.texture_enabled[i] = state_manager.is_texture_enabled(i);
        ff_state.color_op[i] = state_manager.get_texture_stage_state(i, D3DTSS_COLOROP);
        ff_state.alpha_op[i] = state_manager.get_texture_stage_state(i, D3DTSS_ALPHAOP);
        
        // Copy bump mapping parameters from state manager
        DWORD mat00 = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT00);
        DWORD mat01 = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT01);
        DWORD mat10 = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT10);
        DWORD mat11 = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT11);
        DWORD lscale = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVLSCALE);
        DWORD loffset = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVLOFFSET);
        
        ff_state.bump_env_mat[i][0] = *reinterpret_cast<const float*>(&mat00);
        ff_state.bump_env_mat[i][1] = *reinterpret_cast<const float*>(&mat01);
        ff_state.bump_env_mat[i][2] = *reinterpret_cast<const float*>(&mat10);
        ff_state.bump_env_mat[i][3] = *reinterpret_cast<const float*>(&mat11);
        ff_state.bump_env_lscale[i] = *reinterpret_cast<const float*>(&lscale);
        ff_state.bump_env_loffset[i] = *reinterpret_cast<const float*>(&loffset);
    }
}

// Helper function to set bump mapping uniforms
static void set_bump_mapping_uniforms(const FixedFunctionShader::UniformLocations* uniforms, const StateManager& state_manager) {
    for (int i = 0; i < 8; i++) {
        if (uniforms->bump_env_mat[i] >= 0) {
            // Get bump mapping parameters from state manager
            DWORD mat00_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT00);
            DWORD mat01_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT01);
            DWORD mat10_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT10);
            DWORD mat11_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVMAT11);
            float mat00 = *reinterpret_cast<const float*>(&mat00_dw);
            float mat01 = *reinterpret_cast<const float*>(&mat01_dw);
            float mat10 = *reinterpret_cast<const float*>(&mat10_dw);
            float mat11 = *reinterpret_cast<const float*>(&mat11_dw);
            glUniform4f(uniforms->bump_env_mat[i], mat00, mat01, mat10, mat11);
        }
        
        if (uniforms->bump_env_lscale[i] >= 0) {
            DWORD lscale_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVLSCALE);
            float lscale = *reinterpret_cast<const float*>(&lscale_dw);
            glUniform1f(uniforms->bump_env_lscale[i], lscale);
        }
        
        if (uniforms->bump_env_loffset[i] >= 0) {
            DWORD loffset_dw = state_manager.get_texture_stage_state(i, D3DTSS_BUMPENVLOFFSET);
            float loffset = *reinterpret_cast<const float*>(&loffset_dw);
            glUniform1f(uniforms->bump_env_loffset[i], loffset);
        }
    }
}

CommandBuffer::CommandBuffer(size_t initial_size)
    : write_pos_(0)
    , command_count_(0) {
    buffer_.reserve(initial_size);
}

CommandBuffer::~CommandBuffer() = default;

void CommandBuffer::reset() {
    write_pos_ = 0;
    command_count_ = 0;
}

void CommandBuffer::ensure_space(size_t size) {
    if (write_pos_ + size > buffer_.size()) {
        // Grow buffer by at least 50% or to fit the required size
        size_t new_size = std::max(buffer_.size() * 3 / 2, write_pos_ + size);
        buffer_.resize(new_size);
        DX8GL_TRACE("Command buffer grew to %zu bytes", new_size);
    }
}

void CommandBuffer::execute(StateManager& state_manager,
                           VertexShaderManager* vertex_shader_mgr,
                           PixelShaderManager* pixel_shader_mgr,
                           ShaderProgramManager* shader_program_mgr) {
    if (empty()) {
        return;
    }
    
    DX8GL_TRACE("Executing command buffer with %zu commands (%zu bytes)", 
                command_count_, write_pos_);
    
    // Log command buffer to file
    static int frame_number = 0;
    static bool enable_logging = true;
    
    if (enable_logging && frame_number < 10) {
        char filename[256];
        snprintf(filename, sizeof(filename), "dx8gl_commands_frame_%04d.txt", frame_number);
        FILE* log_file = fopen(filename, "w");
        
        if (log_file) {
            fprintf(log_file, "=== DX8GL Command Buffer Frame %d ===\n", frame_number);
            fprintf(log_file, "Buffer size: %zu bytes, Command count: %zu\n\n", write_pos_, command_count_);
            
            const uint8_t* log_ptr = buffer_.data();
            const uint8_t* log_end = buffer_.data() + write_pos_;
            size_t log_cmd_index = 0;
            
            while (log_ptr < log_end) {
                const Command* log_cmd = reinterpret_cast<const Command*>(log_ptr);
                
                fprintf(log_file, "[%03zu] ", log_cmd_index);
                
                switch (log_cmd->type) {
                    case CommandType::CLEAR: {
                        const ClearCmd* clear = static_cast<const ClearCmd*>(log_cmd);
                        fprintf(log_file, "CLEAR: flags=0x%08X color=0x%08X z=%.3f stencil=%u\n",
                                clear->flags, clear->color, clear->z, clear->stencil);
                        fprintf(log_file, "      → glClear(");
                        bool first = true;
                        if (clear->flags & D3DCLEAR_TARGET) {
                            fprintf(log_file, "GL_COLOR_BUFFER_BIT");
                            first = false;
                        }
                        if (clear->flags & D3DCLEAR_ZBUFFER) {
                            if (!first) fprintf(log_file, " | ");
                            fprintf(log_file, "GL_DEPTH_BUFFER_BIT");
                            first = false;
                        }
                        if (clear->flags & D3DCLEAR_STENCIL) {
                            if (!first) fprintf(log_file, " | ");
                            fprintf(log_file, "GL_STENCIL_BUFFER_BIT");
                        }
                        fprintf(log_file, ")\n");
                        break;
                    }
                    
                    case CommandType::SET_RENDER_STATE: {
                        const SetRenderStateCmd* rs = static_cast<const SetRenderStateCmd*>(log_cmd);
                        fprintf(log_file, "SET_RENDER_STATE: state=%u value=%u\n", rs->state, rs->value);
                        break;
                    }
                    
                    case CommandType::SET_TRANSFORM: {
                        const SetTransformCmd* transform = static_cast<const SetTransformCmd*>(log_cmd);
                        const char* transform_name = "UNKNOWN";
                        switch (transform->state) {
                            case D3DTS_WORLD: transform_name = "WORLD"; break;
                            case D3DTS_VIEW: transform_name = "VIEW"; break;
                            case D3DTS_PROJECTION: transform_name = "PROJECTION"; break;
                            default: break;
                        }
                        fprintf(log_file, "SET_TRANSFORM: %s\n", transform_name);
                        fprintf(log_file, "      → glUniformMatrix4fv(u_%s)\n", transform_name);
                        fprintf(log_file, "        Matrix: [%.3f %.3f %.3f %.3f]\n", 
                                transform->matrix._11, transform->matrix._12, transform->matrix._13, transform->matrix._14);
                        fprintf(log_file, "                [%.3f %.3f %.3f %.3f]\n",
                                transform->matrix._21, transform->matrix._22, transform->matrix._23, transform->matrix._24);
                        fprintf(log_file, "                [%.3f %.3f %.3f %.3f]\n",
                                transform->matrix._31, transform->matrix._32, transform->matrix._33, transform->matrix._34);
                        fprintf(log_file, "                [%.3f %.3f %.3f %.3f]\n",
                                transform->matrix._41, transform->matrix._42, transform->matrix._43, transform->matrix._44);
                        break;
                    }
                    
                    case CommandType::SET_STREAM_SOURCE: {
                        const SetStreamSourceCmd* stream = static_cast<const SetStreamSourceCmd*>(log_cmd);
                        fprintf(log_file, "SET_STREAM_SOURCE: stream=%u buffer=0x%lX stride=%u\n",
                                stream->stream, stream->vertex_buffer, stream->stride);
                        fprintf(log_file, "      → glBindBuffer(GL_ARRAY_BUFFER, ...)\n");
                        break;
                    }
                    
                    case CommandType::SET_INDICES: {
                        const SetIndicesCmd* indices = static_cast<const SetIndicesCmd*>(log_cmd);
                        fprintf(log_file, "SET_INDICES: buffer=0x%lX base_vertex=%u\n",
                                indices->index_buffer, indices->base_vertex_index);
                        fprintf(log_file, "      → glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)\n");
                        break;
                    }
                    
                    case CommandType::DRAW_INDEXED_PRIMITIVE: {
                        const DrawIndexedPrimitiveCmd* draw = static_cast<const DrawIndexedPrimitiveCmd*>(log_cmd);
                        fprintf(log_file, "DRAW_INDEXED_PRIMITIVE: type=%d min_idx=%u num_verts=%u start_idx=%u prim_count=%u\n",
                                draw->primitive_type, draw->min_index, draw->num_vertices, 
                                draw->start_index, draw->primitive_count);
                        fprintf(log_file, "      → glDrawElements(GL_TRIANGLES, %u, GL_UNSIGNED_SHORT, ...)\n",
                                draw->primitive_count * 3);
                        break;
                    }
                    
                    case CommandType::SET_VIEWPORT: {
                        const SetViewportCmd* vp = static_cast<const SetViewportCmd*>(log_cmd);
                        fprintf(log_file, "SET_VIEWPORT: x=%u y=%u w=%u h=%u minZ=%.3f maxZ=%.3f\n",
                                vp->viewport.X, vp->viewport.Y, vp->viewport.Width, vp->viewport.Height,
                                vp->viewport.MinZ, vp->viewport.MaxZ);
                        fprintf(log_file, "      → glViewport(%u, %u, %u, %u)\n",
                                vp->viewport.X, vp->viewport.Y, vp->viewport.Width, vp->viewport.Height);
                        break;
                    }
                    
                    case CommandType::DRAW_PRIMITIVE_UP: {
                        const DrawPrimitiveUPCmd* draw = static_cast<const DrawPrimitiveUPCmd*>(log_cmd);
                        const uint8_t* vertex_data = log_ptr + sizeof(DrawPrimitiveUPCmd);
                        
                        const char* prim_type_str = "UNKNOWN";
                        const char* gl_mode_str = "GL_TRIANGLES";
                        UINT vertex_count = 0;
                        
                        switch (draw->primitive_type) {
                            case D3DPT_POINTLIST: 
                                prim_type_str = "POINTLIST"; 
                                gl_mode_str = "GL_POINTS";
                                vertex_count = draw->primitive_count; 
                                break;
                            case D3DPT_LINELIST: 
                                prim_type_str = "LINELIST"; 
                                gl_mode_str = "GL_LINES";
                                vertex_count = draw->primitive_count * 2; 
                                break;
                            case D3DPT_LINESTRIP: 
                                prim_type_str = "LINESTRIP"; 
                                gl_mode_str = "GL_LINE_STRIP";
                                vertex_count = draw->primitive_count + 1; 
                                break;
                            case D3DPT_TRIANGLELIST: 
                                prim_type_str = "TRIANGLELIST"; 
                                gl_mode_str = "GL_TRIANGLES";
                                vertex_count = draw->primitive_count * 3; 
                                break;
                            case D3DPT_TRIANGLESTRIP: 
                                prim_type_str = "TRIANGLESTRIP"; 
                                gl_mode_str = "GL_TRIANGLE_STRIP";
                                vertex_count = draw->primitive_count + 2; 
                                break;
                            case D3DPT_TRIANGLEFAN: 
                                prim_type_str = "TRIANGLEFAN"; 
                                gl_mode_str = "GL_TRIANGLE_FAN";
                                vertex_count = draw->primitive_count + 2; 
                                break;
                        }
                        
                        fprintf(log_file, "DRAW_PRIMITIVE_UP: type=%s prim_count=%u stride=%u vertices=%u fvf=0x%04X\n",
                                prim_type_str, draw->primitive_count, draw->vertex_stride, vertex_count, draw->fvf);
                        fprintf(log_file, "      → glDrawArrays(%s, 0, %u) [%s vertices]\n", gl_mode_str, vertex_count,
                                (draw->fvf & D3DFVF_XYZRHW) ? "XYZRHW" : "XYZ");
                        
                        // Log first vertex for HUD debugging
                        if (vertex_count > 0 && draw->vertex_stride >= 28) { // HUD vertex format
                            const float* pos = (const float*)vertex_data;
                            const uint32_t* color = (const uint32_t*)(vertex_data + 16); // After XYZRHW
                            const float* uv = (const float*)(vertex_data + 20); // After color
                            fprintf(log_file, "      First vertex: pos(%.1f,%.1f,%.2f,%.2f) color(0x%08x) uv(%.3f,%.3f)\n",
                                    pos[0], pos[1], pos[2], pos[3], *color, uv[0], uv[1]);
                        }
                        break;
                    }
                    
                    case CommandType::DRAW_INDEXED_PRIMITIVE_UP: {
                        const auto* draw = static_cast<const DrawIndexedPrimitiveUPCmd*>(log_cmd);
                        const char* prim_type_str = "UNKNOWN";
                        
                        // Calculate index count
                        UINT index_count = 0;
                        const char* gl_mode_str = "GL_TRIANGLES";
                        switch (draw->primitive_type) {
                            case D3DPT_POINTLIST:
                                prim_type_str = "POINTLIST";
                                index_count = draw->primitive_count;
                                gl_mode_str = "GL_POINTS";
                                break;
                            case D3DPT_LINELIST:
                                prim_type_str = "LINELIST";
                                index_count = draw->primitive_count * 2;
                                gl_mode_str = "GL_LINES";
                                break;
                            case D3DPT_LINESTRIP:
                                prim_type_str = "LINESTRIP";
                                index_count = draw->primitive_count + 1;
                                gl_mode_str = "GL_LINE_STRIP";
                                break;
                            case D3DPT_TRIANGLELIST:
                                prim_type_str = "TRIANGLELIST";
                                index_count = draw->primitive_count * 3;
                                gl_mode_str = "GL_TRIANGLES";
                                break;
                            case D3DPT_TRIANGLESTRIP:
                                prim_type_str = "TRIANGLESTRIP";
                                index_count = draw->primitive_count + 2;
                                gl_mode_str = "GL_TRIANGLE_STRIP";
                                break;
                            case D3DPT_TRIANGLEFAN:
                                prim_type_str = "TRIANGLEFAN";
                                index_count = draw->primitive_count + 2;
                                gl_mode_str = "GL_TRIANGLE_FAN";
                                break;
                        }
                        
                        fprintf(log_file, "DRAW_INDEXED_PRIMITIVE_UP: type=%s min_idx=%u num_verts=%u prim_count=%u stride=%u indices=%u fvf=0x%04X\n",
                                prim_type_str, draw->min_vertex_index, draw->num_vertices, draw->primitive_count, 
                                draw->vertex_stride, index_count, draw->fvf);
                        fprintf(log_file, "      → glDrawElements(%s, %u, %s, ...) [%s vertices]\n", 
                                gl_mode_str, index_count, 
                                (draw->index_format == D3DFMT_INDEX16) ? "GL_UNSIGNED_SHORT" : "GL_UNSIGNED_INT",
                                (draw->fvf & D3DFVF_XYZRHW) ? "XYZRHW" : "XYZ");
                        
                        // Show first vertex data if available
                        const uint8_t* data_start = reinterpret_cast<const uint8_t*>(log_cmd) + sizeof(DrawIndexedPrimitiveUPCmd);
                        size_t index_size = (draw->index_format == D3DFMT_INDEX16) ? 2 : 4;
                        size_t index_data_size = index_count * index_size;
                        const uint8_t* vertex_data = data_start + index_data_size;
                        
                        if (draw->num_vertices > 0 && draw->vertex_stride >= 16) {
                            const float* pos = (const float*)vertex_data;
                            if (draw->fvf & D3DFVF_XYZRHW) {
                                fprintf(log_file, "      First vertex: pos(%.1f,%.1f,%.2f,%.2f)", 
                                        pos[0], pos[1], pos[2], pos[3]);
                            } else {
                                fprintf(log_file, "      First vertex: pos(%.1f,%.1f,%.1f)", 
                                        pos[0], pos[1], pos[2]);
                            }
                            
                            // Add color if present
                            if (draw->fvf & D3DFVF_DIFFUSE) {
                                size_t color_offset = (draw->fvf & D3DFVF_XYZRHW) ? 16 : 12;
                                if (draw->fvf & D3DFVF_NORMAL) color_offset += 12;
                                const uint32_t* color = (const uint32_t*)(vertex_data + color_offset);
                                fprintf(log_file, " color(0x%08x)", *color);
                            }
                            
                            fprintf(log_file, "\n");
                        }
                        break;
                    }
                    
                    default:
                        fprintf(log_file, "CommandType %d (size=%u)\n", (int)log_cmd->type, log_cmd->size);
                        break;
                }
                
                log_ptr += log_cmd->size;
                log_cmd_index++;
            }
            
            fprintf(log_file, "\n=== End of Command Buffer ===\n");
            fclose(log_file);
            
            std::cout << "Saved command buffer to " << filename << std::endl;
        }
        
        frame_number++;
    }
    
    // OSMesa context is always current
    
    // Create fixed function shader if needed
    static std::unique_ptr<FixedFunctionShader> ff_shader;
    if (!ff_shader) {
        ff_shader = std::make_unique<FixedFunctionShader>();
    }
    
    // Track current bindings
    struct StreamSource {
        Direct3DVertexBuffer8* vb = nullptr;
        UINT stride = 0;
    };
    StreamSource stream_sources[16] = {};  // D3D8 supports up to 16 streams
    Direct3DIndexBuffer8* current_ib = nullptr;
    UINT base_vertex_index = 0;
    
    const uint8_t* read_ptr = buffer_.data();
    const uint8_t* end_ptr = buffer_.data() + write_pos_;
    
    while (read_ptr < end_ptr) {
        const Command* cmd = reinterpret_cast<const Command*>(read_ptr);
        
        switch (cmd->type) {
            case CommandType::SET_RENDER_STATE: {
                const auto* rs_cmd = static_cast<const SetRenderStateCmd*>(cmd);
                state_manager.set_render_state(rs_cmd->state, rs_cmd->value);
                break;
            }
            
            case CommandType::SET_TEXTURE_STAGE_STATE: {
                const auto* tss_cmd = static_cast<const SetTextureStageStateCmd*>(cmd);
                state_manager.set_texture_stage_state(tss_cmd->stage, tss_cmd->type, tss_cmd->value);
                break;
            }
            
            case CommandType::SET_TRANSFORM: {
                const auto* t_cmd = static_cast<const SetTransformCmd*>(cmd);
                state_manager.set_transform(t_cmd->state, &t_cmd->matrix);
                break;
            }
            
            case CommandType::SET_MATERIAL: {
                const auto* m_cmd = static_cast<const SetMaterialCmd*>(cmd);
                state_manager.set_material(&m_cmd->material);
                break;
            }
            
            case CommandType::SET_LIGHT: {
                const auto* l_cmd = static_cast<const SetLightCmd*>(cmd);
                state_manager.set_light(l_cmd->index, &l_cmd->light);
                state_manager.light_enable(l_cmd->index, l_cmd->enable);
                break;
            }
            
            case CommandType::SET_VIEWPORT: {
                const auto* v_cmd = static_cast<const SetViewportCmd*>(cmd);
                state_manager.set_viewport(&v_cmd->viewport);
                break;
            }
            
            case CommandType::SET_SCISSOR_RECT: {
                const auto* s_cmd = static_cast<const SetScissorRectCmd*>(cmd);
                state_manager.set_scissor_rect(s_cmd->rect, s_cmd->enable);
                break;
            }
            
            case CommandType::SET_TEXTURE: {
                const auto* t_cmd = static_cast<const SetTextureCmd*>(cmd);
                DX8GL_INFO("EXECUTE: SET_TEXTURE stage=%u texture=%p", t_cmd->stage, 
                           reinterpret_cast<void*>(t_cmd->texture));
                
                // Bind texture to the specified stage
                if (t_cmd->stage < 8) {  // D3D8 supports up to 8 texture stages
                    glActiveTexture(GL_TEXTURE0 + t_cmd->stage);
                    
                    if (t_cmd->texture) {
                        // Cast back to texture object and bind
                        auto* texture = reinterpret_cast<Direct3DTexture8*>(t_cmd->texture);
                        GLuint gl_texture = texture->get_gl_texture();
                        glBindTexture(GL_TEXTURE_2D, gl_texture);
                        
                        // Update state manager to track bound texture
                        state_manager.set_texture_enabled(t_cmd->stage, true);
                        
                        // Apply texture states (filtering, addressing) for this texture
                        state_manager.apply_texture_states();
                    } else {
                        // Unbind texture
                        glBindTexture(GL_TEXTURE_2D, 0);
                        state_manager.set_texture_enabled(t_cmd->stage, false);
                    }
                }
                break;
            }
            
            case CommandType::SET_STREAM_SOURCE: {
                const auto* ss_cmd = static_cast<const SetStreamSourceCmd*>(cmd);
                if (ss_cmd->stream < 16) {
                    stream_sources[ss_cmd->stream].vb = 
                        reinterpret_cast<Direct3DVertexBuffer8*>(ss_cmd->vertex_buffer);
                    stream_sources[ss_cmd->stream].stride = ss_cmd->stride;
                    DX8GL_TRACE("SET_STREAM_SOURCE stream=%u vb=%p stride=%u", 
                               ss_cmd->stream, reinterpret_cast<void*>(ss_cmd->vertex_buffer), 
                               ss_cmd->stride);
                } else {
                    DX8GL_WARNING("Invalid stream number %u", ss_cmd->stream);
                }
                break;
            }
            
            case CommandType::SET_INDICES: {
                const auto* i_cmd = static_cast<const SetIndicesCmd*>(cmd);
                current_ib = reinterpret_cast<Direct3DIndexBuffer8*>(i_cmd->index_buffer);
                base_vertex_index = i_cmd->base_vertex_index;
                DX8GL_TRACE("SET_INDICES ib=%p base=%u", 
                           reinterpret_cast<void*>(i_cmd->index_buffer), 
                           i_cmd->base_vertex_index);
                break;
            }
            
            case CommandType::DRAW_PRIMITIVE: {
                const auto* dp_cmd = static_cast<const DrawPrimitiveCmd*>(cmd);
                DX8GL_INFO("DRAW_PRIMITIVE type=%d start=%u count=%u", 
                           dp_cmd->primitive_type, dp_cmd->start_vertex, 
                           dp_cmd->primitive_count);
                
                if (!stream_sources[0].vb) {
                    DX8GL_ERROR("DRAW_PRIMITIVE: No vertex buffer bound to stream 0");
                    break;
                }
                
                // Apply current state and generate shader
                state_manager.apply_render_states();
                
                GLuint program = 0;
                
                // Check if we're using vertex/pixel shaders
                bool using_vertex_shader = vertex_shader_mgr && vertex_shader_mgr->is_using_vertex_shader();
                bool has_shader_program_mgr = shader_program_mgr != nullptr;
                
                DX8GL_INFO("Shader check: vertex_shader_mgr=%p, using_vertex_shader=%d, shader_program_mgr=%p", 
                          vertex_shader_mgr, using_vertex_shader, shader_program_mgr);
                
                if (using_vertex_shader && has_shader_program_mgr) {
                    // Use the shader program manager to get the combined VS/PS program
                    DX8GL_INFO("Using ShaderProgramManager for rendering");
                    shader_program_mgr->apply_shader_state();
                    program = shader_program_mgr->get_current_program();
                    if (!program) {
                        DX8GL_ERROR("Failed to get shader program from ShaderProgramManager");
                        break;
                    }
                    DX8GL_INFO("Got shader program %u from ShaderProgramManager", program);
                } else {
                    // Use fixed function pipeline
                    FixedFunctionState ff_state;
                    populate_fixed_function_state(ff_state, state_manager, stream_sources[0].vb->get_fvf());
                    
                    program = ff_shader->get_program(ff_state);
                    if (!program) {
                        DX8GL_ERROR("Failed to get shader program");
                        break;
                    }
                    
                    glUseProgram(program);
                }
                
                // Set uniforms based on shader type
                if (!vertex_shader_mgr || !vertex_shader_mgr->is_using_vertex_shader()) {
                    // Fixed function pipeline - set uniforms
                    const auto* uniforms = ff_shader->get_uniform_locations(program);
                    if (!uniforms) {
                        DX8GL_ERROR("Failed to get uniform locations");
                        break;
                    }
                    DX8GL_INFO("Got uniform locations for program %u", program);
                    
                    // Set matrices
                    D3DMATRIX world, view, proj;
                    state_manager.get_transform(D3DTS_WORLD, &world);
                    state_manager.get_transform(D3DTS_VIEW, &view);
                    state_manager.get_transform(D3DTS_PROJECTION, &proj);
                    
                    if (uniforms->world_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->world_matrix, 1, GL_TRUE, (const GLfloat*)&world);
                    }
                    if (uniforms->view_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->view_matrix, 1, GL_TRUE, (const GLfloat*)&view);
                    }
                    if (uniforms->projection_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->projection_matrix, 1, GL_TRUE, (const GLfloat*)&proj);
                    }
                    
                    // Calculate and set worldViewProj matrix
                    DX8GL_INFO("worldViewProj uniform location: %d", uniforms->world_view_proj_matrix);
                    if (uniforms->world_view_proj_matrix >= 0) {
                        // Get the combined world-view-projection matrix from state manager
                        const D3DMATRIX* wvp = state_manager.get_world_view_projection_matrix();
                        if (wvp) {
                            DX8GL_INFO("Setting worldViewProj matrix: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
                                       wvp->_11, wvp->_12, wvp->_13, wvp->_14,
                                       wvp->_21, wvp->_22, wvp->_23, wvp->_24,
                                       wvp->_31, wvp->_32, wvp->_33, wvp->_34,
                                       wvp->_41, wvp->_42, wvp->_43, wvp->_44);
                            glUniformMatrix4fv(uniforms->world_view_proj_matrix, 1, GL_TRUE, (const GLfloat*)wvp);
                        }
                    } else {
                        DX8GL_INFO("worldViewProj uniform not found in shader!");
                    }
                    
                    // Set viewport size uniform for XYZRHW coordinate conversion
                    if (uniforms->viewport_size >= 0) {
                        D3DVIEWPORT8 viewport;
                        state_manager.get_viewport(&viewport);
                        glUniform2f(uniforms->viewport_size, static_cast<float>(viewport.Width), static_cast<float>(viewport.Height));
                        DX8GL_INFO("Set viewport_size uniform: %dx%d", viewport.Width, viewport.Height);
                    }
                    
                    // Set lighting uniforms if lighting is enabled
                    if (state_manager.get_render_state(D3DRS_LIGHTING) != 0) {
                        // Set material uniforms
                        D3DMATERIAL8 material;
                        state_manager.get_material(&material);
                        
                        if (uniforms->material_ambient >= 0) {
                            glUniform4f(uniforms->material_ambient, material.Ambient.r, material.Ambient.g, material.Ambient.b, material.Ambient.a);
                        }
                        if (uniforms->material_diffuse >= 0) {
                            glUniform4f(uniforms->material_diffuse, material.Diffuse.r, material.Diffuse.g, material.Diffuse.b, material.Diffuse.a);
                        }
                        if (uniforms->material_specular >= 0) {
                            glUniform4f(uniforms->material_specular, material.Specular.r, material.Specular.g, material.Specular.b, material.Specular.a);
                        }
                        if (uniforms->material_emissive >= 0) {
                            glUniform4f(uniforms->material_emissive, material.Emissive.r, material.Emissive.g, material.Emissive.b, material.Emissive.a);
                        }
                        if (uniforms->material_power >= 0) {
                            glUniform1f(uniforms->material_power, material.Power);
                        }
                        
                        // Set ambient light
                        DWORD ambient = state_manager.get_render_state(D3DRS_AMBIENT);
                        float r = ((ambient >> 16) & 0xFF) / 255.0f;
                        float g = ((ambient >> 8) & 0xFF) / 255.0f;
                        float b = (ambient & 0xFF) / 255.0f;
                        float a = ((ambient >> 24) & 0xFF) / 255.0f;
                        if (uniforms->ambient_light >= 0) {
                            glUniform4f(uniforms->ambient_light, r, g, b, a);
                        }
                        
                        // Set light uniforms
                        int num_active_lights = 0;
                        for (int i = 0; i < 8; i++) {
                            if (state_manager.is_light_enabled(i)) {
                                D3DLIGHT8 light;
                                state_manager.get_light(i, &light);
                                
                                if (uniforms->light_position[num_active_lights] >= 0) {
                                    if (light.Type == D3DLIGHT_DIRECTIONAL) {
                                        // For directional lights, use negative direction as position at infinity
                                        glUniform3f(uniforms->light_position[num_active_lights], 
                                                   -light.Direction.x * 1000.0f, 
                                                   -light.Direction.y * 1000.0f, 
                                                   -light.Direction.z * 1000.0f);
                                    } else {
                                        glUniform3f(uniforms->light_position[num_active_lights], 
                                                   light.Position.x, light.Position.y, light.Position.z);
                                    }
                                }
                                
                                if (uniforms->light_diffuse[num_active_lights] >= 0) {
                                    glUniform4f(uniforms->light_diffuse[num_active_lights], 
                                               light.Diffuse.r, light.Diffuse.g, light.Diffuse.b, light.Diffuse.a);
                                }
                                
                                num_active_lights++;
                                if (num_active_lights >= 8) break;  // Max 8 lights in shader
                            }
                        }
                        
                        // Set bump mapping uniforms
                        set_bump_mapping_uniforms(uniforms, state_manager);
                    }
                } else {
                    // Vertex/pixel shader pipeline - uniforms are set by ShaderProgramManager
                    DX8GL_INFO("Using vertex/pixel shaders - uniforms already set by ShaderProgramManager");
                }
                
                // Bind vertex buffer
                stream_sources[0].vb->bind();
                
                // Setup vertex attributes based on FVF
                GLuint position_loc = 0;
                GLuint normal_loc = 1;
                GLuint color_loc = 2;
                GLuint texcoord_loc = 3;
                
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)0);
                DX8GL_INFO("Enabled position attribute at location %u, stride %u", position_loc, stream_sources[0].stride);
                
                size_t offset = 3 * sizeof(float);
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL) {
                    glEnableVertexAttribArray(normal_loc);
                    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)offset);
                    offset += 3 * sizeof(float);
                }
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_DIFFUSE) {
                    glEnableVertexAttribArray(color_loc);
                    glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stream_sources[0].stride, (void*)offset);
                    DX8GL_INFO("Enabled color attribute at location %u, offset %zu, stride %u", color_loc, offset, stream_sources[0].stride);
                    offset += sizeof(DWORD);
                }
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_TEX1) {
                    glEnableVertexAttribArray(texcoord_loc);
                    glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)offset);
                }
                
                // Convert D3D primitive type to OpenGL
                GLenum gl_mode = GL_TRIANGLES;
                GLsizei vertex_count = 0;
                
                switch (dp_cmd->primitive_type) {
                    case D3DPT_TRIANGLELIST:
                        gl_mode = GL_TRIANGLES;
                        vertex_count = dp_cmd->primitive_count * 3;
                        break;
                    case D3DPT_TRIANGLESTRIP:
                        gl_mode = GL_TRIANGLE_STRIP;
                        vertex_count = dp_cmd->primitive_count + 2;
                        break;
                    case D3DPT_TRIANGLEFAN:
                        gl_mode = GL_TRIANGLE_FAN;
                        vertex_count = dp_cmd->primitive_count + 2;
                        break;
                    default:
                        DX8GL_ERROR("Unsupported primitive type: %d", dp_cmd->primitive_type);
                        break;
                }
                
                // Draw
                DX8GL_INFO("glDrawArrays: mode=%d, start=%u, count=%d", 
                          gl_mode, dp_cmd->start_vertex, vertex_count);
                
                // Check depth test state
                GLboolean depth_test_enabled = GL_FALSE;
                glGetBooleanv(GL_DEPTH_TEST, &depth_test_enabled);
                DX8GL_INFO("Depth test %s", depth_test_enabled ? "enabled" : "disabled");
                
                glDrawArrays(gl_mode, dp_cmd->start_vertex, vertex_count);
                
                // Check for OpenGL errors
                if (check_gl_error_safe("glDrawArrays in DRAW_PRIMITIVE")) {
                    DX8GL_ERROR("OpenGL error detected after DrawArrays - attempting recovery");
                }
                
                // Unbind VAO
                glBindVertexArray(0);
                
                // Check for errors
                GLenum error = glGetError();
                if (error != GL_NO_ERROR) {
                    DX8GL_ERROR("OpenGL error in DRAW_PRIMITIVE: 0x%x", error);
                } else {
                    DX8GL_INFO("Draw completed successfully");
                }
                
                // Disable vertex attributes
                glDisableVertexAttribArray(position_loc);
                if (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL) {
                    glDisableVertexAttribArray(normal_loc);
                }
                if (stream_sources[0].vb->get_fvf() & D3DFVF_DIFFUSE) {
                    glDisableVertexAttribArray(color_loc);
                }
                if (stream_sources[0].vb->get_fvf() & D3DFVF_TEX1) {
                    glDisableVertexAttribArray(texcoord_loc);
                }
                
                break;
            }
            
            case CommandType::DRAW_INDEXED_PRIMITIVE: {
                const auto* dip_cmd = static_cast<const DrawIndexedPrimitiveCmd*>(cmd);
                DX8GL_TRACE("DRAW_INDEXED_PRIMITIVE type=%d min=%u num=%u start=%u count=%u", 
                           dip_cmd->primitive_type, dip_cmd->min_index, dip_cmd->num_vertices,
                           dip_cmd->start_index, dip_cmd->primitive_count);
                
                if (!stream_sources[0].vb || !current_ib) {
                    DX8GL_ERROR("DRAW_INDEXED_PRIMITIVE: No vertex or index buffer bound");
                    break;
                }
                
                // Apply current state and generate shader
                state_manager.apply_render_states();
                
                GLuint program = 0;
                
                // Check if we're using vertex/pixel shaders
                bool using_vertex_shader = vertex_shader_mgr && vertex_shader_mgr->is_using_vertex_shader();
                bool has_shader_program_mgr = shader_program_mgr != nullptr;
                
                DX8GL_INFO("DrawIndexedPrimitive Shader check: vertex_shader_mgr=%p, using_vertex_shader=%d, shader_program_mgr=%p", 
                          vertex_shader_mgr, using_vertex_shader, shader_program_mgr);
                
                if (using_vertex_shader && has_shader_program_mgr) {
                    // Use the shader program manager to get the combined VS/PS program
                    DX8GL_INFO("Using ShaderProgramManager for DrawIndexedPrimitive");
                    shader_program_mgr->apply_shader_state();
                    program = shader_program_mgr->get_current_program();
                    if (!program) {
                        DX8GL_ERROR("Failed to get shader program from ShaderProgramManager");
                        break;
                    }
                    DX8GL_INFO("Got shader program %u from ShaderProgramManager", program);
                } else {
                    // Use fixed function pipeline
                    DX8GL_INFO("Using fixed function pipeline for DrawIndexedPrimitive");
                    FixedFunctionState ff_state;
                    populate_fixed_function_state(ff_state, state_manager, stream_sources[0].vb->get_fvf());
                    DX8GL_INFO("EXECUTE: DrawIndexedPrimitive texture_enabled[0]=%s", 
                               ff_state.texture_enabled[0] ? "true" : "false");
                    
                    program = ff_shader->get_program(ff_state);
                    if (!program) {
                        DX8GL_ERROR("Failed to get shader program");
                        break;
                    }
                    
                    glUseProgram(program);
                }
                
                // Set uniforms based on shader type
                if (!using_vertex_shader || !has_shader_program_mgr) {
                    // Fixed function pipeline - set uniforms
                    const auto* uniforms = ff_shader->get_uniform_locations(program);
                    if (!uniforms) {
                        DX8GL_ERROR("Failed to get uniform locations");
                        break;
                    }
                    DX8GL_INFO("Got uniform locations for program %u", program);
                    
                    // Set matrices
                    D3DMATRIX world, view, proj;
                    state_manager.get_transform(D3DTS_WORLD, &world);
                    state_manager.get_transform(D3DTS_VIEW, &view);
                    state_manager.get_transform(D3DTS_PROJECTION, &proj);
                    
                    if (uniforms->world_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->world_matrix, 1, GL_TRUE, (const GLfloat*)&world);
                    }
                    if (uniforms->view_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->view_matrix, 1, GL_TRUE, (const GLfloat*)&view);
                    }
                    if (uniforms->projection_matrix >= 0) {
                        glUniformMatrix4fv(uniforms->projection_matrix, 1, GL_TRUE, (const GLfloat*)&proj);
                    }
                    
                    // Calculate and set worldViewProj matrix
                    DX8GL_INFO("worldViewProj uniform location: %d", uniforms->world_view_proj_matrix);
                    if (uniforms->world_view_proj_matrix >= 0) {
                        // Get the combined world-view-projection matrix from state manager
                        const D3DMATRIX* wvp = state_manager.get_world_view_projection_matrix();
                        if (wvp) {
                        DX8GL_INFO("Setting worldViewProj matrix: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
                                   wvp->_11, wvp->_12, wvp->_13, wvp->_14,
                                   wvp->_21, wvp->_22, wvp->_23, wvp->_24,
                                   wvp->_31, wvp->_32, wvp->_33, wvp->_34,
                                   wvp->_41, wvp->_42, wvp->_43, wvp->_44);
                        glUniformMatrix4fv(uniforms->world_view_proj_matrix, 1, GL_TRUE, (const GLfloat*)wvp);
                    }
                } else {
                    DX8GL_INFO("worldViewProj uniform not found in shader!");
                }
                
                // Set viewport size uniform for XYZRHW coordinate conversion
                if (uniforms->viewport_size >= 0) {
                    D3DVIEWPORT8 viewport;
                    state_manager.get_viewport(&viewport);
                    glUniform2f(uniforms->viewport_size, static_cast<float>(viewport.Width), static_cast<float>(viewport.Height));
                    DX8GL_INFO("Set viewport_size uniform: %dx%d", viewport.Width, viewport.Height);
                }
                
                // Calculate and set normal matrix if needed
                if (uniforms->normal_matrix >= 0 && (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL)) {
                    // Normal matrix is the transpose of the inverse of the upper 3x3 of the world matrix
                    // For now, assuming uniform scaling, we can just use the upper 3x3 of world matrix
                    glm::mat3 normalMatrix(
                        world._11, world._12, world._13,
                        world._21, world._22, world._23,
                        world._31, world._32, world._33
                    );
                    glUniformMatrix3fv(uniforms->normal_matrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
                }
                
                // Set lighting uniforms if lighting is enabled
                if (state_manager.get_render_state(D3DRS_LIGHTING) != 0 && (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL)) {
                    // Set material uniforms
                    D3DMATERIAL8 material;
                    state_manager.get_material(&material);
                    
                    if (uniforms->material_ambient >= 0) {
                        glUniform4f(uniforms->material_ambient, material.Ambient.r, material.Ambient.g, material.Ambient.b, material.Ambient.a);
                    }
                    if (uniforms->material_diffuse >= 0) {
                        glUniform4f(uniforms->material_diffuse, material.Diffuse.r, material.Diffuse.g, material.Diffuse.b, material.Diffuse.a);
                    }
                    if (uniforms->material_specular >= 0) {
                        glUniform4f(uniforms->material_specular, material.Specular.r, material.Specular.g, material.Specular.b, material.Specular.a);
                    }
                    if (uniforms->material_emissive >= 0) {
                        glUniform4f(uniforms->material_emissive, material.Emissive.r, material.Emissive.g, material.Emissive.b, material.Emissive.a);
                    }
                    if (uniforms->material_power >= 0) {
                        glUniform1f(uniforms->material_power, material.Power);
                    }
                    
                    // Set ambient light
                    DWORD ambient = state_manager.get_render_state(D3DRS_AMBIENT);
                    float r = ((ambient >> 16) & 0xFF) / 255.0f;
                    float g = ((ambient >> 8) & 0xFF) / 255.0f;
                    float b = (ambient & 0xFF) / 255.0f;
                    float a = ((ambient >> 24) & 0xFF) / 255.0f;
                    if (uniforms->ambient_light >= 0) {
                        glUniform4f(uniforms->ambient_light, r, g, b, a);
                    }
                    
                    // Set light uniforms
                    int num_active_lights = 0;
                    for (int i = 0; i < 8; i++) {
                        if (state_manager.is_light_enabled(i)) {
                            D3DLIGHT8 light;
                            state_manager.get_light(i, &light);
                            
                            if (uniforms->light_position[num_active_lights] >= 0) {
                                if (light.Type == D3DLIGHT_DIRECTIONAL) {
                                    // For directional lights, use negative direction as position at infinity
                                    glUniform3f(uniforms->light_position[num_active_lights], 
                                               -light.Direction.x * 1000.0f, 
                                               -light.Direction.y * 1000.0f, 
                                               -light.Direction.z * 1000.0f);
                                } else {
                                    glUniform3f(uniforms->light_position[num_active_lights], 
                                               light.Position.x, light.Position.y, light.Position.z);
                                }
                            }
                            
                            if (uniforms->light_diffuse[num_active_lights] >= 0) {
                                glUniform4f(uniforms->light_diffuse[num_active_lights], 
                                           light.Diffuse.r, light.Diffuse.g, light.Diffuse.b, light.Diffuse.a);
                            }
                            
                            num_active_lights++;
                            if (num_active_lights >= 8) break;  // Max 8 lights in shader
                        }
                    }
                    
                    // Set bump mapping uniforms
                    set_bump_mapping_uniforms(uniforms, state_manager);
                }
                } else {
                    // Vertex/pixel shader pipeline - uniforms are set by ShaderProgramManager
                    DX8GL_INFO("Using vertex/pixel shaders - uniforms already set by ShaderProgramManager");
                }
                
                // Create and bind a VAO for Core Profile
                GLuint vao;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);
                
                // Bind vertex buffer
                stream_sources[0].vb->bind();
                
                // Setup vertex attributes based on FVF
                GLuint position_loc = 0;
                GLuint normal_loc = 1;
                GLuint color_loc = 2;
                GLuint texcoord_loc = 3;
                
                glEnableVertexAttribArray(position_loc);
                glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)0);
                DX8GL_INFO("Enabled position attribute at location %u, stride %u", position_loc, stream_sources[0].stride);
                
                size_t offset = 3 * sizeof(float);
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL) {
                    glEnableVertexAttribArray(normal_loc);
                    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)offset);
                    offset += 3 * sizeof(float);
                }
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_DIFFUSE) {
                    glEnableVertexAttribArray(color_loc);
                    glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stream_sources[0].stride, (void*)offset);
                    DX8GL_INFO("Enabled color attribute at location %u, offset %zu, stride %u", color_loc, offset, stream_sources[0].stride);
                    offset += sizeof(DWORD);
                }
                
                if (stream_sources[0].vb->get_fvf() & D3DFVF_TEX1) {
                    glEnableVertexAttribArray(texcoord_loc);
                    glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, stream_sources[0].stride, (void*)offset);
                    DX8GL_INFO("Enabled texcoord0 attribute at location %u, offset %zu, stride %u", texcoord_loc, offset, stream_sources[0].stride);
                    offset += 2 * sizeof(float);
                }
                
                // Bind index buffer
                current_ib->bind();
                
                // Convert D3D primitive type to OpenGL
                GLenum gl_mode = GL_TRIANGLES;
                GLsizei index_count = 0;
                
                switch (dip_cmd->primitive_type) {
                    case D3DPT_TRIANGLELIST:
                        gl_mode = GL_TRIANGLES;
                        index_count = dip_cmd->primitive_count * 3;
                        break;
                    case D3DPT_TRIANGLESTRIP:
                        gl_mode = GL_TRIANGLE_STRIP;
                        index_count = dip_cmd->primitive_count + 2;
                        break;
                    case D3DPT_TRIANGLEFAN:
                        gl_mode = GL_TRIANGLE_FAN;
                        index_count = dip_cmd->primitive_count + 2;
                        break;
                    default:
                        DX8GL_ERROR("Unsupported primitive type: %d", dip_cmd->primitive_type);
                        break;
                }
                
                // Calculate index offset
                size_t index_offset = dip_cmd->start_index * current_ib->get_index_size();
                
                // Debug: Log draw call parameters
                static int draw_debug_count = 0;
                if (draw_debug_count < 5) {
                    DX8GL_INFO("DrawElements: mode=%s, count=%u, type=%s, offset=%zu",
                               gl_mode == GL_TRIANGLES ? "GL_TRIANGLES" : "OTHER",
                               index_count,
                               current_ib->get_gl_type() == GL_UNSIGNED_SHORT ? "GL_UNSIGNED_SHORT" : "GL_UNSIGNED_INT",
                               index_offset);
                    
                    // Check if vertex buffer is bound
                    GLint vbo = 0;
                    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
                    DX8GL_INFO("  Vertex buffer bound: %d", vbo);
                    
                    // Check if index buffer is bound
                    GLint ibo = 0;
                    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ibo);
                    DX8GL_INFO("  Index buffer bound: %d", ibo);
                    
                    draw_debug_count++;
                }
                
                // Draw
                DX8GL_INFO("Calling glDrawElements: mode=%s, count=%d, type=%s, offset=%zu",
                          gl_mode == GL_TRIANGLES ? "GL_TRIANGLES" : "OTHER",
                          index_count, 
                          current_ib->get_gl_type() == GL_UNSIGNED_SHORT ? "GL_UNSIGNED_SHORT" : "GL_UNSIGNED_INT",
                          index_offset);
                glDrawElements(gl_mode, index_count, current_ib->get_gl_type(), (void*)index_offset);
                
                // Check for OpenGL errors
                if (check_gl_error_safe("glDrawElements in DRAW_INDEXED_PRIMITIVE")) {
                    DX8GL_ERROR("OpenGL error detected after DrawElements - attempting recovery");
                    // Don't crash, just continue
                }
                
                // Debug: Check if anything was drawn
                {
                    static int draw_debug_count = 0;
                    if (draw_debug_count < 5) {
                        GLint viewport[4];
                        glGetIntegerv(GL_VIEWPORT, viewport);
                        DX8GL_INFO("Draw successful - viewport: %d,%d %dx%d", 
                                   viewport[0], viewport[1], viewport[2], viewport[3]);
                        
                        // Check shader program
                        GLint current_program;
                        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
                        DX8GL_INFO("Current shader program: %d", current_program);
                        
                        // Check if depth buffer is bound
                        GLint framebuffer_binding;
                        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer_binding);
                        DX8GL_INFO("Framebuffer binding: %d", framebuffer_binding);
                        
                        // Check depth test state
                        GLboolean depth_test_enabled;
                        glGetBooleanv(GL_DEPTH_TEST, &depth_test_enabled);
                        DX8GL_INFO("Depth test enabled: %s", depth_test_enabled ? "yes" : "no");
                        
                        // Check depth write mask
                        GLboolean depth_write_mask;
                        glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write_mask);
                        DX8GL_INFO("Depth write mask: %s", depth_write_mask ? "yes" : "no");
                        
                        draw_debug_count++;
                    }
                }
                
                // Disable vertex attributes
                glDisableVertexAttribArray(position_loc);
                if (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL) {
                    glDisableVertexAttribArray(normal_loc);
                }
                if (stream_sources[0].vb->get_fvf() & D3DFVF_DIFFUSE) {
                    glDisableVertexAttribArray(color_loc);
                }
                if (stream_sources[0].vb->get_fvf() & D3DFVF_TEX1) {
                    glDisableVertexAttribArray(texcoord_loc);
                }
                
                // Unbind and delete VAO
                glBindVertexArray(0);
                glDeleteVertexArrays(1, &vao);
                
                break;
            }
            
            case CommandType::DRAW_PRIMITIVE_UP: {
                const auto* dpup_cmd = static_cast<const DrawPrimitiveUPCmd*>(cmd);
                const void* vertex_data = read_ptr + sizeof(DrawPrimitiveUPCmd);
                
                // Enhanced logging for HUD debugging
                const char* prim_type_str = "UNKNOWN";
                switch (dpup_cmd->primitive_type) {
                    case D3DPT_POINTLIST: prim_type_str = "POINTLIST"; break;
                    case D3DPT_LINELIST: prim_type_str = "LINELIST"; break;
                    case D3DPT_LINESTRIP: prim_type_str = "LINESTRIP"; break;
                    case D3DPT_TRIANGLELIST: prim_type_str = "TRIANGLELIST"; break;
                    case D3DPT_TRIANGLESTRIP: prim_type_str = "TRIANGLESTRIP"; break;
                    case D3DPT_TRIANGLEFAN: prim_type_str = "TRIANGLEFAN"; break;
                }
                
                DX8GL_INFO("EXECUTE: DRAW_PRIMITIVE_UP type=%s(%d) prim_count=%u stride=%u", 
                           prim_type_str, dpup_cmd->primitive_type, dpup_cmd->primitive_count, 
                           dpup_cmd->vertex_stride);
                
                // Create a temporary VBO for immediate mode data
                GLuint temp_vbo;
                glGenBuffers(1, &temp_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, temp_vbo);
                
                // Calculate vertex count from primitive count
                UINT vertex_count = 0;
                GLenum gl_mode = GL_TRIANGLES;
                
                switch (dpup_cmd->primitive_type) {
                    case D3DPT_POINTLIST:
                        vertex_count = dpup_cmd->primitive_count;
                        gl_mode = GL_POINTS;
                        break;
                    case D3DPT_LINELIST:
                        vertex_count = dpup_cmd->primitive_count * 2;
                        gl_mode = GL_LINES;
                        break;
                    case D3DPT_LINESTRIP:
                        vertex_count = dpup_cmd->primitive_count + 1;
                        gl_mode = GL_LINE_STRIP;
                        break;
                    case D3DPT_TRIANGLELIST:
                        vertex_count = dpup_cmd->primitive_count * 3;
                        gl_mode = GL_TRIANGLES;
                        break;
                    case D3DPT_TRIANGLESTRIP:
                        vertex_count = dpup_cmd->primitive_count + 2;
                        gl_mode = GL_TRIANGLE_STRIP;
                        break;
                    case D3DPT_TRIANGLEFAN:
                        vertex_count = dpup_cmd->primitive_count + 2;
                        gl_mode = GL_TRIANGLE_FAN;
                        break;
                }
                
                // Upload vertex data
                size_t data_size = vertex_count * dpup_cmd->vertex_stride;
                DX8GL_INFO("Uploading %zu bytes of vertex data to VBO %u (count=%u, stride=%u)", 
                           data_size, temp_vbo, vertex_count, dpup_cmd->vertex_stride);
                
                // Debug: Print vertex data with enhanced HUD details
                const uint8_t* vdata = (const uint8_t*)vertex_data;
                for (UINT i = 0; i < vertex_count && i < 4; i++) {
                    const float* pos = (const float*)(vdata + i * dpup_cmd->vertex_stride);
                    const uint32_t* color = (const uint32_t*)(vdata + i * dpup_cmd->vertex_stride + 16); // XYZRHW format
                    const float* uv = (const float*)(vdata + i * dpup_cmd->vertex_stride + 20); // UV after color
                    DX8GL_INFO("  HUD Vertex %u: pos(%.2f,%.2f,%.2f,%.2f) color(0x%08x) uv(%.3f,%.3f)", 
                               i, pos[0], pos[1], pos[2], pos[3], *color, uv[0], uv[1]);
                }
                
                glBufferData(GL_ARRAY_BUFFER, data_size, vertex_data, GL_STREAM_DRAW);
                
                // Apply current state and generate shader
                state_manager.apply_render_states();
                
                // Get or create shader for current state
                // Use FVF stored with the command (fixes HUD vertex format issue)
                DWORD current_fvf = dpup_cmd->fvf;
                if (current_fvf == 0) {
                    // Fallback to position-only if no FVF is set
                    current_fvf = D3DFVF_XYZ;
                }
                
                FixedFunctionState ff_state;
                populate_fixed_function_state(ff_state, state_manager, current_fvf);
                
                // Log render state for HUD debugging
                DX8GL_INFO("  HUD Render State: lighting=%s texture0=%s alpha_test=%s fog=%s", 
                           ff_state.lighting_enabled ? "ON" : "OFF",
                           ff_state.texture_enabled[0] ? "ON" : "OFF", 
                           ff_state.alpha_test_enabled ? "ON" : "OFF",
                           ff_state.fog_enabled ? "ON" : "OFF");
                
                DX8GL_INFO("DrawPrimitiveUP using FVF 0x%04X (XYZRHW=%s)", 
                           current_fvf, (current_fvf & D3DFVF_XYZRHW) ? "YES" : "NO");
                
                GLuint program = ff_shader->get_program(ff_state);
                if (!program) {
                    DX8GL_ERROR("Failed to get shader program for immediate mode");
                    break;
                }
                
                glUseProgram(program);
                
                // Set uniforms
                const auto* uniforms = ff_shader->get_uniform_locations(program);
                if (!uniforms) {
                    DX8GL_ERROR("Failed to get uniform locations");
                    break;
                }
                DX8GL_INFO("Got uniform locations for program %u", program);
                
                // Set matrices
                D3DMATRIX world, view, proj;
                state_manager.get_transform(D3DTS_WORLD, &world);
                state_manager.get_transform(D3DTS_VIEW, &view);
                state_manager.get_transform(D3DTS_PROJECTION, &proj);
                
                if (uniforms->world_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->world_matrix, 1, GL_FALSE, (const GLfloat*)&world);
                }
                if (uniforms->view_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->view_matrix, 1, GL_FALSE, (const GLfloat*)&view);
                }
                if (uniforms->projection_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->projection_matrix, 1, GL_FALSE, (const GLfloat*)&proj);
                }
                
                // Calculate and set worldViewProj matrix
                DX8GL_INFO("worldViewProj uniform location: %d", uniforms->world_view_proj_matrix);
                if (uniforms->world_view_proj_matrix >= 0) {
                    // Get the combined world-view-projection matrix from state manager
                    const D3DMATRIX* wvp = state_manager.get_world_view_projection_matrix();
                    if (wvp) {
                        DX8GL_INFO("Setting worldViewProj matrix: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
                                   wvp->_11, wvp->_12, wvp->_13, wvp->_14,
                                   wvp->_21, wvp->_22, wvp->_23, wvp->_24,
                                   wvp->_31, wvp->_32, wvp->_33, wvp->_34,
                                   wvp->_41, wvp->_42, wvp->_43, wvp->_44);
                        glUniformMatrix4fv(uniforms->world_view_proj_matrix, 1, GL_TRUE, (const GLfloat*)wvp);
                    }
                } else {
                    DX8GL_INFO("worldViewProj uniform not found in shader!");
                }
                
                // Set viewport size uniform for XYZRHW coordinate conversion
                if (uniforms->viewport_size >= 0) {
                    D3DVIEWPORT8 viewport;
                    state_manager.get_viewport(&viewport);
                    glUniform2f(uniforms->viewport_size, static_cast<float>(viewport.Width), static_cast<float>(viewport.Height));
                    DX8GL_INFO("Set viewport_size uniform: %dx%d", viewport.Width, viewport.Height);
                }
                
                // Calculate and set normal matrix if needed
                if (uniforms->normal_matrix >= 0 && (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL)) {
                    // Normal matrix is the transpose of the inverse of the upper 3x3 of the world matrix
                    // For now, assuming uniform scaling, we can just use the upper 3x3 of world matrix
                    glm::mat3 normalMatrix(
                        world._11, world._12, world._13,
                        world._21, world._22, world._23,
                        world._31, world._32, world._33
                    );
                    glUniformMatrix3fv(uniforms->normal_matrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
                }
                
                // Set bump mapping uniforms
                set_bump_mapping_uniforms(uniforms, state_manager);
                
                // Get or create VAO for this immediate mode rendering
                VAOManager& vao_mgr = get_vao_manager();
                fprintf(stderr, "[DEBUG] About to get VAO for FVF 0x%x, program %u, VBO %u, stride %u\n", 
                        current_fvf, program, temp_vbo, dpup_cmd->vertex_stride);
                DX8GL_INFO("Creating VAO for FVF 0x%x, program %u, VBO %u, stride %u", 
                           current_fvf, program, temp_vbo, dpup_cmd->vertex_stride);
                GLuint vao = vao_mgr.get_vao(current_fvf, program, temp_vbo, dpup_cmd->vertex_stride);
                fprintf(stderr, "[DEBUG] Got VAO %u\n", vao);
                
                // Bind the VAO
                glBindVertexArray(vao);
                
                // Draw the primitives
                DX8GL_INFO("Drawing %u vertices with mode %s (0x%x)", vertex_count, 
                           gl_mode == GL_TRIANGLES ? "GL_TRIANGLES" : 
                           gl_mode == GL_TRIANGLE_STRIP ? "GL_TRIANGLE_STRIP" : 
                           gl_mode == GL_POINTS ? "GL_POINTS" : "OTHER", gl_mode);
                
                // Check GL state before draw
                GLboolean depth_test = GL_FALSE;
                glGetBooleanv(GL_DEPTH_TEST, &depth_test);
                DX8GL_INFO("Before draw: GL_DEPTH_TEST = %s", depth_test ? "enabled" : "disabled");
                
                glDrawArrays(gl_mode, 0, vertex_count);
                
                // Check for OpenGL errors
                if (check_gl_error_safe("glDrawArrays in DRAW_PRIMITIVE_UP")) {
                    DX8GL_ERROR("OpenGL error detected after DrawArrays - attempting recovery");
                }
                GLenum error = glGetError();
                if (error != GL_NO_ERROR) {
                    DX8GL_ERROR("OpenGL error after DrawArrays: 0x%04x", error);
                } else {
                    DX8GL_INFO("Draw completed successfully");
                }
                
                // Unbind VAO
                glBindVertexArray(0);
                
                // Clean up is handled by VAO now, no need to disable vertex attributes
                
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glDeleteBuffers(1, &temp_vbo);
                
                break;
            }
            
            case CommandType::DRAW_INDEXED_PRIMITIVE_UP: {
                const auto* dipup_cmd = static_cast<const DrawIndexedPrimitiveUPCmd*>(cmd);
                
                // Calculate data pointers
                const uint8_t* data_start = read_ptr + sizeof(DrawIndexedPrimitiveUPCmd);
                size_t index_size = (dipup_cmd->index_format == D3DFMT_INDEX16) ? 2 : 4;
                
                // Calculate index count from primitive count
                UINT index_count = 0;
                GLenum gl_mode = GL_TRIANGLES;
                
                switch (dipup_cmd->primitive_type) {
                    case D3DPT_POINTLIST:
                        index_count = dipup_cmd->primitive_count;
                        gl_mode = GL_POINTS;
                        break;
                    case D3DPT_LINELIST:
                        index_count = dipup_cmd->primitive_count * 2;
                        gl_mode = GL_LINES;
                        break;
                    case D3DPT_LINESTRIP:
                        index_count = dipup_cmd->primitive_count + 1;
                        gl_mode = GL_LINE_STRIP;
                        break;
                    case D3DPT_TRIANGLELIST:
                        index_count = dipup_cmd->primitive_count * 3;
                        gl_mode = GL_TRIANGLES;
                        break;
                    case D3DPT_TRIANGLESTRIP:
                        index_count = dipup_cmd->primitive_count + 2;
                        gl_mode = GL_TRIANGLE_STRIP;
                        break;
                    case D3DPT_TRIANGLEFAN:
                        index_count = dipup_cmd->primitive_count + 2;
                        gl_mode = GL_TRIANGLE_FAN;
                        break;
                }
                
                size_t index_data_size = index_count * index_size;
                const void* index_data = data_start;
                const void* vertex_data = data_start + index_data_size;
                
                DX8GL_TRACE("DRAW_INDEXED_PRIMITIVE_UP type=%d min=%u num=%u count=%u", 
                           dipup_cmd->primitive_type, dipup_cmd->min_vertex_index,
                           dipup_cmd->num_vertices, dipup_cmd->primitive_count);
                
                // Create temporary VBO for vertex data
                GLuint temp_vbo;
                glGenBuffers(1, &temp_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, temp_vbo);
                
                // Upload vertex data
                size_t vertex_data_size = dipup_cmd->num_vertices * dipup_cmd->vertex_stride;
                glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STREAM_DRAW);
                
                // Create temporary IBO for index data
                GLuint temp_ibo;
                glGenBuffers(1, &temp_ibo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, temp_ibo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_data_size, index_data, GL_STREAM_DRAW);
                
                // Apply current state and generate shader
                state_manager.apply_render_states();
                
                // Get or create shader for current state
                // Use FVF stored with the command (fixes vertex format timing issues)
                DWORD current_fvf = dipup_cmd->fvf;
                if (current_fvf == 0) {
                    // Fallback to position-only if no FVF is set
                    current_fvf = D3DFVF_XYZ;
                }
                
                DX8GL_INFO("DrawIndexedPrimitiveUP using FVF 0x%04X (XYZRHW=%s)", 
                           current_fvf, (current_fvf & D3DFVF_XYZRHW) ? "YES" : "NO");
                
                FixedFunctionState ff_state;
                populate_fixed_function_state(ff_state, state_manager, current_fvf);
                
                GLuint program = ff_shader->get_program(ff_state);
                if (!program) {
                    DX8GL_ERROR("Failed to get shader program for indexed immediate mode");
                    break;
                }
                
                glUseProgram(program);
                
                // Set uniforms
                const auto* uniforms = ff_shader->get_uniform_locations(program);
                if (!uniforms) {
                    DX8GL_ERROR("Failed to get uniform locations");
                    break;
                }
                DX8GL_INFO("Got uniform locations for program %u", program);
                
                // Set matrices
                D3DMATRIX world, view, proj;
                state_manager.get_transform(D3DTS_WORLD, &world);
                state_manager.get_transform(D3DTS_VIEW, &view);
                state_manager.get_transform(D3DTS_PROJECTION, &proj);
                
                if (uniforms->world_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->world_matrix, 1, GL_FALSE, (const GLfloat*)&world);
                }
                if (uniforms->view_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->view_matrix, 1, GL_FALSE, (const GLfloat*)&view);
                }
                if (uniforms->projection_matrix >= 0) {
                    glUniformMatrix4fv(uniforms->projection_matrix, 1, GL_FALSE, (const GLfloat*)&proj);
                }
                
                // Calculate and set worldViewProj matrix
                DX8GL_INFO("worldViewProj uniform location: %d", uniforms->world_view_proj_matrix);
                if (uniforms->world_view_proj_matrix >= 0) {
                    // Get the combined world-view-projection matrix from state manager
                    const D3DMATRIX* wvp = state_manager.get_world_view_projection_matrix();
                    if (wvp) {
                        DX8GL_INFO("Setting worldViewProj matrix: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
                                   wvp->_11, wvp->_12, wvp->_13, wvp->_14,
                                   wvp->_21, wvp->_22, wvp->_23, wvp->_24,
                                   wvp->_31, wvp->_32, wvp->_33, wvp->_34,
                                   wvp->_41, wvp->_42, wvp->_43, wvp->_44);
                        glUniformMatrix4fv(uniforms->world_view_proj_matrix, 1, GL_TRUE, (const GLfloat*)wvp);
                    }
                } else {
                    DX8GL_INFO("worldViewProj uniform not found in shader!");
                }
                
                // Set viewport size uniform for XYZRHW coordinate conversion
                if (uniforms->viewport_size >= 0) {
                    D3DVIEWPORT8 viewport;
                    state_manager.get_viewport(&viewport);
                    glUniform2f(uniforms->viewport_size, static_cast<float>(viewport.Width), static_cast<float>(viewport.Height));
                    DX8GL_INFO("Set viewport_size uniform: %dx%d", viewport.Width, viewport.Height);
                }
                
                // Calculate and set normal matrix if needed
                if (uniforms->normal_matrix >= 0 && (stream_sources[0].vb->get_fvf() & D3DFVF_NORMAL)) {
                    // Normal matrix is the transpose of the inverse of the upper 3x3 of the world matrix
                    // For now, assuming uniform scaling, we can just use the upper 3x3 of world matrix
                    glm::mat3 normalMatrix(
                        world._11, world._12, world._13,
                        world._21, world._22, world._23,
                        world._31, world._32, world._33
                    );
                    glUniformMatrix3fv(uniforms->normal_matrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
                }
                
                // Set bump mapping uniforms
                set_bump_mapping_uniforms(uniforms, state_manager);
                
                // Set up vertex attributes based on FVF
                FVFParser::setup_vertex_attributes(current_fvf, program, dipup_cmd->vertex_stride);
                
                // Draw the indexed primitives
                GLenum index_type = (dipup_cmd->index_format == D3DFMT_INDEX16) ? 
                                   GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
                glDrawElements(gl_mode, index_count, index_type, nullptr);
                
                // Clean up - disable all possible vertex attributes
                GLint max_attribs;
                glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
                for (GLint i = 0; i < max_attribs && i < 8; i++) {
                    glDisableVertexAttribArray(i);
                }
                
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                glDeleteBuffers(1, &temp_vbo);
                glDeleteBuffers(1, &temp_ibo);
                
                break;
            }
            
            case CommandType::CLEAR: {
                const auto* c_cmd = static_cast<const ClearCmd*>(cmd);
                const D3DRECT* rects = nullptr;
                if (c_cmd->count > 0) {
                    rects = reinterpret_cast<const D3DRECT*>(read_ptr + sizeof(ClearCmd));
                }
                state_manager.clear(c_cmd->count, rects, c_cmd->flags, 
                                   c_cmd->color, c_cmd->z, c_cmd->stencil);
                break;
            }
            
            default:
                DX8GL_ERROR("Unknown command type: %d", static_cast<int>(cmd->type));
                break;
        }
        
        read_ptr += cmd->size;
    }
}

void CommandBuffer::draw_primitive_up(D3DPRIMITIVETYPE primitive_type, UINT start_vertex,
                                     UINT primitive_count, const void* vertex_data,
                                     size_t vertex_data_size, UINT vertex_stride, DWORD fvf) {
    // Allocate command with space for vertex data
    auto* cmd = allocate_command_with_data<DrawPrimitiveUPCmd>(vertex_data_size);
    cmd->primitive_type = primitive_type;
    cmd->primitive_count = primitive_count;
    cmd->vertex_stride = vertex_stride;
    cmd->fvf = fvf;  // Store FVF with the command
    
    // Copy vertex data after the command
    void* data_ptr = get_command_data(cmd);
    memcpy(data_ptr, vertex_data, vertex_data_size);
}

void CommandBuffer::draw_indexed_primitive_up(D3DPRIMITIVETYPE primitive_type, UINT min_vertex_index,
                                             UINT num_vertices, UINT primitive_count,
                                             const void* index_data, size_t index_data_size,
                                             D3DFORMAT index_format, const void* vertex_data,
                                             size_t vertex_data_size, UINT vertex_stride, DWORD fvf) {
    // Allocate command with space for both index and vertex data
    size_t total_data_size = index_data_size + vertex_data_size;
    auto* cmd = allocate_command_with_data<DrawIndexedPrimitiveUPCmd>(total_data_size);
    cmd->primitive_type = primitive_type;
    cmd->min_vertex_index = min_vertex_index;
    cmd->num_vertices = num_vertices;
    cmd->primitive_count = primitive_count;
    cmd->index_format = index_format;
    cmd->vertex_stride = vertex_stride;
    cmd->fvf = fvf;  // Store FVF with the command
    
    // Copy index data first, then vertex data
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(get_command_data(cmd));
    memcpy(data_ptr, index_data, index_data_size);
    memcpy(data_ptr + index_data_size, vertex_data, vertex_data_size);
}

// CommandBufferPool implementation
CommandBufferPool::CommandBufferPool() 
    : total_allocated_(0) {
}

CommandBufferPool::~CommandBufferPool() = default;

std::unique_ptr<CommandBuffer> CommandBufferPool::acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!pool_.empty()) {
        auto buffer = std::move(pool_.back());
        pool_.pop_back();
        buffer->reset();
        return buffer;
    }
    
    total_allocated_++;
    return std::make_unique<CommandBuffer>();
}

void CommandBufferPool::release(std::unique_ptr<CommandBuffer> buffer) {
    if (!buffer) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Only keep buffers in the pool if they're not too large
    const size_t max_pooled_size = 1024 * 1024;  // 1MB
    if (buffer->get_capacity() <= max_pooled_size) {
        pool_.push_back(std::move(buffer));
    }
}

void CommandBufferPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.clear();
}

size_t CommandBufferPool::get_pool_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.size();
}

size_t CommandBufferPool::get_total_allocated() const {
    return total_allocated_;
}

} // namespace dx8gl