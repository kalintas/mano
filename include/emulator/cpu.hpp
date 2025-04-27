#ifndef MANO_CPU_HPP
#define MANO_CPU_HPP

#include <cstdint>
#include <array>

namespace mano {

class Cpu {
  public:

    void fetch();
    void decode();
    void execute();
    void store();

  enum Registers : std::size_t {
    AR,
    PC,
    DR,
    AC,
    INPR,
    IR,
    TR,
    OUTR
  };

  std::array<std::uint16_t, 8> registers;
  
  private:
};

} // namespace mano

#endif
