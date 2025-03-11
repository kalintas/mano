#ifndef MANO_EMULATOR_HPP
#define MANO_EMULATOR_HPP

#include <SDL.h>

namespace mano {

class Emulator {
  public:
    Emulator() {}

    void init();
    void start();
};
} // namespace mano

#endif
