#include "dx8_shader_translator.h"
#include "logger.h"
#include "d3d8_constants.h"
#include "gl3_headers.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <exception>
#include <stdexcept>
#include <set>

namespace dx8gl {

DX8ShaderTranslator::DX8ShaderTranslator() 
    : major_version_(1), minor_version_(1), shader_type_(SHADER_TYPE_VERTEX),
      uses_position_(false), uses_color_(false), uses_normal_(false) {
}

DX8ShaderTranslator::~DX8ShaderTranslator() {
}

bool DX8ShaderTranslator::parse_shader(const std::string& source, std::string& error) {
    std::istringstream stream(source);
    std::string line;
    
    // Clear previous state
    instructions_.clear();
    constants_.clear();
    defines_.clear();
    bytecode_.clear();
    texture_coords_used_.clear();
    output_textures_used_.clear();
    
    // Don't add version token yet - we need to determine shader type first
    
    while (std::getline(stream, line)) {
        // Remove comments
        size_t comment_pos = line.find(';');
        if (comment_pos != std::string::npos && 
            (comment_pos == 0 || line[comment_pos-1] != '\'')) {
            line = line.substr(0, comment_pos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (!parse_line(line)) {
            error = "Failed to parse line: " + line;
            return false;
        }
    }
    
    // Add end token
    bytecode_.push_back(0x0000FFFF);
    
    return true;
}

bool DX8ShaderTranslator::parse_line(const std::string& line) {
    // Check for preprocessor directives
    if (line[0] == '#') {
        return parse_define(line);
    }
    
    // Check for version declaration
    if (line.substr(0, 3) == "vs.") {
        // Vertex shader version
        shader_type_ = SHADER_TYPE_VERTEX;
        
        // Parse version number
        if (line.length() >= 6 && line[3] >= '1' && line[5] >= '0') {
            major_version_ = line[3] - '0';
            minor_version_ = line[5] - '0';
        } else {
            major_version_ = 1;
            minor_version_ = 1;
        }
        
        // Add version token to bytecode
        DWORD version_token = 0xFFFE0000 | (major_version_ << 8) | minor_version_;
        bytecode_.push_back(version_token); // e.g., 0xFFFE0101 for vs_1_1
        
        return true;
    } else if (line.substr(0, 3) == "ps.") {
        // Pixel shader version
        shader_type_ = SHADER_TYPE_PIXEL;
        
        // Parse version number
        if (line.length() >= 6 && line[3] >= '1' && line[5] >= '0') {
            major_version_ = line[3] - '0';
            minor_version_ = line[5] - '0';
        } else {
            major_version_ = 1;
            minor_version_ = 1;
        }
        
        // Add version token to bytecode
        DWORD version_token = 0xFFFF0000 | (major_version_ << 8) | minor_version_;
        bytecode_.push_back(version_token); // e.g., 0xFFFF0104 for ps_1_4
        
        return true;
    }
    
    // Otherwise it's an instruction
    return parse_instruction(line);
}

bool DX8ShaderTranslator::parse_define(const std::string& line) {
    std::regex define_regex("#define\\s+(\\w+)\\s+(.+)");
    std::smatch match;
    
    if (std::regex_match(line, match, define_regex)) {
        std::string name = match[1];
        std::string value_str = match[2];
        
        // Try to parse as integer
        try {
            int value = std::stoi(value_str);
            defines_[name] = value;
            
            // Track constant definitions
            if (name.substr(0, 3) == "CV_") {
                ShaderConstant constant;
                constant.name = name;
                constant.index = value;
                constant.count = 1;
                
                // Check for matrix constants (they use multiple registers)
                if (name.find("WORLDVIEWPROJ") != std::string::npos ||
                    name.find("TEXPROJ") != std::string::npos) {
                    constant.count = 4;
                }
                
                constants_.push_back(constant);
            }
        } catch (...) {
            // Not a numeric define, ignore for now
        }
    }
    
    return true;
}

bool DX8ShaderTranslator::parse_instruction(const std::string& line) {
    std::istringstream stream(line);
    std::string opcode_str;
    stream >> opcode_str;
    
    // Convert to lowercase
    std::transform(opcode_str.begin(), opcode_str.end(), opcode_str.begin(), ::tolower);
    
    ShaderInstruction inst;
    inst.modifier = MODIFIER_NONE;
    
    // Special handling for dcl_* instructions
    if (opcode_str.substr(0, 4) == "dcl_") {
        inst.opcode = D3DSIO_DCL;
        // The usage type (position, normal, texcoord) is stored in the modifier field for now
        std::string usage = opcode_str.substr(4);
        // For dcl instructions, we'll parse this differently
        
        // Parse the register
        std::string reg_str;
        stream >> reg_str;
        if (!parse_register(reg_str, inst.dest)) {
            return false;
        }
        
        // Store the usage type as metadata (we'll need to enhance this)
        instructions_.push_back(inst);
        
        // Add to bytecode
        DWORD token = (inst.opcode & 0xFFFF) | ((0) << 16); // dcl has no additional info in high word
        bytecode_.push_back(token);
        
        // Add destination token
        DWORD dest_token = encode_register(inst.dest);
        bytecode_.push_back(dest_token);
        
        // Add usage token based on the dcl type
        DWORD usage_token = 0;
        if (usage == "position") {
            usage_token = (0 << 0) | (D3DVSDT_FLOAT3 << 16); // position usage
            uses_position_ = true;
        } else if (usage == "normal") {
            usage_token = (3 << 0) | (D3DVSDT_FLOAT3 << 16); // normal usage
            uses_normal_ = true;
        } else if (usage == "texcoord") {
            usage_token = (5 << 0) | (D3DVSDT_FLOAT2 << 16); // texcoord usage
            texture_coords_used_.insert(inst.dest.index);
        }
        bytecode_.push_back(usage_token);
        
        return true;
    }
    
    // Check for instruction modifiers
    size_t modifier_pos = opcode_str.find('_');
    if (modifier_pos != std::string::npos) {
        std::string modifier = opcode_str.substr(modifier_pos);
        opcode_str = opcode_str.substr(0, modifier_pos);
        
        if (modifier == "_sat") inst.modifier = MODIFIER_SAT;
        else if (modifier == "_x2") inst.modifier = MODIFIER_X2;
        else if (modifier == "_x4") inst.modifier = MODIFIER_X4;
        else if (modifier == "_d2") inst.modifier = MODIFIER_D2;
        else if (modifier == "_bias") inst.modifier = MODIFIER_BIAS;
        else if (modifier == "_bx2") inst.modifier = MODIFIER_BX2;
        else if (modifier == "_comp") inst.modifier = MODIFIER_COMP;
    }
    
    // Parse opcode
    if (opcode_str == "mov") inst.opcode = D3DSIO_MOV;
    else if (opcode_str == "add") inst.opcode = D3DSIO_ADD;
    else if (opcode_str == "sub") inst.opcode = D3DSIO_SUB;
    else if (opcode_str == "mad") inst.opcode = D3DSIO_MAD;
    else if (opcode_str == "mul") inst.opcode = D3DSIO_MUL;
    else if (opcode_str == "rcp") inst.opcode = D3DSIO_RCP;
    else if (opcode_str == "rsq") inst.opcode = D3DSIO_RSQ;
    else if (opcode_str == "dp3") inst.opcode = D3DSIO_DP3;
    else if (opcode_str == "dp4") inst.opcode = D3DSIO_DP4;
    else if (opcode_str == "min") inst.opcode = D3DSIO_MIN;
    else if (opcode_str == "max") inst.opcode = D3DSIO_MAX;
    else if (opcode_str == "slt") inst.opcode = D3DSIO_SLT;
    else if (opcode_str == "sge") inst.opcode = D3DSIO_SGE;
    else if (opcode_str == "m4x4") inst.opcode = D3DSIO_M4x4;
    else if (opcode_str == "m4x3") inst.opcode = D3DSIO_M4x3;
    else if (opcode_str == "m3x4") inst.opcode = D3DSIO_M3x4;
    else if (opcode_str == "m3x3") inst.opcode = D3DSIO_M3x3;
    else if (opcode_str == "m3x2") inst.opcode = D3DSIO_M3x2;
    else if (opcode_str == "exp") inst.opcode = D3DSIO_EXP;
    else if (opcode_str == "log") inst.opcode = D3DSIO_LOG;
    else if (opcode_str == "lit") inst.opcode = D3DSIO_LIT;
    else if (opcode_str == "dst") inst.opcode = D3DSIO_DST;
    else if (opcode_str == "lrp") inst.opcode = D3DSIO_LRP;
    else if (opcode_str == "frc") inst.opcode = D3DSIO_FRC;
    else if (opcode_str == "expp") inst.opcode = D3DSIO_EXPP;
    else if (opcode_str == "logp") inst.opcode = D3DSIO_LOGP;
    // Pixel shader specific instructions
    else if (opcode_str == "tex") inst.opcode = D3DSIO_TEX;
    else if (opcode_str == "texcoord") inst.opcode = D3DSIO_TEXCOORD;
    else if (opcode_str == "texkill") inst.opcode = D3DSIO_TEXKILL;
    else if (opcode_str == "cnd") inst.opcode = D3DSIO_CND;
    else if (opcode_str == "cmp") inst.opcode = D3DSIO_CMP;
    else if (opcode_str == "bem") inst.opcode = D3DSIO_BEM;
    else if (opcode_str == "phase") inst.opcode = D3DSIO_PHASE;
    else if (opcode_str == "mul_sat") inst.opcode = D3DSIO_MUL_SAT;
    else if (opcode_str == "mad_sat") inst.opcode = D3DSIO_MAD_SAT;
    else if (opcode_str == "nop") inst.opcode = D3DSIO_NOP;
    else if (opcode_str == "dcl") inst.opcode = D3DSIO_DCL;
    else if (opcode_str == "def") inst.opcode = D3DSIO_DEF;
    else if (opcode_str == "sincos") inst.opcode = D3DSIO_SINCOS;
    else if (opcode_str == "texld") inst.opcode = D3DSIO_TEX; // texld is the same as tex
    else {
        DX8GL_ERROR("Unknown opcode: %s", opcode_str.c_str());
        return false;
    }
    
    // Parse operands
    std::string operands;
    std::getline(stream, operands);
    
    // Debug: show what we're parsing
    if (inst.opcode == D3DSIO_DEF) {
        DX8GL_INFO("DEF operands string: '%s'", operands.c_str());
    }
    
    // Split by comma
    std::vector<std::string> tokens;
    std::stringstream ss(operands);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            token = token.substr(start, end - start + 1);
            tokens.push_back(token);
        }
    }
    
    if (tokens.empty()) {
        return false;
    }
    
    // Special handling for DEF instruction
    if (inst.opcode == D3DSIO_DEF) {
        // DEF has format: def c#, float, float, float, float
        DX8GL_INFO("DEF instruction found, tokens.size()=%zu", tokens.size());
        for (size_t i = 0; i < tokens.size(); i++) {
            DX8GL_INFO("  Token[%zu]: '%s'", i, tokens[i].c_str());
        }
        
        if (tokens.size() < 5) {
            DX8GL_ERROR("DEF instruction requires 5 tokens: register and 4 float values, got %zu", tokens.size());
            return false;
        }
        
        // First token is the constant register
        if (!parse_register(tokens[0], inst.dest)) {
            return false;
        }
        
        // Verify it's a constant register
        if (inst.dest.type != D3DSPR_CONST) {
            DX8GL_ERROR("DEF instruction requires constant register");
            return false;
        }
        
        // Store the constant values (we'll encode them into bytecode)
        instructions_.push_back(inst);
        
        // Encode DEF instruction to bytecode
        DWORD opcode_token = (inst.opcode & 0xFFFF) | (4 << 24); // 4 DWORDs follow
        bytecode_.push_back(opcode_token);
        
        // Encode destination register
        DWORD dest_token = encode_register(inst.dest);
        bytecode_.push_back(dest_token);
        
        // Encode the 4 float values
        for (int i = 1; i <= 4; i++) {
            try {
                float value = std::stof(tokens[i]);
                DWORD float_bits = *reinterpret_cast<DWORD*>(&value);
                bytecode_.push_back(float_bits);
            } catch (const std::exception& e) {
                DX8GL_ERROR("Failed to parse float value in DEF instruction: %s", tokens[i].c_str());
                return false;
            }
        }
        
        return true;
    }
    
    // First token is destination
    if (!parse_register(tokens[0], inst.dest)) {
        return false;
    }
    
    // Remaining tokens are sources
    for (size_t i = 1; i < tokens.size(); i++) {
        ShaderInstruction::Register src;
        if (!parse_register(tokens[i], src)) {
            return false;
        }
        inst.sources.push_back(src);
    }
    
    instructions_.push_back(inst);
    instruction_to_bytecode(inst);
    
    return true;
}

DWORD DX8ShaderTranslator::encode_register(const ShaderInstruction::Register& reg) {
    DWORD token = 0;
    
    // Encode register type (bits 28-30)
    token |= (reg.type & 0x7) << 28;
    
    // Encode register index (bits 0-10)
    token |= (reg.index & 0x7FF);
    
    // Encode write mask for destination registers (bits 16-19)
    if (!reg.write_mask.empty()) {
        DWORD mask = 0;
        if (reg.write_mask.find('x') != std::string::npos) mask |= 1;
        if (reg.write_mask.find('y') != std::string::npos) mask |= 2;
        if (reg.write_mask.find('z') != std::string::npos) mask |= 4;
        if (reg.write_mask.find('w') != std::string::npos) mask |= 8;
        token |= (mask << 16);
    } else {
        // Default to all components
        token |= (0xF << 16);
    }
    
    // Encode swizzle for source registers (bits 16-23)
    if (!reg.swizzle.empty() && reg.swizzle != "xyzw") {
        // TODO: Implement swizzle encoding
    }
    
    // Source modifier bit 13
    if (reg.negate) {
        token |= (1 << 13);
    }
    
    return token;
}

bool DX8ShaderTranslator::parse_register(const std::string& token, ShaderInstruction::Register& reg) {
    std::string tok = token;
    reg.negate = false;
    reg.src_modifier = MODIFIER_NONE;
    
    // Check for negation
    if (tok[0] == '-') {
        reg.negate = true;
        tok = tok.substr(1);
    }
    
    // Check for source modifiers (1-r0, r0_bias, etc)
    if (tok.find("1-") == 0) {
        reg.src_modifier = MODIFIER_COMP;
        tok = tok.substr(2);
    } else {
        size_t mod_pos = tok.find('_');
        if (mod_pos != std::string::npos && mod_pos < tok.find('.')) {
            std::string modifier = tok.substr(mod_pos);
            size_t dot_in_mod = modifier.find('.');
            if (dot_in_mod != std::string::npos) {
                modifier = modifier.substr(0, dot_in_mod);
            }
            
            if (modifier == "_bias") reg.src_modifier = MODIFIER_BIAS;
            else if (modifier == "_bx2") reg.src_modifier = MODIFIER_BX2;
            else if (modifier == "_x2") reg.src_modifier = MODIFIER_X2;
            else if (modifier == "_x4") reg.src_modifier = MODIFIER_X4;
            else if (modifier == "_d2") reg.src_modifier = MODIFIER_D2;
            
            if (reg.src_modifier != MODIFIER_NONE) {
                tok = tok.substr(0, mod_pos);
                if (dot_in_mod != std::string::npos) {
                    tok += modifier.substr(dot_in_mod);
                }
            }
        }
    }
    
    // Handle swizzle and write mask
    size_t dot_pos = tok.find('.');
    if (dot_pos != std::string::npos) {
        reg.swizzle = tok.substr(dot_pos + 1);
        reg.write_mask = reg.swizzle; // For dest registers
        tok = tok.substr(0, dot_pos);
    } else {
        reg.swizzle = "xyzw"; // Default full swizzle
        reg.write_mask = "xyzw";
    }
    
    // Replace defined constants
    for (const auto& def : defines_) {
        if (tok == def.first) {
            tok = "c[" + std::to_string(def.second) + "]";
            break;
        }
    }
    
    // Parse register type and index
    if (tok[0] == 'r') {
        reg.type = D3DSPR_TEMP;
        reg.index = std::stoi(tok.substr(1));
    } else if (tok[0] == 'v') {
        reg.type = D3DSPR_INPUT;
        reg.index = std::stoi(tok.substr(1));
        
        // Track input usage based on actual vertex shader inputs
        if (reg.index == 0) uses_position_ = true;
        else if (reg.index == 1) uses_normal_ = true;
        else if (reg.index == 2) uses_color_ = true;
        else if (reg.index >= 3) {
            int tex_index = reg.index - 3;
            if (std::find(texture_coords_used_.begin(), texture_coords_used_.end(), tex_index) == texture_coords_used_.end()) {
                texture_coords_used_.insert(tex_index);
            }
        }
    } else if (tok.substr(0, 2) == "c[") {
        reg.type = D3DSPR_CONST;
        size_t end = tok.find(']');
        std::string index_expr = tok.substr(2, end - 2);
        
        // Check for relative addressing (e.g., c[a0.x + 5])
        if (index_expr.find("a0") != std::string::npos) {
            // Parse relative addressing
            size_t plus_pos = index_expr.find('+');
            if (plus_pos != std::string::npos) {
                std::string offset_str = index_expr.substr(plus_pos + 1);
                // Trim whitespace
                offset_str.erase(0, offset_str.find_first_not_of(" \t"));
                offset_str.erase(offset_str.find_last_not_of(" \t") + 1);
                reg.index = std::stoi(offset_str);
                // Mark this as using relative addressing (we'll need to add a flag for this)
                // For now, we'll encode it in the upper bits
                reg.index |= 0x8000; // Set bit 15 to indicate relative addressing
            } else {
                // Just a0 with no offset
                reg.index = 0x8000;
            }
        } else {
            // Simple constant index
            reg.index = std::stoi(index_expr);
        }
    } else if (tok[0] == 'c' && std::isdigit(tok[1])) {
        // Handle simple constant format like c0, c1, etc.
        reg.type = D3DSPR_CONST;
        reg.index = std::stoi(tok.substr(1));
    } else if (tok == "a0") {
        // Address register (vertex shader only)
        if (shader_type_ != SHADER_TYPE_VERTEX) {
            DX8GL_ERROR("Address register a0 not valid in pixel shader");
            return false;
        }
        reg.type = D3DSPR_ADDR;
        reg.index = 0;
    } else if (tok[0] == 't' && std::isdigit(tok[1])) {
        // Handle texture coordinate registers in pixel shaders (t0, t1, etc.)
        if (shader_type_ == SHADER_TYPE_PIXEL) {
            reg.type = D3DSPR_TEXTURE;
            reg.index = std::stoi(tok.substr(1));
        } else {
            // In vertex shaders, this would be an error
            DX8GL_ERROR("Texture register %s not valid in vertex shader", tok.c_str());
            return false;
        }
    } else if (tok == "oPos") {
        reg.type = D3DSPR_RASTOUT;
        reg.index = 0; // Position
    } else if (tok.substr(0, 2) == "oD") {
        reg.type = D3DSPR_ATTROUT;
        reg.index = std::stoi(tok.substr(2)); // Color index
    } else if (tok.substr(0, 2) == "oT") {
        reg.type = D3DSPR_ATTROUT;
        reg.index = std::stoi(tok.substr(2)) + 8; // Texture index + 8
        
        int tex_index = reg.index - 8;
        if (std::find(output_textures_used_.begin(), output_textures_used_.end(), tex_index) == output_textures_used_.end()) {
            output_textures_used_.insert(tex_index);
        }
    } else if (tok == "V_POSITION") {
        reg.type = D3DSPR_INPUT;
        reg.index = 0;
        uses_position_ = true;
    } else if (tok == "V_NORMAL") {
        reg.type = D3DSPR_INPUT;
        reg.index = 1;
        uses_normal_ = true;
    } else if (tok == "V_DIFFUSE") {
        reg.type = D3DSPR_INPUT;
        reg.index = 2;
        uses_color_ = true;
    } else if (tok == "V_TEXTURE") {
        reg.type = D3DSPR_INPUT;
        reg.index = 3;
        if (std::find(texture_coords_used_.begin(), texture_coords_used_.end(), 0) == texture_coords_used_.end()) {
            texture_coords_used_.insert(0);
        }
    } else if (tok == "V_S") {
        reg.type = D3DSPR_INPUT;
        reg.index = 4;
    } else if (tok == "V_T") {
        reg.type = D3DSPR_INPUT;
        reg.index = 5;
    } else if (tok == "V_SxT") {
        reg.type = D3DSPR_INPUT;
        reg.index = 6;
    } else if (tok == "HALF_ANGLE") {
        reg.type = D3DSPR_TEMP;
        reg.index = 0;
    } else if (tok == "S_WORLD") {
        reg.type = D3DSPR_TEMP;
        reg.index = 1;
    } else if (tok == "T_WORLD") {
        reg.type = D3DSPR_TEMP;
        reg.index = 2;
    } else if (tok == "SxT_WORLD") {
        reg.type = D3DSPR_TEMP;
        reg.index = 3;
    } else if (tok == "LIGHT_LOCAL") {
        reg.type = D3DSPR_TEMP;
        reg.index = 4;
    } else if (tok == "LIGHT_0") {
        reg.type = D3DSPR_TEMP;
        reg.index = 5;
    } else if (tok == "LIGHT_1") {
        reg.type = D3DSPR_TEMP;
        reg.index = 6;
    } else if (tok == "LIGHT_2") {
        reg.type = D3DSPR_TEMP;
        reg.index = 7;
    } else if (tok == "LIGHT_3") {
        reg.type = D3DSPR_TEMP;
        reg.index = 8;
    } else if (tok == "COL") {
        reg.type = D3DSPR_TEMP;
        reg.index = 9;
    } else if (tok == "WORLD_NORMAL") {
        reg.type = D3DSPR_TEMP;
        reg.index = 10;
    } else if (tok == "EYE_VECTOR" || tok == "WORLD_VERTEX") {
        reg.type = D3DSPR_TEMP;
        reg.index = 11;
    } else {
        DX8GL_ERROR("Unknown register: %s", tok.c_str());
        return false;
    }
    
    return true;
}

void DX8ShaderTranslator::instruction_to_bytecode(const ShaderInstruction& inst) {
    // This is a simplified bytecode generation
    // Real implementation would need proper encoding
    
    DWORD opcode_token = inst.opcode;
    bytecode_.push_back(opcode_token);
    
    // Encode destination
    DWORD dest_token = (inst.dest.type << 28) | (inst.dest.index & 0x7FF);
    bytecode_.push_back(dest_token);
    
    // Encode sources
    for (const auto& src : inst.sources) {
        DWORD src_token = (src.type << 28) | (src.index & 0x7FF);
        if (src.negate) src_token |= 0x80000000;
        bytecode_.push_back(src_token);
    }
}

std::string DX8ShaderTranslator::generate_glsl() {
    if (shader_type_ == SHADER_TYPE_PIXEL) {
        return generate_pixel_glsl();
    } else {
        return generate_vertex_glsl();
    }
}

std::string DX8ShaderTranslator::generate_vertex_glsl() {
    std::ostringstream glsl;
    
    // Target OpenGL 4.5 Core as requested
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "ES") != nullptr;
    
