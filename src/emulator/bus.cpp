
#include "emulator/bus.hpp"

#include <cstdint>

#include "emulator/cpu.hpp"

namespace mano {

void Bus::read_memory() {
    memory_io = memory[cpu.registers.get(Registers::AR)];
}

void Bus::write_memory() {
    memory[cpu.registers.get(Registers::AR)] = memory_io;
}

void Bus::load(Selection dest, Selection source) {
    std::uint16_t value = 0;
    switch (source) {
        case Selection::AR:
            value = cpu.registers.get(Registers::AR);
            break;
        case Selection::PC:
            value = cpu.registers.get(Registers::PC);
            break;
        case Selection::DR:
            value = cpu.registers.get(Registers::DR);
            break;
        case Selection::AC:
            value = cpu.registers.get(Registers::AC);
            break;
        case Selection::IR:
            value = cpu.registers.get(Registers::IR);
            break;
        case Selection::TR:
            value = cpu.registers.get(Registers::TR);
            break;
        case Selection::MemoryUnit:
            value = memory_io;
            break;
        case Selection::OUTR:
        case Selection::None:
            break;
    }

    switch (dest) {
        case Selection::AR:
            cpu.registers.set(Registers::AR, value);
            break;
        case Selection::PC:
            cpu.registers.set(Registers::PC, value);
            break;
        case Selection::DR:
            cpu.registers.set(Registers::DR, value);
            break;
        case Selection::AC:
            cpu.registers.set(Registers::AC, value);
            break;
        case Selection::IR:
            cpu.registers.set(Registers::IR, value);
            break;
        case Selection::TR:
            cpu.registers.set(Registers::TR, value);
            break;
        case Selection::MemoryUnit:
            memory_io = value;
            break;
        case Selection::OUTR:
            cpu.registers.set(Registers::OUTR, value);
            break;
        case Selection::None:
            break;
    }
    last_dest = dest;
    last_source = source;
    transfer_value = value;
}
} // namespace mano
