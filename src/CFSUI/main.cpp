// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "my_imconfig.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

inline float L1Distance(const ImVec2 &a, const ImVec2 &b) {
    return fabs(a.x-b.x) + fabs(a.y-b.y);
}

inline float L2Distance(const ImVec2 &a, const ImVec2 &b) {
    return std::hypot(a.x-b.x, a.y-b.y);
}


struct Node {
    ImVec2 pos;
    ImVec2 prevCtrlPos;
    ImVec2 nextCtrlPos;
    Node *prev;
    Node *next;
    Node() : pos(), prevCtrlPos(), nextCtrlPos(), prev(nullptr), next(nullptr) {}
    explicit Node(ImVec2 thePos) : pos(thePos), prevCtrlPos(), nextCtrlPos(), prev(nullptr), next(nullptr) {}
    Node(Node *thePrev, Node *theNext) : pos(), prevCtrlPos(), nextCtrlPos(), prev(thePrev), next(theNext) {}
};

class Contour {
public:
    explicit Contour() {
        size = 1;
        head = tail = new Node();
        head->prev = head->next = tail;
        color_selected = ImGui::GetColorU32(IM_COL32(13, 153, 255, 255));
        color_hovered = ImGui::GetColorU32(IM_COL32(50, 200, 255, 255));
        color_path_node = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
        color_ctrl_node = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
    }
    ~Contour() {
        Node *curr = head;
        do {
            Node *next = curr->next;
            delete curr;
            curr = next;
        } while (curr != tail);
    }
    Node *insert_after(Node *node) {
        size++;
        Node *node_next = node->next;
        node->next = new Node(node, node_next);
        node_next->prev = node->next;
        return node->next;
    }
    void delete_node(Node *node) {
        if (size <= 1 || node == nullptr) {
            return;
        }
        size--;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
    }
    Node &front() {
        return *head;
    }

    void getNearestNode(const ImVec2 &pos, const Node *&nearest_node_ptr, int &nearest_node_type, float &min_dis) {
        nearest_node_ptr = nullptr;
        nearest_node_type = 0; // 0 for path node, 1 for previous control node, 2 for next control node
        min_dis = std::numeric_limits<float>::max();
        Node *curr = head;
        auto updateMin = [&](const Node *now, const ImVec2 &node_pos, int node_type) {
            float dis = L2Distance(node_pos, pos);
            if (dis < min_dis) {
                min_dis = dis;
                nearest_node_ptr = now;
                nearest_node_type = node_type;
            }
        };
        do {
            updateMin(curr, curr->pos, 0);
            updateMin(curr, curr->prevCtrlPos, 1);
            updateMin(curr, curr->nextCtrlPos, 2);
            curr = curr->next;
        } while (curr != tail);
    }
    void drawNodes(ImDrawList *draw_list, ImVec2 origin, float radius) {
        Node *curr = head;
        do {
            draw_list->AddCircleFilled(ImVec2(origin.x+curr->pos.x, origin.y+curr->pos.y), radius, color_path_node);
            draw_list->AddCircleFilled(ImVec2(origin.x+curr->prevCtrlPos.x, origin.y+curr->prevCtrlPos.y), radius, color_ctrl_node);
            draw_list->AddCircleFilled(ImVec2(origin.x+curr->nextCtrlPos.x, origin.y+curr->nextCtrlPos.y), radius, color_ctrl_node);
        } while (curr != tail);
    }
private:
    size_t size;
    Node *head;
    Node *tail;
    ImU32 color_selected;
    ImU32 color_hovered;
    ImU32 color_path_node;
    ImU32 color_ctrl_node;
};


//-----------------------------------------------------------------------------
// [SECTION] Example App: Custom Rendering using ImDrawList API / ShowExampleAppCustomRendering()
//-----------------------------------------------------------------------------

