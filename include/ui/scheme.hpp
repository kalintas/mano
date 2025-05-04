#ifndef MANO_SCHEME_HPP
#define MANO_SCHEME_HPP

#include <imgui.h>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "emulator/cpu.hpp"
#include "emulator/emulator.hpp"

namespace mano::ui {

struct CircuitBox {
    struct Path {
        enum Operation {
            Read,
            Write,
            ReadWrite
        };
        std::vector<ImVec2> points;
        Operation op;
    };

    CircuitBox(
        std::string box_name,
        float box_x,
        float box_y,
        float box_width,
        float box_height) :
        name(std::move(box_name)),
        x(box_x),
        y(box_y),
        width(box_width),
        height(box_height) {}

    void add_path(std::vector<ImVec2> path, Path::Operation op) {
        paths.emplace_back(std::move(path), op);
    }

    void render(ImDrawList* draw_list) const;

    static constexpr ImU32 BOX_COLOR = IM_COL32(180, 180, 180, 255);

    std::string name;

    float x;
    float y;
    float width;
    float height;

    std::vector<Path> paths;
};

class Animation {
public: 
    Animation(std::uint16_t val) : value(val) {}

    void set_points(std::vector<ImVec2> pts) {
        points = std::move(pts);
        pos = points[0];
    }

    bool render();
    
    std::size_t get_render_count() const {
        return render_count;
    }

private:
    std::vector<ImVec2> points;
    std::size_t current_point = 0;
    std::size_t render_count = 0;
     
    ImVec2 pos = {};
    std::uint16_t value;
};

class Scheme {
  public:
    Scheme(float x, float y, float width, float height);

    void update(Emulator& emulator);
    void render(Emulator& emulator);

  private:
    float x;
    float y;
    float width;
    float height;

    std::vector<CircuitBox> boxes;
    
    std::vector<Animation> animations;

    Registers old_registers;
    Registers new_registers;
    
    Alu old_alu;
    Alu new_alu; 

    std::uint16_t old_memory_io = 0;
    std::uint16_t new_memory_io = 0;
};

} // namespace mano::ui

#endif
