#ifndef DX8GL_SHADER_BYTECODE_ASSEMBLER_H
#define DX8GL_SHADER_BYTECODE_ASSEMBLER_H

#include <vector>
#include <string>
#include <unordered_map>
#include "d3d8_types.h"

namespace dx8gl {

// DirectX 8 shader bytecode format constants
namespace ShaderBytecode {
    // Version tokens
    const DWORD VS_1_1 = 0xFFFE0101;
    const DWORD PS_1_1 = 0xFFFF0101;
    const DWORD PS_1_2 = 0xFFFF0102;
    const DWORD PS_1_3 = 0xFFFF0103;
    const DWORD PS_1_4 = 0xFFFF0104;
    const DWORD END_TOKEN = 0x0000FFFF;
    
    // Instruction token format (bits)
    // [31]    - Instruction present (always 1)
    // [30]    - Co-issue (parallel execution in alpha pipe)
    // [29-24] - Predicate (not used in DX8)
    // [23-16] - Instruction length (including this token)
    // [15-0]  - Opcode
    
    // Parameter token format (bits)
    // [31]    - Parameter present (always 1)
    // [30-28] - Register type
    // [27]    - Relative addressing (vs only)
    // [26-24] - Reserved
    // [23-16] - Write mask / Source swizzle
    // [15-13] - Result modifier
    // [12-11] - Result shift scale
    // [10-0]  - Register number
    
    // Opcodes
    enum Opcode {
        OP_NOP = 0,
        OP_MOV = 1,
        OP_ADD = 2,
        OP_SUB = 3,
        OP_MAD = 4,
        OP_MUL = 5,
        OP_RCP = 6,
        OP_RSQ = 7,
        OP_DP3 = 8,
        OP_DP4 = 9,
        OP_MIN = 10,
        OP_MAX = 11,
        OP_SLT = 12,
        OP_SGE = 13,
        OP_EXP = 14,
        OP_LOG = 15,
        OP_LIT = 16,
        OP_DST = 17,
        OP_LRP = 18,
        OP_FRC = 19,
        OP_M4x4 = 20,
        OP_M4x3 = 21,
        OP_M3x4 = 22,
        OP_M3x3 = 23,
        OP_M3x2 = 24,
        OP_CALL = 25,
        OP_CALLNZ = 26,
        OP_LOOP = 27,
        OP_RET = 28,
        OP_ENDLOOP = 29,
        OP_LABEL = 30,
        OP_DCL = 31,
        OP_POW = 32,
        OP_CRS = 33,
        OP_SGN = 34,
        OP_ABS = 35,
        OP_NRM = 36,
        OP_SINCOS = 37,
        OP_REP = 38,
        OP_ENDREP = 39,
        OP_IF = 40,
        OP_IFC = 41,
        OP_ELSE = 42,
        OP_ENDIF = 43,
        OP_BREAK = 44,
        OP_BREAKC = 45,
        OP_MOVA = 46,
        OP_DEFB = 47,
        OP_DEFI = 48,
        // Pixel shader specific
        OP_TEXKILL = 65,
        OP_TEX = 66,
        OP_TEXBEM = 67,
        OP_TEXBEML = 68,
        OP_TEXREG2AR = 69,
        OP_TEXREG2GB = 70,
        OP_TEXM3x2PAD = 71,
        OP_TEXM3x2TEX = 72,
        OP_TEXM3x3PAD = 73,
        OP_TEXM3x3TEX = 74,
        OP_TEXM3x3SPEC = 76,
        OP_TEXM3x3VSPEC = 77,
        OP_EXPP = 78,
        OP_LOGP = 79,
        OP_CND = 80,
        OP_DEF = 81,
        OP_TEXREG2RGB = 82,
        OP_TEXDP3TEX = 83,
        OP_TEXM3x2DEPTH = 84,
        OP_TEXDP3 = 85,
        OP_TEXM3x3 = 86,
        OP_TEXDEPTH = 87,
        OP_CMP = 88,
        OP_BEM = 89,
        OP_DP2ADD = 90,
        OP_DSX = 91,
        OP_DSY = 92,
        OP_TEXLDD = 93,
        OP_SETP = 94,
        OP_TEXLDL = 95,
        OP_BREAKP = 96,
        OP_PHASE = 0xFFFD,
        OP_COMMENT = 0xFFFE,
        OP_END = 0xFFFF
    };
    
    // Register types
    enum RegisterType {
        REG_TEMP = 0,
        REG_INPUT = 1,
        REG_CONST = 2,
        REG_ADDR = 3,
        REG_TEXTURE = 3, // PS: texture coordinate
        REG_RASTOUT = 4,
        REG_ATTROUT = 5,
        REG_TEXCRDOUT = 6,
        REG_OUTPUT = 6,  // PS: output color
        REG_CONSTINT = 7,
        REG_COLOROUT = 8,
        REG_DEPTHOUT = 9,
        REG_SAMPLER = 10,
        REG_CONST2 = 11,
        REG_CONST3 = 12,
        REG_CONST4 = 13,
        REG_CONSTBOOL = 14,
        REG_LOOP = 15,
        REG_TEMPFLOAT16 = 16,
        REG_MISCTYPE = 17,
        REG_LABEL = 18,
        REG_PREDICATE = 19
    };
    
    // Write masks
    enum WriteMask {
        WRITEMASK_X = 0x1,
        WRITEMASK_Y = 0x2,
        WRITEMASK_Z = 0x4,
        WRITEMASK_W = 0x8,
        WRITEMASK_ALL = 0xF
    };
    
