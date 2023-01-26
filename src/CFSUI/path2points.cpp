#include "path2points.h"
#include <imgui.h>
#include <vector>
#include <cmath>

namespace CFSUI {
    void Path2Points::path2Points(std::vector<ImVec2>& points) {
        points.emplace_back(p0);
        auto s = step;
        while (true) {
            auto t = getCurveParameter(s);
            if (t > 1.f || t < 0.f) break;
            points.emplace_back(calc_x(t), calc_y(t));
            s += step;
        }
        points.emplace_back(p3);

    }
    void Path2Points::setPoints(const std::vector<ImVec2>& points, size_t i) {
        if (i + 3 >= points.size()) return;
        p0 = points[i];
        p1 = points[i+1];
        p2 = points[i+2];
        p3 = points[i+3];
    }
    inline float Path2Points::calc_x(float t) const {
        return p0.x*std::pow(1.f-t, 3.f) + 3.f*p1.x*t*std::pow(1.f-t, 2.f) + 3.f*p2.x*std::pow(t, 2.f)*(1.f-t) + p3.x * std::pow(t, 3.f);
    }

    inline float Path2Points::calc_y(float t) const {
        return p0.y*std::pow(1.f-t, 3.f) + 3.f*p1.y*t*std::pow(1.f-t, 2.f) + 3.f*p2.y*std::pow(t, 2.f)*(1.f-t) + p3.y * std::pow(t, 3.f);
    }

    inline float Path2Points::speed(float t) const {
        auto dx = (-p0.x + 3.f*p1.x - 3.f*p2.x + p3.x) * t*t + 6.f*(p0.x - 2.f*p1.x + p2.x)*t + 3.f*(p1.x-p0.x);
        auto dy = (-p0.y + 3.f*p1.y - 3.f*p2.y + p3.y) * t*t + 6.f*(p0.y - 2.f*p1.y + p2.y)*t + 3.f*(p1.y-p0.y);
        return std::hypot(dx, dy);
    }

    inline float Path2Points::getCurveParameter(float s) const {
        float t = 0.f, h = s / static_cast<float>(imax);
        for (int i = 0; i < imax; i++) {
            const float k1 = h / speed(t);
            const float k2 = h / speed(t + k1/2.f);
            const float k3 = h / speed(t + k2/2.f);
            const float k4 = h / speed(t + k3);
            t += (k1 + 2.f * (k2 + k3) + k4) / 6.f;
        }
        return t;
    }

    void Path2Points::setStep(float theStep) {
        step = theStep;
    }

    void Path2Points::setImax(int theImax) {
        imax = theImax;
    }
}