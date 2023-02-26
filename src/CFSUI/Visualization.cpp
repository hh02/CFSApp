#include "Visualization.h"
#include <iostream>
namespace CFSUI::Visualization {
    void getPointsFromFile(std::vector<ImVec2>& points) {
        std::ifstream fin {"./0.path", std::ios::in};
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
    void animateCFS(const std::vector<ImVec2> &points, float speed, bool replay) {
        auto draw_list = ImGui::GetWindowDrawList();
        static size_t curr {0};
        static double refresh_time {ImGui::GetTime()};
        if (replay) {
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
        static const auto color = IM_COL32(0, 255, 0, 255);
        static const auto thickness = 2.0f;
        draw_list->PathStroke(color, ImDrawFlags_None, thickness);

    }
}