#include <emscripten/html5.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_stdlib.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
    #include <SDL_opengles2.h>
#else
    #include <SDL_opengl.h>
#endif

#include "application.hpp"
#include "emulator/instructions.hpp"

namespace mano {

static constexpr std::size_t SCREEN_WIDTH = 1024;
static constexpr std::size_t SCREEN_HEIGHT = 720;
static constexpr float FSCREEN_WIDTH = static_cast<float>(SCREEN_WIDTH);
static constexpr float FSCREEN_HEIGHT = static_cast<float>(SCREEN_HEIGHT);

static constexpr std::size_t IO_STRING_CAPACITY = 300;

static constexpr auto EXAMPLE_CODE =
    R"(ORG 000
ZRO, BUN R1
BUN S1
ORG 10
S1, LDA N1
ADD N2
STA SUM
LDA N1
AND MSK
STA RES
LDA N1
BSA SUB
LDA N1
ISZ CNT
BUN SKP
LDA N1
SKP, LDA N1

CLA
CLE
LDA N1
CMA
CME
CIR
CIL
INC
SPA
LDA N1
SNA
LDA N1
SZA
LDA N1
SZE
LDA N1
HLT

R1, INP
OUT
SKI
LDA N1
SKO
LDA N1
ION
IOF
BUN ZRO I

SUB, DEC 0
    LDA N2
    BUN SUB I

ORG 100
N1, DEC 5
N2, DEC 3
SUM, DEC 0
MSK, HEX FFF0
RES, DEC 0
CNT, DEC 0

END)";

static constexpr double DEFAULT_CLOCK_RATE = 2.0;

void main_loop(void* arg) {
    Application* app = static_cast<Application*>(arg);
    if (app) {
        app->update();
        app->render();
    }
}

Application::Application() :
    scheme(FSCREEN_WIDTH * 0.5f, 0, FSCREEN_WIDTH * 0.5f, FSCREEN_HEIGHT),
    input_code(EXAMPLE_CODE),
    clock_rate(DEFAULT_CLOCK_RATE),
    clock_period(1.0 / DEFAULT_CLOCK_RATE),
    input_string(IO_STRING_CAPACITY, '\0') {
    emulator =
        std::make_unique<Emulator>(assembler.assemble(EXAMPLE_CODE).value());
}

bool Application::start() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER)
        != 0) {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // GL ES 3.0 + SDL
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow(
        "Mano",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        window_flags
    );
    if (!window) {
        std::cerr << "Error: Window creation failed: " << SDL_GetError()
                  << std::endl;
        return false;
    }

    std::cout << "Creating GL context\n";
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        std::cerr << "Error: OpenGL context creation failed: " << SDL_GetError()
                  << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, gl_context);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup light theme
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Add borders to frames
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;

    // Adjust colors for better contrast
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);

    // First add the default font
    io.Fonts->AddFontDefault();

    // Then add a larger font for the code editor
    ImFontConfig font_config;
    font_config.SizePixels = 20.0f;
    code_font = io.Fonts->AddFontDefault(&font_config);
    io.Fonts->Build();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    std::cout << "Initialization complete\n";
    // Store instance and set up emscripten main loop
    emscripten_set_main_loop_arg(main_loop, this, -1, true);
    return true;
}

void Application::cycle_emulator() {

    if (input_str_len > 0 && !emulator->cpu.fgi) {
        const auto value = input_string[0];
        input_string.erase(0, 1);

        emulator->cpu.registers.set(Registers::INPR, static_cast<std::uint8_t>(value));
        emulator->cpu.fgi = true;
    }

    emulator->cycle();
    scheme.update(*emulator);

    if (!emulator->cpu.fgo) {
        const auto value = emulator->cpu.registers.get(Registers::OUTR);
        output_string.push_back(static_cast<char>(value));
        emulator->cpu.fgo = true;
    }
}

void Application::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    if (emulator_running) {
        std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        const auto elapsed_microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(
                now - last_frame_tp
            )
                .count();
        elapsed_time += static_cast<double>(elapsed_microseconds) / 1000000.0;
        last_frame_tp = now;
        const double cycles = std::round(elapsed_time * clock_rate);

        std::cout << elapsed_time << " " << cycles << std::endl;

        for (int i = 0; i < static_cast<int>(cycles); ++i) {
            cycle_emulator();
        }

        elapsed_time -= cycles * clock_period;
    }
}

