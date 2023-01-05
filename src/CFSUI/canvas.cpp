// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "my_imconfig.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
#include "main.h"
#include "canvas.h"

namespace CFSUI::Canvas {
    inline float L2Distance(const ImVec2 &a, const ImVec2 &b) {
        return std::hypot(a.x - b.x, a.y - b.y);
    }


    void showCanvas(bool *open) {
        if (ImGui::Begin("Canvas", open)) {
            static ImVec2 scrolling(0.0f, 0.0f);
            static std::vector<Path> paths;
            static bool is_inserting_node = false;
            if (ImGui::Button("New contour")) {
                paths.emplace_back();
                is_inserting_node = true;
            }


            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO &io = ImGui::GetIO();
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(30, 30, 30, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton("canvas", canvas_sz,
                                   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_active = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // mouse status
            const bool is_mouse_left_button_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            const bool is_mouse_left_button_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            const bool is_mouse_left_button_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            const bool is_mouse_right_button_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);

            static ImVec2 mouse_pos_clicked;


            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }


            if (!paths.empty()) {
                // set properties
                static float point_radius = 4.0f;
                static ImU32 point_color = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
                static ImU32 ctrl_color = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
                static ImU32 selected_color = ImGui::GetColorU32(IM_COL32(13, 153, 255, 255));
                static ImU32 hovered_color = ImGui::GetColorU32(IM_COL32(50, 200, 255, 255));
                static ImU32 curve_color = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
                static float curve_thickness = 2.0f;
                static float handle_thickness = 1.0f;
                // TODO: use better name
                static float threshold = 6.0f; // distance threshold for selecting a point


                // hover and select----------------------
                bool is_hovering_point = false;
                size_t nearest_path_idx;
                size_t nearest_node_idx;
                size_t nearest_point_idx;

                static size_t selected_path_idx = 0;
                static size_t selected_node_idx = 0;
                static size_t selected_point_idx = 0;

                // update hovered and selected point-----------------
                float min_dis{std::numeric_limits<float>::max()};

                auto updateMin = [&](const ImVec2& point, size_t path_idx, size_t node_idx, size_t point_idx) {
                    float dis = L2Distance(point, mouse_pos_in_canvas);
                    if (dis < min_dis) {
                        min_dis = dis;
                        nearest_path_idx = path_idx;
                        nearest_node_idx = node_idx;
                        nearest_point_idx = point_idx;
                    }

                };
                // path point
                for (size_t i = 0; i < paths.size(); i++) {
                    const auto &nodes = paths[i].nodes;
                    for (size_t j = 0; j < nodes.size(); j++) {
                        updateMin(nodes[j][0], i, j, 0);
                    }
                }
                // visible control point
                if (selected_node_idx > 0 || (selected_node_idx == 0 && paths[selected_path_idx].is_closed)) {
                    size_t prev_node_idx = selected_node_idx ? selected_node_idx-1 : paths[selected_path_idx].nodes.size() - 1;
                    updateMin(paths[selected_path_idx].nodes[selected_node_idx][1], selected_path_idx, selected_node_idx, 1);
                    updateMin(paths[selected_path_idx].nodes[prev_node_idx][2], selected_path_idx, prev_node_idx, 2);
                }
                if (min_dis < threshold) {
                    is_hovering_point = true;

                    // only path point can be selected
                    if (is_mouse_left_button_clicked && nearest_point_idx == 0) {
                        selected_path_idx = nearest_path_idx;
                        selected_node_idx = nearest_node_idx;
                        selected_point_idx = nearest_point_idx;
                    }
                }
                // end update hovered and selected points
                if (is_hovering_point) {
                    ImGui::SetMouseCursor(7);
                } else {
                    ImGui::SetMouseCursor(0);
                }


                auto& selected_path = paths[selected_path_idx];
                auto& selected_nodes = selected_path.nodes;

                const auto& hovered_path = paths[nearest_path_idx];
                const auto& hovered_nodes = hovered_path.nodes;


                auto insertNodeBack = [&]() {
                    selected_nodes.emplace_back();
                    is_inserting_node = true;
                    selected_node_idx = selected_nodes.size() - 1;
                };

                if (is_mouse_left_button_clicked) {
                    mouse_pos_clicked = mouse_pos_in_canvas;
                    if (is_inserting_node) {
                        if (selected_nodes.size() == 1) {
                            insertNodeBack();
                        } else {

                            is_inserting_node = false;
                        }
                    } else if (is_hovering_point && !hovered_path.is_closed && nearest_node_idx == hovered_nodes.size()-1) {
                        insertNodeBack();
                    }
                }

                auto& selected_node = selected_nodes[selected_node_idx];
                auto& selected_node_prev = selected_node_idx ? selected_nodes[selected_node_idx-1] : selected_nodes.back();
                const auto& hovered_node = hovered_nodes[nearest_node_idx];
                const auto& hovered_node_prev = nearest_node_idx ? hovered_nodes[nearest_node_idx-1] : hovered_nodes.back();

                // update the position of inserting and related points_selected
                if (is_inserting_node) {

                    selected_node[0] = mouse_pos_in_canvas;
                    if (selected_node_idx > 0) {
                        // modify the control point
                        const auto& p0 = selected_node_prev[0];
                        auto& p1 = selected_node_prev[2];
                        auto& p2 = selected_node[1];
                        const auto& p3 = selected_node[0];
                        float dx = (p3.x - p0.x) / 3.0f;
                        float dy = (p3.y - p0.y) / 3.0f;
                        auto d = ImVec2Sub(p3, p0);
                        d.x /= 3.0f;
                        d.y /= 3.0f;
                        p1 = ImVec2Add(p0, d);
                        p2 = ImVec2Sub(p3, d);
                    }

                }

                if (is_mouse_left_button_released) {

                }

                // 1. draw normal curves
                for (const auto& path : paths) {
                    const auto& nodes = path.nodes;
                    for (size_t i = 1; i < nodes.size(); i++) {
                        draw_list->AddBezierCubic(ImVec2Add(origin, nodes[i-1][0]), ImVec2Add(origin, nodes[i-1][2]),
                                                  ImVec2Add(origin, nodes[i][1]), ImVec2Add(origin, nodes[i][0]),
                                                  point_color, curve_thickness);
                    }
                    if (path.is_closed) {
                        draw_list->AddBezierCubic(ImVec2Add(origin, nodes.back()[0]), ImVec2Add(origin, nodes.back()[2]),
                                                  ImVec2Add(origin, nodes.front()[1]), ImVec2Add(origin, nodes.front()[0]),
                                                  point_color, curve_thickness);
                    }
                }
                // 2. draw hovered curves
                if (is_hovering_point && !is_inserting_node
                    && (nearest_node_idx > 0 || (nearest_node_idx == 0 && hovered_path.is_closed))) {
                    draw_list->AddBezierCubic(ImVec2Add(origin, hovered_node_prev[0]), ImVec2Add(origin, hovered_node_prev[2]),
                                              ImVec2Add(origin, hovered_node[1]), ImVec2Add(origin, hovered_node[0]),
                                              hovered_color, curve_thickness);
                }
                if (selected_node_idx > 0 || (selected_node_idx == 0 && selected_path.is_closed)) {
                    // 3. draw selected curves
                    draw_list->AddBezierCubic(ImVec2Add(origin, selected_node_prev[0]), ImVec2Add(origin, selected_node_prev[2]),
                                              ImVec2Add(origin, selected_node[1]), ImVec2Add(origin, selected_node[0]),
                                              selected_color, curve_thickness);
                    // 4. draw control handler
                    draw_list->AddLine(ImVec2Add(origin, selected_node_prev[0]), ImVec2Add(origin, selected_node_prev[2]), ctrl_color, handle_thickness);
                    draw_list->AddLine(ImVec2Add(origin, selected_node[0]), ImVec2Add(origin, selected_node[1]), ctrl_color, handle_thickness);
                }
                // 5. draw path point
                for (const auto& path : paths) {
                    for (const auto& node : path.nodes) {
                        draw_list->AddCircleFilled(ImVec2Add(origin, node[0]), point_radius, point_color);
                    }
                }
                // 6. draw control point
                if (selected_node_idx > 0 || (selected_node_idx == 0 && selected_path.is_closed)) {
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node_prev[2]), point_radius, ctrl_color);
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node[1]), point_radius, ctrl_color);
                }
                // 6. draw hovered point
                if (is_hovering_point && !is_inserting_node) {
                    draw_list->AddCircleFilled(ImVec2Add(origin, hovered_node[nearest_point_idx]), point_radius, hovered_color);
                }
                // 7. draw selected point
                draw_list->AddCircleFilled(ImVec2Add(origin,selected_node[0]), point_radius, selected_color);
            }


            ImGui::End();
        }


    }

}