// Demonstrate using the low-level ImDrawList to draw custom shapes.
static void ShowExampleAppCustomRendering(bool* p_open)
{
    if (!ImGui::Begin("Example: Custom rendering", p_open))
    {
        ImGui::End();
        return;
    }

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of
    // overloaded operators, etc. Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your
    // types and ImVec2/ImVec4. Dear ImGui defines overloaded operators but they are internal to imgui.cpp and not
    // exposed outside (to avoid messing with your types) In this example we are not using the maths operators!

    if (ImGui::BeginTabBar("##TabBar"))
    {
        if (ImGui::BeginTabItem("Canvas"))
        {
            int test__test_underline;
            test__test_underline = 10;
            static std::vector<Contour> contours;
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;

            ImGui::Checkbox("Enable grid", &opt_enable_grid);
            ImGui::Checkbox("Enable context menu", &opt_enable_context_menu);
            ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

            // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
            // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
            // To use a child window instead we could use, e.g:
            //      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
            //      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
            //      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
            //      ImGui::PopStyleColor();
            //      ImGui::PopStyleVar();
            //      [...]
            //      ImGui::EndChild();

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_active = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
            Eigen::Vector2d test(1, 2);


            // draw new contour
            if (true) {
                contours.emplace_back();
                static bool is_moving = true;
                Contour &contour = contours.back();
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    is_moving = false;
                }
                Node &current_node = contour.front();
                if (is_moving) {
                    current_node.pos = mouse_pos_in_canvas;
                }
            }


            // Insert New Node
//            if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
//                if (!adding_line) {
//                    points.push_back(mouse_pos_in_canvas);
//                    points.push_back(mouse_pos_in_canvas);
//                    adding_line = true;
//                } else {
//                    if (points.Size > 2 && L1Distance(points.front(), mouse_pos_in_canvas) < 10) {
//                        points.back() = points.front();
//                        adding_line = false;
//                    } else {
//                        points.push_back(mouse_pos_in_canvas);
//                    }
//                }
//            }
//            if (adding_line)
//            {
//                points.back() = mouse_pos_in_canvas;
//                //if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
//                //    adding_line = false;
//            }


            // Pan (we use a zero mouse threshold when there's no context menu)
            // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
            const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Context menu (under default mouse threshold)
//            ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
//            if (opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
//                ImGui::OpenPopupOnItemClick("context");
//            if (ImGui::BeginPopup("context"))
//            {
//                if (adding_line)
//                    points.resize(points.size() - 2);
//                adding_line = false;
//                if (ImGui::MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
//                if (ImGui::MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); }
//                ImGui::EndPopup();
//            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
            }

            static Contour &contour = contours.back();
            // get nearest nodes
            const Node *nearest_node_ptr = nullptr;
            // 0 node, 1 previous control node, 2 next control node
            int nearest_node_type = 0;
            float min_dis = std::numeric_limits<float>::max();
            contour.getNearestNode(mouse_pos_in_canvas, nearest_node_ptr, nearest_node_type, min_dis);








            bool hovered = (min_dis < 5);
            static bool selected = false;
            static const ImVec2 *selected_node_ptr = nullptr;
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                selected = true;
                selected_node_ptr = nearest_node_ptr;
            }
            if (hovered) {
                ImGui::SetMouseCursor(7);
                draw_list->AddCircleFilled(ImVec2(origin.x+nearest_node_ptr->x, origin.y+nearest_node_ptr->y), 4, color_hovered);
            } else {
                ImGui::SetMouseCursor(0);
            }
            if (selected) {
                draw_list->AddCircleFilled(ImVec2(origin.x+selected_node_ptr-))


            }
            if (hovered) {
                if (selected) {
                    ImGui::OpenPopup("draw_option_popup");

                } else {
                }
            } else {
                ImGui::SetMouseCursor(0);
            }

            if (ImGui::BeginPopup("draw_option_popup")) {

                if (ImGui::Selectable("Insert After")) {

                }
                if (ImGui::Selectable("Delete")) {

                }
                ImGui::EndPopup();
            }



            if (min_dis < 5) {
                hovered = true;

                draw_list->AddCircleFilled(ImVec2(origin.x+nearest_node_ptr->x, origin.y+nearest_node_ptr->y), 4, color_hovered);
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    selected = true;
                }
                if () {
                    ImGui::OpenPopup("draw_option_popup");
                }
            } else {
                ImGui::SetMouseCursor(0);
            }