    if (is_es) {
        glsl << "#version 300 es\n";
        glsl << "precision highp float;\n\n";
    } else {
        glsl << "#version 450 core\n\n";
    }
    
    // Attributes (inputs)
    glsl << "// Vertex attributes\n";
    if (uses_position_) {
        glsl << "attribute vec4 a_position;\n";
    }
    if (uses_normal_) {
        glsl << "attribute vec3 a_normal;\n";
    }
    if (uses_color_) {
        glsl << "attribute vec4 a_color;\n";
    }
    for (int tex : texture_coords_used_) {
        glsl << "attribute vec2 a_texcoord" << tex << ";\n";
    }
    // Add tangent space attributes if needed
    bool needs_tangent_space = false;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_INPUT && inst.dest.index >= 4 && inst.dest.index <= 6) {
            needs_tangent_space = true;
            break;
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_INPUT && src.index >= 4 && src.index <= 6) {
                needs_tangent_space = true;
                break;
            }
        }
    }
    if (needs_tangent_space) {
        glsl << "attribute vec3 a_tangent_s;\n";
        glsl << "attribute vec3 a_tangent_t;\n";
        glsl << "attribute vec3 a_binormal;\n";
    }
    glsl << "\n";
    
    // Uniforms (constants)
    glsl << "// Shader constants\n";
    std::unordered_map<int, std::string> const_names;
    
    // Check if relative addressing is used
    bool uses_relative_addressing = false;
    std::set<int> constants_used;
    
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_CONST) {
            int index = inst.dest.index & 0x7FFF;
            if (inst.dest.index & 0x8000) {
                uses_relative_addressing = true;
            } else {
                constants_used.insert(index);
            }
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_CONST) {
                int index = src.index & 0x7FFF;
                if (src.index & 0x8000) {
                    uses_relative_addressing = true;
                } else {
                    constants_used.insert(index);
                    
                    // For matrix operations, add the additional constant indices
                    if (inst.opcode == D3DSIO_M4x4) {
                        // m4x4 uses 4 consecutive constants starting from the source index
                        for (int i = 0; i < 4; i++) {
                            constants_used.insert(index + i);
                        }
                    } else if (inst.opcode == D3DSIO_M4x3) {
                        // m4x3 uses 3 consecutive constants starting from the source index  
                        for (int i = 0; i < 3; i++) {
                            constants_used.insert(index + i);
                        }
                    } else if (inst.opcode == D3DSIO_M3x3) {
                        // m3x3 uses 3 consecutive constants starting from the source index
                        for (int i = 0; i < 3; i++) {
                            constants_used.insert(index + i);
                        }
                    }
                }
            }
        }
    }
    
    if (uses_relative_addressing) {
        // If relative addressing is used, declare all constants as an array
        glsl << "uniform vec4 c[96];\n";
    } else {
        // Declare all used constants as vec4 uniforms
        for (int const_index : constants_used) {
            glsl << "uniform vec4 c" << const_index << ";\n";
        }
    }
    
    // Also handle any constants defined in the constants_ collection
    for (const auto& constant : constants_) {
        if (constants_used.find(constant.index) == constants_used.end()) {
            if (constant.count == 1) {
                glsl << "uniform vec4 c" << constant.index << "; // " << constant.name << "\n";
                const_names[constant.index] = "c" + std::to_string(constant.index);
            } else if (constant.count == 4) {
                glsl << "uniform mat4 c" << constant.index << "_" << (constant.index + 3) << "; // " << constant.name << "\n";
                const_names[constant.index] = "c" + std::to_string(constant.index) + "_" + std::to_string(constant.index + 3);
            }
        }
    }
    glsl << "\n";
    
    // Varyings (outputs) - only declare what's actually used
    glsl << "// Outputs to fragment shader\n";
    
    // Always declare v_color0 for now since many shaders implicitly use it
    // TODO: Implement proper varying usage tracking
    glsl << "varying vec4 v_color0;\n";
    for (int tex : output_textures_used_) {
        if (tex == 0 || tex == 3) {
            glsl << "varying vec4 v_texcoord" << tex << ";\n";
        } else {
            glsl << "varying vec2 v_texcoord" << tex << ";\n";
        }
    }
    glsl << "\n";
    
    // Main function
    glsl << "void main() {\n";
    
    // Temporary registers and address register
    std::unordered_map<int, bool> temp_registers_used;
    bool uses_address_register = false;
    
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_TEMP) {
            temp_registers_used[inst.dest.index] = true;
        } else if (inst.dest.type == D3DSPR_ADDR) {
            uses_address_register = true;
        }
        
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_TEMP) {
                temp_registers_used[src.index] = true;
            } else if (src.type == D3DSPR_ADDR) {
                uses_address_register = true;
            } else if (src.type == D3DSPR_CONST && (src.index & 0x8000)) {
                // Relative addressing detected
                uses_address_register = true;
            }
        }
    }
    
    // Declare address register if used
    if (uses_address_register) {
        glsl << "    ivec4 a0 = ivec4(0);\n";
    }
    
    // Declare temp registers
    for (const auto& temp : temp_registers_used) {
        glsl << "    vec4 r" << temp.first << ";\n";
    }
    if (!temp_registers_used.empty() || uses_address_register) {
        glsl << "\n";
    }
    
    // Convert instructions to GLSL
    for (const auto& inst : instructions_) {
        glsl << "    " << instruction_to_glsl(inst) << "\n";
    }
    
    // Set default outputs if not set
    bool has_position = false;
    bool has_color = false;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_RASTOUT && inst.dest.index == 0) {
            has_position = true;
        }
        if (inst.dest.type == D3DSPR_ATTROUT && inst.dest.index < 8) {
            has_color = true;
        }
    }
    
    if (!has_position) {
        glsl << "    gl_Position = vec4(0.0);\n";
    }
    if (!has_color) {
        // Initialize v_color0 if it wasn't set by the shader
        glsl << "    v_color0 = vec4(1.0);\n";
    }
    
    glsl << "}\n";
    
    return glsl.str();
}

