#ifndef MONO_APPLICATION_HPP
#define MONO_APPLICATION_HPP

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/bind.h>

#include "emulator/emulator.hpp"

namespace mono {

class Application {
  public:
    Application();
    ~Application();

    bool start();

    void update();
    void render();

  private:
    mano::Emulator emulator;

    SDL_Window* window;
    SDL_GLContext gl_context;
};
} // namespace mono

EMSCRIPTEN_BINDINGS(module) {
    emscripten::class_<mono::Application>("Application")
        .constructor<>()
        .function("start", &mono::Application::start);
}

#endif