//            for (int n = 0; n < nodes.Size-1; n++)
//                draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
            draw_list->PopClipRect();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Primitives"))
        {
            ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Draw gradients
            // (note that those are currently exacerbating our sRGB/Linear issues)
            // Calling ImGui::GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the IM_COL32() directly as well..
            ImGui::Text("Gradients");
            ImVec2 gradient_size = ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());
            {
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = ImGui::GetColorU32(IM_COL32(0, 0, 0, 255));
                ImU32 col_b = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                ImGui::InvisibleButton("##gradient1", gradient_size);
            }
            {
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
                ImU32 col_b = ImGui::GetColorU32(IM_COL32(255, 0, 0, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                ImGui::InvisibleButton("##gradient2", gradient_size);
            }

            // Draw a bunch of primitives
            ImGui::Text("All primitives");
            static float sz = 36.0f;
            static float thickness = 3.0f;
            static int ngon_sides = 6;
            static bool circle_segments_override = false;
            static int circle_segments_override_v = 12;
            static bool curve_segments_override = false;
            static int curve_segments_override_v = 8;
            static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            ImGui::DragFloat("Size", &sz, 0.2f, 2.0f, 100.0f, "%.0f");
            ImGui::DragFloat("Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
            ImGui::SliderInt("N-gon sides", &ngon_sides, 3, 12);
            ImGui::Checkbox("##circlesegmentoverride", &circle_segments_override);
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            circle_segments_override |= ImGui::SliderInt("Circle segments override", &circle_segments_override_v, 3, 40);
            ImGui::Checkbox("##curvessegmentoverride", &curve_segments_override);
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            curve_segments_override |= ImGui::SliderInt("Curves segments override", &curve_segments_override_v, 3, 40);
            ImGui::ColorEdit4("Color", &colf.x);

            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImU32 col = ImColor(colf);
            const float spacing = 10.0f;
            const ImDrawFlags corners_tl_br = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight;
            const float rounding = sz / 5.0f;
            const int circle_segments = circle_segments_override ? circle_segments_override_v : 0;
            const int curve_segments = curve_segments_override ? curve_segments_override_v : 0;
            float x = p.x + 4.0f;
            float y = p.y + 4.0f;
            for (int n = 0; n < 2; n++)
            {
                // First line uses a thickness of 1.0f, second line uses the configurable thickness
                float th = (n == 0) ? 1.0f : thickness;
                draw_list->AddNgon(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, ngon_sides, th);                 x += sz + spacing;  // N-gon
                draw_list->AddCircle(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments, th);          x += sz + spacing;  // Circle
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 0.0f, ImDrawFlags_None, th);          x += sz + spacing;  // Square
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, ImDrawFlags_None, th);      x += sz + spacing;  // Square with all rounded corners
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, corners_tl_br, th);         x += sz + spacing;  // Square with two rounded corners
                draw_list->AddTriangle(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col, th);x += sz + spacing;  // Triangle
                //draw_list->AddTriangle(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col, th);x+= sz*0.4f + spacing; // Thin triangle
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y), col, th);                                       x += sz + spacing;  // Horizontal line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col, th);                                       x += spacing;       // Vertical line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y + sz), col, th);                                  x += sz + spacing;  // Diagonal line

                // Quadratic Bezier Curve (3 control points)
                ImVec2 cp3[3] = { ImVec2(x, y + sz * 0.6f), ImVec2(x + sz * 0.5f, y - sz * 0.4f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierQuadratic(cp3[0], cp3[1], cp3[2], col, th, curve_segments); x += sz + spacing;

                // Cubic Bezier Curve (4 control points)
                ImVec2 cp4[4] = { ImVec2(x, y), ImVec2(x + sz * 1.3f, y + sz * 0.3f), ImVec2(x + sz - sz * 1.3f, y + sz - sz * 0.3f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierCubic(cp4[0], cp4[1], cp4[2], cp4[3], col, th, curve_segments);

                x = p.x + 4;
                y += sz + spacing;
            }
            draw_list->AddNgonFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz*0.5f, col, ngon_sides);               x += sz + spacing;  // N-gon
            draw_list->AddCircleFilled(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments);            x += sz + spacing;  // Circle
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col);                                    x += sz + spacing;  // Square
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f);                             x += sz + spacing;  // Square with all rounded corners
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f, corners_tl_br);              x += sz + spacing;  // Square with two rounded corners
            draw_list->AddTriangleFilled(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col);  x += sz + spacing;  // Triangle
            //draw_list->AddTriangleFilled(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col); x += sz*0.4f + spacing; // Thin triangle
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + thickness), col);                             x += sz + spacing;  // Horizontal line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + thickness, y + sz), col);                             x += spacing * 2.0f;// Vertical line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), col);                                      x += sz;            // Pixel (faster than AddLine)
            draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + sz, y + sz), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));

            ImGui::Dummy(ImVec2((sz + spacing) * 10.2f, (sz + spacing) * 3.0f));
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BG/FG draw lists"))
        {
            static bool draw_bg = true;
            static bool draw_fg = true;
            ImGui::Checkbox("Draw in Background draw list", &draw_bg);
            ImGui::SameLine(); HelpMarker("The Background draw list will be rendered below every Dear ImGui windows.");
            ImGui::Checkbox("Draw in Foreground draw list", &draw_fg);
            ImGui::SameLine(); HelpMarker("The Foreground draw list will be rendered over every Dear ImGui windows.");
            ImVec2 window_pos = ImGui::GetWindowPos();
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
            if (draw_bg)
                ImGui::GetBackgroundDrawList()->AddCircle(window_center, window_size.x * 0.6f, IM_COL32(255, 0, 0, 200), 0, 10 + 4);
            if (draw_fg)
                ImGui::GetForegroundDrawList()->AddCircle(window_center, window_size.y * 0.6f, IM_COL32(0, 255, 0, 200), 0, 10);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}


int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool open = true;
        ShowExampleAppCustomRendering(&open);
        static bool demo_open = true;
        ImGui::ShowDemoWindow(&demo_open);


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}