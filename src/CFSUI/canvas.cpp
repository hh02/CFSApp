#include "my_imconfig.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include "main.h"
#include "canvas.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"


namespace CFSUI::Canvas {
    const float eps = 2.0f;
    inline float L2Distance(const ImVec2 &a, const ImVec2 &b) {
        return std::hypot(a.x - b.x, a.y - b.y);
    }
    // Simple helper function to load an image into a OpenGL texture with common settings
    bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
    {
        // Load from file
        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, nullptr, 4);
        if (image_data == nullptr)
            return false;

        // Create a OpenGL texture identifier
        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        *out_texture = image_texture;
        *out_width = image_width;
        *out_height = image_height;

        return true;
    }
    void showCanvas(bool *open) {
        if (ImGui::Begin("Canvas", open)) {
            static ImVec2 scrolling(0.0f, 0.0f);
            static std::vector<Path> paths;
            static std::vector<Image> images;
            bool is_clicked_button = ImGui::Button("New path");
            ImGui::SameLine();
            if (ImGui::Button("Open image")) {
                static char const * filterPatterns[2] = { "*.jpg", "*.png" };
                auto filename = tinyfd_openFileDialog(
                        "Open an image",
                        "",
                        2,
                        filterPatterns,
                        "image files",
                        0);
                int image_width = 0;
                int image_height = 0;
                GLuint image_texture = 0;
                bool ret = LoadTextureFromFile(filename, &image_texture, &image_width, &image_height);
                IM_ASSERT(ret);
                images.emplace_back(image_texture, image_width, image_height);
            }

            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO &io = ImGui::GetIO();
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(229, 229, 229, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton("canvas", canvas_sz,
                                   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_active = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // mouse status
            const bool is_mouse_left_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && is_hovered;
            const bool is_mouse_left_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && is_hovered;
            const bool is_mouse_moved = (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);

            static ImVec2 mouse_moved_distance;


            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }


            // set properties
            static float point_radius = 4.0f;
            static const float radius_bigger_than = 2.0f;
            static ImU32 ctrl_color = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
            static ImU32 normal_color = ImGui::GetColorU32(IM_COL32(0, 0, 0, 255));
            static ImU32 selected_color = ImGui::GetColorU32(IM_COL32(13, 153, 255, 255));
            static ImU32 hovered_color = ImGui::GetColorU32(IM_COL32(50, 200, 255, 255));
            static float curve_thickness = 2.0f;
            static float handle_thickness = 1.5f;
            // TODO: use better name
            static float threshold = 6.0f; // distance threshold for selecting a point
            static float handle_threshold = 4.0f;

            // hover and select----------------------
            static ObjectType hovered_type = ObjectType::None;
            static size_t hovered_path_idx = 0;
            static size_t hovered_node_idx = 0;
            static size_t hovered_point_idx = 0;
            static Image* hovered_image_ptr = nullptr;

            static ObjectType selected_type = ObjectType::None;
            static size_t selected_path_idx = 0;
            static size_t selected_node_idx = 0;
            static const Image* selected_image_ptr = nullptr;
            static bool has_prev = false;
            static size_t selected_node_prev_idx = 0;
            static bool has_next = false;
            static size_t selected_node_next_idx = 0;

            static std::vector<ImVec2*> moving_points_ptr;

            // state machine

            // state
            static auto Normal_s = sml::state<class Normal>;
            static auto Inserting_s = sml::state<class Inserting>;
            static auto Moving_s = sml::state<class Moving>;

            // event
            struct clicked_button {};
            struct mouse_moved {};
            struct mouse_left_clicked {};
            struct mouse_left_released {};

            // guard
            static auto is_last_path_closed = [] {
                return paths.empty() || paths.back().is_closed;
            };
            static auto is_blank = [] {
                return hovered_type == ObjectType::None;
            };
            static auto is_start_point = []() {
                return L2Distance(paths[selected_path_idx].nodes.back()[0], paths[selected_path_idx].nodes.front()[0]) < threshold;

            };
            static auto is_inserting_first = [] {
                return paths[selected_path_idx].nodes.size() == 1;
            };
            static auto is_dragged = []{
                return std::hypot(mouse_moved_distance.x, mouse_moved_distance.y) > eps;
            };
            static auto is_open_point = [] {
                return hovered_type == ObjectType::PathPoint && (!paths[hovered_path_idx].is_closed)
                       && (hovered_node_idx == paths[hovered_path_idx].nodes.size()-1);
            };


            // action
            static auto new_path = []{
                paths.emplace_back();
                selected_path_idx = paths.size() - 1;
                selected_node_idx = paths.back().nodes.size() - 1;
            };

            static auto update_inserting_node_pos = [&mouse_pos_in_canvas] {
                auto& selected_node = paths[selected_path_idx].nodes[selected_node_idx];
                selected_node[0] = mouse_pos_in_canvas;
                if (selected_node_idx == 0) {
                    selected_node[1] = mouse_pos_in_canvas;
                    selected_node[2] = mouse_pos_in_canvas;
                    return;
                }
                // modify the control point
                auto& selected_node_prev = paths[selected_path_idx].nodes[selected_node_idx-1];

                const auto& p0 = selected_node_prev[0];
                auto& p1 = selected_node_prev[2];
                auto& p2 = selected_node[1];
                const auto& p3 = selected_node[0];

                auto d = ImVec2Sub(p3, p0);
                d.x /= 3.0f;
                d.y /= 3.0f;
                p1 = ImVec2Add(p0, d);
                p2 = ImVec2Sub(p3, d);
            };

            static auto update_selected = [] {
                // select node only when select the path point of the node
                if (hovered_type == ObjectType::PathPoint) {
                    selected_type = hovered_type;
                    selected_path_idx = hovered_path_idx;
                    selected_node_idx = hovered_node_idx;

                    const size_t len = paths[selected_path_idx].nodes.size();
                    const bool is_closed = paths[selected_path_idx].is_closed;
                    has_prev = selected_node_idx > 0 || (selected_node_idx == 0 && is_closed);
                    has_next = selected_node_idx+1 < len || (selected_node_idx + 1 == len && is_closed);
                    if (has_prev) {
                        selected_node_prev_idx = selected_node_idx ? selected_node_idx-1 : len-1;
                    }
                    if (has_next) {
                        selected_node_next_idx = selected_node_idx+1 < len ? selected_node_idx+1 : 0;
                    }
                    return;
                }
                if (hovered_type == ObjectType::Image) {
                    selected_type = hovered_type;
                    selected_image_ptr = hovered_image_ptr;
                    return;
                }
                // selected path
            };
            static auto new_node = []{
                paths[selected_path_idx].nodes.emplace_back();
                hovered_type = ObjectType::PathPoint;
                hovered_path_idx = selected_path_idx;
                hovered_node_idx = paths[selected_path_idx].nodes.size() - 1;
                hovered_point_idx = 0;
                update_selected();
                update_inserting_node_pos();
            };
            static auto close_path = [] {
                auto& selected_nodes = paths[selected_path_idx].nodes;
                selected_nodes.front()[1] = selected_nodes.back()[1];
                paths[selected_path_idx].nodes.pop_back();
                paths[selected_path_idx].is_closed = true;
                selected_node_idx = 0;
            };
            static auto unselect = [] {
                selected_type = ObjectType::None;
            };
            static auto update_hovered = [&mouse_pos_in_canvas] {
                float min_dis = std::numeric_limits<float>::max();

                auto updateMin = [&min_dis](float dis, ObjectType type, size_t path_idx, size_t node_idx, size_t point_idx) {
                    if (dis < min_dis) {
                        min_dis = dis;
                        hovered_type = type;
                        hovered_path_idx = path_idx;
                        hovered_node_idx = node_idx;
                        hovered_point_idx = point_idx;
                    }
                };
                // get the nearest point
                // ctrl point
                if (selected_type == ObjectType::PathPoint) {
                    const auto& selected_nodes = paths[selected_path_idx].nodes;
                    updateMin(L2Distance(selected_nodes[selected_node_idx][1], mouse_pos_in_canvas),ObjectType::CtrlPoint, selected_path_idx, selected_node_idx, 1);
                    updateMin(L2Distance(selected_nodes[selected_node_idx][2], mouse_pos_in_canvas),ObjectType::CtrlPoint, selected_path_idx, selected_node_idx, 2);
                    if (has_prev) {
                        updateMin(L2Distance(selected_nodes[selected_node_prev_idx][2], mouse_pos_in_canvas),ObjectType::CtrlPoint, selected_path_idx, selected_node_prev_idx, 2);
                    }
                    if (has_next) {
                        updateMin(L2Distance(selected_nodes[selected_node_next_idx][1], mouse_pos_in_canvas),ObjectType::CtrlPoint, selected_path_idx, selected_node_next_idx, 1);
                    }

                }
                // path point
                for (size_t i = 0; i < paths.size(); i++) {
                    const auto& nodes = paths[i].nodes;
                    for (size_t j = 0; j < nodes.size(); j++) {
                        updateMin(L2Distance(nodes[j][0], mouse_pos_in_canvas), ObjectType::PathPoint, i, j, 0);
                    }
                }

                if (min_dis < threshold) return;

                // resizing image point
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Image) {
                    updateMin(L2Distance(selected_image_ptr->p_min, mouse_pos_in_canvas), ObjectType::BoundTopLeft, 0, 0, 0);
                    updateMin(L2Distance(selected_image_ptr->p_max, mouse_pos_in_canvas), ObjectType::BoundBottomRight, 0, 0, 0);
                    updateMin(L2Distance(ImVec2(selected_image_ptr->p_max.x, selected_image_ptr->p_min.y), mouse_pos_in_canvas), ObjectType::BoundTopRight, 0, 0, 0);
                    updateMin(L2Distance(ImVec2(selected_image_ptr->p_min.x, selected_image_ptr->p_max.y), mouse_pos_in_canvas), ObjectType::BoundBottomLeft, 0, 0, 0);
                }
                if (min_dis < threshold) return;

                // resizing image handle
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Image) {
                    if (selected_image_ptr->p_min.x <= mouse_pos_in_canvas.x && mouse_pos_in_canvas.x <= selected_image_ptr->p_max.x) {
                        updateMin(fabsf(mouse_pos_in_canvas.y-selected_image_ptr->p_min.y), ObjectType::BoundTop, 0, 0, 0);
                        updateMin(fabsf(mouse_pos_in_canvas.y-selected_image_ptr->p_max.y), ObjectType::BoundBottom, 0, 0, 0);
                    }
                    if (selected_image_ptr->p_min.y <= mouse_pos_in_canvas.y && mouse_pos_in_canvas.y <= selected_image_ptr->p_max.y) {
                        updateMin(fabsf(mouse_pos_in_canvas.x-selected_image_ptr->p_min.x), ObjectType::BoundLeft, 0, 0, 0);
                        updateMin(fabsf(mouse_pos_in_canvas.x-selected_image_ptr->p_max.x), ObjectType::BoundRight, 0, 0, 0);
                    }
                }
                if (min_dis < handle_threshold) return;

                for (auto it = images.rbegin(); it != images.rend(); it++) {
                    if ((mouse_pos_in_canvas.x >= it->p_min.x && mouse_pos_in_canvas.x <= it->p_max.x)
                    && (mouse_pos_in_canvas.y >= it->p_min.y && mouse_pos_in_canvas.y <= it->p_max.y)) {
                        hovered_type = ObjectType::Image;
                        hovered_image_ptr = &(*it);
                        return;
                    }

                }
                hovered_type = ObjectType::None;
            };


            // set moving context (moving points, mouse moved distance)
            static auto set_moving_context = [] {
                // mouse moved distance
                mouse_moved_distance = ImVec2(0.0f, 0.0f);

                // moving points
                // clear vector
                std::vector<ImVec2*>().swap(moving_points_ptr);
                if (hovered_type == ObjectType::PathPoint) {
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].nodes[hovered_node_idx][hovered_point_idx]);
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].nodes[hovered_node_idx][1]);
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].nodes[hovered_node_idx][2]);
                } else if (hovered_type == ObjectType::CtrlPoint) {
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].nodes[hovered_node_idx][hovered_point_idx]);
                } else if (hovered_type == ObjectType::Image) {
                    moving_points_ptr.emplace_back(&hovered_image_ptr->p_min);
                    moving_points_ptr.emplace_back(&hovered_image_ptr->p_max);
                }
                // moving path
            };

            // points and moved distance
            static auto update_moving_context = [&io] {
                for (const auto point_ptr : moving_points_ptr) {
                    (*point_ptr).x += io.MouseDelta.x;
                    (*point_ptr).y += io.MouseDelta.y;
                }
                mouse_moved_distance.x += std::fabsf(io.MouseDelta.x);
                mouse_moved_distance.y += std::fabsf(io.MouseDelta.y);
            };
            static auto draw_big_start_point = [draw_list, &origin] {
                draw_list->AddCircleFilled(ImVec2Add(origin, paths[selected_path_idx].nodes.front()[0]), point_radius+ radius_bigger_than, normal_color);

            };

            class transition_table {
            public:
                auto operator()() {
                    using namespace sml;
                    return make_transition_table(
                            * Normal_s + event<mouse_moved> / update_hovered,
                            Normal_s + event<clicked_button> [is_last_path_closed] / (new_path, new_node) = Inserting_s,
                            Normal_s + event<mouse_left_clicked> [is_blank] / unselect,
                            Normal_s + event<mouse_left_clicked> [!is_blank] / (update_selected, set_moving_context) = Moving_s,
                            Inserting_s + event<mouse_left_clicked> [is_inserting_first] / new_node,
                            Inserting_s + event<mouse_left_clicked> [!is_inserting_first && !is_start_point] = Normal_s,
                            Inserting_s + event<mouse_left_clicked> [!is_inserting_first && is_start_point] / close_path = Normal_s,
                            Inserting_s + event<mouse_moved> / update_inserting_node_pos,
                            Inserting_s + event<mouse_moved> [is_start_point] / draw_big_start_point,
                            Moving_s + event<mouse_left_released> [!is_dragged && is_open_point] / new_node = Inserting_s,
                            Moving_s + event<mouse_left_released> [is_dragged  || !is_open_point] = Normal_s,
                            Moving_s + event<mouse_moved> / update_moving_context
                    );
                }
            };
            static sml::sm<transition_table> state_machine;

            if (is_clicked_button) {
                state_machine.process_event(clicked_button{});
            }
            if (is_mouse_moved) {
                state_machine.process_event(mouse_moved{});
            }
            if (is_mouse_left_clicked) {
                state_machine.process_event(mouse_left_clicked{});
            }
            if (is_mouse_left_released) {
                state_machine.process_event(mouse_left_released{});
            }

            if (hovered_type == ObjectType::PathPoint || hovered_type == ObjectType::CtrlPoint) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            } else if (hovered_type == ObjectType::BoundTop || hovered_type == ObjectType::BoundBottom) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            } else if (hovered_type == ObjectType::BoundLeft || hovered_type == ObjectType::BoundRight) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            } else if (hovered_type == ObjectType::BoundTopLeft || hovered_type == ObjectType::BoundBottomRight) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            } else if (hovered_type == ObjectType::BoundTopRight || hovered_type == ObjectType::BoundBottomLeft) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            } else {
                ImGui::SetMouseCursor(0);
            }

