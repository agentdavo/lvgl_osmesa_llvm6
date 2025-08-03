#include "shader_bytecode_assembler.h"
#include "logger.h"
#include <cstring>
#include <algorithm>

namespace dx8gl {

ShaderBytecodeAssembler::ShaderBytecodeAssembler() 
    : version_(0), has_version_(false), 
      pending_result_mod_(ShaderBytecode::RESMOD_NONE),
      pending_result_shift_(ShaderBytecode::RESSHIFT_NONE),
      pending_coissue_(false) {
}

ShaderBytecodeAssembler::~ShaderBytecodeAssembler() {
}

void ShaderBytecodeAssembler::clear() {
    bytecode_.clear();
    version_ = 0;
    has_version_ = false;
    pending_result_mod_ = ShaderBytecode::RESMOD_NONE;
    pending_result_shift_ = ShaderBytecode::RESSHIFT_NONE;
    pending_coissue_ = false;
}

void ShaderBytecodeAssembler::set_version(DWORD version) {
    if (!has_version_) {
        bytecode_.push_back(version);
        version_ = version;
        has_version_ = true;
    }
}

void ShaderBytecodeAssembler::add_def(int reg, float x, float y, float z, float w) {
    // def instruction format:
    // Token 0: Instruction (DEF opcode)
    // Token 1: Destination parameter (const register)
    // Token 2-5: Four float values
    
    DWORD inst_token = build_instruction_token(ShaderBytecode::OP_DEF, 5, false);
    bytecode_.push_back(inst_token);
    
    DWORD dest_token = build_parameter_token(ShaderBytecode::REG_CONST, reg, true, 
                                            ShaderBytecode::WRITEMASK_ALL);
    bytecode_.push_back(dest_token);
    
    // Add float values as DWORDs
    bytecode_.push_back(*reinterpret_cast<DWORD*>(&x));
    bytecode_.push_back(*reinterpret_cast<DWORD*>(&y));
    bytecode_.push_back(*reinterpret_cast<DWORD*>(&z));
    bytecode_.push_back(*reinterpret_cast<DWORD*>(&w));
}

void ShaderBytecodeAssembler::add_instruction(ShaderBytecode::Opcode opcode,
                                             int dest_reg, ShaderBytecode::RegisterType dest_type,
                                             DWORD dest_mask) {
    DWORD inst_token = build_instruction_token(opcode, 1, pending_coissue_);
    bytecode_.push_back(inst_token);
    
    DWORD dest_token = build_dest_parameter(dest_type, dest_reg, dest_mask,
                                           pending_result_mod_, pending_result_shift_);
    bytecode_.push_back(dest_token);
    
    // Reset modifiers
    pending_result_mod_ = ShaderBytecode::RESMOD_NONE;
    pending_result_shift_ = ShaderBytecode::RESSHIFT_NONE;
    pending_coissue_ = false;
}

void ShaderBytecodeAssembler::add_instruction(ShaderBytecode::Opcode opcode,
                                             int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                                             int src_reg, ShaderBytecode::RegisterType src_type, DWORD src_swizzle,
                                             ShaderBytecode::SourceModifier src_mod) {
    DWORD inst_token = build_instruction_token(opcode, 2, pending_coissue_);
    bytecode_.push_back(inst_token);
    
    DWORD dest_token = build_dest_parameter(dest_type, dest_reg, dest_mask,
                                           pending_result_mod_, pending_result_shift_);
    bytecode_.push_back(dest_token);
    
    DWORD src_token = build_parameter_token(src_type, src_reg, false, src_swizzle, src_mod);
    bytecode_.push_back(src_token);
    
    // Reset modifiers
    pending_result_mod_ = ShaderBytecode::RESMOD_NONE;
    pending_result_shift_ = ShaderBytecode::RESSHIFT_NONE;
    pending_coissue_ = false;
}

void ShaderBytecodeAssembler::add_instruction(ShaderBytecode::Opcode opcode,
                                             int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                                             int src0_reg, ShaderBytecode::RegisterType src0_type, DWORD src0_swizzle,
                                             int src1_reg, ShaderBytecode::RegisterType src1_type, DWORD src1_swizzle,
                                             ShaderBytecode::SourceModifier src0_mod,
                                             ShaderBytecode::SourceModifier src1_mod) {
    DWORD inst_token = build_instruction_token(opcode, 3, pending_coissue_);
    bytecode_.push_back(inst_token);
    
    DWORD dest_token = build_dest_parameter(dest_type, dest_reg, dest_mask,
                                           pending_result_mod_, pending_result_shift_);
    bytecode_.push_back(dest_token);
    
    DWORD src0_token = build_parameter_token(src0_type, src0_reg, false, src0_swizzle, src0_mod);
    bytecode_.push_back(src0_token);
    
    DWORD src1_token = build_parameter_token(src1_type, src1_reg, false, src1_swizzle, src1_mod);
    bytecode_.push_back(src1_token);
    
    // Reset modifiers
    pending_result_mod_ = ShaderBytecode::RESMOD_NONE;
    pending_result_shift_ = ShaderBytecode::RESSHIFT_NONE;
    pending_coissue_ = false;
}

void ShaderBytecodeAssembler::add_instruction(ShaderBytecode::Opcode opcode,
                                             int dest_reg, ShaderBytecode::RegisterType dest_type, DWORD dest_mask,
                                             int src0_reg, ShaderBytecode::RegisterType src0_type, DWORD src0_swizzle,
                                             int src1_reg, ShaderBytecode::RegisterType src1_type, DWORD src1_swizzle,
                                             int src2_reg, ShaderBytecode::RegisterType src2_type, DWORD src2_swizzle,
                                             ShaderBytecode::SourceModifier src0_mod,
                                             ShaderBytecode::SourceModifier src1_mod,
                                             ShaderBytecode::SourceModifier src2_mod) {
    DWORD inst_token = build_instruction_token(opcode, 4, pending_coissue_);
    bytecode_.push_back(inst_token);
    
    DWORD dest_token = build_dest_parameter(dest_type, dest_reg, dest_mask,
                                           pending_result_mod_, pending_result_shift_);
    bytecode_.push_back(dest_token);
    
    DWORD src0_token = build_parameter_token(src0_type, src0_reg, false, src0_swizzle, src0_mod);
    bytecode_.push_back(src0_token);
    
    DWORD src1_token = build_parameter_token(src1_type, src1_reg, false, src1_swizzle, src1_mod);
    bytecode_.push_back(src1_token);
    
    DWORD src2_token = build_parameter_token(src2_type, src2_reg, false, src2_swizzle, src2_mod);
    bytecode_.push_back(src2_token);
    
    // Reset modifiers
    pending_result_mod_ = ShaderBytecode::RESMOD_NONE;
    pending_result_shift_ = ShaderBytecode::RESSHIFT_NONE;
    pending_coissue_ = false;
}

void ShaderBytecodeAssembler::set_instruction_modifier(ShaderBytecode::ResultModifier result_mod,
                                                      ShaderBytecode::ResultShift result_shift) {
    pending_result_mod_ = result_mod;
    pending_result_shift_ = result_shift;
}

void ShaderBytecodeAssembler::set_coissue(bool coissue) {
    pending_coissue_ = coissue;
}

void ShaderBytecodeAssembler::add_phase() {
    DWORD inst_token = build_instruction_token(ShaderBytecode::OP_PHASE, 0, false);
    bytecode_.push_back(inst_token);
}

void ShaderBytecodeAssembler::add_comment(const std::string& comment) {
    // Comment format:
    // Token 0: Comment opcode with length
    // Token 1+: Comment data (packed as DWORDs)
    
    size_t comment_len = comment.length();
    size_t dword_count = (comment_len + 3) / 4; // Round up to DWORD boundary
    
    DWORD inst_token = ShaderBytecode::OP_COMMENT | ((dword_count + 1) << 16) | 0x80000000;
    bytecode_.push_back(inst_token);
    
    // Pack comment string into DWORDs
    for (size_t i = 0; i < dword_count; i++) {
        DWORD packed = 0;
        for (size_t j = 0; j < 4 && (i * 4 + j) < comment_len; j++) {
            packed |= (static_cast<DWORD>(comment[i * 4 + j]) << (j * 8));
        }
        bytecode_.push_back(packed);
    }
}

std::vector<DWORD> ShaderBytecodeAssembler::get_bytecode() {
    if (!bytecode_.empty() && bytecode_.back() != ShaderBytecode::END_TOKEN) {
        bytecode_.push_back(ShaderBytecode::END_TOKEN);
    }
    return bytecode_;
}

DWORD ShaderBytecodeAssembler::build_instruction_token(ShaderBytecode::Opcode opcode, 
                                                       int param_count, bool coissue) {
    DWORD token = 0x80000000; // Instruction present bit
    
    if (coissue) {
        token |= 0x40000000; // Co-issue bit
    }
    
    // Instruction length (including this token)
    token |= ((param_count + 1) << 24);
    
    // Opcode
    token |= (opcode & 0xFFFF);
    
    return token;
}

DWORD ShaderBytecodeAssembler::build_parameter_token(ShaderBytecode::RegisterType type, 
                                                     int reg_num, bool is_dest, 
                                                     DWORD mask_or_swizzle,
                                                     ShaderBytecode::SourceModifier src_mod) {
    DWORD token = 0x80000000; // Parameter present bit
    
    // Register type
    token |= (type << 28);
    
    // Register number
    token |= (reg_num & 0x7FF);
    
    if (is_dest) {
        // Write mask (bits 16-19)
        token |= ((mask_or_swizzle & 0xF) << 16);
    } else {
        // Source swizzle (bits 16-23)
        token |= ((mask_or_swizzle & 0xFF) << 16);
        
        // Source modifier (bits 24-27)
        token |= ((src_mod & 0xF) << 24);
    }
    
    return token;
}

DWORD ShaderBytecodeAssembler::build_dest_parameter(ShaderBytecode::RegisterType type, 
                                                    int reg_num, DWORD write_mask,
                                                    ShaderBytecode::ResultModifier result_mod,
                                                    ShaderBytecode::ResultShift result_shift) {
    DWORD token = build_parameter_token(type, reg_num, true, write_mask);
    
    // Result modifier (bits 13-15)
    token |= ((result_mod & 0x7) << 13);
    
    // Result shift (bits 11-12 for shift amount, bit 15 for direction)
    if (result_shift != ShaderBytecode::RESSHIFT_NONE) {
        if (result_shift >= ShaderBytecode::RESSHIFT_D128) {
            // Division shifts
            token |= 0x8000; // Set bit 15 for division
            token |= (((15 - result_shift) & 0x3) << 11);
        } else {
            // Multiplication shifts
            token |= ((result_shift & 0x3) << 11);
        }
    }
    
    return token;
}

DWORD ShaderBytecodeAssembler::encode_swizzle(const std::string& swizzle) {
    DWORD result = 0;
    
    // Map component letters to indices
    auto component_to_index = [](char c) -> DWORD {
        switch (c) {
            case 'x': case 'r': return 0;
            case 'y': case 'g': return 1;
            case 'z': case 'b': return 2;
            case 'w': case 'a': return 3;
            default: return 0;
        }
    };
    
    // Default to .xyzw if empty
    std::string sw = swizzle.empty() ? "xyzw" : swizzle;
    
    // Pad with last component if necessary
    while (sw.length() < 4) {
        sw += sw.back();
    }
    
    // Encode each component (2 bits each)
    for (int i = 0; i < 4; i++) {
        result |= (component_to_index(sw[i]) << (i * 2));
    }
    
    return result;
}

DWORD ShaderBytecodeAssembler::encode_write_mask(const std::string& mask) {
    DWORD result = 0;
    
    if (mask.empty()) {
        return ShaderBytecode::WRITEMASK_ALL;
    }
    
    for (char c : mask) {
        switch (c) {
            case 'x': case 'r': result |= ShaderBytecode::WRITEMASK_X; break;
            case 'y': case 'g': result |= ShaderBytecode::WRITEMASK_Y; break;
            case 'z': case 'b': result |= ShaderBytecode::WRITEMASK_Z; break;
            case 'w': case 'a': result |= ShaderBytecode::WRITEMASK_W; break;
        }
    }
    
    return result;
}

} // namespace dx8gl