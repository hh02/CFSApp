//
// Created by huihao on 2023/1/5.
//

#ifndef CFSAPP_PATHEDITOR_H
#define CFSAPP_PATHEDITOR_H
// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#define IMGUI_USER_CONFIG "my_imconfig.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include "sml.hpp"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
namespace sml=boost::sml;

namespace CFSUI::PathEditor {
    enum class ObjectType {
        None,
        PathPoint,
        CtrlPoint,
        Path,
        PathTop,
        PathBottom,
        PathLeft,
        PathRight,
        PathTopLeft,
        PathTopRight,
        PathBottomLeft,
        PathBottomRight,
        Image,
        ImageTop,
        ImageBottom,
        ImageLeft,
        ImageRight,
        ImageTopLeft,
        ImageTopRight,
        ImageBottomLeft,
        ImageBottomRight,
    };

    struct Path {
        std::vector<ImVec2> points;
        bool is_closed;
        ImVec2 p_min;
        ImVec2 p_max;

        // A path at least has one node
        Path() : points(), is_closed(false), p_min(), p_max() {}
    };

    struct Image {
        GLuint texture;
        ImVec2 p_min;
        ImVec2 p_max;
        bool locked;
        Image(GLuint image_texture, float image_width, float image_height) :texture(image_texture),
                                                                            p_min(10.0f, 10.0f),
                                                                            p_max(10.0f+image_width, 10.0f+image_height),
                                                                            locked(false) {}
    };

    class History {
    public:
        explicit History(int theCapacity = 10) : capacity(theCapacity), head(0), tail(0), curr(0), paths_states(theCapacity), images_states(theCapacity){}
        void push_back(const std::vector<Path>& paths, const std::vector<Image>& images) {
            curr = (curr + 1) % capacity;
            tail = curr;
            if (tail == head) {
                head = (head + 1) % capacity;
            }
            paths_states[curr] = paths;
            images_states[curr] = images;
        }

        void undo(std::vector<Path>& paths, std::vector<Image>& images) {
            if (curr == head) {
                return;
            }
            curr = (curr - 1 + capacity) % capacity;
            paths = paths_states[curr];
            images = images_states[curr];
        }

        void redo(std::vector<Path>& paths, std::vector<Image>& images) {
            if (curr == tail) {
                return;
            }
            curr = (curr + 1) % capacity;
            paths = paths_states[curr];
            images = images_states[curr];
        }
    private:
        int capacity;
        int head;
        int tail;
        int curr;
        std::vector<std::vector<Path>> paths_states;
        std::vector<std::vector<Image>> images_states;
    };

    struct Clipboard {
        Clipboard() : objectType(ObjectType::None) {}
        ObjectType objectType;
        union {
            Path path;
            Image image;
        };
        ~Clipboard() {}
    };



    inline float L2Distance(const ImVec2 &a, const ImVec2 &b);


    void showPathEditor(bool *open);
}
#endif //CFSAPP_PATHEDITOR_H
