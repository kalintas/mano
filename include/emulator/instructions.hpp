#ifndef MANO_INSTRUCTIONS_HPP
#define MANO_INSTRUCTIONS_HPP

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

namespace mano {

struct Instruction {
    std::string_view mnemonic;
    std::uint16_t opcode;
    bool mri; // Whether is a memory-reference instruction or not.
    std::string_view description;
    
    /*
     * Returns the 12 bit address in MRI instructions.
     * */
    constexpr std::uint16_t get_address() const {
        return opcode & 0x0FFF;
    }
    

    /*
     * Returns whether the instruction is indirect in MRI instructions.
     * */
    constexpr bool is_indirect() const {
        return (opcode & 0x8000) != 0;   
    }

    static constexpr std::optional<Instruction> from_mnemonic(const std::string_view mnemonic);
    static constexpr std::optional<Instruction> from_opcode(const std::uint16_t opcode); 
};

constexpr std::array<Instruction, 25> INSTRUCTIONS = {
    // Opcodes in MRI instruction only signify the bits 2-4.
    // First bit is whether its in direct or indirect addressing mode.
    Instruction {"AND", 0x0, true, "AND M to AC"},
    Instruction {"ADD", 0x1, true, "Add M to AC, carry to E"},
    Instruction {"LDA", 0x2, true, "Load AC from M"},
    Instruction {"STA", 0x3, true, "Store AC in M"},
    Instruction {"BUN", 0x4, true, "Branch unconditionally to m"},
    Instruction {"BSA", 0x5, true, "Save return address in m and branch to m + 1"},
    Instruction {"ISZ", 0x6, true, "Increment M and skip if zero"},
    // Non MRI instructions
    Instruction {"CLA", 0x7800, false, "Clear AC"},
    Instruction {"CLE", 0x7400, false, "Clear E"},
    Instruction {"CMA", 0x7200, false, "Complement AC"},
    Instruction {"CME", 0x7100, false, "Complement E"},
    Instruction {"CIR", 0x7080, false, "Circulate right E and AC"},
    Instruction {"CIL", 0x7040, false, "Circulate left E and AC"},
    Instruction {"INC", 0x7020, false, "Increment AC"},
    Instruction {"SPA", 0x7010, false, "Skip if AC is positive"},
    Instruction {"SNA", 0x7008, false, "Skip if AC is negative"},
    Instruction {"SZA", 0x7004, false, "Skip if AC is zero"},
    Instruction {"SZE", 0x7002, false, "Skip if E is zero"},
    Instruction {"HLT", 0x7001, false, "Halt computer"},
    Instruction {"INP", 0xF800, false, "Input information and clear flag"},
    Instruction {"OUT", 0xF400, false, "Output information and clear flag"},
    Instruction {"SKI", 0xF200, false, "Skip if input flag is on"},
    Instruction {"SKO", 0xF100, false, "Skip if output flag is on"},
    Instruction {"ION", 0xF080, false, "Turn interrupt on"},
    Instruction {"IOF", 0xF040, false, "Turn interrupt off"}
};


constexpr std::optional<Instruction> Instruction::from_mnemonic(const std::string_view mnemonic_str) {
    for (const auto& instruction: INSTRUCTIONS) {
        if (instruction.mnemonic == mnemonic_str) {
            return instruction;
        }
    }
    
    return {};
}


constexpr std::optional<Instruction> Instruction::from_opcode(const std::uint16_t opcode) {
    // First check the non MRI instructions.
    for (std::size_t i = 7; i < INSTRUCTIONS.size(); ++i) {
        if (INSTRUCTIONS[i].opcode == opcode) {
            return INSTRUCTIONS[i];
        }
    }
    // Then try MRI instructions.
    std::size_t index = (opcode >> 12) & 0x7;   
    if (index <= 6) {
        Instruction instruction = INSTRUCTIONS[index];
        instruction.opcode = opcode;
        return instruction;    
    }
    return {};
}

} // namespace mano

#endif