std::string DX8ShaderTranslator::apply_modifier(const std::string& value, InstructionModifier modifier) {
    switch (modifier) {
        case MODIFIER_SAT:
            return "clamp(" + value + ", 0.0, 1.0)";
        case MODIFIER_X2:
            return "(" + value + " * 2.0)";
        case MODIFIER_X4:
            return "(" + value + " * 4.0)";
        case MODIFIER_D2:
            return "(" + value + " * 0.5)";
        case MODIFIER_BIAS:
            return "(" + value + " - 0.5)";
        case MODIFIER_BX2:
            return "((" + value + " - 0.5) * 2.0)";
        case MODIFIER_COMP:
            return "(1.0 - " + value + ")";
        default:
            return value;
    }
}

std::string DX8ShaderTranslator::register_to_glsl(const ShaderInstruction::Register& reg) {
    std::string result;
    bool needs_modifier = (reg.src_modifier != MODIFIER_NONE);
    
    if (reg.negate && !needs_modifier) result = "-";
    
    switch (reg.type) {
        case D3DSPR_TEMP:
            result += "r" + std::to_string(reg.index);
            break;
        case D3DSPR_INPUT:
            // Map vertex shader inputs based on FVF or DCL declarations
            if (shader_type_ == SHADER_TYPE_VERTEX) {
                if (reg.index == 0) result += "a_position";
                else if (reg.index == 1) result += "vec4(a_normal, 0.0)";
                else if (reg.index == 2) {
                    // Check if this was declared as texcoord
                    if (texture_coords_used_.count(2) > 0) {
                        result += "vec4(a_texcoord2, 0.0, 1.0)";
                    } else {
                        result += "a_color";
                    }
                }
                else if (reg.index == 3) result += "vec4(a_texcoord0, 0.0, 1.0)";
                else if (reg.index == 4) result += "vec4(a_tangent_s, 0.0)";
                else if (reg.index == 5) result += "vec4(a_tangent_t, 0.0)";
                else if (reg.index == 6) result += "vec4(a_binormal, 0.0)";
                else result += "vec4(a_texcoord" + std::to_string(reg.index - 3) + ", 0.0, 1.0)";
            }
            break;
        case D3DSPR_CONST:
            // Check for relative addressing (bit 15 set)
            if (reg.index & 0x8000) {
                // Relative addressing with a0
                int offset = reg.index & 0x7FFF;
                if (offset > 0) {
                    result += "c[int(a0.x) + " + std::to_string(offset) + "]";
                } else {
                    result += "c[int(a0.x)]";
                }
            } else {
                // Check if this is part of a matrix
                for (const auto& constant : constants_) {
                    if (reg.index >= constant.index && reg.index < constant.index + constant.count && constant.count > 1) {
                        // It's a matrix constant - we need to handle matrix row access
                        int row = reg.index - constant.index;
                        result += "c" + std::to_string(constant.index) + "_" + std::to_string(constant.index + constant.count - 1) + "[" + std::to_string(row) + "]";
                        return result;
                    }
                }
                // Use ps_ prefix for pixel shader constants
                if (shader_type_ == SHADER_TYPE_PIXEL) {
                    result += "ps_c" + std::to_string(reg.index);
                } else {
                    result += "c" + std::to_string(reg.index);
                }
            }
            break;
        case D3DSPR_ADDR:
            result += "a0";
            break;
        case D3DSPR_RASTOUT:
            if (reg.index == 0) result += "gl_Position";
            break;
        case D3DSPR_ATTROUT:
            if (reg.index == 0) result += "v_color0";
            else if (reg.index == 1) result += "v_color1";
            else if (reg.index >= 8) result += "v_texcoord" + std::to_string(reg.index - 8);
            break;
    }
    
    // Apply swizzle if not default
    if (reg.swizzle != "xyzw") {
        // Handle special cases for scalars
        if (reg.swizzle.length() == 1) {
            result += "." + reg.swizzle;
        } else if (reg.swizzle == "xyz") {
            result += ".xyz";
        } else if (reg.swizzle != "xyzw") {
            result += "." + reg.swizzle;
        }
    }
    
    // Apply source modifier
    if (reg.src_modifier != MODIFIER_NONE) {
        result = apply_modifier(result, reg.src_modifier);
    }
    
    // Apply negation after modifier
    if (reg.negate && needs_modifier) {
        result = "-" + result;
    }
    
    return result;
}

