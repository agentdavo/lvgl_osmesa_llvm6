#include "wgsl_shader_translator.h"
#include "logger.h"
#include <sstream>
#include <iomanip>

namespace dx8gl {

WGSLShaderTranslator::WGSLShaderTranslator() 
    : DX8ShaderTranslator() {
}

WGSLShaderTranslator::~WGSLShaderTranslator() {
}

// Generate WGSL shader from parsed bytecode
std::string WGSLShaderTranslator::generate_wgsl() {
    if (get_shader_type() == SHADER_TYPE_VERTEX) {
        return generate_vertex_wgsl();
    } else {
        return generate_fragment_wgsl();
    }
}

// Generate vertex shader WGSL
std::string WGSLShaderTranslator::generate_vertex_wgsl() {
    std::stringstream ss;
    
    // Generate input/output structures
    ss << generate_vertex_input_struct() << "\n";
    ss << generate_vertex_output_struct() << "\n";
    
    // Generate uniform bindings
    ss << generate_uniform_bindings() << "\n";
    
    // Main vertex function
    ss << "@vertex\n";
    ss << "fn vs_main(input: VertexInput) -> VertexOutput {\n";
    ss << "    var output: VertexOutput;\n";
    
    // Declare temporary registers
    std::set<int> temp_regs_used;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_TEMP) {
            temp_regs_used.insert(inst.dest.index);
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_TEMP) {
                temp_regs_used.insert(src.index);
            }
        }
    }
    
    for (int reg : temp_regs_used) {
        ss << "    var r" << reg << ": vec4<f32> = vec4<f32>(0.0);\n";
    }
    
    // Process instructions
    for (const auto& inst : instructions_) {
        if (inst.opcode == D3DSIO_END) break;
        ss << "    " << instruction_to_wgsl(inst, true) << "\n";
    }
    
    // Ensure position is written
    if (!uses_position_) {
        ss << "    output.position = vec4<f32>(0.0, 0.0, 0.0, 1.0);\n";
    }
    
    ss << "    return output;\n";
    ss << "}\n";
    
    return ss.str();
}

// Generate fragment shader WGSL
std::string WGSLShaderTranslator::generate_fragment_wgsl() {
    std::stringstream ss;
    
    // Generate input structure (matches vertex output)
    ss << generate_fragment_input_struct() << "\n";
    
    // Generate texture bindings
    ss << generate_texture_bindings() << "\n";
    
    // Generate uniform bindings
    ss << generate_uniform_bindings() << "\n";
    
    // Main fragment function
    ss << "@fragment\n";
    ss << "fn fs_main(input: FragmentInput) -> @location(0) vec4<f32> {\n";
    
    // Declare temporary registers
    std::set<int> temp_regs_used;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_TEMP) {
            temp_regs_used.insert(inst.dest.index);
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_TEMP) {
                temp_regs_used.insert(src.index);
            }
        }
    }
    
    for (int reg : temp_regs_used) {
        ss << "    var r" << reg << ": vec4<f32> = vec4<f32>(0.0);\n";
    }
    
    // Declare color output register
    ss << "    var color_out: vec4<f32> = vec4<f32>(0.0, 0.0, 0.0, 1.0);\n";
    
    // Process instructions
    for (const auto& inst : instructions_) {
        if (inst.opcode == D3DSIO_END) break;
        ss << "    " << instruction_to_wgsl(inst, false) << "\n";
    }
    
    ss << "    return color_out;\n";
    ss << "}\n";
    
    return ss.str();
}

