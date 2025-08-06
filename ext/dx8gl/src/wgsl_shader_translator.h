#ifndef DX8GL_WGSL_SHADER_TRANSLATOR_H
#define DX8GL_WGSL_SHADER_TRANSLATOR_H

#include "dx8_shader_translator.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace dx8gl {

/**
 * Translates DirectX 8 shader bytecode to WGSL (WebGPU Shading Language)
 * Extends the base DX8ShaderTranslator to generate WGSL instead of GLSL
 */
class WGSLShaderTranslator : public DX8ShaderTranslator {
public:
    WGSLShaderTranslator();
    ~WGSLShaderTranslator();
    
    // Generate WGSL shader from parsed bytecode
    std::string generate_wgsl();
    
    // Generate fixed-function WGSL shaders
    static std::string generate_fixed_function_vertex_wgsl(
        bool lighting_enabled,
        bool fog_enabled,
        int num_textures,
        bool color_vertex,
        bool transform_texcoords
    );
    
    static std::string generate_fixed_function_fragment_wgsl(
        bool alpha_test_enabled,
        bool fog_enabled,
        int num_textures,
        bool vertex_color
    );
    
    // Set binding points for resources
    void set_uniform_binding(uint32_t group, uint32_t binding) {
        uniform_group_ = group;
        uniform_binding_ = binding;
    }
    
    void set_texture_binding_start(uint32_t group, uint32_t binding) {
        texture_group_ = group;
        texture_binding_start_ = binding;
    }
    
    void set_sampler_binding_start(uint32_t group, uint32_t binding) {
        sampler_group_ = group;
        sampler_binding_start_ = binding;
    }
    
private:
    // WGSL generation helpers
    std::string generate_vertex_wgsl();
    std::string generate_fragment_wgsl();
    
    // Convert register references to WGSL
    std::string register_to_wgsl(const ShaderInstruction::Register& reg, bool is_vertex);
    std::string instruction_to_wgsl(const ShaderInstruction& inst, bool is_vertex);
    
    // Apply instruction modifiers in WGSL
    std::string apply_wgsl_modifier(const std::string& value, InstructionModifier modifier);
    
    // Generate struct definitions
    std::string generate_vertex_input_struct();
    std::string generate_vertex_output_struct();
    std::string generate_fragment_input_struct();
    std::string generate_uniform_struct();
    
    // Generate binding declarations
    std::string generate_uniform_bindings();
    std::string generate_texture_bindings();
    
    // Helper to convert swizzle to WGSL format
    std::string convert_swizzle_to_wgsl(const std::string& swizzle);
    std::string convert_write_mask_to_wgsl(const std::string& mask);
    
    // Binding locations
    uint32_t uniform_group_ = 0;
    uint32_t uniform_binding_ = 0;
    uint32_t texture_group_ = 1;
    uint32_t texture_binding_start_ = 0;
    uint32_t sampler_group_ = 1;
    uint32_t sampler_binding_start_ = 8; // After textures
    
    // Track used features for optimization
    bool uses_derivatives_ = false;
    bool uses_clip_distance_ = false;
    
    // Temporary register declarations
    std::unordered_map<int, std::string> temp_registers_;
    
    // Constant buffer data
    struct ConstantData {
        float values[4];
        bool defined;
    };
    std::unordered_map<int, ConstantData> constant_values_;
};

/**
 * Fixed-function pipeline state for WGSL generation
 */
struct FixedFunctionState {
    // Transform state
    bool world_transform_enabled = true;
    bool view_transform_enabled = true;
    bool projection_transform_enabled = true;
    bool texture_transform_enabled[8] = {false};
    
    // Lighting state
    bool lighting_enabled = false;
    bool normalize_normals = false;
    int num_lights = 0;
    
    struct Light {
        enum Type { DIRECTIONAL, POINT, SPOT };
        Type type = DIRECTIONAL;
        bool enabled = false;
    };
    Light lights[8];
    
    // Material state
    bool color_material_enabled = false;
    bool specular_enabled = false;
    
    // Fog state
    bool fog_enabled = false;
    enum FogMode { FOG_LINEAR, FOG_EXP, FOG_EXP2 };
    FogMode fog_mode = FOG_LINEAR;
    
    // Texture state
    int num_textures = 0;
    bool texture_enabled[8] = {false};
    enum TextureOp { 
        TEXOP_DISABLE, TEXOP_SELECTARG1, TEXOP_SELECTARG2,
        TEXOP_MODULATE, TEXOP_ADD, TEXOP_BLEND
    };
    TextureOp color_op[8] = {TEXOP_MODULATE};
    TextureOp alpha_op[8] = {TEXOP_SELECTARG1};
    
    // Vertex format
    bool has_position = true;
    bool has_normal = false;
    bool has_diffuse = false;
    bool has_specular = false;
    bool has_texcoord[8] = {false};
    
    // Output control
    bool alpha_test_enabled = false;
    bool alpha_blend_enabled = false;
};

/**
 * Generates complete WGSL shader pair from fixed-function state
 */
class FixedFunctionWGSLGenerator {
public:
    // Generate vertex shader
    static std::string generate_vertex_shader(const FixedFunctionState& state);
    
    // Generate fragment shader
    static std::string generate_fragment_shader(const FixedFunctionState& state);
    
    // Generate both shaders
    static std::pair<std::string, std::string> generate_shader_pair(const FixedFunctionState& state);
    
private:
    // Helper functions for shader generation
    static std::string generate_transform_code(const FixedFunctionState& state);
    static std::string generate_lighting_code(const FixedFunctionState& state);
    static std::string generate_fog_code(const FixedFunctionState& state);
    static std::string generate_texture_sampling_code(const FixedFunctionState& state);
    static std::string generate_texture_combine_code(const FixedFunctionState& state);
};

} // namespace dx8gl

#endif // DX8GL_WGSL_SHADER_TRANSLATOR_H