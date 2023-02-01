// #include "my_imconfig.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include "canvas.h"
#include "svg.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"
#include "CubicSpline.h"


namespace CFSUI::Canvas {
    const float collinear_eps = 0.1f;
    inline float L2Distance(const ImVec2 &a, const ImVec2 &b) {
        return std::hypot(a.x - b.x, a.y - b.y);
    }
    // Simple helper function to load an image into a OpenGL texture with common settings
    bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
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
    ImVec2 translate(0.0f, 0.0f);
    float scaling = 1.0f;


    inline ImVec2 transform(const ImVec2& point) {
        return {point.x*scaling+translate.x, point.y*scaling+translate.y};
    }

    void showCanvas(bool *open) {
        if (ImGui::Begin("Canvas", open)) {
            static std::vector<Path> paths;
            static std::vector<Image> images;
            static History history(10);
            static Clipboard clipboard;
            static CubicSplineTest::ClosestPointSolver solver;
            bool is_clicked_button = ImGui::Button("New path");
            ImGui::SameLine();
            if (ImGui::Button("Open image")) {
                static char const * filterPatterns[2] = { "*.jpg", "*.png" };
                auto filename = tinyfd_openFileDialog(
                        "Open an image",
                        "",
                        2,
                        filterPatterns,
                        "*.jpg, *.png",
                        0);
                if (filename != nullptr) {
                    int image_width = 0;
                    int image_height = 0;
                    GLuint image_texture = 0;
                    bool ret = LoadTextureFromFile(filename, &image_texture, &image_width, &image_height);
                    IM_ASSERT(ret);
                    images.emplace_back(image_texture, image_width, image_height);
                    history.push_back(paths, images);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Open path")) {
                static char const * filterPatterns[1] = {"*.svg"};
                auto filename = tinyfd_openFileDialog(
                        "Open a svg",
                        "",
                        1,
                        filterPatterns,
                        "*.svg",
                        0);
                if (filename != nullptr) {
                    svg::load_path(filename, paths);
                    history.push_back(paths, images);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Save paths")) {
                static char const* filterPatterns[1] = {"*.svg"};
                auto filename = tinyfd_saveFileDialog(
                        "Save paths",
                        "",
                        1,
                        filterPatterns,
                        "svg files"
                        );
                if (filename != nullptr) {
                    svg::save_path(filename, paths);
                }
            }
            ImGui::SameLine();
            static bool preview_mode = false;
            ImGui::Checkbox("Preview mode", &preview_mode);

            static ImVec4 normal_color_vec{0.0f, 0.0f, 0.0f, 1.0f};
            static ImVec4 ctrl_color_vec {0.5f, 0.5f, 0.5f, 1.0f};
            static ImVec4 selected_color_vec {0.0f, 0.6f, 1.0f, 1.0f};
            static ImVec4 hovered_color_vec {0.2f, 0.8f, 1.0f, 1.0f};

            static const ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
            ImGui::Text("Normal color: ");
            ImGui::SameLine();
            ImGui::ColorEdit4("EditNormalColor", (float*)&normal_color_vec, flags);

            ImGui::Text("Control color: ");
            ImGui::SameLine();
            ImGui::ColorEdit4("EditCtrlColor", (float*)&ctrl_color_vec, flags);

            ImGui::Text("Selected color: ");
            ImGui::SameLine();
            ImGui::ColorEdit4("EditSelectedColor", (float*)&selected_color_vec, flags);

            ImGui::Text("Hovered color: ");
            ImGui::SameLine();
            ImGui::ColorEdit4("EditHoveredColor", (float*)&hovered_color_vec, flags);

            static ImVec2 canvas_p0_prev(0.0f, 0.0f);
            const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            const ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            translate.x += (canvas_p0.x - canvas_p0_prev.x);
            translate.y += (canvas_p0.y - canvas_p0_prev.y);
            canvas_p0_prev = canvas_p0;


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


            // mouse status
            const bool is_mouse_left_clicked = is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            const bool is_mouse_left_released = is_hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            const bool is_mouse_right_released = is_hovered &&  ImGui::IsMouseReleased(ImGuiMouseButton_Right);
            const bool is_mouse_right_dragging = is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right);
            const bool is_mouse_moved = is_hovered && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0);
            const bool is_mouse_scrolled = is_hovered && (io.MouseWheel != 0);
            const ImVec2 mouse_pos((io.MousePos.x - translate.x) / scaling, (io.MousePos.y - translate.y) / scaling);

            // keyboard status
            const bool is_keyboard_undo = io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Z]);
            const bool is_keyboard_redo = io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Y]);
            const bool is_keyboard_cut = io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_X]);
            const bool is_keyboard_copy = io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_C]);
            const bool is_keyboard_paste = io.KeyCtrl && ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_V]);

            // set properties
            static float point_radius = 4.0f;
            static const float radius_bigger_than = 2.0f;
            ImU32 ctrl_color = ImGui::GetColorU32(ctrl_color_vec);
            ImU32 normal_color = ImGui::GetColorU32(normal_color_vec);
            ImU32 selected_color = ImGui::GetColorU32(selected_color_vec);
            ImU32 hovered_color = ImGui::GetColorU32(hovered_color_vec);
            static float curve_thickness = 2.0f;
            static float handle_thickness = 2.0f;
            static float bounding_thickness = 2.0f;
            // TODO: use better name
            static const float point_threshold = 8.0f; // distance point_threshold for hovering
            static const float line_threshold = 4.0f;

            // hover and select----------------------
            static ObjectType hovered_type = ObjectType::None;
            static size_t hovered_path_idx = 0;
            static size_t hovered_point_idx = 0;
            static size_t hovered_image_idx = 0;

            static ObjectType selected_type = ObjectType::None;
            static size_t selected_path_idx = 0;
            static size_t selected_point_idx = 0;
            static size_t selected_image_idx = 0;
            static bool has_prev = false;
            static size_t prev_p0_idx = 0;
            static size_t prev_p1_idx = 0;
            static size_t prev_p2_idx = 0;
            static bool has_next = false;
            static size_t next_p3_idx = 0;

            static std::vector<ImVec2*> moving_points_ptr;
            static float moving_distance = 0.f;
            static const float moving_threshold = 0.1f;

            static bool draw_big_start_point = false;

            // state machine

            // state
            static auto Normal_s = sml::state<class Normal>;
            static auto Dragging_s = sml::state<class Dragging>;
            static auto Inserting_s = sml::state<class Inserting>;
            static auto Moving_s = sml::state<class Moving>;

            // event
            struct clicked_button {};
            struct mouse_moved {};
            struct mouse_scrolled {};
            struct mouse_left_clicked {};
            struct mouse_left_released {};
            struct mouse_right_released {};
            struct mouse_right_dragging {};

            struct keyboard_undo {};
            struct keyboard_redo {};
            struct keyboard_cut {};
            struct keyboard_copy {};
            struct keyboard_paste {};

            // guard
            static auto is_last_path_closed = [] {
                return paths.empty() || paths.back().is_closed;
            };
            static auto is_image = [] {
                return hovered_type == ObjectType::Image;
            };
            static auto is_path = [] {
                return hovered_type == ObjectType::Path;
            };
            static auto is_blank = [] {
                return hovered_type == ObjectType::None;
            };
            static auto is_start_point = []() {
                size_t siz = paths[selected_path_idx].points.size();
                return L2Distance(paths[selected_path_idx].points[siz-3], paths[selected_path_idx].points[0]) < point_threshold;
            };
            static auto is_inserting_first = [] {
                return paths[selected_path_idx].points.size() == 3;
            };
            static auto is_moving_moved = [] {
                return moving_distance > moving_threshold;
            };
            static auto is_open_point = [] {
                return hovered_type == ObjectType::PathPoint && (!paths[hovered_path_idx].is_closed)
                       && (hovered_point_idx == paths[hovered_path_idx].points.size() - 3);
            };


            // action
            static auto new_path = []{
                paths.emplace_back();
                hovered_path_idx = paths.size() - 1;
                selected_path_idx = paths.size() - 1;
            };

            static auto update_inserting_points_pos = [&mouse_pos] {
                auto& points = paths[selected_path_idx].points;
                points[selected_point_idx] = mouse_pos;

                draw_big_start_point = (points.size() > 3)
                        && (L2Distance(points[selected_point_idx], points[0]) < point_threshold);

                if (selected_point_idx == 0) {
                    points[1] = mouse_pos;
                    points[2] = mouse_pos;
                    return;
                }

                // modify the control point

                const auto& p0 = points[selected_point_idx-3];
                auto& p1 = points[selected_point_idx-2];
                auto& p2 = points[selected_point_idx-1];
                const auto& p3 = points[selected_point_idx];

                float dx = (p3.x - p0.x) / 3.0f;
                float dy = (p3.y - p0.y) / 3.0f;

                p1.x = p0.x + dx;
                p1.y = p0.y + dy;
                p2.x = p3.x - dx;
                p2.y = p3.y - dy;
            };

            static auto update_selected = [] {

                // select node only when select the path point of the node
                if (hovered_type == ObjectType::PathPoint) {
                    selected_type = hovered_type;
                    selected_path_idx = hovered_path_idx;
                    selected_point_idx = hovered_point_idx;

                    const size_t siz = paths[selected_path_idx].points.size();
                    const bool is_closed = paths[selected_path_idx].is_closed;
                    has_prev = (selected_point_idx > 0) || (selected_point_idx == 0 && is_closed);
                    has_next = (selected_point_idx+3 < siz) || (selected_point_idx+3 == siz && is_closed);
                    if (has_prev) {
                        prev_p0_idx = selected_point_idx ? selected_point_idx - 3 : siz - 3;
                        prev_p1_idx = selected_point_idx ? selected_point_idx - 2 : siz - 2;
                        prev_p2_idx = selected_point_idx ? selected_point_idx - 1 : siz - 1;
                    }
                    if (has_next) {
                        next_p3_idx = selected_point_idx+3 < siz ? selected_point_idx+3 : 0;
                    }
                }
                else if (hovered_type == ObjectType::Path) {
                    selected_type = hovered_type;
                    selected_path_idx = hovered_path_idx;

                    // update path's p_min and p_max
                    ImVec2 p_min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                    ImVec2 p_max(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
                    if (paths[selected_path_idx].is_closed) {
                        paths[selected_path_idx].points.emplace_back(paths[selected_path_idx].points[0]);
                    }
                    const auto& points = paths[selected_path_idx].points;
                    for (size_t i = 0; i+3 < points.size(); i += 3) {
                        const float xa = 3.f * (-points[i].x + 3.f*points[i+1].x - 3.f*points[i+2].x + points[i+3].x);
                        const float xb = 6.f * (points[i].x - 2.f*points[i+1].x + points[i+2].x);
                        const float xc = 3.f * (points[i+1].x - points[i].x);
                        if (std::fabs(xa - 0.f) > 0.001) {
                            float t = (-xb + std::sqrt(xb*xb - 4.f*xa*xc)) / (2.f*xa);
                            if (0.f < t && t < 1.f) {
                                const float val = points[i].x*std::pow(1.f-t, 3.f) + 3.f*points[i+1].x*t*std::pow(1.f-t, 2.f) + 3.f*points[i+2].x*std::pow(t, 2.f)*(1.f-t) + points[i+3].x * std::pow(t, 3.f);
                                p_min.x = std::min(p_min.x, val);
                                p_max.x = std::max(p_max.x, val);
                            }
                            t = (-xb - std::sqrt(xb*xb - 4.f*xa*xc)) / (2.f*xa);
                            if (0.f < t && t < 1.f) {
                                const float val = points[i].x*std::pow(1.f-t, 3.f) + 3.f*points[i+1].x*t*std::pow(1.f-t, 2.f) + 3.f*points[i+2].x*std::pow(t, 2.f)*(1.f-t) + points[i+3].x * std::pow(t, 3.f);
                                p_min.x = std::min(p_min.x, val);
                                p_max.x = std::max(p_max.x, val);
                            }
                        }
                        const float ya = 3.f * (-points[i].y + 3.f*points[i+1].y - 3.f*points[i+2].y + points[i+3].y);
                        const float yb = 6.f * (points[i].y - 2.f*points[i+1].y + points[i+2].y);
                        const float yc = 3.f * (points[i+1].y - points[i].y);
                        if (std::fabs(ya - 0.f) > 0.001) {
                            float t = (-yb + std::sqrt(yb*yb - 4.f*ya*yc)) / (2.f*ya);
                            if (0.f < t && t < 1.f) {
                                const float val = points[i].y*std::pow(1.f-t, 3.f) + 3.f*points[i+1].y*t*std::pow(1.f-t, 2.f) + 3.f*points[i+2].y*std::pow(t, 2.f)*(1.f-t) + points[i+3].y * std::pow(t, 3.f);
                                p_min.y = std::min(p_min.y, val);
                                p_max.y = std::max(p_max.y, val);
                            }
                            t = (-yb - std::sqrt(yb*yb - 4.f*ya*yc)) / (2.f*ya);
                            if (0.f < t && t < 1.f) {
                                const float val = points[i].y*std::pow(1.f-t, 3.f) + 3.f*points[i+1].y*t*std::pow(1.f-t, 2.f) + 3.f*points[i+2].y*std::pow(t, 2.f)*(1.f-t) + points[i+3].y * std::pow(t, 3.f);
                                p_min.y = std::min(p_min.y, val);
                                p_max.y = std::max(p_max.y, val);
                            }
                        }
                        p_min.x = std::min(p_min.x, points[i].x);
                        p_min.y = std::min(p_min.y, points[i].y);
                        p_max.x = std::max(p_max.x, points[i].x);
                        p_max.y = std::max(p_max.y, points[i].y);
                    }
                    // update the last point
                    if (!paths[selected_path_idx].is_closed) {
                        p_min.x = std::min(p_min.x, points[points.size()-3].x);
                        p_min.y = std::min(p_min.y, points[points.size()-3].y);
                        p_max.x = std::max(p_max.x, points[points.size()-3].x);
                        p_max.y = std::max(p_max.y, points[points.size()-3].y);
                    }
                    else {
                        paths[selected_path_idx].points.pop_back();
                    }
                    paths[selected_path_idx].p_min = p_min;
                    paths[selected_path_idx].p_max = p_max;
                }
                else if (hovered_type == ObjectType::Image) {
                    selected_type = hovered_type;
                    selected_image_idx = hovered_image_idx;
                }
                else if (hovered_type == ObjectType::None){
                    selected_type = hovered_type;
                }

            };
            static auto new_node = []{
                for (int i = 0; i < 3; i++) {
                    paths[selected_path_idx].points.emplace_back();
                }
                hovered_type = ObjectType::PathPoint;
                hovered_path_idx = selected_path_idx;
                hovered_point_idx = paths[selected_path_idx].points.size()-3;
                update_selected();
                update_inserting_points_pos();
            };
            static auto close_path = [] {
                for (int i = 0; i < 3; i++) {
                    paths[selected_path_idx].points.pop_back();
                }
                paths[selected_path_idx].is_closed = true;
                draw_big_start_point = false;
                hovered_type = ObjectType::PathPoint;
                hovered_point_idx = 0;
                update_selected();
            };
            static auto update_hovered = [&mouse_pos] {
                float min_dis = std::numeric_limits<float>::max();
                auto updateMin = [&min_dis](float dis, ObjectType type, size_t path_idx, size_t point_idx) {
                    if (dis < min_dis) {
                        min_dis = dis;
                        hovered_type = type;
                        hovered_path_idx = path_idx;
                        hovered_point_idx = point_idx;
                    }
                };
                // get the nearest point
                if (!preview_mode) {
                    // nearest ctrl point
                    if (selected_type == ObjectType::PathPoint) {
                        const auto& points = paths[selected_path_idx].points;
                        if (has_prev) {
                            updateMin(L2Distance(points[prev_p1_idx], mouse_pos), ObjectType::CtrlPoint, selected_path_idx, prev_p1_idx);
                            updateMin(L2Distance(points[prev_p2_idx], mouse_pos), ObjectType::CtrlPoint, selected_path_idx, prev_p2_idx);
                        }
                        if (has_next) {
                            updateMin(L2Distance(points[selected_point_idx+1], mouse_pos), ObjectType::CtrlPoint, selected_path_idx, selected_point_idx + 1);
                            updateMin(L2Distance(points[selected_point_idx+2], mouse_pos), ObjectType::CtrlPoint, selected_path_idx, selected_point_idx + 2);
                        }
                    }
                    // path point
                    for (size_t i = 0; i < paths.size(); i++) {
                        if (selected_type == ObjectType::Path && selected_path_idx == i) continue;
                        const auto& points = paths[i].points;
                        for (size_t j = 0; j < points.size(); j += 3) {
                            updateMin(L2Distance(points[j], mouse_pos), ObjectType::PathPoint, i, j);
                        }
                    }
                    if (min_dis < point_threshold) return;
                }


                // resizing path point
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Path) {
                    const auto& path = paths[selected_path_idx];
                    updateMin(L2Distance(path.p_min, mouse_pos), ObjectType::PathTopLeft, 0, 0);
                    updateMin(L2Distance(path.p_max, mouse_pos), ObjectType::PathBottomRight, 0, 0);
                    updateMin(L2Distance({path.p_max.x, path.p_min.y}, mouse_pos), ObjectType::PathTopRight, 0, 0);
                    updateMin(L2Distance({path.p_min.x, path.p_max.y}, mouse_pos), ObjectType::PathBottomLeft, 0, 0);
                }
                if (min_dis < point_threshold) return;

                // resizing path handle
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Path) {
                    const auto& path = paths[selected_path_idx];
                    if (path.p_min.x < mouse_pos.x && mouse_pos.x < path.p_max.x) {
                        updateMin(std::fabs(mouse_pos.y - path.p_min.y), ObjectType::PathTop, 0, 0);
                        updateMin(std::fabs(mouse_pos.y - path.p_max.y), ObjectType::PathBottom, 0, 0);
                    }
                    if (path.p_min.y < mouse_pos.y && mouse_pos.y < path.p_max.y) {
                        updateMin(std::fabs(mouse_pos.x - path.p_min.x), ObjectType::PathLeft, 0, 0);
                        updateMin(std::fabs(mouse_pos.x - path.p_max.x), ObjectType::PathRight, 0, 0);
                    }
                }
                if (min_dis < line_threshold) return;


                // resizing image point
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Image && !images[selected_image_idx].locked) {
                    const auto& image = images[selected_image_idx];
                    updateMin(L2Distance(image.p_min, mouse_pos), ObjectType::ImageTopLeft, 0, 0);
                    updateMin(L2Distance(image.p_max, mouse_pos), ObjectType::ImageBottomRight, 0, 0);
                    updateMin(L2Distance({image.p_max.x, image.p_min.y}, mouse_pos), ObjectType::ImageTopRight, 0, 0);
                    updateMin(L2Distance({image.p_min.x, image.p_max.y}, mouse_pos), ObjectType::ImageBottomLeft, 0, 0);
                }
                if (min_dis < point_threshold) return;

                // resizing image handle
                min_dis = std::numeric_limits<float>::max();
                if (selected_type == ObjectType::Image && !images[selected_image_idx].locked) {
                    const auto& image = images[selected_image_idx];
                    if (image.p_min.x < mouse_pos.x && mouse_pos.x < image.p_max.x) {
                        updateMin(std::fabs(mouse_pos.y - image.p_min.y), ObjectType::ImageTop, 0, 0);
                        updateMin(std::fabs(mouse_pos.y - image.p_max.y), ObjectType::ImageBottom, 0, 0);
                    }
                    if (image.p_min.y < mouse_pos.y && mouse_pos.y < image.p_max.y) {
                        updateMin(std::fabs(mouse_pos.x - image.p_min.x), ObjectType::ImageLeft, 0, 0);
                        updateMin(std::fabs(mouse_pos.x - image.p_max.x), ObjectType::ImageRight, 0, 0);
                    }
                }
                if (min_dis < line_threshold) return;

                // path
                min_dis = std::numeric_limits<float>::max();

                for (size_t i = 0; i < paths.size(); i++) {
                    std::vector<CubicSplineTest::WorldSpace> points;
                    for (const auto& point : paths[i].points) {
                        points.push_back({point.x, point.y, 0.0f});
                    }
                    if (paths[i].is_closed) {
                        points.push_back(points[0]);
                    }

                    CubicSplineTest::CubicBezierPath bezier_path(&points[0], (int)points.size());
                    auto solution = bezier_path.ClosestPointToPath({mouse_pos.x, mouse_pos.y, 0.0f}, &solver);
                    updateMin(L2Distance({solution.x, solution.y}, mouse_pos), ObjectType::Path, i, 0);

                    // handle collinear
                    for (size_t j = 0; j + 3 < points.size(); j += 3) {
                        const float p0x = points[j].x, p0y = points[j].y;
                        const float p1x = points[j+1].x, p1y = points[j+1].y;
                        const float p2x = points[j+2].x, p2y = points[j+2].y;
                        const float p3x = points[j+3].x, p3y = points[j+3].y;
                        const float px = mouse_pos.x, py = mouse_pos.y;
                        // collinear
                        if (std::fabs((p1x-p0x)*(p3y-p0y) - (p3x-p0x)*(p1y-p0y)) < collinear_eps
                            && std::fabs((p2x-p0x)*(p3y-p0y) - (p3x-p0x)*(p2y-p0y)) < collinear_eps) {
                            float cross = (p3x - p0x) * (px - p0x) + (p3y - p0y) * (py - p0y);
                            if (cross < 0) continue;
                            float d2 = (p3x - p0x) * (p3x - p0x) + (p3y - p0y) * (p3y - p0y);
                            if (cross > d2) continue;
                            float r = cross / d2;
                            updateMin(std::hypot(p0x+(p3x-p0x)*r-px, p0y+(p3y-p0y)*r-py), ObjectType::Path, i, 0);
                        }
                    }
                }
                if (min_dis < line_threshold) return;

                // image
                for (int i = static_cast<int>(images.size()) - 1; i >= 0; i--) {
                    if ((images[i].p_min.x <= mouse_pos.x && mouse_pos.x <= images[i].p_max.x)
                    && (images[i].p_min.y <= mouse_pos.y && mouse_pos.y <= images[i].p_max.y)) {
                        hovered_type = ObjectType::Image;
                        hovered_image_idx = i;
                        return;
                    }
                }
                hovered_type = ObjectType::None;
            };

            // set moving context (moving points, mouse moved distance)
            static auto set_moving_context = [] {
                moving_distance = 0.f;
                // clear vector
                std::vector<ImVec2*>().swap(moving_points_ptr);
                if (hovered_type == ObjectType::None) return;
                // move
                if (hovered_type == ObjectType::CtrlPoint) {
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].points[hovered_point_idx]);
                }
                // select and move
                else if (hovered_type == ObjectType::PathPoint) {
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].points[hovered_point_idx]);
                    if (has_prev) {
                        moving_points_ptr.emplace_back(&paths[hovered_path_idx].points[prev_p2_idx]);
                    }
                    if (has_next) {
                        moving_points_ptr.emplace_back(&paths[hovered_path_idx].points[hovered_point_idx+1]);
                    }
                }
                else if (hovered_type == ObjectType::Image) {
                    if (!images[hovered_image_idx].locked) {
                        moving_points_ptr.emplace_back(&images[hovered_image_idx].p_min);
                        moving_points_ptr.emplace_back(&images[hovered_image_idx].p_max);
                    }
                }
                else if (hovered_type == ObjectType::Path) {
                    for (auto& point : paths[hovered_path_idx].points) {
                        moving_points_ptr.emplace_back(&point);
                    }
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].p_min);
                    moving_points_ptr.emplace_back(&paths[hovered_path_idx].p_max);
                }
            };

            // points and moved distance
            static auto update_moving_context = [&io, &mouse_pos] {
                moving_distance += std::hypot(io.MouseDelta.x, io.MouseDelta.y);

                for (const auto point_ptr : moving_points_ptr) {
                    (*point_ptr).x += io.MouseDelta.x / scaling;
                    (*point_ptr).y += io.MouseDelta.y / scaling;
                }

                if (selected_type == ObjectType::Path) {
                    auto& path = paths[selected_path_idx];
                    if (hovered_type == ObjectType::PathTop) {
                        const auto p_max_y = path.p_max.y;
                        const auto ratio = (p_max_y - mouse_pos.y) / (p_max_y - path.p_min.y);
                        path.p_min.y = mouse_pos.y;
                        for (auto& point : path.points) {
                            point.y = p_max_y - ratio * (p_max_y - point.y);
                        }
                    }
                    else if (hovered_type == ObjectType::PathBottom) {
                        const auto p_min_y = path.p_min.y;
                        const auto ratio = (mouse_pos.y - p_min_y) / (path.p_max.y - p_min_y);
                        path.p_max.y = mouse_pos.y;
                        for (auto& point : path.points) {
                            point.y = p_min_y + ratio * (point.y - p_min_y);
                        }
                    }
                    else if (hovered_type == ObjectType::PathLeft) {
                        const auto p_max_x = path.p_max.x;
                        const auto ratio = (p_max_x - mouse_pos.x) / (path.p_max.x - path.p_min.x);
                        path.p_min.x = mouse_pos.x;
                        for (auto& point : path.points) {
                            point.x = p_max_x - ratio * (p_max_x - point.x);
                        }
                    }
                    else if (hovered_type == ObjectType::PathRight) {
                        const auto p_min_x = path.p_min.x;
                        const auto ratio = (mouse_pos.x - p_min_x) / (path.p_max.x - path.p_min.x);

                        path.p_max.x = mouse_pos.x;
                        for (auto& point : path.points) {
                            point.x = p_min_x + ratio * (point.x - p_min_x);
                        }

                    }
                    else if (hovered_type == ObjectType::PathTopLeft) {
                        const auto p_max_x = path.p_max.x;
                        const auto p_max_y = path.p_max.y;
                        const auto ratio = std::max((p_max_x - mouse_pos.x) / (p_max_x - path.p_min.x),
                                                    (p_max_y - mouse_pos.y) / (p_max_y - path.p_min.y));

                        path.p_min.x = p_max_x - ratio * (p_max_x - path.p_min.x);
                        path.p_min.y = p_max_y - ratio * (p_max_y - path.p_min.y);
                        for (auto& point : path.points) {
                            point.x = p_max_x - ratio * (p_max_x - point.x);
                            point.y = p_max_y - ratio * (p_max_y - point.y);
                        }
                    }
                    else if (hovered_type == ObjectType::PathTopRight) {
                        const auto p_min_x = path.p_min.x;
                        const auto p_max_y = path.p_max.y;
                        const auto ratio = std::max((mouse_pos.x - p_min_x) / (path.p_max.x - p_min_x),
                                                    (p_max_y - mouse_pos.y) / (p_max_y - path.p_min.y));

                        path.p_max.x = p_min_x + ratio * (path.p_max.x - p_min_x);
                        path.p_min.y = p_max_y - ratio * (p_max_y - path.p_min.y);
                        for (auto& point : path.points) {
                            point.x = p_min_x + ratio * (point.x - p_min_x);
                            point.y = p_max_y - ratio * (p_max_y - point.y);
                        }
                    }
                    else if (hovered_type == ObjectType::PathBottomLeft) {
                        const auto p_max_x = path.p_max.x;
                        const auto p_min_y = path.p_min.y;
                        const auto ratio = std::max((p_max_x - mouse_pos.x) / (p_max_x - path.p_min.x),
                                                    (mouse_pos.y - p_min_y) / (path.p_max.y - p_min_y));

                        path.p_min.x = p_max_x - ratio * (p_max_x - path.p_min.x);
                        path.p_max.y = p_min_y + ratio * (path.p_max.y - p_min_y);
                        for (auto& point : path.points) {
                            point.x = p_max_x - ratio * (p_max_x - point.x);
                            point.y = p_min_y + ratio * (point.y - p_min_y);
                        }
                    }
                    else if (hovered_type == ObjectType::PathBottomRight) {
                        const auto p_min_x = path.p_min.x;
                        const auto p_min_y = path.p_min.y;
                        const auto ratio = std::max((mouse_pos.x - p_min_x) / (path.p_max.x - p_min_x),
                                                    (mouse_pos.y - p_min_y) / (path.p_max.y - p_min_y));

                        path.p_max.x = p_min_x + ratio * (path.p_max.x - p_min_x);
                        path.p_max.y = p_min_y + ratio * (path.p_max.y - p_min_y);
                        for (auto& point : path.points) {
                            point.x = p_min_x + ratio * (point.x - p_min_x);
                            point.y = p_min_y + ratio * (point.y - p_min_y);
                        }

                    }

                }
                else if (selected_type == ObjectType::Image) {
                    auto& image = images[selected_image_idx];
                    if (hovered_type == ObjectType::ImageTop) {
                        image.p_min.y = mouse_pos.y;
                    }
                    else if (hovered_type == ObjectType::ImageBottom) {
                        image.p_max.y = mouse_pos.y;
                    }
                    else if (hovered_type == ObjectType::ImageLeft) {
                        image.p_min.x = mouse_pos.x;
                    }
                    else if (hovered_type == ObjectType::ImageRight) {
                        image.p_max.x = mouse_pos.x;
                    }
                    else if (hovered_type == ObjectType::ImageTopLeft) {
                        const auto ratio = std::max((image.p_max.x - mouse_pos.x) / (image.p_max.x - image.p_min.x),
                                                    (image.p_max.y - mouse_pos.y) / (image.p_max.y - image.p_min.y));
                        image.p_min.x = image.p_max.x - ratio * (image.p_max.x - image.p_min.x);
                        image.p_min.y = image.p_max.y - ratio * (image.p_max.y - image.p_min.y);
                    }
                    else if (hovered_type == ObjectType::ImageTopRight) {
                        const auto ratio = std::max((mouse_pos.x - image.p_min.x) / (image.p_max.x - image.p_min.x),
                                                    (image.p_max.y - mouse_pos.y) / (image.p_max.y - image.p_min.y));
                        image.p_max.x = image.p_min.x + ratio * (image.p_max.x - image.p_min.x);
                        image.p_min.y = image.p_max.y - ratio * (image.p_max.y - image.p_min.y);
                    }
                    else if (hovered_type == ObjectType::ImageBottomLeft) {
                        const auto ratio = std::max((image.p_max.x - mouse_pos.x) / (image.p_max.x - image.p_min.x),
                                                    (mouse_pos.y - image.p_min.y) / (image.p_max.y - image.p_min.y));
                        image.p_min.x = image.p_max.x - ratio * (image.p_max.x - image.p_min.x);
                        image.p_max.y = image.p_min.y + ratio * (image.p_max.y - image.p_min.y);
                    }
                    else if (hovered_type == ObjectType::ImageBottomRight) {
                        const auto ratio = std::max((mouse_pos.x - image.p_min.x) / (image.p_max.x - image.p_min.x),
                                                    (mouse_pos.y - image.p_min.y) / (image.p_max.y - image.p_min.y));
                        image.p_max.x = image.p_min.x + ratio * (image.p_max.x - image.p_min.x);
                        image.p_max.y = image.p_min.y + ratio * (image.p_max.y - image.p_min.y);
                    }
                }
            };
            static auto show_path_popup = [] {
                ImGui::OpenPopup("path_popup");
            };
            static auto show_image_popup = [] {
                ImGui::OpenPopup("image_popup");
            };
            static auto show_context_popup = [] {
                ImGui::OpenPopup("context_popup");
            };
            static auto zoom = [&io] {
                const float c = io.MouseWheel > 0 ? 1.05f : 0.95f;
                scaling *= c;
                translate.x = translate.x * c + (1.0f - c) * io.MousePos.x;
                translate.y = translate.y * c + (1.0f - c) * io.MousePos.y;
            };
            static auto update_translate = [&io] {
                translate.x += io.MouseDelta.x;
                translate.y += io.MouseDelta.y;
            };

            static auto undo = [] {
                history.undo(paths, images);
                selected_type = ObjectType::None;
                update_hovered();
            };
            static auto redo = [] {
                history.redo(paths, images);
                selected_type = ObjectType::None;
                update_hovered();
            };
            static auto update_history = [] {
                history.push_back(paths, images);
            };
            static auto cut = [] {
                if (selected_type == ObjectType::None) {
                    return;
                }
                if (selected_type == ObjectType::Path) {
                    clipboard.objectType = selected_type;
                    clipboard.path = paths[selected_path_idx];
                    paths.erase(paths.begin() + (long long)selected_path_idx);
                    selected_type = ObjectType::None;
                    update_hovered();
                    return;
                }
                if (selected_type == ObjectType::Image) {
                    clipboard.objectType = selected_type;
                    clipboard.image = images[selected_image_idx];
                    images.erase(images.begin() + (long long)selected_image_idx);
                    selected_type = ObjectType::None;
                    update_hovered();
                    return;
                }
            };
            static auto copy = [] {
                if (selected_type == ObjectType::None) {
                    return;
                }
                if (selected_type == ObjectType::Path) {
                    clipboard.objectType = selected_type;
                    clipboard.path = paths[selected_path_idx];
                    return;
                }
                if (selected_type == ObjectType::Image) {
                    clipboard.objectType = selected_type;
                    clipboard.image = images[selected_image_idx];
                    return;
                }
            };
            static auto paste = [] {
                if (clipboard.objectType == ObjectType::None) {
                    return;
                }
                if (clipboard.objectType == ObjectType::Path) {
                    paths.emplace_back(clipboard.path);
                    selected_type = clipboard.objectType;
                    selected_path_idx = paths.size() - 1;
                    return;
                }
                if (clipboard.objectType == ObjectType::Image) {
                    images.emplace_back(clipboard.image);
                    selected_type = clipboard.objectType;
                    selected_image_idx = images.size()-1;
                    return;
                }
            };


            class TransitionTable {
            public:
                auto operator()() {
                    using namespace sml;
                    return make_transition_table(
                            * Normal_s + event<mouse_moved> / update_hovered,
                            Normal_s + event<mouse_scrolled> / zoom,
                            Normal_s + event<clicked_button> [is_last_path_closed] / (new_path, new_node) = Inserting_s,
                            Normal_s + event<mouse_left_clicked> / (update_selected, set_moving_context) = Moving_s,
                            Normal_s + event<mouse_right_dragging> = Dragging_s,
                            Normal_s + event<mouse_right_released> [is_path] / (update_selected, show_path_popup),
                            Normal_s + event<mouse_right_released> [is_image] / (update_selected, show_image_popup),
                            Normal_s + event<mouse_right_released> [is_blank] / (update_selected, show_context_popup),
                            Normal_s + event<keyboard_undo> / undo,
                            Normal_s + event<keyboard_redo> /redo,
                            Normal_s + event<keyboard_cut> / cut,
                            Normal_s + event<keyboard_copy> / copy,
                            Normal_s + event<keyboard_paste> / paste,
                            Dragging_s + event<mouse_right_released> = Normal_s,
                            Dragging_s + event<mouse_moved> / update_translate,
                            Inserting_s + event<mouse_left_released> [is_inserting_first] / new_node,
                            Inserting_s + event<mouse_left_released> [!is_inserting_first && !is_start_point] / update_history = Normal_s,
                            Inserting_s + event<mouse_left_clicked> [!is_inserting_first && is_start_point] / (close_path, update_history) = Normal_s,
                            Inserting_s + event<mouse_moved> / update_inserting_points_pos,
                            Moving_s + event<mouse_left_released> [!is_moving_moved && is_open_point] / new_node = Inserting_s,
                            Moving_s + event<mouse_left_released> [!is_moving_moved && !is_open_point] = Normal_s,
                            Moving_s + event<mouse_left_released> [is_moving_moved] / update_history = Normal_s,
                            Moving_s + event<mouse_moved> / update_moving_context
                    );
                }
            };
            static sml::sm<TransitionTable> state_machine;

            if (is_clicked_button) {
                state_machine.process_event(clicked_button{});
            }
            if (is_mouse_moved) {
                state_machine.process_event(mouse_moved{});
            }
            if (is_mouse_scrolled) {
                state_machine.process_event(mouse_scrolled{});
            }
            if (is_mouse_left_clicked) {
                state_machine.process_event(mouse_left_clicked{});
            }
            if (is_mouse_left_released) {
                state_machine.process_event(mouse_left_released{});
            }
            if (is_mouse_right_dragging) {
                state_machine.process_event(mouse_right_dragging{});
            }
            if (is_mouse_right_released) {
                state_machine.process_event(mouse_right_released{});
            }
            if (is_keyboard_undo) {
                state_machine.process_event(keyboard_undo{});
            }
            if (is_keyboard_redo) {
                state_machine.process_event(keyboard_redo{});
            }
            if (is_keyboard_cut) {
                state_machine.process_event(keyboard_cut{});
            }
            if (is_keyboard_copy) {
                state_machine.process_event(keyboard_copy{});
            }
            if (is_keyboard_paste) {
                state_machine.process_event(keyboard_paste{});
            }


            if (hovered_type == ObjectType::PathTop || hovered_type == ObjectType::PathBottom
                || hovered_type == ObjectType::ImageTop || hovered_type == ObjectType::ImageBottom) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            else if (hovered_type == ObjectType::PathLeft || hovered_type == ObjectType::PathRight
                    || hovered_type == ObjectType::ImageLeft || hovered_type == ObjectType::ImageRight) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            else if (hovered_type == ObjectType::PathTopLeft || hovered_type == ObjectType::PathBottomRight
                     || hovered_type == ObjectType::ImageTopLeft || hovered_type == ObjectType::ImageBottomRight) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            }
            else if (hovered_type == ObjectType::PathTopRight || hovered_type == ObjectType::PathBottomLeft
                     || hovered_type == ObjectType::ImageTopRight || hovered_type == ObjectType::ImageBottomLeft) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            }
            else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            }

            // TODO: better name
            if (ImGui::BeginPopup("path_popup")) {
                if (ImGui::Selectable("cut")) {
                    cut();
                }
                if (ImGui::Selectable("copy")) {
                    copy();
                }
                if (ImGui::Selectable("delete")) {
                    paths.erase(paths.begin() + static_cast<long long>(selected_path_idx));
                    selected_type = ObjectType::None;
                    update_hovered();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("image_popup")) {
                if (ImGui::Selectable("lock")) {
                    images[selected_image_idx].locked = true;
                }
                if (ImGui::Selectable("unlock")) {
                    images[selected_image_idx].locked = false;
                }
                if (ImGui::Selectable("up")) {
                    if (hovered_image_idx + 1 < images.size()) {
                        std::swap(images[selected_image_idx], images[selected_image_idx+1]);
                        selected_image_idx++;
                    }

                }
                if (ImGui::Selectable("down")) {
                    if (selected_image_idx > 0) {
                        std::swap(images[selected_image_idx], images[selected_image_idx-1]);
                        selected_image_idx--;
                    }

                }
                if (ImGui::Selectable("cut")) {
                    cut();
                }
                if (ImGui::Selectable("copy")) {
                    copy();
                }
                if (ImGui::Selectable("delete")) {
                    images.erase(images.begin() + static_cast<long long>(selected_image_idx));
                    selected_type = ObjectType::None;
                    update_hovered();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("context_popup")) {
                if (ImGui::Selectable("new path")) {
                    state_machine.process_event(clicked_button{});
                }
                if (ImGui::Selectable("paste here")) {
                    paste();
                    if (clipboard.objectType == ObjectType::Path) {
                        const float dx = mouse_pos.x - clipboard.path.p_min.x;
                        const float dy = mouse_pos.y - clipboard.path.p_min.y;
                        for (auto& point : paths.back().points) {
                            point.x += dx;
                            point.y += dy;
                        }
                    }
                    if (clipboard.objectType == ObjectType::Image) {
                        const float dx = mouse_pos.x - clipboard.image.p_min.x;
                        const float dy = mouse_pos.y - clipboard.image.p_min.y;
                        images.back().p_min.x += dx;
                        images.back().p_min.y += dy;
                        images.back().p_max.x += dx;
                        images.back().p_max.y += dy;
                    }
                }
                ImGui::EndPopup();
            }

//            state_machine.visit_current_states([](auto state) { std::cout << state.c_str() << std::endl; });

            draw_list->PushClipRect(canvas_p0, canvas_p1, true);

            // layer 1 images ----------
            // 1.1 draw images
            for (const auto& image : images) {
                draw_list->AddImage((void*)(intptr_t)image.texture,transform(image.p_min), transform(image.p_max));
            }


            // layer 2 lines and curves----------
            ImVec2 dummy;
            has_prev = has_prev && (selected_type == ObjectType::PathPoint);
            has_next = has_next && (selected_type == ObjectType::PathPoint);
            const auto& prev_p0 = has_prev ? paths[selected_path_idx].points[prev_p0_idx] : dummy;
            const auto& prev_p1 = has_prev ? paths[selected_path_idx].points[prev_p1_idx] : dummy;
            const auto& prev_p2 = has_prev ? paths[selected_path_idx].points[prev_p2_idx] : dummy;
            const auto& next_p1 = has_next ? paths[selected_path_idx].points[selected_point_idx+1] : dummy;
            const auto& next_p2 = has_next ? paths[selected_path_idx].points[selected_point_idx+2] : dummy;
            const auto& next_p3 = has_next ? paths[selected_path_idx].points[next_p3_idx] : dummy;
            const auto& selected_p = selected_type==ObjectType::PathPoint ? paths[selected_path_idx].points[selected_point_idx] : dummy;


            // 2.1 draw normal curves
            // bezier curves
            for (const auto& path : paths) {
                const auto& points = path.points;
                if (points.empty()) continue;
                draw_list->PathLineTo(transform(points[0]));
                const auto siz = points.size();
                for (size_t i = 0; i+3 < siz; i += 3) {
                    draw_list->PathBezierCubicCurveTo(transform(points[i+1]), transform(points[i+2]), transform(points[i+3]));
                }
                if (path.is_closed) {
                    draw_list->PathBezierCubicCurveTo(transform(points[siz-2]), transform(points[siz-1]), transform(points[0]));
                }
                draw_list->PathStroke(normal_color, 0, curve_thickness);
            }

            if (hovered_type == ObjectType::Path){
                const auto& points = paths[hovered_path_idx].points;
                draw_list->PathLineTo(transform(points[0]));
                const auto siz = points.size();
                for (size_t i = 0; i + 3 < siz; i += 3) {
                    draw_list->PathBezierCubicCurveTo(transform(points[i+1]), transform(points[i+2]), transform(points[i+3]));
                }
                if (paths[hovered_path_idx].is_closed) {
                    draw_list->PathBezierCubicCurveTo(transform(points[siz-2]), transform(points[siz-1]), transform(points[0]));
                }
                draw_list->PathStroke(hovered_color, 0, curve_thickness);
            }
            // 2.2 draw selected curves and ctrl handle
            if (selected_type == ObjectType::PathPoint && !preview_mode) {
                // draw curves
                if (has_prev) {
                    draw_list->AddBezierCubic(transform(prev_p0), transform(prev_p1),
                                              transform(prev_p2), transform(selected_p),
                                              selected_color, curve_thickness);
                }
                if (has_next) {
                    draw_list->AddBezierCubic(transform(selected_p), transform(next_p1),
                                              transform(next_p2), transform(next_p3),
                                              selected_color, curve_thickness);
                }
                // draw control handle
                if (has_prev) {
                    draw_list->AddLine(transform(selected_p), transform(prev_p2), ctrl_color, handle_thickness);
                    draw_list->AddLine(transform(prev_p0), transform(prev_p1), ctrl_color, handle_thickness);
                }
                if (has_next) {
                    draw_list->AddLine(transform(selected_p), transform(next_p1), ctrl_color, handle_thickness);
                    draw_list->AddLine(transform(next_p3), transform(next_p2), ctrl_color, handle_thickness);
                }
            }
            // 2.3 selected path's bounding box
            else if (selected_type == ObjectType::Path) {
                draw_list->AddRect(transform(paths[selected_path_idx].p_min),
                                   transform(paths[selected_path_idx].p_max),
                                   selected_color, 0.0f, 0, bounding_thickness);

            }
            // 2.4 draw selected image's bounding box
            else if (selected_type == ObjectType::Image && !images[selected_image_idx].locked) {
                draw_list->AddRect(transform(images[selected_image_idx].p_min),
                                   transform(images[selected_image_idx].p_max),
                                   selected_color, 0.0f, 0, bounding_thickness);

            }
            // 2.5 draw hovered image's bounding box
            if (hovered_type == ObjectType::Image && !images[hovered_image_idx].locked && selected_type != ObjectType::Image) {
                draw_list->AddRect(transform(images[hovered_image_idx].p_min),
                                   transform(images[hovered_image_idx].p_max),
                                   hovered_color, 0.0f, 0, 2.0f);
            }
            // layer 3 draw points----------
            // 3.1 draw normal path point

            if (!preview_mode) {
                for (size_t i = 0; i < paths.size(); i++) {
                    if (selected_type == ObjectType::Path && selected_path_idx == i) continue;
                    const auto& points = paths[i].points;
                    // normal point
                    for (size_t j = 0; j < points.size(); j += 3) {
                        draw_list->AddCircleFilled(transform(points[j]), point_radius, normal_color);
                    }
                    // open point
                    if (!paths[i].is_closed) {
                        draw_list->AddCircle(transform(points[points.size()-3]), point_radius+radius_bigger_than, hovered_color);
                    }
                }
                // draw big start point
                if (draw_big_start_point) {
                    draw_list->AddCircleFilled(transform(paths[selected_path_idx].points[0]), point_radius+radius_bigger_than, normal_color);
                }
                // 3.2 draw selected/hovered point and control point
                if (selected_type == ObjectType::PathPoint) {
                    draw_list->AddCircleFilled(transform(selected_p), point_radius, selected_color);
                    if (has_prev) {
                        draw_list->AddCircleFilled(transform(prev_p2), point_radius, ctrl_color);
                        draw_list->AddCircleFilled(transform(prev_p1), point_radius, ctrl_color);
                    }
                    if (has_next) {
                        draw_list->AddCircleFilled(transform(next_p1), point_radius, ctrl_color);
                        draw_list->AddCircleFilled(transform(next_p2), point_radius, ctrl_color);
                    }
                }
                if (hovered_type == ObjectType::PathPoint || hovered_type == ObjectType::CtrlPoint) {
                    draw_list->AddCircleFilled(transform(paths[hovered_path_idx].points[hovered_point_idx]), point_radius, hovered_color);
                }
            }

            // 3.3 draw selected image's resizing point
            if (selected_type == ObjectType::Image && !images[selected_image_idx].locked) {
                const auto& image = images[selected_image_idx];
                draw_list->AddCircleFilled(transform(image.p_min), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(ImVec2(image.p_min.x, image.p_max.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(ImVec2(image.p_max.x, image.p_min.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(image.p_max), point_radius, selected_color);
            }
            // 3.4 draw selected path's resizing point
            else if (selected_type == ObjectType::Path) {
                const auto& path = paths[selected_path_idx];
                draw_list->AddCircleFilled(transform(path.p_min), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(ImVec2(path.p_min.x, path.p_max.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(ImVec2(path.p_max.x, path.p_min.y)), point_radius, selected_color);
                draw_list->AddCircleFilled(transform(path.p_max), point_radius, selected_color);
            }

            draw_list->PopClipRect();


            ImGui::End();
        }
    }

}