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
       
    enum class Selection : std::size_t {
        AR = 0,
        PC,
        DR,
        AC,
        IR,
        TR,
        MemoryUnit,
        OUTR,
        None,
    };

    /*
     * Reads the AR register and stores the M[AR]
     * */
    void read_memory();
    /*
     * Reads the AR register and writes value to the M[AR].
     * */
    void write_memory();
   
    /*
     * Loads the value in the source to the dest.
     * */
    void load(Selection dest, Selection source);

    Selection last_dest = Selection::None;
    Selection last_source = Selection::None;
    std::uint16_t transfer_value = 0;

private:
    Cpu& cpu;
    Memory& memory;
    std::uint16_t memory_io = 0;
};
}

#endif


