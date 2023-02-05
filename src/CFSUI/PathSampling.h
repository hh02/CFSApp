#ifndef CFSAPP_PATHSAMPLING_H
#define CFSAPP_PATHSAMPLING_H

#include <imgui.h>
#include <vector>

namespace CFSUI {
    class PathSampling {
    public:
        PathSampling() : imax{60}, step{5.f} {}
        PathSampling(int theImax, float theStep) : imax{theImax}, step{theStep} {}
        void pathSamplingByLength(std::vector<ImVec2>& points);
        void pathSamplingByTime(std::vector<ImVec2>& points);
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
    };
} // CFSUI

#endif //CFSAPP_PATHSAMPLING_H