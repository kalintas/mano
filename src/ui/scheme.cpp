#include "ui/scheme.hpp"

#include <imgui.h>

#include <cstdint>
#include <vector>

#include "emulator/cpu.hpp"

namespace mano::ui {

static constexpr ImU32 BOX_COLOR = IM_COL32(240, 240, 240, 255);
static constexpr ImU32 BOX_OUTLINE_COLOR = IM_COL32(200, 200, 200, 255);

static constexpr ImU32 TEXT_COLOR = IM_COL32(0, 0, 0, 255);
static constexpr ImU32 READ_LINE_COLOR =
    IM_COL32(153, 187, 255, 255); // Pale Green
static constexpr ImU32 WRITE_LINE_COLOR =
    IM_COL32(255, 153, 187, 255); // Light Pink
static constexpr ImU32 VALUE_CHANGED_COLOR = IM_COL32(0, 190, 0, 255); // Dark Green


//static constexpr ImU32 READ_WRITE_LINE_COLOR =
//   IM_COL32(130, 130, 130, 255);

void CircuitBox::render(ImDrawList* draw_list) const {
    draw_list
        ->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), BOX_COLOR);
    draw_list
        ->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), BOX_OUTLINE_COLOR);
    if (!name.empty()) {
        draw_list->AddText(
            ImVec2(x + 10.0f, y + height * 0.25f),
            TEXT_COLOR,
            name.c_str()
        );
    }

    const float arrow_size = 7.0f;
    for (const auto& path : paths) {
        const auto& points = path.points;

        ImU32 color;
        switch (path.op) {
            case Path::Read:
                color = READ_LINE_COLOR;
                break;
            case Path::Write:
                color = WRITE_LINE_COLOR;
                break;
            case Path::ReadWrite:
                color = BOX_OUTLINE_COLOR;
                break;
        }

        draw_list->AddPolyline(
            points.data(),
            static_cast<int>(points.size()),
            color,
            ImDrawFlags_RoundCornersAll,
            4.0f
        );

        if (points.size() < 2) {
            continue;
        }

        auto draw_arrow = [&](ImVec2 start, ImVec2 end) {
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float length = std::sqrt(dx * dx + dy * dy);
            if (length < 0.0001f) {
                return;
            }

            dx /= length;
            dy /= length;

            ImVec2 arrow_left(
                end.x - arrow_size * (dx + dy),
                end.y - arrow_size * (dy - dx)
            );
            ImVec2 arrow_right(
                end.x - arrow_size * (dx - dy),
                end.y - arrow_size * (dy + dx)
            );

            draw_list->AddTriangleFilled(arrow_left, end, arrow_right, color);
        };

        switch (path.op) {
            case Path::Read:
                draw_arrow(points[1], points[0]);
                break;
            case Path::Write:
                draw_arrow(
                    points[points.size() - 2],
                    points[points.size() - 1]
                );
                break;
            case Path::ReadWrite:
                draw_arrow(points[1], points[0]);
                draw_arrow(
                    points[points.size() - 2],
                    points[points.size() - 1]
                );
                break;
        }
    }
}

