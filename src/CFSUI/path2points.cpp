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
            points.emplace_back(getPoint(t));
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

    inline ImVec2 Path2Points::getPoint(float t) const {
        const float s = 1.f - t;
        return {
                p0.x * s * s * s + 3.f*p1.x * s * s * t + 3.f*p2.x * s * t * t + p3.x * t * t * t,
                p0.y * s * s * s + 3.f*p1.y * s * s * t + 3.f*p2.y * s * t * t + p3.y * t * t * t
        };

    }

    inline float Path2Points::speed(float t) const {
        auto dx = 3.f * (-p0.x + 3.f*p1.x - 3.f*p2.x + p3.x) * t*t + 6.f*(p0.x - 2.f*p1.x + p2.x)*t + 3.f*(p1.x-p0.x);
        auto dy = 3.f * (-p0.y + 3.f*p1.y - 3.f*p2.y + p3.y) * t*t + 6.f*(p0.y - 2.f*p1.y + p2.y)*t + 3.f*(p1.y-p0.y);
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
