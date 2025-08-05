#include "shader_bytecode_disassembler.h"
#include "logger.h"
#include <iomanip>

namespace dx8gl {

bool ShaderBytecodeDisassembler::disassemble(const std::vector<DWORD>& bytecode, std::string& assembly) {
    if (bytecode.empty()) {
        return false;
    }
    
    std::ostringstream out;
    size_t pos = 0;
    
    // Check for version token
    DWORD version = bytecode[pos++];
    bool is_vertex_shader = false;
    
    switch (version) {
        case ShaderBytecode::VS_1_1:
            out << "vs.1.1\n";
            is_vertex_shader = true;
            break;
        case ShaderBytecode::PS_1_1:
            out << "ps.1.1\n";
            break;
        case ShaderBytecode::PS_1_2:
            out << "ps.1.2\n";
            break;
        case ShaderBytecode::PS_1_3:
            out << "ps.1.3\n";
            break;
        case ShaderBytecode::PS_1_4:
            out << "ps.1.4\n";
            break;
        default:
            DX8GL_ERROR("Unknown shader version: 0x%08X", version);
            return false;
    }
    
    // Process instructions
    while (pos < bytecode.size()) {
        DWORD inst_token = bytecode[pos];
        
        // Check for end token
        if (inst_token == ShaderBytecode::END_TOKEN) {
            break;
        }
        
        // Check for special tokens
        if ((inst_token & 0x80000000) == 0) {
            // Not an instruction token
            pos++;
            continue;
        }
        
        int inst_length = get_instruction_length(inst_token);
        ShaderBytecode::Opcode opcode = get_opcode(inst_token);
        bool coissue = is_coissue(inst_token);
        
        // Handle special instructions
        if (opcode == ShaderBytecode::OP_DEF) {
            // DEF instruction: def c#, x, y, z, w
            if (pos + 5 < bytecode.size()) {
                DWORD dest_token = bytecode[pos + 1];
                int reg_num = get_register_number(dest_token);
                
                // The next 4 DWORDs are float values
                float* x = reinterpret_cast<float*>(&bytecode[pos + 2]);
                float* y = reinterpret_cast<float*>(&bytecode[pos + 3]);
                float* z = reinterpret_cast<float*>(&bytecode[pos + 4]);
                float* w = reinterpret_cast<float*>(&bytecode[pos + 5]);
                
                out << "    def c" << reg_num << ", " 
                    << *x << ", " << *y << ", " << *z << ", " << *w << "\n";
            }
            pos += 6;
            continue;
        }
        
        if (opcode == ShaderBytecode::OP_DCL) {
            // DCL instruction for vertex shader inputs
            if (pos + 2 < bytecode.size()) {
                DWORD usage_token = bytecode[pos + 1];
                DWORD dest_token = bytecode[pos + 2];
                
                // Extract usage type and index
                DWORD usage = usage_token & 0x1F;
                DWORD usage_index = (usage_token >> 16) & 0xF;
                
                std::string usage_str;
                switch (usage) {
                    case 0: usage_str = "position"; break;
                    case 1: usage_str = "blendweight"; break;
                    case 2: usage_str = "blendindices"; break;
                    case 3: usage_str = "normal"; break;
                    case 4: usage_str = "psize"; break;
                    case 5: usage_str = "texcoord"; break;
                    case 6: usage_str = "tangent"; break;
                    case 7: usage_str = "binormal"; break;
                    case 8: usage_str = "tessfactor"; break;
                    case 9: usage_str = "positiont"; break;
                    case 10: usage_str = "color"; break;
                    case 11: usage_str = "fog"; break;
                    case 12: usage_str = "depth"; break;
                    case 13: usage_str = "sample"; break;
                    default: usage_str = "unknown"; break;
                }
                
                out << "    dcl_" << usage_str;
                if (usage_index > 0 || usage == 5 || usage == 10) {
                    out << usage_index;
                }
                out << " " << register_to_string(dest_token, true) << "\n";
            }
            pos += 3;
            continue;
        }
        
        // Regular instruction
        out << "    ";
        if (coissue) {
            out << "+";
        }
        
        out << opcode_to_string(opcode);
        
        // Process parameters
        bool first_param = true;
        for (int i = 1; i < inst_length && (pos + i) < bytecode.size(); i++) {
            DWORD param_token = bytecode[pos + i];
            
            if (first_param) {
                out << " ";
                first_param = false;
            } else {
                out << ", ";
            }
            
            bool is_dest = (i == 1); // First parameter is usually destination
            out << register_to_string(param_token, is_dest);
        }
        
        out << "\n";
        pos += inst_length;
    }
    
    assembly = out.str();
    return true;
}

std::string ShaderBytecodeDisassembler::opcode_to_string(DWORD opcode) {
    switch (opcode) {
        case ShaderBytecode::OP_NOP: return "nop";
        case ShaderBytecode::OP_MOV: return "mov";
        case ShaderBytecode::OP_ADD: return "add";
        case ShaderBytecode::OP_SUB: return "sub";
        case ShaderBytecode::OP_MAD: return "mad";
        case ShaderBytecode::OP_MUL: return "mul";
        case ShaderBytecode::OP_RCP: return "rcp";
        case ShaderBytecode::OP_RSQ: return "rsq";
        case ShaderBytecode::OP_DP3: return "dp3";
        case ShaderBytecode::OP_DP4: return "dp4";
        case ShaderBytecode::OP_MIN: return "min";
        case ShaderBytecode::OP_MAX: return "max";
        case ShaderBytecode::OP_SLT: return "slt";
        case ShaderBytecode::OP_SGE: return "sge";
        case ShaderBytecode::OP_EXP: return "exp";
        case ShaderBytecode::OP_LOG: return "log";
        case ShaderBytecode::OP_LIT: return "lit";
        case ShaderBytecode::OP_DST: return "dst";
        case ShaderBytecode::OP_LRP: return "lrp";
        case ShaderBytecode::OP_FRC: return "frc";
        case ShaderBytecode::OP_M4x4: return "m4x4";
        case ShaderBytecode::OP_M4x3: return "m4x3";
        case ShaderBytecode::OP_M3x4: return "m3x4";
        case ShaderBytecode::OP_M3x3: return "m3x3";
        case ShaderBytecode::OP_M3x2: return "m3x2";
        case ShaderBytecode::OP_SINCOS: return "sincos";
        case ShaderBytecode::OP_MOVA: return "mova";
        case ShaderBytecode::OP_TEXKILL: return "texkill";
        case ShaderBytecode::OP_TEX: return "tex";
        case ShaderBytecode::OP_CND: return "cnd";
        case ShaderBytecode::OP_CMP: return "cmp";
        case ShaderBytecode::OP_BEM: return "bem";
        case ShaderBytecode::OP_DP2ADD: return "dp2add";
        default: return "unknown";
    }
}

std::string ShaderBytecodeDisassembler::register_to_string(DWORD token, bool is_dest) {
    ShaderBytecode::RegisterType reg_type = get_register_type(token);
    int reg_num = get_register_number(token);
    
    std::ostringstream out;
    
    // Handle source modifiers
    if (!is_dest) {
        ShaderBytecode::SourceModifier src_mod = get_source_modifier(token);
        if (src_mod == ShaderBytecode::SRCMOD_NEGATE) {
            out << "-";
        }
    }
    
    // Register prefix
    switch (reg_type) {
        case ShaderBytecode::REG_TEMP:
            out << "r" << reg_num;
            break;
        case ShaderBytecode::REG_INPUT:
            out << "v" << reg_num;
            break;
        case ShaderBytecode::REG_CONST:
            out << "c" << reg_num;
            if (has_relative_addressing(token)) {
                out << "[a0.x]";
            }
            break;
        case ShaderBytecode::REG_TEXTURE:
            out << "t" << reg_num;
            break;
        case ShaderBytecode::REG_RASTOUT:
            switch (reg_num) {
                case 0: out << "oPos"; break;
                case 1: out << "oFog"; break;
                case 2: out << "oPts"; break;
                default: out << "oRast" << reg_num; break;
            }
            break;
        case ShaderBytecode::REG_ATTROUT:
            if (reg_num < 2) {
                out << "oD" << reg_num;
            } else {
                out << "oT" << (reg_num - 2);
            }
            break;
        case ShaderBytecode::REG_OUTPUT:
            out << "oC" << reg_num;
            break;
        default:
            out << "?" << reg_num;
            break;
    }
    
    // Handle swizzle/mask
    DWORD swizzle_mask = (token >> 16) & 0xFF;
    
    if (is_dest) {
        std::string mask = decode_write_mask(swizzle_mask);
        if (mask != ".xyzw") {
            out << mask;
        }
    } else {
        std::string swizzle = decode_swizzle(swizzle_mask);
        if (swizzle != ".xyzw") {
            out << swizzle;
        }
    }
    
    return out.str();
}

std::string ShaderBytecodeDisassembler::decode_swizzle(DWORD swizzle) {
    const char components[] = "xyzw";
    std::string result = ".";
    
    result += components[(swizzle >> 0) & 0x3];
    result += components[(swizzle >> 2) & 0x3];
    result += components[(swizzle >> 4) & 0x3];
    result += components[(swizzle >> 6) & 0x3];
    
    // Simplify if all same component
    if (result == ".xxxx") return ".x";
    if (result == ".yyyy") return ".y";
    if (result == ".zzzz") return ".z";
    if (result == ".wwww") return ".w";
    
    return result;
}

std::string ShaderBytecodeDisassembler::decode_write_mask(DWORD mask) {
    std::string result = ".";
    
    if (mask & ShaderBytecode::WRITEMASK_X) result += "x";
    if (mask & ShaderBytecode::WRITEMASK_Y) result += "y";
    if (mask & ShaderBytecode::WRITEMASK_Z) result += "z";
    if (mask & ShaderBytecode::WRITEMASK_W) result += "w";
    
    return result;
}

int ShaderBytecodeDisassembler::get_instruction_length(DWORD inst_token) {
    return (inst_token >> 24) & 0xF;
}

ShaderBytecode::Opcode ShaderBytecodeDisassembler::get_opcode(DWORD inst_token) {
    return static_cast<ShaderBytecode::Opcode>(inst_token & 0xFFFF);
}

bool ShaderBytecodeDisassembler::is_coissue(DWORD inst_token) {
    return (inst_token & 0x40000000) != 0;
}

ShaderBytecode::RegisterType ShaderBytecodeDisassembler::get_register_type(DWORD param_token) {
    return static_cast<ShaderBytecode::RegisterType>((param_token >> 28) & 0x7);
}

int ShaderBytecodeDisassembler::get_register_number(DWORD param_token) {
    return param_token & 0x7FF;
}

ShaderBytecode::SourceModifier ShaderBytecodeDisassembler::get_source_modifier(DWORD param_token) {
    return static_cast<ShaderBytecode::SourceModifier>((param_token >> 24) & 0xF);
}

bool ShaderBytecodeDisassembler::has_relative_addressing(DWORD param_token) {
    return (param_token & 0x08000000) != 0;
}

} // namespace dx8gl