//            state_machine.visit_current_states([](auto state) { std::cout << state.c_str() << std::endl; });

            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            // 1. draw images
            for (const auto& image : images) {
                draw_list->AddImage((void*)(intptr_t)image.texture,
                                    ImVec2Add(origin, image.p_min), ImVec2Add(origin, image.p_max),
//                                    image.p_min, image.p_max,
                                    ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
            }

            Node dummy_node;
            const auto &selected_node = selected_type == ObjectType::PathPoint ? paths[selected_path_idx].nodes[selected_node_idx] : dummy_node;
            const auto &selected_node_prev = has_prev ? paths[selected_path_idx].nodes[selected_node_prev_idx] : dummy_node;
            const auto &selected_node_next = has_next? paths[selected_path_idx].nodes[selected_node_next_idx] : dummy_node;

            // layer one----------
            // 2. draw normal curves
            for (const auto& path : paths) {
                const auto& nodes = path.nodes;
                for (size_t i = 1; i < nodes.size(); i++) {
                    draw_list->AddBezierCubic(ImVec2Add(origin, nodes[i-1][0]), ImVec2Add(origin, nodes[i-1][2]),
                                              ImVec2Add(origin, nodes[i][1]), ImVec2Add(origin, nodes[i][0]),
                                              normal_color, curve_thickness);
                }
                if (path.is_closed) {
                    draw_list->AddBezierCubic(ImVec2Add(origin, nodes.back()[0]), ImVec2Add(origin, nodes.back()[2]),
                                              ImVec2Add(origin, nodes.front()[1]), ImVec2Add(origin, nodes.front()[0]),
                                              normal_color, curve_thickness);
                }
            }
            // 3. draw selected
            // 3.1 selected node
            if (selected_type == ObjectType::PathPoint) {
                // draw curves
                if (has_prev) {
                    draw_list->AddBezierCubic(ImVec2Add(origin, selected_node_prev[0]), ImVec2Add(origin, selected_node_prev[2]),
                                              ImVec2Add(origin, selected_node[1]), ImVec2Add(origin, selected_node[0]),
                                              selected_color, curve_thickness);
                }
                if (has_next) {
                    draw_list->AddBezierCubic(ImVec2Add(origin, selected_node[0]), ImVec2Add(origin, selected_node[2]),
                                              ImVec2Add(origin, selected_node_next[1]), ImVec2Add(origin, selected_node_next[0]),
                                              selected_color, curve_thickness);
                }
                // draw control handle
                if (has_prev) {
                    draw_list->AddLine(ImVec2Add(origin, selected_node[0]), ImVec2Add(origin, selected_node[1]), ctrl_color, handle_thickness);
                    draw_list->AddLine(ImVec2Add(origin, selected_node_prev[0]), ImVec2Add(origin, selected_node_prev[2]), ctrl_color, handle_thickness);
                }
                if (has_next) {
                    draw_list->AddLine(ImVec2Add(origin, selected_node[0]), ImVec2Add(origin, selected_node[2]), ctrl_color, handle_thickness);
                    draw_list->AddLine(ImVec2Add(origin, selected_node_next[0]), ImVec2Add(origin, selected_node_next[1]), ctrl_color, handle_thickness);
                }
            }
            // 3.2 selected image
            else if (selected_type == ObjectType::Image) {
                draw_list->AddRect(ImVec2Add(origin, selected_image_ptr->p_min),
                                   ImVec2Add(origin,selected_image_ptr->p_max),
                                   selected_color, 0.0f, 0, 2.0f);

            }
            // 3.3 selected path
            else if (selected_type == ObjectType::Path) {

            }
            // layer two----------
            // 4. draw normal path point
            for (const auto& path : paths) {
                for (const auto node : path.nodes) {
                    draw_list->AddCircleFilled(ImVec2Add(origin, node[0]), point_radius, normal_color);
                }
            }
            // 5. draw selected
            // 5.1 draw selected point and control point
            if (selected_type == ObjectType::PathPoint) {
                draw_list->AddCircleFilled(ImVec2Add(origin,selected_node[0]), point_radius, selected_color);
                if (has_prev) {
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node[1]), point_radius, ctrl_color);
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node_prev[2]), point_radius, ctrl_color);
                }
                if (has_next) {
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node[2]), point_radius, ctrl_color);
                    draw_list->AddCircleFilled(ImVec2Add(origin, selected_node_next[1]), point_radius, ctrl_color);
                }
            }
            // 5.2 draw selected image's point
            else if (selected_type == ObjectType::Image) {
                draw_list->AddCircleFilled(ImVec2Add(origin, selected_image_ptr->p_min), point_radius, selected_color);
                draw_list->AddCircleFilled(ImVec2Add(origin, ImVec2(selected_image_ptr->p_min.x, selected_image_ptr->p_max.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(ImVec2Add(origin, ImVec2(selected_image_ptr->p_max.x, selected_image_ptr->p_min.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(ImVec2Add(origin, selected_image_ptr->p_max), point_radius, selected_color);
            }
            else if (selected_type == ObjectType::Path) {

            }
            // 6. draw hovered point
            if (hovered_type == ObjectType::PathPoint || hovered_type == ObjectType::CtrlPoint) {
                draw_list->AddCircleFilled(ImVec2Add(origin, paths[hovered_path_idx].nodes[hovered_node_idx][hovered_point_idx]), point_radius, hovered_color);
            }
            draw_list->PopClipRect();


            ImGui::End();
        }


    }

}