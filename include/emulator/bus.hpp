#ifndef MANO_BUS_HPP
#define MANO_BUS_HPP

#include <array>
#include <cstdint>

namespace mano {

class Cpu;

static constexpr std::size_t MEMORY_SIZE = 4096;
using Memory = std::array<std::uint16_t, MEMORY_SIZE>;

class Bus {
public:
    Bus(Cpu& cpu_ref, Memory& memory_ref) : cpu(cpu_ref), memory(memory_ref) {}
    
    std::uint16_t read_memory(std::uint16_t address) {
        if (address < MEMORY_SIZE) {
            return memory[address];
        } 
        return 0;
    } 
    
    void write_memory(std::uint16_t address, std::uint16_t value) {
        if (address < MEMORY_SIZE) {
            memory[address] = value;
        }
    }

    std::uint16_t read_register(std::size_t index);
    void write_register(std::size_t index, std::uint16_t value);

private:
    Cpu& cpu;
    Memory& memory;
};
}

#endif


