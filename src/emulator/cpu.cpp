#include "emulator/cpu.hpp"
#include "emulator/bus.hpp"
#include "emulator/instructions.hpp"

namespace mano {


void Cpu::cycle_once(Bus& bus) {
    auto cycle = sequence_counter;
    sequence_counter += 1;
    switch (cycle) {
        // Fetch
        case 0:
            cycle_name = "Fetch R'T0";
            // R'T0: AR <- PC
            bus.load(Bus::Selection::AR, Bus::Selection::PC);
            return;
        case 1:
            cycle_name = "Fetch R'T1";
            // R'T1:  
            // IR <- M[AR]
            bus.read_memory(); 
            bus.load(Bus::Selection::IR, Bus::Selection::MemoryUnit);
            // PC <- PC + 1
            registers.set(Registers::PC, registers.get(Registers::PC) + 1);
            return;
        // Decode
        case 2:
            cycle_name = "Decode R'T2";
            // R'T2:  
            if (std::optional<Instruction> decode = Instruction::from_opcode(registers.get(Registers::IR))) {
                instruction = decode.value();
                // AR <- IR(0 ~ 11)   
                bus.load(Bus::Selection::AR, Bus::Selection::IR);;
                // I <- IR(15)
                indirect = static_cast<bool>(registers.get(Registers::IR) >> 15);
            } 
            return;
        case 3:
            if (!instruction.mri) {
                auto ac = registers.get(Registers::AC);
                switch (instruction.instr) {
                    case Instr::CLA:
                        registers.set(Registers::AC, 0);
                        break;
                    case Instr::CLE:
                        alu.e = false;
                        break;
                    case Instr::CMA:
                        alu.load(registers, Registers::AC);
                        registers.set(Registers::AC, alu.operate(Instr::CMA));
                        break;
                    case Instr::CME:
                        alu.operate(Instr::CME);
                        break;
                    case Instr::CIR:
                        alu.load(registers, Registers::AC);
                        registers.set(Registers::AC, alu.operate(Instr::CIR));
                        break;
                    case Instr::CIL:
                        alu.load(registers, Registers::AC);
                        registers.set(Registers::AC, alu.operate(Instr::CIL));
                        break;
                    case Instr::INC:
                        registers.set(Registers::AC, ac + 1); 
                        break;
                    case Instr::SPA:
                        if ((ac >> 15) == 0) {
                            registers.set(Registers::PC, registers.get(Registers::PC) + 1);
                        }
                        break;
                    case Instr::SNA:
                        if ((ac >> 15) != 0) {
                            registers.set(Registers::PC, registers.get(Registers::PC) + 1);
                        }
                        break;
                    case Instr::SZA:
                        if (ac == 0) {
                            registers.set(Registers::PC, registers.get(Registers::PC) + 1);
                        }
                        break;
                    case Instr::SZE:
                        if (alu.e == 0) {
                            registers.set(Registers::PC, registers.get(Registers::PC) + 1);
                        }
                        break;
                    case Instr::HLT:
                        break;
                    case Instr::INP:
                        break;
                    case Instr::OUT:
                        break;
                    case Instr::SKI:
                        break;
                    case Instr::SKO:
                        break;
                    case Instr::ION:
                        break;
                    case Instr::IOF:
                        break;
                    default:
                        break;
                }
                cycle_name = instruction.cycle_name;
                sequence_counter = 0; // Reset the cycle counter.
            } else if (indirect) {
                cycle_name = "Decode D7'IT3";  
                // AR <- M[AR]
                bus.read_memory();
                bus.load(Bus::Selection::AR, Bus::Selection::MemoryUnit);
            } else {
                cycle_name = "T3";  
            }
            return;
        default:
            break;
    }

    // Implement the MRI instructions. 
    switch (instruction.instr) {
        case Instr::AND:
            if (cycle == 4) {
                cycle_name = "D0T4";
                // DR <- M[AR]
                bus.read_memory();
                bus.load(Bus::Selection::DR, Bus::Selection::MemoryUnit);
                return;
            } else {
                cycle_name = "D0T5";
                // AC <- AC & DR
                alu.load(registers, Registers::AC, Registers::DR);
                registers.set(Registers::AC, alu.operate(Instr::AND));
            }
            break;
        case Instr::ADD:
            if (cycle == 4) {
                cycle_name = "D1T4";
                // DR <- M[AR]
                bus.read_memory();
                bus.load(Bus::Selection::DR, Bus::Selection::MemoryUnit);
                return;
            } else {
                cycle_name = "D1T5";
                // AC <- AC + DR
                alu.load(registers, Registers::AC, Registers::DR);
                registers.set(Registers::AC, alu.operate(Instr::ADD));
            }
            break;
        case Instr::LDA:
            if (cycle == 4) {
                cycle_name = "D2T4";
                // DR <- M[AR]
                bus.read_memory();
                bus.load(Bus::Selection::DR, Bus::Selection::MemoryUnit);
                return;
            } else {
                cycle_name = "D2T5";
                // AC <- DR
                alu.load(registers, Registers::DR);
                registers.set(Registers::AC, alu.operate(Instr::LDA));
            }
            break;
        case Instr::STA:
            cycle_name = "D3T4";
            // M[AR] <- AC
            bus.load(Bus::Selection::MemoryUnit, Bus::Selection::AC);
            bus.write_memory();
            break;
        case Instr::BUN:
            cycle_name = "D4T4";
            // PC <- AR 
            bus.load(Bus::Selection::PC, Bus::Selection::AR);
            break;
        case Instr::BSA:
            if (cycle == 4) {
                cycle_name = "D5T4";
                // M[AR] <- PC
                bus.load(Bus::Selection::MemoryUnit, Bus::Selection::PC);
                bus.write_memory();
                return;
            } else {
                cycle_name = "D5T5";
                bus.load(Bus::Selection::PC, Bus::Selection::AR);  
            }
            break;
        case Instr::ISZ:
            if (cycle == 4) {
                cycle_name = "D6T4";
                // DR <- M[AR]
                bus.read_memory();
                bus.load(Bus::Selection::DR, Bus::Selection::MemoryUnit);
                return;
            } else if (cycle == 5) {
                cycle_name = "D6T5";
                // DR <- DR + 1
                registers.set(Registers::DR, registers.get(Registers::DR) + 1); 
                return;
            } else {
                cycle_name = "D6T6";
                // M[AR] <- DR 
                bus.load(Bus::Selection::MemoryUnit, Bus::Selection::DR);
                bus.write_memory();
                if (registers.get(Registers::DR) == 0) {

                    registers.set(Registers::PC, registers.get(Registers::PC) + 1);
                }
            }
            break;
        default:
            break;
    }

    sequence_counter = 0;
}


void Cpu::cycle(Bus& bus, std::size_t cycle_count) {
    for (std::size_t i = 0; i += cycle_count; ++i) {
        cycle_once(bus);
    }
}

}
