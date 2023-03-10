#include "Visualization.h"
#include <iostream>
namespace CFSUI::Visualization {
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
    void animateCFS(const std::vector<ImVec2> &points) {
        auto draw_list = ImGui::GetWindowDrawList();

        bool is_clicked_replay = ImGui::Button("Replay");

        static float speed {1.0f};
        ImGui::InputFloat("speed", &speed, 0.1f, 1.0f, "%.1f");

        static float thickness {1.0f};
        ImGui::InputFloat("thickness", &thickness, 0.1f, 1.0f, "%.1f");

        static ImVec4 CFS_color_vec {0.0f, 1.0f, 0.0f, 1.0f};
        static const ImGuiColorEditFlags color_editor_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
        ImGui::ColorEdit4("Editor CFS Color", (float*) &CFS_color_vec, color_editor_flags);


        static size_t curr {0};
        static double refresh_time {ImGui::GetTime()};
        if (is_clicked_replay) {
            curr = 0;
            refresh_time = ImGui::GetTime();
        }

        double delta = 15.0 / static_cast<double>(points.size()); // animate 15s
        delta /= speed;
        while (refresh_time < ImGui::GetTime()) {
            if (curr < points.size()) {
                curr++;
            }
            refresh_time += delta;
        }

        ImVec2 p = ImGui::GetCursorScreenPos();
        p.x += 100;
        p.y += 100;
        for (size_t i = 0; i < curr; i++) {
            draw_list->PathLineTo({points[i].x+p.x, points[i].y+p.y});
        }

        draw_list->PathStroke(ImGui::GetColorU32(CFS_color_vec), ImDrawFlags_None, thickness);

    }

    ImVec2 translate {0.0f, 0.0f};
    float scaling {5.0f};

    inline ImVec2 transform(const ImVec2& point) {
        return {point.x*scaling+translate.x, point.y*scaling+translate.y};
    }

    void showVisualization(const std::vector<ImVec2> &points, float tool_path_size) {
        // Visualization Window modal
        if (ImGui::BeginPopupModal(u8"可视化 CFS")) {
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            static float siz {2.0f};
            ImGui::SliderFloat("visualization tool path size", &siz, 0.f, 10.f);
            static int progress {1000};
            ImGui::SliderInt("progress slider", &progress, 0, points.size());
            ImGui::InputInt("progress input", &progress);

            ImGuiIO &io = ImGui::GetIO();
            ImVec2 mouse_pos_rel {(io.MousePos.x - translate.x) / scaling, (io.MousePos.y - translate.y) / scaling};

            static ImVec2 canvas_center_prev {0.0f, 0.0f};
            const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            const ImVec2 canvas_center {canvas_p0.x + canvas_sz.x / 2.0f, canvas_p0.y + canvas_sz.y / 2.0f};

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
            static ImVec2 pre_point {1e3, 1e3};
            for (size_t i = 0; i < progress; i++) {
                if (std::pow(points[i].x-pre_point.x, 2)+std::pow(points[i].y-pre_point.y, 2) < tool_path_size/2) {
                    continue;
                }
                draw_list->PathLineTo(transform(points[i]));
                pre_point = points[i];
//                draw_list->AddCircleFilled(transform(points[i]), siz*scaling/2.0, IM_COL32(0, 0, 0, 80));
            }
            draw_list->PathStroke(IM_COL32(0, 0, 0, 80), ImDrawFlags_None, siz*scaling);

/*
            float min_len {1000000.0f};
            float max_len {0.0f};
            for (size_t i = 0; i + 1 < points.size(); i++) {
                auto len = std::hypot(points[i+1].x-points[i].x, points[i+1].y-points[i].y);
                min_len = std::min(min_len, len);
                max_len = std::max(max_len, len);
            }
            std::cout << min_len << ", " << max_len << std::endl;
            exit(0);
*/


/*
            for (const auto& point : points) {
                draw_list->PathLineTo(transform(point));
            }

            ImDrawFlags draw_flags = ImDrawFlags_None;
            draw_list->PathStroke(ImGui::GetColorU32({0, 1.0f, 0, 0.3f}), draw_flags, siz*scaling);
*/


            ImGui::EndPopup();
        }
    }
}