static constexpr float BUS_X = 30.0f;

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
    boxes.reserve(11);

    const float padding = 20.0f;
    const float box_height = 30.0f;
    const float reg_width = 120.0f;
    const float bus_width = 30.0f;

    // Common bus position (left side)

    // Memory unit
    float mem_width = 200.0f;
    float mem_height = 50.0f;
    float mem_x = x + BUS_X + bus_width + padding + 100.0f;
    float mem_y = y + padding * 2;

    // Registers layout
    const char* registers[] = {"AR", "PC", "DR", "AC", "IR", "TR"};
    float start_y = mem_y + mem_height + padding * 2;
    float spacing = (height - start_y - padding) / 7;
    float reg_x = x + BUS_X + bus_width + padding + 100.0f;

    // Add registers
    for (int i = 0; i < 6; i++) {
        float y_pos = start_y + static_cast<float>(i) * spacing;
        boxes.emplace_back(registers[i], reg_x, y_pos, reg_width, box_height);
    }

    // Add memory unit
    boxes.emplace_back("", mem_x, mem_y, mem_width, mem_height);

    boxes.emplace_back(
        "OUTR",
        reg_x,
        start_y + 6 * spacing,
        reg_width,
        box_height
    );
    boxes.emplace_back(
        "INPR",
        reg_x,
        start_y + 7 * spacing,
        reg_width,
        box_height
    );

    auto& ac =
        boxes[static_cast<std::size_t>(Bus::Selection::AC)]; // AC register

    // Adder and logic unit
    float alu_width = 100.0f;
    float alu_height = 100.0f;
    float alu_x = ac.x + reg_width + padding * 2;
    float alu_y = ac.y + (box_height - alu_height) * 0.5f;

    // Add ALU
    boxes.emplace_back("", alu_x, alu_y, alu_width, alu_height);

    boxes.emplace_back("E", alu_x, alu_y - 40.0f, 50.0f, 27.5f);

    // Move INPR beneath ALU
    auto& inpr = boxes[8];
    inpr.y = alu_y + alu_height + padding + 30.0f;
    inpr.x = alu_x + (alu_width - reg_width) / 2; // Center INPR under ALU

    // Add connection paths
    // Memory unit paths
    auto& memory = boxes[static_cast<std::size_t>(Bus::Selection::MemoryUnit)];
    memory.add_path(
        {
            ImVec2(x + BUS_X + bus_width, mem_y + mem_height / 2),
            ImVec2(mem_x, mem_y + mem_height / 2),
        },
        CircuitBox::Path::ReadWrite
    );

    // Add paths for registers
    for (std::size_t i = 0; i < 8; i++) {
        if (i == static_cast<std::size_t>(Bus::Selection::MemoryUnit)) {
            continue;
        }

        auto& reg = boxes[i];
        auto op = CircuitBox::Path::ReadWrite;

        if (i == static_cast<std::size_t>(Bus::Selection::AC)) {
            op = CircuitBox::Path::Write;
        }

        // Bus to register (read)
        reg.add_path(
            {
                ImVec2(reg.x, reg.y + box_height / 2),
                ImVec2(x + BUS_X + bus_width, reg.y + box_height / 2),
            },
            op
        );
    }

    // Special connections for AR to Memory
    auto& ar = boxes[static_cast<std::size_t>(Bus::Selection::AR)];
    ar.add_path(
        {
            ImVec2(ar.x + reg_width - 20.0f, mem_y + mem_height),
            ImVec2(ar.x + reg_width - 20.0f, ar.y),
        },
        CircuitBox::Path::Read
    );

    // ALU connections
    auto& alu = boxes[9];
    auto& dr = boxes[static_cast<std::size_t>(Bus::Selection::DR)];
    
    // ALU to DR 
    alu.add_path(
        {
            ImVec2(alu.x + alu_width, alu.y + 15.0f),
            ImVec2(alu.x + alu_width + 30.0f, alu.y + 15.0f),
            ImVec2(alu.x + alu_width + 30.0f, dr.y + box_height / 2 - 30.0f),
            ImVec2(dr.x + reg_width + 20.0f, dr.y + box_height / 2 - 30.0f),
            ImVec2(dr.x + reg_width + 20.0f, dr.y + box_height / 2),
            ImVec2(dr.x + reg_width, dr.y + box_height / 2),
        },
        CircuitBox::Path::Read
    );
    
    // ALU to AC
    alu.add_path(
        {
            ImVec2(alu.x + alu.width, alu.y + alu.height - 15.0f),
            ImVec2(alu.x + alu.width + 15.0f, alu.y + alu.height - 15.0f),
            ImVec2(alu.x + alu.width + 15.0f, ac.y + ac.height + 42.5f),
            ImVec2(ac.x + ac.width / 2, ac.y + ac.height + 42.5f),
            ImVec2(ac.x + ac.width / 2, ac.y + ac.height),
        },
        CircuitBox::Path::Read
    );

    // ALU to INPR 
    alu.add_path(
        {
            ImVec2(alu.x + alu.width, alu.y + alu.height / 2),
            ImVec2(inpr.x + reg_width + 40.0f, alu.y + alu.height / 2),
            ImVec2(inpr.x + reg_width + 40.0f, inpr.y + inpr.height / 2),
            ImVec2(inpr.x + reg_width, inpr.y + inpr.height / 2),
        },
        CircuitBox::Path::Read
    );

    // ALU to AC
    alu.add_path(
        {
            ImVec2(alu_x, alu_y + alu_height / 2),
            ImVec2(ac.x + reg_width, ac.y + box_height / 2),
        },
        CircuitBox::Path::Write
    );

    auto& e = boxes[10];

    e.add_path(
        {ImVec2(alu_x + 20.0f, alu_y), ImVec2(alu_x + 20.0f, e.y + e.height)},
        CircuitBox::Path::Write
    );
}

