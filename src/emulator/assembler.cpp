#include <cctype>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

#include "emulator/assembler.hpp"
#include "emulator/instructions.hpp"

namespace mano {

std::optional<std::string_view>
Assembler::get_next_token() {
    std::size_t start = std::string_view::npos;
    for (; index < code.size(); ++index) {
        if (code[index] == '\n' || code[index] == '\r') {
            if (start == std::string_view::npos) {
                return {};
            } else {
                return code.substr(start, index - start);
            }
        }

        if (code[index] != ',' && !std::isspace(code[index])) {
            if (start == std::string_view::npos) {
                start = index;
            }
        } else if (start != std::string_view::npos) {
            return code.substr(start, index - start);
        }
    }
    if (start != std::string_view::npos) {
        return code.substr(start);
    }
    return {};
}


bool Assembler::is_token_label() {
    while (index < code.size()) {
        if (code[index] == '\n' || code[index] == '\r') {
            return false;
        }

        if (code[index] == ',') {
            // It is a label.
            return true;
        } else if (!std::isspace(code[index])) {
            // It is not a label.
            return false;
        }
        index += 1;
    }
    return false;
}

bool Assembler::line_end() {
    bool is_comment = false; 
    for (; index < code.size(); ++index) {
        switch (code[index]) {
            // Recognize both carriage return, 
            //     newline or both as new line character.
            case '\r':
                index += 1;
                if (index < code.size() && code[index] == '\n') {
                    index += 1; 
                }
                current_line += 1;
                return true;
            case '\n':
                index += 1;       
                current_line += 1;
                return true;
            case '/':
                is_comment = true;
                break;
            // Whitespace
            case ' ':
            case '\t':
                break;
            default:
                if (!is_comment) {
                    std::size_t start = index;
                    do {
                        index += 1;
                    } while (index < code.size() && !std::isspace(code[index]));
                    add_error("Unexpected symbol: {}", 
                        code.substr(start, index - start));
                }
                return false;
        } 
    }
    return true;
}

bool Assembler::first_pass() {
    lc = 0;
    while (index < code.size()) {
        if (auto token_optional = get_next_token()) {
            std::string_view token = *token_optional;
            if (token == "END") {
                while (index < code.size()) {
                    if (!line_end()) {
                        return false;
                    }
                } 

                return true;
            }
            if (token == "ORG") {
                if (auto lc_optional = 
                    get_next_int<std::uint16_t>(16, MEMORY_SIZE)) {
                    lc = *lc_optional;
                    if (!line_end()) {
                        return false;
                    }
                } else {
                    return false;
                }
                continue;
            }
            // Check if it is a label.
            if (is_token_label()) {
                if (token.size() <= 3 && std::isalpha(token[0])) {
                    symbol_table.insert({ token, lc });
                } else {
                    add_error("Invalid symbol, symbols should be at most 3 characters long and must start with a letter: {}", token);
                }
            }
        

            lc += 1;
            if (lc >= MEMORY_SIZE) {
                add_error("Program exceeds memory size");
                return false;
            }
        }
        

        // We don't check anything more than ORG, END and symbols in the first round.
        // So iterate until next line.
        while (index < code.size() && code[index] != '\r' && code[index] != '\n') {
            index += 1;
        } 
        if (code[index] == '\r') {
            if (index + 1 < code.size() && code[index + 1] == '\n') {
                index += 1;
            }
        }
        index += 1;
    }

    add_error("There was no END instruction in the code.");
    return false;
}

bool Assembler::second_pass() {
    lc = 0;
    while (index < code.size()) {
        if (auto token_optional = get_next_token()) {
            std::string_view token = *token_optional;
            // Pseudo instructions
            if (token == "END") {
                return true;
            }
            if (token == "ORG") {
                // We dont have to check the int this time.
                // Because we already did int he first pass.
                lc = get_next_int<std::uint16_t>(16, MEMORY_SIZE).value();
                line_end();
                continue;
            }

            // Check if the token is a label.
            if (is_token_label()) {
                if ((token_optional = get_next_token())) {
                    token = *token_optional;
                } else {
                    add_error("Excpected an instruction after the label.");
                    return false; 
                }
            }
            
            if (token == "DEC") {
                if (auto val = get_next_int<std::int8_t>(10)) {
                    memory[lc] = static_cast<std::uint8_t>(*val);
                } else {
                    return false;
                }
            } else if (token == "HEX") {
                if (auto val = get_next_int<std::uint8_t>(16)) {
                    memory[lc] = *val;
                } else {
                    return false;
                }
            } else if (auto instruction_optional = Instruction::from_mnemonic(token)) {
                auto instruction = *instruction_optional;
                if (instruction.mri) {
                    // A memory-reference instruction 
                    // Get the operands.
                    if (auto symbol_optional = get_next_token()) {
                        auto symbol = *symbol_optional;
                        
                        auto symbol_it = symbol_table.find(symbol);
                        if (symbol_it == symbol_table.end()) {
                            add_error("Unrecognized symbol \"{}\" after the {} instruction.", symbol, instruction.mnemonic);
                            return false; 
                        }
                         
                        std::uint16_t indirect = 0;
                            
                        if (auto indirect_optional = get_next_token()) {
                            if (*indirect_optional == "I") {
                                indirect = 1;
                            } else {
                                add_error("Unrecognized symbol \"{}\" after the {} instruction, Expected the indirect address instruction detonator (I).", *indirect_optional, instruction.mnemonic);
                                return false;
                            }
                        }

                        std::uint16_t value = 
                            static_cast<std::uint16_t>(indirect << 15) |
                            static_cast<std::uint16_t>(instruction.opcode << 12) |
                            symbol_it->second;   
                        
                        memory[lc] =  value; 
                    } else {
                        add_error("Excpected a symbol after the {} instruction.", instruction.mnemonic);
                        return false;
                    }
                } else {
                    memory[lc] =  instruction.opcode; 
                }
            } else {
                add_error("Unrecognized operation, \"{}\" is not a valid instruction.", token);
                return false; 
            }
             
            lc += 1;
        }

        if (!line_end()) {
            return false;
        }
    }

    return true;
}

std::optional<Emulator> Assembler::assemble(const std::string_view code_str) {
    // Reset the state.
    symbol_table.clear();
    errors.clear();
    
    memory.fill(0xFFFF);
    
    code = code_str;
    index = 0;
    current_line = 1;
    if (!first_pass()) {
        return {};
    }
    index = 0;
    current_line = 1;
    if (!second_pass()) {
        return {};
    }
    return Emulator{ memory };
}

} // namespace mano
