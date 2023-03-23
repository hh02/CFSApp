//
// Created by huihao on 2023/2/26.
//

#ifndef CFSAPP_VISUALIZATION_H
#define CFSAPP_VISUALIZATION_H
#include <vector>
#include <fstream>
#define IMGUI_USER_CONFIG "my_imconfig.h"
#include "my_imconfig.h"
#include <imgui.h>

namespace CFSUI::Visualization {
    const int VisualizationType_Default{0};
    const int VisualizationType_Fill{1};
    const int VisualizationType_3D{2};

    extern int visualization_type;

    void getPointsFromFile(std::vector<ImVec2>& points);
    void visualizeCFS(const std::vector<ImVec2>& points, float tool_path_size);
}

#endif //CFSAPP_VISUALIZATION_H