void Scheme::update(Emulator& emulator) {
    if (emulator.bus.last_source != Bus::Selection::None
        && emulator.bus.last_dest != Bus::Selection::None) {
        auto& source =
            boxes[static_cast<std::size_t>(emulator.bus.last_source)];
        auto& dest = boxes[static_cast<std::size_t>(emulator.bus.last_dest)];

        Animation animation{ emulator.bus.transfer_value };
        animation.set_points({
            ImVec2(source.x, source.y + source.height / 2),
            ImVec2(x + BUS_X, source.y + source.height / 2),
            ImVec2(x + BUS_X, dest.y + dest.height / 2),
            ImVec2(dest.x, dest.y + dest.height / 2),
        });
        animations.push_back(std::move(animation));

        emulator.bus.last_dest = Bus::Selection::None;
        emulator.bus.last_source = Bus::Selection::None;
        emulator.bus.transfer_value = 0;
    }
    auto add_animation = [&] (Registers::Id reg, std::uint16_t value) {
        Animation animation{ value };
        
        const auto& alu = boxes[9];
        std::size_t path_index = 0;

        switch (reg) {
            case Registers::DR:
                path_index = 0;
                break;
            case Registers::AC:
                path_index = 1;
                break;
            case Registers::INPR:
                path_index = 2;
                break;
            default:
                return;
        }
        
        const auto& points = alu.paths[path_index].points;
        std::vector<ImVec2> result;
        result.reserve(points.size());
        for (auto it = points.rbegin(); it != points.rend(); ++it) {
            result.push_back(*it);
        }
        animation.set_points(std::move(result));
        animations.push_back(std::move(animation));
    };

    if (emulator.cpu.alu.a_register) {
        add_animation(emulator.cpu.alu.a_register.value(), emulator.cpu.alu.a);
    }
    if (emulator.cpu.alu.b_register) {
        add_animation(emulator.cpu.alu.b_register.value(), emulator.cpu.alu.a);
    }

    emulator.cpu.alu.a_register = {}; 
    emulator.cpu.alu.b_register = {}; 

    old_registers = new_registers;
    new_registers = emulator.cpu.registers;

    old_memory_io = new_memory_io;
    new_memory_io =
        emulator.get_memory()[emulator.cpu.registers.get(Registers::AR)];

    old_alu = new_alu;
    new_alu = emulator.cpu.alu;
}