std::string DX8ShaderTranslator::instruction_to_glsl(const ShaderInstruction& inst) {
    std::string dest = register_to_glsl(inst.dest);
    std::string result;
    
    switch (inst.opcode) {
        case D3DSIO_MOV:
            if (inst.dest.type == D3DSPR_ADDR) {
                // Moving to address register - convert float to int
                result = dest + " = ivec4(" + register_to_glsl(inst.sources[0]) + ");";
            } else {
                result = dest + " = " + register_to_glsl(inst.sources[0]) + ";";
            }
            break;
            
        case D3DSIO_ADD:
            result = dest + " = " + register_to_glsl(inst.sources[0]) + " + " + register_to_glsl(inst.sources[1]) + ";";
            break;
            
        case D3DSIO_SUB:
            result = dest + " = " + register_to_glsl(inst.sources[0]) + " - " + register_to_glsl(inst.sources[1]) + ";";
            break;
            
        case D3DSIO_MAD:
            result = dest + " = " + register_to_glsl(inst.sources[0]) + " * " + register_to_glsl(inst.sources[1]) + " + " + register_to_glsl(inst.sources[2]) + ";";
            break;
            
        case D3DSIO_MUL:
            result = dest + " = " + register_to_glsl(inst.sources[0]) + " * " + register_to_glsl(inst.sources[1]) + ";";
            break;
            
        case D3DSIO_RCP:
            return dest + " = vec4(1.0) / " + register_to_glsl(inst.sources[0]) + ";";
            
        case D3DSIO_RSQ:
            return dest + " = inversesqrt(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_DP3:
            return dest + " = vec4(dot(" + register_to_glsl(inst.sources[0]) + ".xyz, " + register_to_glsl(inst.sources[1]) + ".xyz));";
            
        case D3DSIO_DP4:
            return dest + " = vec4(dot(" + register_to_glsl(inst.sources[0]) + ", " + register_to_glsl(inst.sources[1]) + "));";
            
        case D3DSIO_MIN:
            return dest + " = min(" + register_to_glsl(inst.sources[0]) + ", " + register_to_glsl(inst.sources[1]) + ");";
            
        case D3DSIO_MAX:
            return dest + " = max(" + register_to_glsl(inst.sources[0]) + ", " + register_to_glsl(inst.sources[1]) + ");";
            
        case D3DSIO_M4x4: {
            // For m4x4, the matrix is in the second source register
            // Extract base register for matrix
            ShaderInstruction::Register mat_base = inst.sources[1];
            std::string vec = register_to_glsl(inst.sources[0]);
            result = dest + " = ";
            
            // Check if it's a constant matrix
            if (mat_base.type == D3DSPR_CONST) {
                // Find the matrix constant
                for (const auto& constant : constants_) {
                    if (mat_base.index >= constant.index && mat_base.index < constant.index + constant.count && constant.count == 4) {
                        std::string mat_name = "c" + std::to_string(constant.index) + "_" + std::to_string(constant.index + 3);
                        return result + mat_name + " * " + vec + ";";
                    }
                }
                // Fallback: manual dot products
                result += "vec4(";
                for (int i = 0; i < 4; i++) {
                    if (i > 0) result += ", ";
                    result += "dot(" + vec + ", c" + std::to_string(mat_base.index + i) + ")";
                }
                result += ");";
            }
            break;
        }
            
        case D3DSIO_M4x3: {
            ShaderInstruction::Register mat_base = inst.sources[1];
            std::string vec = register_to_glsl(inst.sources[0]);
            result = dest + ".xyz = vec3(";
            for (int i = 0; i < 3; i++) {
                if (i > 0) result += ", ";
                result += "dot(" + vec + ", c" + std::to_string(mat_base.index + i) + ")";
            }
            result += ");";
            break;
        }
            
        case D3DSIO_M3x3: {
            ShaderInstruction::Register mat_base = inst.sources[1];
            std::string vec = register_to_glsl(inst.sources[0]);
            result = dest + ".xyz = vec3(";
            for (int i = 0; i < 3; i++) {
                if (i > 0) result += ", ";
                result += "dot(" + vec + ".xyz, c" + std::to_string(mat_base.index + i) + ".xyz)";
            }
            result += ");";
            break;
        }
            
        case D3DSIO_SLT:
            // Set Less Than: dest = (src0 < src1) ? 1.0 : 0.0
            return dest + " = vec4(lessThan(" + register_to_glsl(inst.sources[0]) + ", " + register_to_glsl(inst.sources[1]) + "));";
            
        case D3DSIO_SGE:
            // Set Greater Equal: dest = (src0 >= src1) ? 1.0 : 0.0
            return dest + " = vec4(greaterThanEqual(" + register_to_glsl(inst.sources[0]) + ", " + register_to_glsl(inst.sources[1]) + "));";
            
        case D3DSIO_EXP:
            // Full precision exp2
            return dest + " = exp2(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_LOG:
            // Full precision log2
            return dest + " = log2(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_EXPP:
            // Partial precision exp2 (we'll use full precision in GLES)
            return dest + " = exp2(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_LOGP:
            // Partial precision log2 (we'll use full precision in GLES)
            return dest + " = log2(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_FRC:
            // Fractional part
            return dest + " = fract(" + register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_LIT:
            // Lighting calculation: dest.x = 1, dest.y = max(0, src.x), dest.z = src.x > 0 ? pow(src.y, src.w) : 0, dest.w = 1
            return dest + " = vec4(1.0, max(0.0, " + register_to_glsl(inst.sources[0]) + ".x), " +
                   "(" + register_to_glsl(inst.sources[0]) + ".x > 0.0) ? pow(max(0.0, " + register_to_glsl(inst.sources[0]) + ".y), " + 
                   register_to_glsl(inst.sources[0]) + ".w) : 0.0, 1.0);";
            
        case D3DSIO_DST:
            // Distance vector: dest.x = 1, dest.y = src0.y * src1.y, dest.z = src0.z, dest.w = src1.w
            return dest + " = vec4(1.0, " + register_to_glsl(inst.sources[0]) + ".y * " + register_to_glsl(inst.sources[1]) + ".y, " +
                   register_to_glsl(inst.sources[0]) + ".z, " + register_to_glsl(inst.sources[1]) + ".w);";
            
        case D3DSIO_LRP:
            // Linear interpolation: dest = src0 * src1 + (1 - src0) * src2
            return dest + " = mix(" + register_to_glsl(inst.sources[2]) + ", " + register_to_glsl(inst.sources[1]) + ", " + 
                   register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_DCL:
            // DCL instructions are declarations and don't generate code in the shader body
            return "";
            
        case D3DSIO_DEF:
            // DEF instructions define constants and are handled elsewhere
            return "";
            
        case D3DSIO_SINCOS: {
            // sincos r4.xy, r3.x - computes cos(r3.x) in r4.x and sin(r3.x) in r4.y
            if (inst.sources.size() >= 1) {
                std::string src = register_to_glsl(inst.sources[0]) + ".x";
                return dest + ".x = cos(" + src + "); " + dest + ".y = sin(" + src + ");";
            }
            return "// Invalid sincos instruction";
        }
            
        default:
            return "// Unsupported instruction: " + std::to_string(inst.opcode);
    }
    
    // Apply instruction modifier to the result
    if (inst.modifier != MODIFIER_NONE && !result.empty()) {
        // Find the assignment part
        size_t eq_pos = result.find(" = ");
        if (eq_pos != std::string::npos) {
            std::string lhs = result.substr(0, eq_pos + 3);
            std::string rhs = result.substr(eq_pos + 3);
            // Remove semicolon
            if (rhs.back() == ';') {
                rhs.pop_back();
            }
            
            // Apply modifier to RHS
            rhs = apply_modifier(rhs, inst.modifier);
            result = lhs + rhs + ";";
        }
    }
    
    return result;
}

