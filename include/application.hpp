#ifndef MONO_APPLICATION_HPP
#define MONO_APPLICATION_HPP

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/bind.h>

#include <chrono>
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

    void set_code(const std::string& code); 
    std::string get_code() const;

  private:
    void cycle_emulator();

    Assembler assembler;
    std::unique_ptr<mano::Emulator> emulator;

    mano::ui::Scheme scheme;
    std::string input_code;
    std::string user_input;

    bool emulator_running = false;
    double clock_rate = 0.0;
    double clock_period = 0.0; 

    std::chrono::steady_clock::time_point last_frame_tp;
    double elapsed_time = 0;
    
    std::string input_stream_string;
    std::string output_stream_string;
    bool input_stream_open = false;
    bool output_stream_open = false;

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;

    ImFont* code_font = nullptr;
};
} // namespace mano

EMSCRIPTEN_BINDINGS(module) {
    emscripten::class_<mano::Application>("Application")
        .constructor<>()
        .function("start", &mano::Application::start)
        .function("set_code", &mano::Application::set_code)
        .function("get_code", &mano::Application::get_code);
}

#endif
