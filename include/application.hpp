#ifndef MONO_APPLICATION_HPP
#define MONO_APPLICATION_HPP

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/bind.h>

#include <memory>
#include <string>

#include "emulator/assembler.hpp"
#include "emulator/emulator.hpp"
#include "imgui.h"
#include "ui/scheme.hpp"

namespace mano {

class Application {
  public:
    Application();
    ~Application();

    bool start();

    void update();
    void render();

  private:
    Assembler assembler;
    std::unique_ptr<mano::Emulator> emulator;

    mano::ui::Scheme scheme;
    std::string input_code;
    std::string user_input;

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;

    ImFont* code_font = nullptr;
};
} // namespace mano

EMSCRIPTEN_BINDINGS(module) {
    emscripten::class_<mano::Application>("Application")
        .constructor<>()
        .function("start", &mano::Application::start);
}

#endif
