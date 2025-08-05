#ifndef DX8GL_DX8_SHADER_TRANSLATOR_H
#define DX8GL_DX8_SHADER_TRANSLATOR_H

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include "d3d8_types.h"

namespace dx8gl {

// Shader instruction opcodes (used for both vertex and pixel shaders)
enum D3DVS_OPCODE {
    D3DSIO_NOP = 0,
    D3DSIO_MOV = 1,
    D3DSIO_ADD = 2,
    D3DSIO_SUB = 3,
    D3DSIO_MAD = 4,
    D3DSIO_MUL = 5,
    D3DSIO_RCP = 6,
    D3DSIO_RSQ = 7,
    D3DSIO_DP3 = 8,
    D3DSIO_DP4 = 9,
    D3DSIO_MIN = 10,
    D3DSIO_MAX = 11,
    D3DSIO_SLT = 12,
    D3DSIO_SGE = 13,
    D3DSIO_EXP = 14,
    D3DSIO_LOG = 15,
    D3DSIO_LIT = 16,
    D3DSIO_DST = 17,
    D3DSIO_LRP = 18,
    D3DSIO_FRC = 19,
    D3DSIO_M4x4 = 20,
    D3DSIO_M4x3 = 21,
    D3DSIO_M3x4 = 22,
    D3DSIO_M3x3 = 23,
    D3DSIO_M3x2 = 24,
    // Pixel shader specific instructions
    D3DSIO_TEX = 66,      // tex instruction
    D3DSIO_TEXCOORD = 67, // texcoord instruction  
    D3DSIO_TEXKILL = 65,  // texkill instruction
    D3DSIO_CND = 80,      // conditional
    D3DSIO_CMP = 88,      // compare
    D3DSIO_BEM = 89,      // bump environment map
    D3DSIO_PHASE = 0xFFFD, // phase instruction
    D3DSIO_MUL_SAT = 68,  // mul_sat instruction
    D3DSIO_MAD_SAT = 69,  // mad_sat instruction
    D3DSIO_EXPP = 78,
    D3DSIO_LOGP = 79,
    D3DSIO_DCL = 31,      // declare instruction
    D3DSIO_DEF = 81,      // define constant
    D3DSIO_SINCOS = 37,   // sine and cosine
    // D3DSIO_TEXLD = 66,    // texture load (ps.1.4) - same value as D3DSIO_TEX
    D3DSIO_END = 0xFFFF
};

// Register types (used for both vertex and pixel shaders)
enum D3DVS_REGISTER_TYPE {
    D3DSPR_TEMP = 0,      // r# (temporary)
    D3DSPR_INPUT = 1,     // v# (vertex input) / t# (pixel texture coordinate input)
    D3DSPR_CONST = 2,     // c# (constant)
    D3DSPR_ADDR = 3,      // a0 (address register, vertex shader only)
    D3DSPR_TEXTURE = 4,   // t# (texture register for pixel shaders)
    D3DSPR_RASTOUT = 5,   // oPos, oFog, oPts (vertex shader output)
    D3DSPR_ATTROUT = 6,   // oD#, oT# (vertex shader attribute output)
    D3DSPR_COLOROUT = 7,  // oC# (pixel shader color output)
};

// Instruction modifiers
enum InstructionModifier {
    MODIFIER_NONE = 0,
    MODIFIER_SAT = 1,      // _sat - saturate to [0,1]
    MODIFIER_X2 = 2,       // _x2 - multiply by 2
    MODIFIER_X4 = 3,       // _x4 - multiply by 4
    MODIFIER_D2 = 4,       // _d2 - divide by 2
    MODIFIER_BIAS = 5,     // _bias - subtract 0.5
    MODIFIER_BX2 = 6,      // _bx2 - bias then x2 (x * 2 - 1)
    MODIFIER_COMP = 7      // _comp - complement (1 - x)
};

// Parsed shader instruction
struct ShaderInstruction {
    D3DVS_OPCODE opcode;
    InstructionModifier modifier;
    struct Register {
        D3DVS_REGISTER_TYPE type;
        int index;
        std::string swizzle;  // e.g., "xyzw", "xy", "w"
        std::string write_mask; // e.g., "xyzw", "xy" for destination
        bool negate;
        InstructionModifier src_modifier; // Source register modifiers
    };
    Register dest;
    std::vector<Register> sources;
};

// Shader constant definition
struct ShaderConstant {
    std::string name;
    int index;
    int count;  // Number of constant registers used
};

// Shader types
enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_PIXEL
};

class DX8ShaderTranslator {
public:
    DX8ShaderTranslator();
    ~DX8ShaderTranslator();
    
    // Parse DirectX 8 shader assembly (vertex or pixel)
    bool parse_shader(const std::string& source, std::string& error);
    
    // Generate GLSL ES shader (vertex or fragment)
    std::string generate_glsl();
    
    // Get parsed bytecode (for CreateVertexShader/CreatePixelShader)
    std::vector<DWORD> get_bytecode() const { return bytecode_; }
    
    // Get shader constants info
    const std::vector<ShaderConstant>& get_constants() const { return constants_; }
    
    // Get shader type
    ShaderType get_shader_type() const { return shader_type_; }
    
private:
    // Parsing helpers
    bool parse_line(const std::string& line);
    bool parse_instruction(const std::string& line);
    bool parse_register(const std::string& token, ShaderInstruction::Register& reg);
    bool parse_define(const std::string& line);
    
    // GLSL generation helpers
    std::string generate_vertex_glsl();
    std::string generate_pixel_glsl();
    std::string register_to_glsl(const ShaderInstruction::Register& reg);
    std::string instruction_to_glsl(const ShaderInstruction& inst);
    std::string pixel_register_to_glsl(const ShaderInstruction::Register& reg);
    std::string pixel_instruction_to_glsl(const ShaderInstruction& inst);
    std::string apply_modifier(const std::string& value, InstructionModifier modifier);
    
    // Convert instruction to bytecode
    void instruction_to_bytecode(const ShaderInstruction& inst);
    DWORD encode_register(const ShaderInstruction::Register& reg);
    
    // Member variables
    std::vector<ShaderInstruction> instructions_;
    std::vector<ShaderConstant> constants_;
    std::unordered_map<std::string, int> defines_;
    std::vector<DWORD> bytecode_;
    
    // Shader version and type
    int major_version_;
    int minor_version_;
    ShaderType shader_type_;
    
    // Track which registers are used
    bool uses_position_;
    bool uses_color_;
    bool uses_normal_;
    std::set<int> texture_coords_used_;
    std::set<int> output_textures_used_;
    
    // Track varying usage between vertex and pixel shaders
    std::set<int> varying_colors_used_;      // v_color0, v_color1
    std::set<int> varying_texcoords_used_;   // v_texcoord0-7
};

} // namespace dx8gl

#endif // DX8GL_DX8_SHADER_TRANSLATOR_H