    // Source modifiers
    enum SourceModifier {
        SRCMOD_NONE = 0,
        SRCMOD_NEGATE = 1,
        SRCMOD_BIAS = 2,
        SRCMOD_BIASNEGATE = 3,
        SRCMOD_SIGN = 4,
        SRCMOD_SIGNNEGATE = 5,
        SRCMOD_COMP = 6,
        SRCMOD_X2 = 7,
        SRCMOD_X2NEGATE = 8,
        SRCMOD_DZ = 9,
        SRCMOD_DW = 10,
        SRCMOD_ABS = 11,
        SRCMOD_ABSNEGATE = 12,
        SRCMOD_NOT = 13
    };
    
    // Result modifiers  
    enum ResultModifier {
        RESMOD_NONE = 0,
        RESMOD_SATURATE = 1,
        RESMOD_PARTIALPRECISION = 2,
        RESMOD_CENTROID = 4
    };
    
    // Result shift
    enum ResultShift {
        RESSHIFT_NONE = 0,
        RESSHIFT_X2 = 1,
        RESSHIFT_X4 = 2,
        RESSHIFT_X8 = 3,
        RESSHIFT_X16 = 4,
        RESSHIFT_X32 = 5,
        RESSHIFT_X64 = 6,
        RESSHIFT_X128 = 7,
        RESSHIFT_D2 = 0xF,
        RESSHIFT_D4 = 0xE,
        RESSHIFT_D8 = 0xD,
        RESSHIFT_D16 = 0xC,
        RESSHIFT_D32 = 0xB,
        RESSHIFT_D64 = 0xA,
        RESSHIFT_D128 = 0x9
    };
}

// Shader bytecode assembler
class ShaderBytecodeAssembler {
public:
    ShaderBytecodeAssembler();
    ~ShaderBytecodeAssembler();
    
    // Clear the current bytecode
    void clear();
    
    // Set shader version
    void set_version(DWORD version);
    
    // Add def instruction (constant definition)
    void add_def(int reg, float x, float y, float z, float w);
    
    // Add instruction with destination only (e.g., texkill)
    void add_instruction(ShaderBytecode::Opcode opcode, 
                        int dest_reg, 
                        ShaderBytecode::RegisterType dest_type,
                        DWORD dest_mask = ShaderBytecode::WRITEMASK_ALL);
    
    // Add instruction with one source (e.g., mov, rcp, rsq)
    void add_instruction(ShaderBytecode::Opcode opcode,
                        int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                        int src_reg, ShaderBytecode::RegisterType src_type, DWORD src_swizzle,
                        ShaderBytecode::SourceModifier src_mod = ShaderBytecode::SRCMOD_NONE);
    
    // Add instruction with two sources (e.g., add, mul, dp3)
    void add_instruction(ShaderBytecode::Opcode opcode,
                        int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                        int src0_reg, ShaderBytecode::RegisterType src0_type, DWORD src0_swizzle,
                        int src1_reg, ShaderBytecode::RegisterType src1_type, DWORD src1_swizzle,
                        ShaderBytecode::SourceModifier src0_mod = ShaderBytecode::SRCMOD_NONE,
                        ShaderBytecode::SourceModifier src1_mod = ShaderBytecode::SRCMOD_NONE);
    
    // Add instruction with three sources (e.g., mad, lrp, cnd)
    void add_instruction(ShaderBytecode::Opcode opcode,
                        int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                        int src0_reg, ShaderBytecode::RegisterType src0_type, DWORD src0_swizzle,
                        int src1_reg, ShaderBytecode::RegisterType src1_type, DWORD src1_swizzle,
                        int src2_reg, ShaderBytecode::RegisterType src2_type, DWORD src2_swizzle,
                        ShaderBytecode::SourceModifier src0_mod = ShaderBytecode::SRCMOD_NONE,
                        ShaderBytecode::SourceModifier src1_mod = ShaderBytecode::SRCMOD_NONE,
                        ShaderBytecode::SourceModifier src2_mod = ShaderBytecode::SRCMOD_NONE);
    
    // Add instruction with result modifier (e.g., _sat)
    void set_instruction_modifier(ShaderBytecode::ResultModifier result_mod,
                                 ShaderBytecode::ResultShift result_shift = ShaderBytecode::RESSHIFT_NONE);
    
    // Add co-issue marker (+) for parallel execution
    void set_coissue(bool coissue);
    
    // Add phase instruction (ps.1.4)
    void add_phase();
    
    // Add comment
    void add_comment(const std::string& comment);
    
    // Finalize and get bytecode
    std::vector<DWORD> get_bytecode();
    
    // Get bytecode size in DWORDs
    size_t get_size() const { return bytecode_.size(); }
    
    // Utility functions
    static DWORD encode_swizzle(const std::string& swizzle);
    static DWORD encode_write_mask(const std::string& mask);
    
private:
    // Build instruction token
    DWORD build_instruction_token(ShaderBytecode::Opcode opcode, int param_count, 
                                 bool coissue = false);
    
    // Build parameter token
    DWORD build_parameter_token(ShaderBytecode::RegisterType type, int reg_num,
                               bool is_dest, DWORD mask_or_swizzle,
                               ShaderBytecode::SourceModifier src_mod = ShaderBytecode::SRCMOD_NONE);
    
    // Build destination parameter with modifiers
    DWORD build_dest_parameter(ShaderBytecode::RegisterType type, int reg_num,
                              DWORD write_mask, 
                              ShaderBytecode::ResultModifier result_mod,
                              ShaderBytecode::ResultShift result_shift);
    
    // Member variables
    std::vector<DWORD> bytecode_;
    DWORD version_;
    bool has_version_;
    ShaderBytecode::ResultModifier pending_result_mod_;
    ShaderBytecode::ResultShift pending_result_shift_;
    bool pending_coissue_;
};

} // namespace dx8gl

#endif // DX8GL_SHADER_BYTECODE_ASSEMBLER_H