std::string DX8ShaderTranslator::generate_pixel_glsl() {
    std::ostringstream glsl;
    
    // Target OpenGL 4.5 Core as requested
    const char* version_str = (const char*)glGetString(GL_VERSION);
    bool is_es = version_str && strstr(version_str, "ES") != nullptr;
    
    if (is_es) {
        glsl << "#version 300 es\n";
        glsl << "precision highp float;\n\n";
    } else {
        glsl << "#version 450 core\n\n";
    }
    
    // Uniforms (constants) - ps 1.4 supports c0-c31
    glsl << "// Shader constants\n";
    int max_constants = (major_version_ == 1 && minor_version_ == 4) ? 32 : 8;
    for (int i = 0; i < max_constants; i++) {
        glsl << "uniform vec4 ps_c" << i << ";\n";
    }
    glsl << "\n";
    
    // Texture samplers
    glsl << "// Texture samplers\n";
    std::vector<int> textures_used;
    for (const auto& inst : instructions_) {
        if (inst.opcode == D3DSIO_TEX || inst.opcode == D3DSIO_TEXCOORD) {
            for (const auto& src : inst.sources) {
                if (src.type == D3DSPR_TEXTURE) {
                    if (std::find(textures_used.begin(), textures_used.end(), src.index) == textures_used.end()) {
                        textures_used.push_back(src.index);
                    }
                }
            }
        }
    }
    // Always declare at least 4 texture samplers for compatibility
    for (int i = 0; i < 4; i++) {
        glsl << "uniform sampler2D s" << i << ";\n";
    }
    glsl << "\n";
    
    // Bump mapping uniforms (check if BEM instruction is used)
    bool uses_bump_mapping = false;
    for (const auto& inst : instructions_) {
        if (inst.opcode == D3DSIO_BEM) {
            uses_bump_mapping = true;
            break;
        }
    }
    
    if (uses_bump_mapping) {
        glsl << "// Bump environment mapping matrices\n";
        for (int i = 0; i < 4; i++) {
            glsl << "uniform mat2 u_bumpEnvMat" << i << ";\n";
        }
        glsl << "\n";
    }
    
    // Varyings (inputs from vertex shader)
    glsl << "// Inputs from vertex shader\n";
    glsl << "varying vec4 v_color0;\n";
    // Note: Only declare v_color1 if it's actually used (rarely in DirectX 8)
    
    // Determine which texture registers are used to know which varyings to declare
    std::unordered_map<int, bool> texture_registers_used;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_TEXTURE) {
            texture_registers_used[inst.dest.index] = true;
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_TEXTURE) {
                texture_registers_used[src.index] = true;
            }
        }
    }
    
    // Declare texture coordinate inputs (only the ones actually used)
    // This should match what the vertex shader outputs
    for (const auto& tex : texture_registers_used) {
        glsl << "varying vec4 v_texcoord" << tex.first << ";\n";
    }
    // If no texture registers detected, default to texcoord0 for compatibility
    if (texture_registers_used.empty()) {
        glsl << "varying vec4 v_texcoord0;\n";
    }
    glsl << "\n";
    
    // Main function
    glsl << "void main() {\n";
    
    // Temporary registers
    std::unordered_map<int, bool> temp_registers_used;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_TEMP) {
            temp_registers_used[inst.dest.index] = true;
        }
        for (const auto& src : inst.sources) {
            if (src.type == D3DSPR_TEMP) {
                temp_registers_used[src.index] = true;
            }
        }
    }
    
    // Declare temp registers (r0-r1 for PS 1.4)
    for (const auto& temp : temp_registers_used) {
        glsl << "    vec4 r" << temp.first << ";\n";
    }
    
    // Declare texture registers if needed (only the ones actually used)
    for (const auto& tex : texture_registers_used) {
        glsl << "    vec4 t" << tex.first << ";\n";
    }
    
    // Initialize texture registers from varying inputs
    if (!texture_registers_used.empty()) {
        glsl << "\n";
        glsl << "    // Initialize texture registers from vertex shader outputs\n";
        for (const auto& tex : texture_registers_used) {
            // Initialize from corresponding varying input
            glsl << "    t" << tex.first << " = v_texcoord" << tex.first << ";\n";
        }
    }
    
    if (!temp_registers_used.empty()) {
        glsl << "\n";
    }
    
    // Convert instructions to GLSL
    bool has_phase = false;
    for (const auto& inst : instructions_) {
        if (inst.opcode == D3DSIO_PHASE) {
            has_phase = true;
            glsl << "    // phase\n";
            continue;
        }
        glsl << "    " << pixel_instruction_to_glsl(inst) << "\n";
    }
    
    // Set default output if not set
    bool has_output = false;
    for (const auto& inst : instructions_) {
        if (inst.dest.type == D3DSPR_COLOROUT) {
            has_output = true;
            break;
        }
    }
    
    if (!has_output) {
        // Check if r0 was written to (implicit output)
        if (temp_registers_used.find(0) != temp_registers_used.end()) {
            glsl << "    gl_FragColor = r0;\n";
        } else {
            glsl << "    gl_FragColor = vec4(1.0);\n";
        }
    }
    
    glsl << "}\n";
    
    return glsl.str();
}

