#include <emscripten/html5.h>

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

static constexpr auto EXAMPLE_CODE =
    R"(ORG 000  
LDA N1  
ADD N2  
STA RES  
HLT  

ORG 100  
N1, DEC 5  
N2, DEC 3  
RES, DEC 0  

END)";

static constexpr unsigned int DEFAULT_CLOCK_RATE = 64;

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
    clock_rate(DEFAULT_CLOCK_RATE) {
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

void Application::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
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
    
    /*
    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
    ); // White text
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.2f, 0.2f, 0.2f, 1.0f)
    ); // Dark gray background
*/
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
        }
    }

    ImGui::PopFont();
    //ImGui::PopStyleColor(2);
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

    if (ImGui::Button("Step")) {
        emulator->cycle();
        scheme.update(*emulator);
    }

    ImGui::SameLine();

    if (ImGui::Button("Run")) {
        for (unsigned int i = 0; i < clock_rate; ++i) {
            emulator->cycle();
            scheme.update(*emulator);
        }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(45.0f);
    ImGui::InputScalar("Clock rate", ImGuiDataType_U32, &clock_rate);
    ImGui::PopItemWidth();

    ImGui::Text("%s", emulator->cpu.get_cycle_name().data());

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

    ImGui::End();

    // Input window
    ImGui::SetNextWindowPos(ImVec2(0, screen_size.y - quarter_height));
    ImGui::SetNextWindowSize(
        ImVec2(screen_size.x / 6, quarter_height)
    );
    ImGui::Begin(
        "Input",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Text("Input panel");
    // Add more instruction content here
    ImGui::End();

    // Output window
    ImGui::SetNextWindowPos(
        ImVec2(screen_size.x / 6, screen_size.y - quarter_height)
    );
    ImGui::SetNextWindowSize(ImVec2(screen_size.x / 6, quarter_height));
    ImGui::Begin(
        "Output",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Text("output Panel");
    // Add more instruction content here
    ImGui::End();

    // Output window
    ImGui::SetNextWindowPos(
        ImVec2(screen_size.x / 3, screen_size.y - quarter_height)
    );
    ImGui::SetNextWindowSize(ImVec2(screen_size.x / 6, quarter_height));
    ImGui::Begin(
        "CPU",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Text("cpu");
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
