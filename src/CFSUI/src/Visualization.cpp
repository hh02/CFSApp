#include "Visualization.h"
#include <iostream>
namespace CFSUI::Visualization {
int visualization_type{0};

void getPointsFromFile(std::vector<ImVec2>& points) {
    std::ifstream fin {"./output.path", std::ios::in};
    if (!fin.is_open()) {
        return;
    }

    size_t n {0};
    float tmp;
    // the 4th number is the number of points
    for (int i = 0; i < 3; i++) {
        fin >> tmp;
    }
    fin >> n;
    points = std::vector<ImVec2> {n};

    for (int i = 0; i < n; i++) {
        fin >> points[i].x >> points[i].y;
        // ignore not used value
        fin >> tmp;
        fin >> tmp;
    }
}

ImVec2 translate {0.0f, 0.0f};
float scaling {5.0f};

inline ImVec2 transform(const ImVec2& point) {
    return {point.x*scaling+translate.x, point.y*scaling+translate.y};
}

void visualizeCFS(const std::vector<ImVec2>& points, float tool_path_size) {
    if (points.empty()) {
        return;
    }
    const float point_distance_threshold{tool_path_size * 0.8f};
    static float percentage {100.0f};

    bool is_clicked_animate{false};
    static bool is_animating{false};
    const float speeds[] = {0.5f, 1.0f, 1.5f, 2.0f};
    const char* speeds_text[] = {"0.5x", "1.0x", "1.5x", "2.0x"};
    static int selected_speed_idx{1};
    static float thickness {0.5f};

    static const ImGuiColorEditFlags color_editor_flags = ImGuiColorEditFlags_NoAlpha
//        | ImGuiColorEditFlags_NoInputs
//        | ImGuiColorEditFlags_NoLabel
        ;
    static ImVec4 CFS_color_vec {0.0f, 1.0f, 0.0f, 1.0f};
    static ImVec4 CFS_fill_color_vec {0.0f, 1.0f, 1.0f, 0.3f};

    if (visualization_type == VisualizationType_Default) {
        is_clicked_animate = ImGui::Button(u8"播放");
        ImGui::SameLine();
        if (ImGui::Button(u8"倍速")) {
            ImGui::OpenPopup("speed_select_popup");
        }

        if (ImGui::BeginPopup("speed_select_popup")) {
            for (int i = 0; i < IM_ARRAYSIZE(speeds_text); i++) {
                if (ImGui::Selectable(speeds_text[i])) {
                    selected_speed_idx = i;
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SliderFloat(u8"播放进度", &percentage, 0.0f, 100.0f, "%.1f%%");
        if (is_clicked_animate) {
            is_animating = true;
        }

        ImGui::ColorEdit4("路径颜色##1", (float*) &CFS_color_vec, color_editor_flags);
        ImGui::SliderFloat(u8"路径宽度", &thickness, 0.1f, tool_path_size, "%.2f");
    }
    else if (visualization_type == VisualizationType_Fill) {
        ImGui::ColorEdit4("路径颜色##2", (float*) &CFS_fill_color_vec, color_editor_flags);
    }

    ImGuiIO &io = ImGui::GetIO();
//    ImVec2 mouse_pos_rel {(io.MousePos.x - translate.x) / scaling, (io.MousePos.y - translate.y) / scaling};

    static ImVec2 canvas_center_prev {0.0f, 0.0f};
    const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    const ImVec2 canvas_p1{canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};
    const ImVec2 canvas_center{canvas_p0.x + canvas_sz.x / 2.0f, canvas_p0.y + canvas_sz.y / 2.0f};

    ImGui::InvisibleButton("visualization_invisible_button", canvas_sz,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        translate.x += io.MouseDelta.x;
        translate.y += io.MouseDelta.y;
    }

    if (ImGui::IsItemHovered() && io.MouseWheel != 0) {
        float c = io.MouseWheel > 0 ? 1.05f : 0.95f;
        scaling *= c;
        translate.x = translate.x * c + (1.0f - c) * io.MousePos.x;
        translate.y = translate.y * c + (1.0f - c) * io.MousePos.y;
    }

    translate.x += (canvas_center.x - canvas_center_prev.x);
    translate.y += (canvas_center.y - canvas_center_prev.y);
    canvas_center_prev = canvas_center;

    auto draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(canvas_p0, canvas_p1, true);

    if (visualization_type == VisualizationType_Default) {
        static size_t progress{0};
        static double refresh_time {ImGui::GetTime()};
        if (is_clicked_animate) {
            progress = 0;
            refresh_time = ImGui::GetTime();
        }

        if (is_animating) {
            // default animate 15s
            const double delta{(15.0/static_cast<double>(points.size())) / speeds[selected_speed_idx]};
            while (progress < points.size() && refresh_time < ImGui::GetTime()) {
                progress++;
                refresh_time += delta;
            }
            if (progress == points.size()) {
                is_animating = false;
            }
            percentage = 100.0 * progress / points.size();
        }
        else {
            progress = std::floor(1.0 * points.size() * percentage / 100.0);
        }

        for (size_t i = 0; i < progress; i++) {
            draw_list->PathLineTo(transform(points[i]));
        }
        draw_list->PathStroke(ImGui::GetColorU32(CFS_color_vec), ImDrawFlags_None, thickness*scaling);
    }
    else if (visualization_type == VisualizationType_Fill) {
        static ImVec2 pre_point {1e3, 1e3};
        for (auto point : points) {
            if (std::hypot(point.x-pre_point.x, point.y-pre_point.y) < point_distance_threshold) {
                continue;
            }
            draw_list->PathLineTo(transform(point));
            pre_point = point;
        }
        draw_list->PathStroke(ImGui::GetColorU32(CFS_fill_color_vec), ImDrawFlags_None, tool_path_size*scaling);
    }

    draw_list->PopClipRect();
}

}