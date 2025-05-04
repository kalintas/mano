#ifndef MANO_CPU_HPP
#define MANO_CPU_HPP

#include <cstdint>

#include "emulator/instructions.hpp"

namespace mano {

class Bus;

class Registers {
public:
    enum Id : std::size_t { AR, PC, DR, AC, IR, TR, OUTR, INPR };

    static constexpr std::size_t REGISTER_COUNT = 8;
    
    std::uint16_t get(Id reg) const {
        return registers[reg];
    }
    void set(Id reg, std::uint16_t value) {
        registers[reg] = value;
        switch (reg) {
            case AR:
            case PC:
                registers[reg] &= 0xFFF;
                break;
            case INPR:
            case OUTR:
                registers[reg] &= 0xFF;
                break;
            default:
                break;
        }
    }
    
    std::uint16_t get(std::size_t index) const {
        if (index >= registers.size()) {
            return 0;
        }
        return registers[index];
    }

    void set(std::size_t index, std::uint16_t value) {
        set(static_cast<Id>(index), value); 
    }
    
private:
    std::array<std::uint16_t, REGISTER_COUNT> registers{};
};

class Alu {
public:
    
    void load(const Registers& registers, Registers::Id input_a) {
        a = registers.get(input_a);
        a_register = input_a;
    }
    void load(const Registers& registers, Registers::Id input_a, Registers::Id input_b) {
        a = registers.get(input_a);
        b = registers.get(input_b);
        a_register = input_a;
        b_register = input_b;
    }
    std::uint16_t operate(Instr instruction) {
        switch (instruction) {
            case Instr::AND:
                operation = "And";
                result = a & b;
                break;
            case Instr::ADD:
                operation = "Add";
                result = a + b;
                break;
            case Instr::LDA:
                operation = "Load";
                result = a;
                break;
            case Instr::CMA:
                operation = "CMA";
                result = ~a;
                break;
            case Instr::CME:
                operation = "CME";
                e = !e;
                break;
            case Instr::CIR:
            {
                operation = "CIR";
                auto val = static_cast<std::uint16_t>((e << 15) | (a >> 1));
                e = a & 0x1;
                result = val;
                break;
            }
            case Instr::CIL:
            {
                operation = "CIL";
                auto val = static_cast<std::uint16_t>(e | (a << 1));
                e = (a >> 15) & 0x1;
                result = val;
                break;
            }
            default: // Unrecognized alu instruction.
                break;
        }
        
        return result;
    }

    std::uint16_t a = 0;
    std::uint16_t b = 0;
    std::uint16_t result = 0;
    bool e = false;

    std::string_view operation = "";
    std::optional<Registers::Id> a_register;
    std::optional<Registers::Id> b_register;
private:
};

class Cpu {
  public:
    void cycle_once(Bus& bus);
    void cycle(Bus& bus, std::size_t cycle_count);


    std::size_t get_sequence_counter() const {
        return sequence_counter;
    }

    std::string_view get_cycle_name() const {
        return cycle_name;
    }

    Registers registers;
    Alu alu;

  private:

    Instruction instruction;
    std::size_t sequence_counter = 0;

    std::string_view cycle_name;

    bool indirect = false;
};

} // namespace mano

#endif
