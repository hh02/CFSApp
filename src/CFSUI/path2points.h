//
// Created by huihao on 2023/1/26.
//

#ifndef CFSAPP_PATH2POINTS_H
#define CFSAPP_PATH2POINTS_H

#include <imgui.h>
#include <vector>


namespace CFSUI {
    class Path2Points {
    public:
        Path2Points() : imax{60}, step{5.f} {}
        Path2Points(int theImax, float theStep) : imax{theImax}, step{theStep} {}
        void path2Points(std::vector<ImVec2>& points);
        void setPoints(const std::vector<ImVec2>& points, size_t i);
        void setStep(float theStep);
        void setImax(int theImax);
    private:
        ImVec2 p0, p1, p2, p3;
        int imax = 60;
        float step;

        [[nodiscard]] float calc_x(float t) const;
        [[nodiscard]] float calc_y(float t) const;
        [[nodiscard]] float speed(float t) const;
        [[nodiscard]] float getCurveParameter(float s) const;
    };
}
#endif //CFSAPP_PATH2POINTS_H