void Application::render() {
    glClearColor(1.00f, 1.00f, 1.00f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Get the total screen size
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    float quarter_width = screen_size.x * 0.25f;
    float quarter_height = screen_size.y * 0.25f;

    // Code window on the far left
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(
        ImVec2(quarter_width, screen_size.y - quarter_height)
    );

    ImGui::Begin(
        "Code",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );

    // Display compilation errors if any
    const auto& errors = assembler.get_errors();
    if (!errors.empty()) {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
        ); // Red color
        for (const auto& error : errors) {
            ImGui::TextWrapped(
                "Line %zu: %s",
                error.line,
                error.message.c_str()
            );
        }
        ImGui::PopStyleColor();
    }

    ImGui::PushFont(code_font);

    ImVec2 available = ImGui::GetContentRegionAvail();
    bool code_changed = ImGui::InputTextMultiline(
        "##code_input",
        &input_code,
        available,
        ImGuiInputTextFlags_AllowTabInput
    );

    if (code_changed) {
        auto compile_result = assembler.assemble(input_code);
        if (compile_result.has_value()) {
            emulator =
                std::make_unique<Emulator>(std::move(compile_result.value()));
            emulator_running = false;
        }
    }

    ImGui::PopFont();
    ImGui::End();

    // Instructions window in the middle
    ImGui::SetNextWindowPos(ImVec2(quarter_width, 0));
    ImGui::SetNextWindowSize(
        ImVec2(quarter_width, screen_size.y - quarter_height)
    );
    ImGui::Begin(
        "Debugger",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );

    ImGui::BeginChild("Controls", ImVec2(0, 20), false);
    if (ImGui::Button("Step")) {
        if (!emulator_running) {
            cycle_emulator();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(emulator_running ? "Stop" : "Run")) {
        emulator_running = !emulator_running;
        if (emulator_running) {
            last_frame_tp = std::chrono::steady_clock::now();
        }
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(45.0f);
    if (ImGui::InputDouble("Clock rate", &clock_rate, 0.0, 0.0, "%.2f")) {
        clock_period = 1.0 / static_cast<double>(clock_rate);
    }
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::BeginChild("MemoryView", ImVec2(0, 0), true);

    for (std::size_t i = 0; i < MEMORY_SIZE; ++i) {
        auto opcode = emulator->get_memory()[i];
        std::string_view mnemonic = "";
        std::string address = "";
        std::string_view indirect = "";

        if (opcode != 0xFFFF) {
            if (auto instruction = Instruction::from_opcode(opcode)) {
                mnemonic = instruction->mnemonic;
                if (instruction->mri) {
                    address = std::format("{:03x}", instruction->get_address());
                }
                if (instruction->is_indirect()) {
                    indirect = "I";
                }
            } else {
                mnemonic = "Undefined";
            }
        }

        auto pc = emulator->cpu.registers.get(Registers::PC);
        if (emulator->cpu.get_sequence_counter() >= 2) {
            pc -= 1;
        }

        if (i == pc) {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
            );
        }
        ImGui::Text(
            "%03zx:  %04x      %s %s %s",
            i,
            opcode,
            mnemonic.data(),
            address.data(),
            indirect.data()
        );
        if (i == pc) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
    ImGui::End();

    const auto small_window_width = screen_size.x / 6;
    // Input window
    ImGui::SetNextWindowPos(ImVec2(0, screen_size.y - quarter_height));
    ImGui::SetNextWindowSize(ImVec2(small_window_width, quarter_height));
    ImGui::Begin(
        "Input Stream",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    if (ImGui::InputTextMultiline(
        "##input",
        input_string.data(),
        IO_STRING_CAPACITY,
        ImVec2(small_window_width - 15, quarter_height - 35)
    )) {
        input_str_len = std::strlen(input_string.c_str());
    }

    // Add more instruction content here
    ImGui::End();

    // Output window
    ImGui::SetNextWindowPos(
        ImVec2(small_window_width, screen_size.y - quarter_height)
    );
    ImGui::SetNextWindowSize(ImVec2(small_window_width, quarter_height));
    ImGui::Begin(
        "Output Stream",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    ImGui::InputTextMultiline(
        "##output",
        output_string.data(),
        output_string.size(),
        ImVec2(small_window_width - 15, quarter_height - 35),
        ImGuiInputTextFlags_ReadOnly
    );
    // Add more instruction content here
    ImGui::End();

    // Output window
    ImGui::SetNextWindowPos(
        ImVec2(screen_size.x / 3, screen_size.y - quarter_height)
    );
    ImGui::SetNextWindowSize(ImVec2(small_window_width, quarter_height));
    ImGui::Begin(
        "CPU",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Text("Cycle: %s", emulator->cpu.get_cycle_name().data());
    ImGui::Checkbox("S", &emulator->cpu.start_stop);
    ImGui::SameLine();
    ImGui::Checkbox("I", &emulator->cpu.indirect);

    ImGui::Checkbox("FGI", &emulator->cpu.fgi);
    ImGui::SameLine();
    ImGui::Checkbox("FGO", &emulator->cpu.fgo);
    ImGui::SameLine();
    ImGui::Checkbox("IEN", &emulator->cpu.ien);
    ImGui::Checkbox("R", &emulator->cpu.r);

    ImGui::Text("Instruction: %s", emulator->cpu.instruction.mnemonic.data());
    ImGui::TextWrapped(
        "Description: %s",
        emulator->cpu.instruction.description.data()
    );

    // Add more instruction content here
    ImGui::End();

    scheme.render(*emulator);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

Application::~Application() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (gl_context)
        SDL_GL_DeleteContext(gl_context);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace mano
