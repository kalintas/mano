
#include "emulator/bus.hpp"
#include <cstddef>
#include <cstdint>
#include "emulator/cpu.hpp"

namespace mano {


std::uint16_t Bus::read_register(std::size_t index) {
    if (index < cpu.registers.size()) {
        return cpu.registers[index];
    }
    return 0;
}

void Bus::write_register(std::size_t index, std::uint16_t value) {
    if (index < cpu.registers.size()) {
        cpu.registers[index] = value;
    }
}



}
