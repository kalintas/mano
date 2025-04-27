#ifndef MANO_SCHEME_HPP
#define MANO_SCHEME_HPP

#include <imgui.h>

#include <functional>
#include <vector>

#include "emulator/emulator.hpp"

namespace mano::ui {

struct CircuitBox {
    struct Path {
        ImU32 color;
        std::vector<ImVec2> points;
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

    void add_path(std::vector<ImVec2> path, ImU32 line_color) {
        paths.emplace_back(line_color, std::move(path));
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

class Scheme {
  public:
    Scheme(float x, float y, float width, float height);

    void update(const Emulator& emulator);
    void render(Emulator& emulator) const;

  private:
    float x;
    float y;
    float width;
    float height;

    std::vector<CircuitBox> boxes;
};

} // namespace mano::ui

#endif
