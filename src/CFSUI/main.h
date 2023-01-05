//
// Created by huihao on 2023/1/5.
//

#ifndef CFSAPP_MAIN_H
#define CFSAPP_MAIN_H

#include <imgui.h>

namespace CFSUI {
    inline ImVec2 ImVec2Add(const ImVec2 &lhs, const ImVec2 &rhs) {
        return {lhs.x+rhs.x, lhs.y+rhs.y};
    }
    inline ImVec2 ImVec2Sub(const ImVec2 &lhs, const ImVec2 &rhs) {
        return {lhs.x - rhs.x, lhs.y-rhs.y};
    }

}
#endif //CFSAPP_MAIN_H
