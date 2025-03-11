#include <emscripten/html5.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
    #include <SDL_opengles2.h>
#else
    #include <SDL_opengl.h>
#endif

#include "application.hpp"

namespace mono {

void main_loop(void* arg) {
    Application* app = static_cast<Application*>(arg);
    if (app) {
        app->update();
        app->render();
    }
}

Application::Application() : window(nullptr), gl_context(nullptr) {
    printf("Application constructor called\n");
}

bool Application::start() {
    printf("Initializing Application\n");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER)
        != 0) {
        printf("Error: SDL_Init failed: %s\n", SDL_GetError());
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
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    printf("Creating window\n");
    window = SDL_CreateWindow(
        "Mano Computer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        window_flags
    );
    if (!window) {
        printf("Error: Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    printf("Creating GL context\n");
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("Error: OpenGL context creation failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(window, gl_context);

    // Setup Dear ImGui context
    printf("Setting up ImGui\n");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Store instance and set up emscripten main loop
    emscripten_set_main_loop_arg(main_loop, this, -1, true);
    printf("Initialization complete\n");
    return true;
}

void Application::update() {
    // Handle SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Show the demo window
    ImGui::ShowDemoWindow(&show_demo_window);
}

void Application::render() {
    ImGui::Render();
    SDL_GL_MakeCurrent(window, gl_context);
    glViewport(
        0,
        0,
        (int)ImGui::GetIO().DisplaySize.x,
        (int)ImGui::GetIO().DisplaySize.y
    );
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

Application::~Application() {
    printf("Application destructor called\n");
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

} // namespace mono
