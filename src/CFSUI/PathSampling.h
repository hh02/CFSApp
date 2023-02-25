#ifndef CFSAPP_PATHSAMPLING_H
#define CFSAPP_PATHSAMPLING_H

#include <imgui.h>
#include <vector>
#include "PathEditor.h"

namespace CFSUI {
    class PathSampling {
    public:
        PathSampling() : imax{60}, step{5.f} {}
        PathSampling(int theImax, float theStep) : imax{theImax}, step{theStep} {}
        std::vector<ImVec2> pathSamplingByLength(const PathEditor::Path& path);
        std::vector<ImVec2> pathSamplingByTime(const PathEditor::Path& path);
        void setPoints(const std::vector<ImVec2>& points, size_t i);
        void setStep(float theStep);
        void setImax(int theImax);
    private:
        ImVec2 p0, p1, p2, p3;
        int imax = 60;
        float step;

        [[nodiscard]] ImVec2 getPoint(float t) const;
        [[nodiscard]] float speed(float t) const;
        [[nodiscard]] float getCurveParameter(float s) const;
        [[nodiscard]] float getCurveLength() const;
    };
} // CFSUI

#endif //CFSAPP_PATHSAMPLING_H
