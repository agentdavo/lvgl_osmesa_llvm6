#ifndef DX8GL_SHADER_BYTECODE_DISASSEMBLER_H
#define DX8GL_SHADER_BYTECODE_DISASSEMBLER_H

#include <vector>
#include <string>
#include <sstream>
#include "d3d8_types.h"
#include "shader_bytecode_assembler.h"

namespace dx8gl {

class ShaderBytecodeDisassembler {
public:
    // Disassemble bytecode to DirectX assembly string
    static bool disassemble(const std::vector<DWORD>& bytecode, std::string& assembly);
    
private:
    // Helper functions
    static std::string opcode_to_string(DWORD opcode);
    static std::string register_to_string(DWORD token, bool is_dest);
    static std::string decode_swizzle(DWORD swizzle);
    static std::string decode_write_mask(DWORD mask);
    static int get_instruction_length(DWORD inst_token);
    static ShaderBytecode::Opcode get_opcode(DWORD inst_token);
    static bool is_coissue(DWORD inst_token);
    
    // Extract register info from parameter token
    static ShaderBytecode::RegisterType get_register_type(DWORD param_token);
    static int get_register_number(DWORD param_token);
    static ShaderBytecode::SourceModifier get_source_modifier(DWORD param_token);
    static bool has_relative_addressing(DWORD param_token);
};

} // namespace dx8gl

#endif // DX8GL_SHADER_BYTECODE_DISASSEMBLER_H