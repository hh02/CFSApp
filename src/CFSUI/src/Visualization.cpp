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
    static int progress{1000000000};
    if (progress >= points.size()) {
        progress = points.size();
    }

    bool is_clicked_animate{false};
    static bool is_animating{false};
    static float speed{1.0f};
    const float thickness {1.0f};

    static const ImGuiColorEditFlags color_editor_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    static ImVec4 CFS_color_vec {0.0f, 1.0f, 0.0f, 1.0f};
    static ImVec4 CFS_fill_color_vec {0.0f, 1.0f, 1.0f, 0.3f};

    ImGui::SliderInt("progress slider", &progress, 0, static_cast<int>(points.size()));

    if (visualization_type == VisualizationType_Default) {
        is_clicked_animate = ImGui::Button(u8"动画");
        if (is_clicked_animate) {
            is_animating = true;
        }

        ImGui::InputFloat("speed", &speed, 0.1f, 1.0f, "%.1f");

        ImGui::ColorEdit4("Editor CFS Color", (float*) &CFS_color_vec, color_editor_flags);
    }
    else if (visualization_type == VisualizationType_Fill) {
        ImGui::ColorEdit4("Editor CFS Fill Color", (float*) &CFS_fill_color_vec, color_editor_flags);
    }

    ImGuiIO &io = ImGui::GetIO();
    ImVec2 mouse_pos_rel {(io.MousePos.x - translate.x) / scaling, (io.MousePos.y - translate.y) / scaling};

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
        static size_t curr {0};
        static double refresh_time {ImGui::GetTime()};
        if (is_clicked_animate) {
            curr = 0;
            refresh_time = ImGui::GetTime();
        }

        if (is_animating) {
            double delta = 15.0 / static_cast<double>(points.size()); // animate 15s
            delta /= speed;
            while (curr < points.size() && refresh_time < ImGui::GetTime()) {
                curr++;
                refresh_time += delta;
            }
            if (curr == points.size()) {
                is_animating = false;
            }

        }

        if (is_animating == false) {
            curr = progress;
        }
        for (size_t i = 0; i < curr; i++) {
            draw_list->PathLineTo(transform(points[i]));
        }
        draw_list->PathStroke(ImGui::GetColorU32(CFS_color_vec), ImDrawFlags_None, thickness);
    }
    else if (visualization_type == VisualizationType_Fill) {
        static ImVec2 pre_point {1e3, 1e3};
        for (size_t i = 0; i < progress; i++) {
            if (std::hypot(points[i].x-pre_point.x, points[i].y-pre_point.y) < point_distance_threshold) {
                continue;
            }
            draw_list->PathLineTo(transform(points[i]));
            pre_point = points[i];
        }
        draw_list->PathStroke(ImGui::GetColorU32(CFS_fill_color_vec), ImDrawFlags_None, tool_path_size*scaling);
    }

    draw_list->PopClipRect();
}

}