// Generate vertex input structure
std::string WGSLShaderTranslator::generate_vertex_input_struct() {
    std::stringstream ss;
    
    ss << "struct VertexInput {\n";
    ss << "    @location(0) position: vec3<f32>,\n";
    
    if (uses_normal_) {
        ss << "    @location(1) normal: vec3<f32>,\n";
    }
    
    if (uses_color_) {
        ss << "    @location(2) color: vec4<f32>,\n";
    }
    
    int location = 3;
    for (int coord : texture_coords_used_) {
        ss << "    @location(" << location++ << ") texcoord" << coord << ": vec2<f32>,\n";
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    size_t last_comma = result.find_last_of(',');
    if (last_comma != std::string::npos) {
        result[last_comma] = ' ';
    }
    
    result += "\n}";
    return result;
}

// Generate vertex output structure
std::string WGSLShaderTranslator::generate_vertex_output_struct() {
    std::stringstream ss;
    
    ss << "struct VertexOutput {\n";
    ss << "    @builtin(position) position: vec4<f32>,\n";
    
    for (int color : varying_colors_used_) {
        ss << "    @location(" << color << ") color" << color << ": vec4<f32>,\n";
    }
    
    for (int coord : varying_texcoords_used_) {
        ss << "    @location(" << (2 + coord) << ") texcoord" << coord << ": vec2<f32>,\n";
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    size_t last_comma = result.find_last_of(',');
    if (last_comma != std::string::npos) {
        result[last_comma] = ' ';
    }
    
    result += "\n}";
    return result;
}

// Generate fragment input structure
std::string WGSLShaderTranslator::generate_fragment_input_struct() {
    std::stringstream ss;
    
    ss << "struct FragmentInput {\n";
    ss << "    @builtin(position) frag_coord: vec4<f32>,\n";
    
    for (int color : varying_colors_used_) {
        ss << "    @location(" << color << ") color" << color << ": vec4<f32>,\n";
    }
    
    for (int coord : varying_texcoords_used_) {
        ss << "    @location(" << (2 + coord) << ") texcoord" << coord << ": vec2<f32>,\n";
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    size_t last_comma = result.find_last_of(',');
    if (last_comma != std::string::npos) {
        result[last_comma] = ' ';
    }
    
    result += "\n}";
    return result;
}

// Generate uniform structure
std::string WGSLShaderTranslator::generate_uniform_struct() {
    std::stringstream ss;
    
    ss << "struct Uniforms {\n";
    ss << "    mvp_matrix: mat4x4<f32>,\n";
    ss << "    world_matrix: mat4x4<f32>,\n";
    ss << "    view_matrix: mat4x4<f32>,\n";
    ss << "    proj_matrix: mat4x4<f32>,\n";
    
    // Add shader constants
    for (const auto& constant : get_constants()) {
        ss << "    const_" << constant.index << ": vec4<f32>,\n";
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    size_t last_comma = result.find_last_of(',');
    if (last_comma != std::string::npos) {
        result[last_comma] = ' ';
    }
    
    result += "\n}";
    return result;
}

// Generate uniform bindings
std::string WGSLShaderTranslator::generate_uniform_bindings() {
    std::stringstream ss;
    
    ss << generate_uniform_struct() << "\n";
    ss << "@group(" << uniform_group_ << ") @binding(" << uniform_binding_ << ")\n";
    ss << "var<uniform> uniforms: Uniforms;\n";
    
    return ss.str();
}

// Generate texture bindings
std::string WGSLShaderTranslator::generate_texture_bindings() {
    std::stringstream ss;
    
    // Generate texture and sampler bindings for used texture stages
    for (int i = 0; i < 8; ++i) {
        if (texture_coords_used_.count(i) || output_textures_used_.count(i)) {
            ss << "@group(" << texture_group_ << ") @binding(" << (texture_binding_start_ + i) << ")\n";
            ss << "var texture" << i << ": texture_2d<f32>;\n";
            
            ss << "@group(" << sampler_group_ << ") @binding(" << (sampler_binding_start_ + i) << ")\n";
            ss << "var sampler" << i << ": sampler;\n\n";
        }
    }
    
    return ss.str();
}

// Convert register to WGSL
std::string WGSLShaderTranslator::register_to_wgsl(const ShaderInstruction::Register& reg, bool is_vertex) {
    std::stringstream ss;
    std::string base;
    
    switch (reg.type) {
        case D3DSPR_TEMP:
            base = "r" + std::to_string(reg.index);
            break;
            
        case D3DSPR_INPUT:
            if (is_vertex) {
                // Vertex shader input
                if (reg.index == 0) base = "vec4<f32>(input.position, 1.0)";
                else if (reg.index == 1) base = "vec4<f32>(input.normal, 0.0)";
                else if (reg.index == 2) base = "input.color";
                else base = "vec4<f32>(input.texcoord" + std::to_string(reg.index - 3) + ", 0.0, 1.0)";
            } else {
                // Fragment shader input (from vertex shader)
                if (reg.index < 2) {
                    base = "input.color" + std::to_string(reg.index);
                } else {
                    base = "vec4<f32>(input.texcoord" + std::to_string(reg.index - 2) + ", 0.0, 1.0)";
                }
            }
            break;
            
        case D3DSPR_CONST:
            base = "uniforms.const_" + std::to_string(reg.index);
            break;
            
        case D3DSPR_TEXTURE:
            // Texture coordinate in pixel shader
            base = "vec4<f32>(input.texcoord" + std::to_string(reg.index) + ", 0.0, 1.0)";
            break;
            
        case D3DSPR_RASTOUT:
            // Vertex shader rasterizer output
            if (reg.index == 0) base = "output.position";
            else base = "output.fog";
            break;
            
        case D3DSPR_ATTROUT:
            // Vertex shader attribute output
            if (reg.index < 2) {
                base = "output.color" + std::to_string(reg.index);
            } else {
                base = "vec4<f32>(output.texcoord" + std::to_string(reg.index - 2) + ", 0.0, 1.0)";
            }
            break;
            
        case D3DSPR_COLOROUT:
            // Pixel shader color output
            base = "color_out";
            break;
            
        default:
            base = "vec4<f32>(0.0)";
            break;
    }
    
    // Apply swizzle
    if (!reg.swizzle.empty() && reg.swizzle != "xyzw") {
        base = base + "." + reg.swizzle;
    }
    
    // Apply negation
    if (reg.negate) {
        base = "-(" + base + ")";
    }
    
    // Apply source modifier
    if (reg.src_modifier != MODIFIER_NONE) {
        base = apply_wgsl_modifier(base, reg.src_modifier);
    }
    
    return base;
}

// Convert instruction to WGSL
std::string WGSLShaderTranslator::instruction_to_wgsl(const ShaderInstruction& inst, bool is_vertex) {
    std::stringstream ss;
    
    // Get destination and source registers
    std::string dest = register_to_wgsl(inst.dest, is_vertex);
    std::vector<std::string> sources;
    for (const auto& src : inst.sources) {
        sources.push_back(register_to_wgsl(src, is_vertex));
    }
    
    // Generate instruction based on opcode
    std::string result;
    switch (inst.opcode) {
        case D3DSIO_MOV:
            result = sources[0];
            break;
            
        case D3DSIO_ADD:
            result = sources[0] + " + " + sources[1];
            break;
            
        case D3DSIO_SUB:
            result = sources[0] + " - " + sources[1];
            break;
            
        case D3DSIO_MUL:
            result = sources[0] + " * " + sources[1];
            break;
            
        case D3DSIO_MAD:
            result = sources[0] + " * " + sources[1] + " + " + sources[2];
            break;
            
        case D3DSIO_DP3:
            result = "vec4<f32>(dot(" + sources[0] + ".xyz, " + sources[1] + ".xyz))";
            break;
            
        case D3DSIO_DP4:
            result = "vec4<f32>(dot(" + sources[0] + ", " + sources[1] + "))";
            break;
            
        case D3DSIO_MIN:
            result = "min(" + sources[0] + ", " + sources[1] + ")";
            break;
            
        case D3DSIO_MAX:
            result = "max(" + sources[0] + ", " + sources[1] + ")";
            break;
            
        case D3DSIO_SLT:
            result = "vec4<f32>(select(1.0, 0.0, " + sources[0] + " >= " + sources[1] + "))";
            break;
            
        case D3DSIO_SGE:
            result = "vec4<f32>(select(0.0, 1.0, " + sources[0] + " >= " + sources[1] + "))";
            break;
            
        case D3DSIO_RSQ:
            result = "vec4<f32>(inverseSqrt(" + sources[0] + ".x))";
            break;
            
        case D3DSIO_RCP:
            result = "vec4<f32>(1.0 / " + sources[0] + ".x)";
            break;
            
        case D3DSIO_EXP:
        case D3DSIO_EXPP:
            result = "vec4<f32>(exp2(" + sources[0] + ".x))";
            break;
            
        case D3DSIO_LOG:
        case D3DSIO_LOGP:
            result = "vec4<f32>(log2(" + sources[0] + ".x))";
            break;
            
        case D3DSIO_FRC:
            result = "fract(" + sources[0] + ")";
            break;
            
        case D3DSIO_LRP:
            result = "mix(" + sources[2] + ", " + sources[1] + ", " + sources[0] + ")";
            break;
            
        case D3DSIO_TEX:
            // Texture sampling
            if (!is_vertex && inst.dest.index < 8) {
                result = "textureSample(texture" + std::to_string(inst.dest.index) + 
                        ", sampler" + std::to_string(inst.dest.index) + 
                        ", input.texcoord" + std::to_string(inst.dest.index) + ")";
            }
            break;
            
        case D3DSIO_TEXKILL:
            // Discard fragment if any component is negative
            ss << "if (any(" + sources[0] + " < vec4<f32>(0.0))) { discard; }";
            return ss.str();
            
        case D3DSIO_SINCOS:
            // SINCOS dst, src (dst.x = cos, dst.y = sin)
            result = "vec4<f32>(cos(" + sources[0] + ".x), sin(" + sources[0] + ".x), 0.0, 1.0)";
            break;
            
        default:
            DX8GL_WARNING("Unhandled WGSL instruction: %d", inst.opcode);
            result = "vec4<f32>(0.0)";
            break;
    }
    
    // Apply destination modifier
    if (inst.modifier != MODIFIER_NONE) {
        result = apply_wgsl_modifier(result, inst.modifier);
    }
    
    // Apply write mask
    if (!inst.dest.write_mask.empty() && inst.dest.write_mask != "xyzw") {
        // Construct masked assignment
        if (inst.dest.type == D3DSPR_TEMP) {
            std::string temp_name = "r" + std::to_string(inst.dest.index);
            for (char c : inst.dest.write_mask) {
                ss << temp_name << "." << c << " = (" << result << ")." << c << "; ";
            }
        } else {
            ss << dest;
            if (dest.find('.') == std::string::npos) {
                ss << "." << inst.dest.write_mask;
            }
            ss << " = " << result << "." << inst.dest.write_mask << ";";
        }
    } else {
        // Full assignment
        if (inst.dest.type == D3DSPR_TEMP) {
            ss << "r" << inst.dest.index << " = " << result << ";";
        } else {
            ss << dest << " = " << result << ";";
        }
    }
    
    return ss.str();
}

// Apply modifier to WGSL expression
std::string WGSLShaderTranslator::apply_wgsl_modifier(const std::string& value, InstructionModifier modifier) {
    switch (modifier) {
        case MODIFIER_SAT:
            return "saturate(" + value + ")";
        case MODIFIER_X2:
            return "(" + value + " * 2.0)";
        case MODIFIER_X4:
            return "(" + value + " * 4.0)";
        case MODIFIER_D2:
            return "(" + value + " * 0.5)";
        case MODIFIER_BIAS:
            return "(" + value + " - 0.5)";
        case MODIFIER_BX2:
            return "(" + value + " * 2.0 - 1.0)";
        case MODIFIER_COMP:
            return "(1.0 - " + value + ")";
        default:
            return value;
    }
}

// Generate fixed-function vertex shader WGSL
std::string WGSLShaderTranslator::generate_fixed_function_vertex_wgsl(
    bool lighting_enabled,
    bool fog_enabled,
    int num_textures,
    bool color_vertex,
    bool transform_texcoords) {
    
    std::stringstream ss;
    
    // Vertex input structure
    ss << "struct VertexInput {\n";
    ss << "    @location(0) position: vec3<f32>,\n";
    if (lighting_enabled) {
        ss << "    @location(1) normal: vec3<f32>,\n";
    }
    if (color_vertex) {
        ss << "    @location(2) color: vec4<f32>,\n";
    }
    for (int i = 0; i < num_textures; ++i) {
        ss << "    @location(" << (3 + i) << ") texcoord" << i << ": vec2<f32>,\n";
    }
    ss.seekp(-2, std::ios_base::end); // Remove last comma
    ss << "\n}\n\n";
    
    // Vertex output structure
    ss << "struct VertexOutput {\n";
    ss << "    @builtin(position) position: vec4<f32>,\n";
    ss << "    @location(0) color: vec4<f32>,\n";
    if (fog_enabled) {
        ss << "    @location(1) fog_factor: f32,\n";
    }
    for (int i = 0; i < num_textures; ++i) {
        ss << "    @location(" << (2 + i) << ") texcoord" << i << ": vec2<f32>,\n";
    }
    ss.seekp(-2, std::ios_base::end); // Remove last comma
    ss << "\n}\n\n";
    
    // Uniforms
    ss << "struct Uniforms {\n";
    ss << "    mvp_matrix: mat4x4<f32>,\n";
    ss << "    world_matrix: mat4x4<f32>,\n";
    ss << "    view_matrix: mat4x4<f32>,\n";
    ss << "    proj_matrix: mat4x4<f32>,\n";
    if (lighting_enabled) {
        ss << "    normal_matrix: mat3x3<f32>,\n";
        ss << "    light_direction: vec3<f32>,\n";
        ss << "    light_color: vec4<f32>,\n";
        ss << "    ambient_color: vec4<f32>,\n";
    }
    if (fog_enabled) {
        ss << "    fog_start: f32,\n";
        ss << "    fog_end: f32,\n";
    }
    if (transform_texcoords) {
        for (int i = 0; i < num_textures; ++i) {
            ss << "    tex_matrix" << i << ": mat4x4<f32>,\n";
        }
    }
    ss.seekp(-2, std::ios_base::end); // Remove last comma
    ss << "\n}\n\n";
    
    ss << "@group(0) @binding(0)\n";
    ss << "var<uniform> uniforms: Uniforms;\n\n";
    
    // Vertex shader main
    ss << "@vertex\n";
    ss << "fn vs_main(input: VertexInput) -> VertexOutput {\n";
    ss << "    var output: VertexOutput;\n\n";
    
    // Transform position
    ss << "    output.position = uniforms.mvp_matrix * vec4<f32>(input.position, 1.0);\n\n";
    
    // Lighting calculation
    if (lighting_enabled) {
        ss << "    // Lighting\n";
        ss << "    let world_normal = normalize(uniforms.normal_matrix * input.normal);\n";
        ss << "    let light_dot = max(dot(world_normal, -uniforms.light_direction), 0.0);\n";
        ss << "    let diffuse = uniforms.light_color * light_dot;\n";
        ss << "    output.color = uniforms.ambient_color + diffuse;\n";
        if (color_vertex) {
            ss << "    output.color = output.color * input.color;\n";
        }
    } else if (color_vertex) {
        ss << "    output.color = input.color;\n";
    } else {
        ss << "    output.color = vec4<f32>(1.0);\n";
    }
    
    // Fog calculation
    if (fog_enabled) {
        ss << "    // Fog\n";
        ss << "    let view_pos = uniforms.view_matrix * uniforms.world_matrix * vec4<f32>(input.position, 1.0);\n";
        ss << "    let fog_dist = length(view_pos.xyz);\n";
        ss << "    output.fog_factor = saturate((uniforms.fog_end - fog_dist) / (uniforms.fog_end - uniforms.fog_start));\n";
    }
    
    // Pass through texture coordinates
    for (int i = 0; i < num_textures; ++i) {
        if (transform_texcoords) {
            ss << "    let tex_coord" << i << " = uniforms.tex_matrix" << i 
               << " * vec4<f32>(input.texcoord" << i << ", 0.0, 1.0);\n";
            ss << "    output.texcoord" << i << " = tex_coord" << i << ".xy;\n";
        } else {
            ss << "    output.texcoord" << i << " = input.texcoord" << i << ";\n";
        }
    }
    
    ss << "\n    return output;\n";
    ss << "}\n";
    
    return ss.str();
}

// Generate fixed-function fragment shader WGSL
std::string WGSLShaderTranslator::generate_fixed_function_fragment_wgsl(
    bool alpha_test_enabled,
    bool fog_enabled,
    int num_textures,
    bool vertex_color) {
    
    std::stringstream ss;
    
    // Fragment input structure
    ss << "struct FragmentInput {\n";
    ss << "    @location(0) color: vec4<f32>,\n";
    if (fog_enabled) {
        ss << "    @location(1) fog_factor: f32,\n";
    }
    for (int i = 0; i < num_textures; ++i) {
        ss << "    @location(" << (2 + i) << ") texcoord" << i << ": vec2<f32>,\n";
    }
    ss.seekp(-2, std::ios_base::end); // Remove last comma
    ss << "\n}\n\n";
    
    // Uniforms
    ss << "struct Uniforms {\n";
    if (alpha_test_enabled) {
        ss << "    alpha_ref: f32,\n";
    }
    if (fog_enabled) {
        ss << "    fog_color: vec4<f32>,\n";
    }
    ss << "    texture_enabled: u32,\n"; // Bitmask of enabled textures
    ss.seekp(-2, std::ios_base::end); // Remove last comma
    ss << "\n}\n\n";
    
    ss << "@group(0) @binding(0)\n";
    ss << "var<uniform> uniforms: Uniforms;\n\n";
    
    // Texture bindings
    for (int i = 0; i < num_textures; ++i) {
        ss << "@group(1) @binding(" << i << ")\n";
        ss << "var texture" << i << ": texture_2d<f32>;\n";
        ss << "@group(1) @binding(" << (8 + i) << ")\n";
        ss << "var sampler" << i << ": sampler;\n\n";
    }
    
    // Fragment shader main
    ss << "@fragment\n";
    ss << "fn fs_main(input: FragmentInput) -> @location(0) vec4<f32> {\n";
    
    if (vertex_color) {
        ss << "    var color = input.color;\n";
    } else {
        ss << "    var color = vec4<f32>(1.0);\n";
    }
    
    // Texture sampling and combining
    if (num_textures > 0) {
        ss << "\n    // Texture sampling\n";
        for (int i = 0; i < num_textures; ++i) {
            ss << "    if ((uniforms.texture_enabled & " << (1u << i) << "u) != 0u) {\n";
            ss << "        let tex_color" << i << " = textureSample(texture" << i 
               << ", sampler" << i << ", input.texcoord" << i << ");\n";
            // Simple modulate for now
            ss << "        color = color * tex_color" << i << ";\n";
            ss << "    }\n";
        }
    }
    
    // Alpha test
    if (alpha_test_enabled) {
        ss << "\n    // Alpha test\n";
        ss << "    if (color.a < uniforms.alpha_ref) {\n";
        ss << "        discard;\n";
        ss << "    }\n";
    }
    
    // Fog blending
    if (fog_enabled) {
        ss << "\n    // Fog blending\n";
        ss << "    color = mix(uniforms.fog_color, color, input.fog_factor);\n";
    }
    
    ss << "\n    return color;\n";
    ss << "}\n";
    
    return ss.str();
}

// FixedFunctionWGSLGenerator implementation

std::string FixedFunctionWGSLGenerator::generate_vertex_shader(const FixedFunctionState& state) {
    std::stringstream ss;
    
    // Generate vertex input structure based on state
    ss << "struct VertexInput {\n";
    ss << "    @location(0) position: vec3<f32>,\n";
    
    int location = 1;
    if (state.has_normal) {
        ss << "    @location(" << location++ << ") normal: vec3<f32>,\n";
    }
    if (state.has_diffuse) {
        ss << "    @location(" << location++ << ") diffuse: vec4<f32>,\n";
    }
    if (state.has_specular) {
        ss << "    @location(" << location++ << ") specular: vec4<f32>,\n";
    }
    for (int i = 0; i < 8; ++i) {
        if (state.has_texcoord[i]) {
            ss << "    @location(" << location++ << ") texcoord" << i << ": vec2<f32>,\n";
        }
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    if (result.size() > 2) {
        result = result.substr(0, result.size() - 2) + "\n";
    }
    ss.str(result);
    ss << "}\n\n";
    
    // Generate the rest of the shader
    ss << generate_transform_code(state);
    
    if (state.lighting_enabled) {
        ss << generate_lighting_code(state);
    }
    
    if (state.fog_enabled) {
        ss << generate_fog_code(state);
    }
    
    return ss.str();
}

std::string FixedFunctionWGSLGenerator::generate_fragment_shader(const FixedFunctionState& state) {
    std::stringstream ss;
    
    // Fragment input matches vertex output
    ss << "struct FragmentInput {\n";
    ss << "    @builtin(position) frag_coord: vec4<f32>,\n";
    ss << "    @location(0) color: vec4<f32>,\n";
    
    int location = 1;
    if (state.fog_enabled) {
        ss << "    @location(" << location++ << ") fog_factor: f32,\n";
    }
    for (int i = 0; i < state.num_textures; ++i) {
        ss << "    @location(" << location++ << ") texcoord" << i << ": vec2<f32>,\n";
    }
    
    // Remove trailing comma
    std::string result = ss.str();
    if (result.size() > 2) {
        result = result.substr(0, result.size() - 2) + "\n";
    }
    ss.str(result);
    ss << "}\n\n";
    
    // Generate texture sampling and combining
    if (state.num_textures > 0) {
        ss << generate_texture_sampling_code(state);
        ss << generate_texture_combine_code(state);
    }
    
    return ss.str();
}

std::pair<std::string, std::string> FixedFunctionWGSLGenerator::generate_shader_pair(const FixedFunctionState& state) {
    return std::make_pair(
        generate_vertex_shader(state),
        generate_fragment_shader(state)
    );
}

std::string FixedFunctionWGSLGenerator::generate_transform_code(const FixedFunctionState& state) {
    std::stringstream ss;
    
    ss << "// Transform code\n";
    ss << "@vertex\n";
    ss << "fn vs_main(input: VertexInput) -> VertexOutput {\n";
    ss << "    var output: VertexOutput;\n";
    ss << "    let pos4 = vec4<f32>(input.position, 1.0);\n";
    
    if (state.world_transform_enabled) {
        ss << "    let world_pos = uniforms.world_matrix * pos4;\n";
    }
    if (state.view_transform_enabled) {
        ss << "    let view_pos = uniforms.view_matrix * world_pos;\n";
    }
    if (state.projection_transform_enabled) {
        ss << "    output.position = uniforms.proj_matrix * view_pos;\n";
    } else {
        ss << "    output.position = pos4;\n";
    }
    
    return ss.str();
}

std::string FixedFunctionWGSLGenerator::generate_lighting_code(const FixedFunctionState& state) {
    std::stringstream ss;
    
    ss << "    // Lighting calculation\n";
    
    if (state.normalize_normals) {
        ss << "    let normal = normalize(input.normal);\n";
    } else {
        ss << "    let normal = input.normal;\n";
    }
    
    ss << "    var total_diffuse = vec3<f32>(0.0);\n";
    ss << "    var total_specular = vec3<f32>(0.0);\n";
    
    for (int i = 0; i < state.num_lights; ++i) {
        if (!state.lights[i].enabled) continue;
        
        switch (state.lights[i].type) {
            case FixedFunctionState::Light::DIRECTIONAL:
                ss << "    {\n";
                ss << "        let light_dir = normalize(uniforms.light" << i << "_direction);\n";
                ss << "        let ndotl = max(dot(normal, -light_dir), 0.0);\n";
                ss << "        total_diffuse = total_diffuse + uniforms.light" << i << "_diffuse.rgb * ndotl;\n";
                
                if (state.specular_enabled) {
                    ss << "        let half_vec = normalize(-light_dir + normalize(-view_pos.xyz));\n";
                    ss << "        let ndoth = max(dot(normal, half_vec), 0.0);\n";
                    ss << "        total_specular = total_specular + uniforms.light" << i 
                       << "_specular.rgb * pow(ndoth, uniforms.material_power);\n";
                }
                ss << "    }\n";
                break;
                
            case FixedFunctionState::Light::POINT:
                // Point light implementation
                break;
                
            case FixedFunctionState::Light::SPOT:
                // Spot light implementation
                break;
        }
    }
    
    ss << "    output.color = vec4<f32>(total_diffuse + total_specular, 1.0);\n";
    
    return ss.str();
}

std::string FixedFunctionWGSLGenerator::generate_fog_code(const FixedFunctionState& state) {
    std::stringstream ss;
    
    ss << "    // Fog calculation\n";
    ss << "    let fog_distance = length(view_pos.xyz);\n";
    
    switch (state.fog_mode) {
        case FixedFunctionState::FOG_LINEAR:
            ss << "    output.fog_factor = saturate((uniforms.fog_end - fog_distance) / "
               << "(uniforms.fog_end - uniforms.fog_start));\n";
            break;
            
        case FixedFunctionState::FOG_EXP:
            ss << "    output.fog_factor = exp(-uniforms.fog_density * fog_distance);\n";
            break;
            
        case FixedFunctionState::FOG_EXP2:
            ss << "    let fog_param = uniforms.fog_density * fog_distance;\n";
            ss << "    output.fog_factor = exp(-fog_param * fog_param);\n";
            break;
    }
    
    return ss.str();
}

std::string FixedFunctionWGSLGenerator::generate_texture_sampling_code(const FixedFunctionState& state) {
    std::stringstream ss;
    
    for (int i = 0; i < state.num_textures; ++i) {
        if (state.texture_enabled[i]) {
            ss << "@group(1) @binding(" << i << ")\n";
            ss << "var texture" << i << ": texture_2d<f32>;\n";
            ss << "@group(1) @binding(" << (i + 8) << ")\n";
            ss << "var sampler" << i << ": sampler;\n";
        }
    }
    
    return ss.str();
}

std::string FixedFunctionWGSLGenerator::generate_texture_combine_code(const FixedFunctionState& state) {
    std::stringstream ss;
    
    ss << "    // Texture combining\n";
    ss << "    var tex_color = vec4<f32>(1.0);\n";
    
    for (int i = 0; i < state.num_textures; ++i) {
        if (!state.texture_enabled[i]) continue;
        
        ss << "    let tex" << i << " = textureSample(texture" << i 
           << ", sampler" << i << ", input.texcoord" << i << ");\n";
        
        // Color operation
        switch (state.color_op[i]) {
            case FixedFunctionState::TEXOP_MODULATE:
                ss << "    tex_color.rgb = tex_color.rgb * tex" << i << ".rgb;\n";
                break;
            case FixedFunctionState::TEXOP_ADD:
                ss << "    tex_color.rgb = tex_color.rgb + tex" << i << ".rgb;\n";
                break;
            case FixedFunctionState::TEXOP_SELECTARG1:
                ss << "    tex_color.rgb = tex_color.rgb;\n";
                break;
            case FixedFunctionState::TEXOP_SELECTARG2:
                ss << "    tex_color.rgb = tex" << i << ".rgb;\n";
                break;
            case FixedFunctionState::TEXOP_BLEND:
                ss << "    tex_color.rgb = mix(tex_color.rgb, tex" << i 
                   << ".rgb, uniforms.tex_blend_factor);\n";
                break;
            default:
                break;
        }
        
        // Alpha operation
        switch (state.alpha_op[i]) {
            case FixedFunctionState::TEXOP_MODULATE:
                ss << "    tex_color.a = tex_color.a * tex" << i << ".a;\n";
                break;
            case FixedFunctionState::TEXOP_ADD:
                ss << "    tex_color.a = tex_color.a + tex" << i << ".a;\n";
                break;
            case FixedFunctionState::TEXOP_SELECTARG1:
                ss << "    tex_color.a = tex_color.a;\n";
                break;
            case FixedFunctionState::TEXOP_SELECTARG2:
                ss << "    tex_color.a = tex" << i << ".a;\n";
                break;
            default:
                break;
        }
    }
    
    ss << "    color = color * tex_color;\n";
    
    return ss.str();
}

} // namespace dx8gl