std::string DX8ShaderTranslator::pixel_instruction_to_glsl(const ShaderInstruction& inst) {
    std::string dest = pixel_register_to_glsl(inst.dest);
    
    switch (inst.opcode) {
        case D3DSIO_MOV:
            return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + ";";
            
        case D3DSIO_ADD:
            return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + " + " + pixel_register_to_glsl(inst.sources[1]) + ";";
            
        case D3DSIO_SUB:
            return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + " - " + pixel_register_to_glsl(inst.sources[1]) + ";";
            
        case D3DSIO_MAD:
            return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + " * " + pixel_register_to_glsl(inst.sources[1]) + " + " + pixel_register_to_glsl(inst.sources[2]) + ";";
            
        case D3DSIO_MUL:
            return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + " * " + pixel_register_to_glsl(inst.sources[1]) + ";";
            
        case D3DSIO_DP3:
            return dest + " = vec4(dot(" + pixel_register_to_glsl(inst.sources[0]) + ".xyz, " + pixel_register_to_glsl(inst.sources[1]) + ".xyz));";
            
        case D3DSIO_DP4:
            return dest + " = vec4(dot(" + pixel_register_to_glsl(inst.sources[0]) + ", " + pixel_register_to_glsl(inst.sources[1]) + "));";
            
        case D3DSIO_TEX: {
            // tex instruction (ps.1.1-1.3) or texld instruction (ps.1.4) - sample texture
            if (major_version_ == 1 && minor_version_ <= 3) {
                // ps.1.1-1.3: tex tn implicitly samples texture n using tn coordinates
                if (inst.dest.type == D3DSPR_TEXTURE) {
                    int tex_unit = inst.dest.index;
                    return dest + " = texture2D(s" + std::to_string(tex_unit) + ", t" + std::to_string(tex_unit) + ".xy);";
                }
            } else if (major_version_ == 1 && minor_version_ == 4) {
                // ps.1.4: texld rn, tn samples texture n using coordinates from tn, stores in rn
                if (inst.sources.size() >= 1 && inst.dest.type == D3DSPR_TEMP) {
                    // Determine texture unit from source register
                    int tex_unit = inst.sources[0].index;
                    std::string coords = pixel_register_to_glsl(inst.sources[0]);
                    return dest + " = texture2D(s" + std::to_string(tex_unit) + ", " + coords + ".xy);";
                }
            }
            return "// Invalid tex/texld instruction";
        }
            
        case D3DSIO_TEXCOORD:
            // texcoord instruction - pass through texture coordinates
            if (inst.sources.empty()) {
                // Simple texcoord tn - copies vn to tn
                int n = inst.dest.index;
                return "t" + std::to_string(n) + " = v_texcoord" + std::to_string(n) + ";";
            } else {
                // texcoord with source
                return dest + " = " + pixel_register_to_glsl(inst.sources[0]) + ";";
            }
            
        case D3DSIO_TEXKILL:
            // Kill pixel if any component is negative
            return "if (any(lessThan(" + pixel_register_to_glsl(inst.sources[0]) + ".xyz, vec3(0.0)))) discard;";
            
        case D3DSIO_CMP:
            // Compare: dest = (src0 >= 0) ? src1 : src2
            return dest + " = mix(" + pixel_register_to_glsl(inst.sources[2]) + ", " + 
                   pixel_register_to_glsl(inst.sources[1]) + ", vec4(greaterThanEqual(" + 
                   pixel_register_to_glsl(inst.sources[0]) + ", vec4(0.0))));";
            
        case D3DSIO_CND:
            // Conditional: dest = (src0 > 0.5) ? src1 : src2  
            return dest + " = mix(" + pixel_register_to_glsl(inst.sources[2]) + ", " + 
                   pixel_register_to_glsl(inst.sources[1]) + ", vec4(greaterThan(" + 
                   pixel_register_to_glsl(inst.sources[0]) + ", vec4(0.5))));";
            
        case D3DSIO_LRP:
            // Linear interpolation
            return dest + " = mix(" + pixel_register_to_glsl(inst.sources[2]) + ", " + 
                   pixel_register_to_glsl(inst.sources[1]) + ", " + pixel_register_to_glsl(inst.sources[0]) + ");";
            
        case D3DSIO_MIN:
            // Minimum of two values
            return dest + " = min(" + pixel_register_to_glsl(inst.sources[0]) + ", " + pixel_register_to_glsl(inst.sources[1]) + ");";
            
        case D3DSIO_MAX:
            // Maximum of two values
            return dest + " = max(" + pixel_register_to_glsl(inst.sources[0]) + ", " + pixel_register_to_glsl(inst.sources[1]) + ");";
            
        case D3DSIO_BEM: {
            // Bump environment mapping: dest = src0 + [bumpmatrix] * src1
            // BEM uses a 2x2 matrix from texture stage state
            // For now, we'll use uniform variables for the bump matrix
            int stage = inst.dest.index; // The destination register index indicates the texture stage
            std::string matrix_uniform = "u_bumpEnvMat" + std::to_string(stage);
            std::string src0 = pixel_register_to_glsl(inst.sources[0]);
            std::string src1 = pixel_register_to_glsl(inst.sources[1]);
            
            // BEM performs: dest.xy = src0.xy + mat2x2 * src1.xy
            // Where mat2x2 is the bump environment matrix for this stage
            return dest + ".xy = " + src0 + ".xy + " + matrix_uniform + " * " + src1 + ".xy;";
        }
            
        case D3DSIO_PHASE:
            // Phase instruction separates texture and arithmetic instructions in ps 1.4
            // In ps 1.4, phase marks the transition from texture addressing to color blending
            return "// --- PHASE: End of texture addressing, beginning of color blending ---";
            
        case D3DSIO_NOP:
            return "// nop";
            
            
        case D3DSIO_DCL:
            // DCL instructions don't generate code in the shader body
            return "";
            
        case D3DSIO_DEF:
            // DEF instructions define constants and are handled elsewhere
            return "";
            
        default:
            return "// Unsupported pixel shader instruction: " + std::to_string(inst.opcode);
    }
}

