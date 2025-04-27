#ifndef MANO_EMULATOR_HPP
#define MANO_EMULATOR_HPP

#include "cpu.hpp"
#include "bus.hpp"

namespace mano {

class Emulator {
  public:
    Emulator() : bus(cpu, memory) {}
    Emulator(Memory emulator_memory) : memory(emulator_memory), bus(cpu, memory)  {}

    const auto& get_memory() const {
        return memory;
    }
     
    void cycle() {
        cpu.fetch();
        cpu.decode();
        cpu.execute();
        cpu.store();
    }

  public:
    Cpu cpu;
  private:
    Memory memory;
    Bus bus;
};

} // namespace mano

#endif
