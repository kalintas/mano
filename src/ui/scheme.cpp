#include "ui/scheme.hpp"

#include <imgui.h>
#include "emulator/cpu.hpp"

namespace mano::ui {

static constexpr ImU32 TEXT_COLOR = IM_COL32(0, 0, 0, 255);
static constexpr ImU32 READ_LINE_COLOR =
    IM_COL32(153, 187, 255, 255); // Pale Green
static constexpr ImU32 WRITE_LINE_COLOR =
    IM_COL32(255, 153, 187, 255); // Light Pink

void CircuitBox::render(ImDrawList* draw_list) const {
    draw_list
        ->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), BOX_COLOR);
    draw_list->AddText(
        ImVec2(x + 10.0f, y + height * 0.25f),
        TEXT_COLOR,
        name.c_str()
    );
    
    const float arrow_size = 7.0f;
    for (const auto& path : paths) {
        const auto& points = path.points;

        draw_list->AddPolyline(
            points.data(),
            static_cast<int>(points.size()),
            path.color,
            ImDrawFlags_RoundCornersAll,
            2.0f
        );

        if (points.size() < 2) {
            continue;
        }

        ImVec2 start = points[points.size() - 2];
        ImVec2 end = points[points.size() - 1];

        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.0001f)
            continue;

        dx /= length;
        dy /= length;

        ImVec2 arrow_tip(end.x, end.y);
        ImVec2 arrow_left(
            end.x - arrow_size * (dx + dy),
            end.y - arrow_size * (dy - dx)
        );
        ImVec2 arrow_right(
            end.x - arrow_size * (dx - dy),
            end.y - arrow_size * (dy + dx)
        );

        draw_list
            ->AddTriangleFilled(arrow_left, arrow_tip, arrow_right, path.color);
    }
}

Scheme::Scheme(
    float scheme_x,
    float scheme_y,
    float scheme_width,
    float scheme_height
) :
    x(scheme_x),
    y(scheme_y),
    width(scheme_width),
    height(scheme_height) {
    boxes.reserve(10);

    const float padding = 20.0f;
    const float box_height = 30.0f;
    const float reg_width = 120.0f;
    const float bus_width = 20.0f;

    // Memory unit
    float mem_width = 200.0f;
    float mem_height = 50.0f;
    float mem_x = x + (width - mem_width) / 2;
    float mem_y = y + padding * 2;

    // Registers
    const char* registers[] = {"AR", "PC", "DR", "AC", "INPR", "IR", "TR", "OUTR"};
    float start_y = mem_y + mem_height + padding * 2;
    float spacing = (height - start_y - padding) / 8;
    float x_pos = x + (width - reg_width) / 2;

    // Add registers
    for (int i = 0; i < 8; i++) {
        float y_pos = start_y + static_cast<float>(i) * spacing;
        if (i == Cpu::INPR) {
            y_pos += 20.0f;
        }
        boxes.emplace_back(registers[i], x_pos, y_pos, reg_width, box_height);
    }

    boxes.emplace_back(
        "Memory unit\n4096 x 16",
        mem_x,
        mem_y,
        mem_width,
        mem_height);

    // Adder and logic
    float adder_width = 100.0f;
    float adder_height = 60.0f;http://localhost:6931/index.html
    float adder_x = x_pos - adder_width - padding * 2;
    float adder_y = start_y + 3 * spacing - adder_height / 4;

    boxes.emplace_back(
        "Adder and\nlogic",
        adder_x,
        adder_y,
        adder_width,
        adder_height);

    // Add connection paths
    // Left and right bus positions
    float left_bus_x = x + padding;
    float right_bus_x = x + width - padding - bus_width;

    // Add paths for memory unit
    auto& memory = boxes[8];
    memory.add_path(
        {
            // From left bus to memory
            ImVec2(left_bus_x + bus_width, mem_y + mem_height / 2),
            ImVec2(mem_x, mem_y + mem_height / 2),
        },
        READ_LINE_COLOR
    );
    memory.add_path(
        {// From memory to right bus
         ImVec2(mem_x + mem_width, mem_y + mem_height / 2),
         ImVec2(right_bus_x, mem_y + mem_height / 2)
        },
        WRITE_LINE_COLOR
    );

    // Add paths for registers
    for (std::size_t i = Cpu::AR; i <= Cpu::OUTR; i++) { // Skip memory unit (index 0)
        if (i == Cpu::INPR) {
            continue;
        }
        auto& reg = boxes[i];
        if (i != Cpu::AC) { // AC
            // Bus to register.
            reg.add_path(
                {ImVec2(left_bus_x + bus_width, reg.y + box_height / 2),
                 ImVec2(reg.x, reg.y + box_height / 2)},
                READ_LINE_COLOR
            );
        }
        if (i != Cpu::OUTR) { // OUTR
            // Path from register to right bus.
            reg.add_path(
                {ImVec2(reg.x + reg_width, reg.y + box_height / 2),
                 ImVec2(right_bus_x, reg.y + box_height / 2)},
                WRITE_LINE_COLOR
            );
        }
    }

    auto& ar = boxes[Cpu::AR];
    ar.add_path(
        {ImVec2(ar.x + reg_width + 20.0f, ar.y + box_height / 2),
         ImVec2(ar.x + reg_width + 20.0f, mem_y + mem_height)},
        READ_LINE_COLOR
    );

    auto& dr = boxes[Cpu::DR];
    dr.add_path(
        {ImVec2(dr.x + reg_width + 20.0f, dr.y + box_height / 2),
         ImVec2(dr.x + reg_width + 20.0f, dr.y + 25.0f + box_height / 2),
         ImVec2(adder_x + adder_width - 20.0f, dr.y + 25.0f + box_height / 2),
         ImVec2(adder_x + adder_width - 20.0f, adder_y)},
        READ_LINE_COLOR
    );

    auto& ac = boxes[Cpu::AC];
    ac.add_path(
        {
            ImVec2(ac.x + reg_width + 20.0f, ac.y + box_height / 2),
            ImVec2(ac.x + reg_width + 20.0f, ac.y + 50.0f + box_height / 2),
            ImVec2(
                adder_x + adder_width - 20.0f,
                ac.y + 50.0f + box_height / 2
            ),
            ImVec2(adder_x + adder_width - 20.0f, adder_y + adder_height),
        },
        READ_LINE_COLOR
    );

    // Add path for INPR to adder
    auto& inpr = boxes[Cpu::INPR];
    inpr.add_path(
        {ImVec2(inpr.x, inpr.y + box_height / 2),
         ImVec2(adder_x + adder_width / 2, inpr.y + box_height / 2),
         ImVec2(adder_x + adder_width / 2, adder_y + adder_height)},
        READ_LINE_COLOR
    );

    auto& adder = boxes[9];
    adder.add_path(
        {ImVec2(adder.x + adder_width, adder.y + adder_height / 2),
         ImVec2(ac.x, ac.y + ac.height / 2)},
        WRITE_LINE_COLOR
    );
}