std::string DX8ShaderTranslator::pixel_register_to_glsl(const ShaderInstruction::Register& reg) {
    std::string result;
    if (reg.negate) result = "-";
    
    switch (reg.type) {
        case D3DSPR_TEMP:
            result += "r" + std::to_string(reg.index);
            break;
            
        case D3DSPR_INPUT:
            // Pixel shader inputs: t0-t3 are texture coordinates, v0-v1 are colors
            if (reg.index < 4) {
                result += "t" + std::to_string(reg.index);
            } else if (reg.index == 4) {
                result += "v_color0";
            } else if (reg.index == 5) {
                result += "v_color1";
            }
            break;
            
        case D3DSPR_CONST:
            // Use ps_ prefix for pixel shader constants
            if (shader_type_ == SHADER_TYPE_PIXEL) {
                result += "ps_c" + std::to_string(reg.index);
            } else {
                result += "c" + std::to_string(reg.index);
            }
            break;
            
        case D3DSPR_TEXTURE:
            result += "t" + std::to_string(reg.index);
            break;
            
        case D3DSPR_COLOROUT:
            result += "gl_FragColor";
            break;
            
        default:
            result += "/* unknown register type */";
            break;
    }
    
    // Apply swizzle if not default
    if (reg.swizzle != "xyzw" && reg.type != D3DSPR_COLOROUT) {
        if (reg.swizzle.length() == 1) {
            result += "." + reg.swizzle;
        } else if (reg.swizzle == "xyz") {
            result += ".xyz";
        } else if (reg.swizzle != "xyzw") {
            result += "." + reg.swizzle;
        }
    }
    
    return result;
}

} // namespace dx8gl