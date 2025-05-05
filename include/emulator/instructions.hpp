#ifndef MANO_INSTRUCTIONS_HPP
#define MANO_INSTRUCTIONS_HPP

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

namespace mano {

enum class Instr: std::size_t {
    AND,
    ADD,
    LDA,
    STA,
    BUN,
    BSA,
    ISZ,
    CLA,
    CLE,
    CMA,
    CME,
    CIR,
    CIL, INC, SPA, SNA, SZA, SZE, HLT, INP, OUT, SKI, SKO, ION, IOF
};

struct Instruction {

    Instr instr;
    std::string_view mnemonic;
    std::uint16_t opcode;
    bool mri; // Whether is a memory-reference instruction or not.
    std::string_view cycle_name;
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
        return (opcode & 0x8000) != 0 && ((opcode & 0xF000) != 0xF000);   
    }

    static constexpr std::optional<Instruction> from_mnemonic(const std::string_view mnemonic);
    static constexpr std::optional<Instruction> from_opcode(const std::uint16_t opcode); 
};



constexpr std::array<Instruction, 25> INSTRUCTIONS = {
    // Opcodes in MRI instruction only signify the bits 2-4.
    // First bit is whether its in direct or indirect addressing mode.
    Instruction { Instr::AND,"AND", 0x0, true, "", "AND M to AC"},
    Instruction { Instr::ADD,"ADD", 0x1, true, "", "Add M to AC, carry to E"},
    Instruction { Instr::LDA,"LDA", 0x2, true, "", "Load AC from M"},
    Instruction { Instr::STA,"STA", 0x3, true, "", "Store AC in M"},
    Instruction { Instr::BUN,"BUN", 0x4, true, "", "Branch unconditionally to m"},
    Instruction { Instr::BSA,"BSA", 0x5, true, "", "Save return address in m and branch to m + 1"},
    Instruction { Instr::ISZ,"ISZ", 0x6, true, "", "Increment M and skip if zero"},
    // Non MRI instructions
    Instruction { Instr::CLA ,"CLA", 0x7800, false, "rB11", "Clear AC"},
    Instruction { Instr::CLE ,"CLE", 0x7400, false, "rB10", "Clear E"},
    Instruction { Instr::CMA ,"CMA", 0x7200, false, "rB9", "Complement AC"},
    Instruction { Instr::CME ,"CME", 0x7100, false, "rB8", "Complement E"},
    Instruction { Instr::CIR ,"CIR", 0x7080, false, "rB7", "Circulate right E and AC"},
    Instruction { Instr::CIL ,"CIL", 0x7040, false, "rB6", "Circulate left E and AC"},
    Instruction { Instr::INC ,"INC", 0x7020, false, "rB5", "Increment AC"},
    Instruction { Instr::SPA ,"SPA", 0x7010, false, "rB4", "Skip if AC is positive"},
    Instruction { Instr::SNA ,"SNA", 0x7008, false, "rB3", "Skip if AC is negative"},
    Instruction { Instr::SZA ,"SZA", 0x7004, false, "rB2", "Skip if AC is zero"},
    Instruction { Instr::SZE ,"SZE", 0x7002, false, "rB1", "Skip if E is zero"},
    Instruction { Instr::HLT ,"HLT", 0x7001, false, "rB0", "Halt computer"},
    Instruction { Instr::INP ,"INP", 0xF800, false, "pB11", "Input information and clear flag"},
    Instruction { Instr::OUT ,"OUT", 0xF400, false, "pB10", "Output information and clear flag"},
    Instruction { Instr::SKI ,"SKI", 0xF200, false, "pB9", "Skip if input flag is on"},
    Instruction { Instr::SKO ,"SKO", 0xF100, false, "pB8", "Skip if output flag is on"},
    Instruction { Instr::ION ,"ION", 0xF080, false, "pB7", "Turn interrupt on"},
    Instruction { Instr::IOF ,"IOF", 0xF040, false, "pB6", "Turn interrupt off"}
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