void Scheme::render(Emulator& emulator) {
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

    // Draw the vertical common bus on the left
    const float padding = 20.0f;
    // Draw vertical bus
    draw_list->AddRectFilled(
        ImVec2(x + BUS_X, y + padding),
        ImVec2(x + BUS_X + 30.0f, y + height - padding),
        BOX_COLOR
    );
    draw_list->AddRect(
        ImVec2(x + BUS_X, y + padding),
        ImVec2(x + BUS_X + 30.0f, y + height - padding),
        BOX_OUTLINE_COLOR  
    );

    // Add bus label
    const char* bus_text = "16-bit Common Bus";
    ImVec2 text_size = ImGui::CalcTextSize(bus_text);
    ImVec2 text_pos(x + BUS_X - 20.0f, y + height - text_size.y / 2 - 10.0f);

    draw_list->AddText(text_pos, TEXT_COLOR, bus_text);

    // Render all boxes
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const auto& box = boxes[i];

        box.render(draw_list);

        if (i == static_cast<std::size_t>(Bus::Selection::MemoryUnit)) {
            ImGui::SetCursorScreenPos(ImVec2(box.x + 10.0f, box.y + 15.0f));
            ImGui::PushItemWidth(40.0f);

            std::uint16_t mem = emulator.get_memory(
            )[emulator.cpu.registers.get(Registers::AR)];

            if (new_memory_io != old_memory_io) {
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImGui::ColorConvertU32ToFloat4(VALUE_CHANGED_COLOR)
                );
            }

            if (ImGui::InputScalar(
                    "##mem",
                    ImGuiDataType_U16,
                    &mem,
                    nullptr,
                    nullptr,
                    "%04X"
                )) {
                emulator.cpu.registers.set(i, mem);
            }

            if (new_memory_io != old_memory_io) {
                ImGui::PopStyleColor();
            }

            ImGui::PopItemWidth();

            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
            ImGui::SetCursorScreenPos(ImVec2(box.x + 55.0f, box.y + 17.5f));
            ImGui::Text("M[AR]");
            ImGui::SetCursorScreenPos(
                ImVec2(box.x + box.width - 85.0f, box.y + 10.0f)
            );
            ImGui::Text("Memory Unit\n4096x16");
            ImGui::PopStyleColor();

        } else if (i <= 8) { // Skip memory unit (0) and ALU (9)
            ImGui::SetCursorScreenPos(
                ImVec2(box.x + box.width - 45.0f, box.y + 5.0f)
            );
            ImGui::PushItemWidth(40.0f);

            std::size_t register_index = i;

            const char* format;
            switch (i) {
                case static_cast<std::size_t>(Bus::Selection::AR): // AR
                case static_cast<std::size_t>(Bus::Selection::PC): // PC
                    format = "%03X";
                    break;
                case static_cast<std::size_t>(Bus::Selection::OUTR): // OUTR
                case 8: // INPR
                    format = "%02X";
                    register_index = i - 1;
                    break;
                default:
                    format = "%04X";
                    break;
            }

            if (old_registers.get(register_index)
                != new_registers.get(register_index)) {
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImGui::ColorConvertU32ToFloat4(VALUE_CHANGED_COLOR)
                );
            }

            std::uint16_t value = emulator.cpu.registers.get(register_index);
            std::string input_id = "##" + box.name;

            if (ImGui::InputScalar(
                    input_id.c_str(),
                    ImGuiDataType_U16,
                    &value,
                    nullptr,
                    nullptr,
                    format
                )) {
                emulator.cpu.registers.set(register_index, value);
            }

            if (old_registers.get(register_index)
                != new_registers.get(register_index)) {
                ImGui::PopStyleColor();
            }

            ImGui::PopItemWidth();
        }
    }

    // Render ALU
    auto& alu = boxes[9];
    ImGui::PushItemWidth(40.0f);

    if (new_alu.operation != old_alu.operation) {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImGui::ColorConvertU32ToFloat4(VALUE_CHANGED_COLOR)
        );
    }

    char operation_str[20] = {};
    std::memcpy(operation_str, emulator.cpu.alu.operation.data(), emulator.cpu.alu.operation.size());
    ImGui::SetCursorScreenPos(ImVec2(alu.x + 5.0f, alu.y + 5.0f));
    ImGui::InputText("##op", operation_str, sizeof(operation_str));
    
    ImGui::SetCursorScreenPos(ImVec2(alu.x + 50.0f, alu.y + 7.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
    ImGui::Text("Op");
    ImGui::PopStyleColor();

    if (new_alu.operation != old_alu.operation) {
        ImGui::PopStyleColor();
    }

    auto render_alu_val = [&] (std::string label, std::uint16_t new_value, std::uint16_t old_value, float y_pos) {
        if (new_value != old_value) {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImGui::ColorConvertU32ToFloat4(VALUE_CHANGED_COLOR)
            );
        }
    
        auto value = new_value;

        ImGui::SetCursorScreenPos(ImVec2(alu.x + 5.0f, alu.y + y_pos));
        ImGui::InputScalar(
            ("##" + label).c_str(),
            ImGuiDataType_U16,
            &value,
            nullptr,
            nullptr,
            "%04X"
        );
         
        if (new_value != old_value) {
            ImGui::PopStyleColor();
        }
         
        ImGui::SetCursorScreenPos(ImVec2(alu.x + 50.0f, alu.y + y_pos + 2.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
        ImGui::Text("%s", label.c_str());
        ImGui::PopStyleColor();
    };
    
    render_alu_val("A", new_alu.a, old_alu.a, 27.5f);
    render_alu_val("B", new_alu.b, old_alu.b, 50.f);
    render_alu_val("R", new_alu.result, old_alu.result, 72.5f);

    ImGui::PopItemWidth();
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
    ImGui::SetCursorScreenPos(
        ImVec2(alu.x + alu.width - 27.5f, alu.y + alu.height - 17.5f)
    );
    ImGui::Text("ALU");
    ImGui::PopStyleColor();

    // Render E carry flag.
    auto& e = boxes[boxes.size() - 1];
    ImGui::SetCursorScreenPos(ImVec2(e.x + e.width - 25.0f, e.y + 5.f));

    ImGui::Checkbox("##E", &emulator.cpu.alu.e);

    for (auto it = animations.begin(); it != animations.end();) {
        if (it - 1 != animations.end()
            && (it - 1)->get_render_count() < 5) {
            break;
        }

        if (it->render()) {
            it = animations.erase(it);
        } else {
            ++it;
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

bool Animation::render() {
    if (current_point < points.size()) {
        constexpr float move = 0.8f;

        auto point = points[current_point];
        float dx = point.x - pos.x;
        float dy = point.y - pos.y;
        float length2 = dx * dx + dy * dy;
        if (length2 > move * move) {
            pos.x += dx * 0.1f;
            pos.y += dy * 0.1f;
        } else {
            current_point += 1;
        }
    } else {
        return true;
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y - 6.0f));
    ImGui::PushItemWidth(40.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);

    ImGui::Text("%04X", value);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    render_count += 1;

    return false;
}

} // namespace mano::ui
