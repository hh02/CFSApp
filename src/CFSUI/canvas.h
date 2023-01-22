//
// Created by huihao on 2023/1/5.
//

#ifndef CFSAPP_CANVAS_H
#define CFSAPP_CANVAS_H
// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
#include "main.h"
#include "sml.hpp"
#include "tinyfiledialogs.h"
// #include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
namespace sml=boost::sml;

namespace CFSUI::Canvas {
    using Node = std::array<ImVec2, 3>;
    // One node has three points: 1 path point, 2 control points
    struct Path {
        std::vector<ImVec2> points;
        bool is_closed;
        ImVec2 p_min;
        ImVec2 p_max;

        // A path at least has one node
        Path() : points(), is_closed(false), p_min(), p_max() {}
    };

    enum class ObjectType {
        None,
        PathPoint,
        CtrlPoint,
        Path,
        Image,
        BoundTop,
        BoundBottom,
        BoundLeft,
        BoundRight,
        BoundTopLeft,
        BoundTopRight,
        BoundBottomLeft,
        BoundBottomRight,
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


    inline float L2Distance(const ImVec2 &a, const ImVec2 &b);


    void showCanvas(bool *open);
}
#endif //CFSAPP_CANVAS_H
