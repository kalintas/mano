#ifndef MANO_EMULATOR_HPP
#define MANO_EMULATOR_HPP

#include "cpu.hpp"
#include "bus.hpp"

namespace mano {

class Emulator {
  public:
    Emulator(Memory emulator_memory) : memory(emulator_memory), bus(cpu, memory)  {}
    Emulator(Emulator&& emulator) : cpu(emulator.cpu), memory(std::move(emulator.memory)), bus(cpu, memory) {}

    const auto& get_memory() const {
        return memory;
    }
     
    void cycle() {
        cpu.cycle_once(bus);
    }

  public:
    Cpu cpu;
    Memory memory;
    Bus bus;
};

} // namespace mano

#endif