void Scheme::update(const Emulator& emulator) {
    // Update logic here if needed
}

void Scheme::render(Emulator& emulator) const {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    ImGui::Begin(
        "Scheme",
        nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
    );

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw buses
    const float padding = 20.0f;
    const float bus_width = 20.0f;
    float left_bus_x = x + padding;
    float right_bus_x = x + width - padding - bus_width;

    // Draw vertical buses
    draw_list->AddRectFilled(
        ImVec2(left_bus_x, y + padding),
        ImVec2(left_bus_x + bus_width, y + height - padding),
        CircuitBox::BOX_COLOR
    );
    draw_list->AddRectFilled(
        ImVec2(right_bus_x, y + padding),
        ImVec2(right_bus_x + bus_width, y + height - padding),
        CircuitBox::BOX_COLOR
    );

    // Draw horizontal bottom bus
    float bottom_y = y + height - padding * 2;
    draw_list->AddRectFilled(
        ImVec2(left_bus_x, bottom_y),
        ImVec2(right_bus_x + bus_width, bottom_y + bus_width),
        CircuitBox::BOX_COLOR
    );

    // Add bus label
    draw_list->AddText(
        ImVec2(x + width / 2 - 60, y + height - padding),
        TEXT_COLOR,
        "16-bit common bus"
    );

    // Render all boxes
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const auto& box = boxes[i];

        box.render(draw_list);
        
        // Draw register values next to the register boxes.
        if (i < emulator.cpu.registers.size()) {
            ImGui::SetCursorScreenPos(ImVec2(box.x + box.width - 45.0f, box.y + 5.0f));
            ImGui::PushItemWidth(40.0f); // Set the width of the InputScalar

            std::string input_id = "##" + box.name;
            ImGui::InputScalar(
                input_id.c_str(),
                ImGuiDataType_U16,
                &emulator.cpu.registers[i],
                nullptr,
                nullptr,
                "%04X"
            );
            ImGui::PopItemWidth(); 
        }

    }

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace mano::ui
