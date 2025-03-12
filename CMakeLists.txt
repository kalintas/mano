cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #

project(mano)

set(MANO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(MANO_SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(
    MANO_SRC_FILES 
    "${MANO_SRC_DIR}/application.cpp" 
    "${MANO_SRC_DIR}/emulator.cpp" 
)

set(MANO_IMGUI_BACKEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-backend")

add_executable(mano 
    ${MANO_SRC_FILES}
    # Imgui backend
    "${MANO_IMGUI_BACKEND_DIR}/imgui_impl_sdl2.cpp"     
    "${MANO_IMGUI_BACKEND_DIR}/imgui_impl_opengl3.cpp" 
)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(mano
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(mano PRIVATE imgui::imgui)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(mano FALSE "" "" "" "")

target_include_directories(mano PRIVATE 
    ${MANO_INCLUDE_DIR} 
    ${MANO_IMGUI_BACKEND_DIR}
)

set_target_properties(mano PROPERTIES LINK_FLAGS "--bind")
set_target_properties(
    mano PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Emscripten specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set_target_properties(mano PROPERTIES
        LINK_FLAGS "-s USE_GLFW=3 -s WASM=1 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 --bind"
    )
endif()

