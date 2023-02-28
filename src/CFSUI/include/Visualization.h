//
// Created by huihao on 2023/2/26.
//

#ifndef CFSAPP_VISUALIZATION_H
#define CFSAPP_VISUALIZATION_H
#include <vector>
#include <fstream>
#include <imgui.h>

namespace CFSUI::Visualization {
    void getPointsFromFile(std::vector<ImVec2>& points);
    void animateCFS(const std::vector<ImVec2> &points, float speed=1.0f, bool replay=false);
}

#endif //CFSAPP_VISUALIZATION_H
