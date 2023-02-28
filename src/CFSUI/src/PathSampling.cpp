#include "PathSampling.h"
#include <cmath>

namespace CFSUI {
    std::vector<ImVec2> PathSampling::pathSamplingByLength(const PathEditor::Path& path) {
        std::vector<ImVec2> points;
        auto samplingCurve = [&]() {
            points.emplace_back(p0);
            auto s = step;
            while (true) {
                auto t = getCurveParameter(s);
                if (t > 1.f || t < 0.f) break;
                points.emplace_back(getPoint(t));
                s += step;
            }
        };
        for (size_t i = 0; i + 3 < path.points.size(); i += 3) {
            p0 = path.points[i];
            p1 = path.points[i+1];
            p2 = path.points[i+2];
            p3 = path.points[i+3];
            samplingCurve();

        }
        if (path.is_closed) {
            p0 = path.points[path.points.size()-3];
            p1 = path.points[path.points.size()-2];
            p2 = path.points[path.points.size()-1];
            p3 = path.points[0];
            samplingCurve();
        }
        return points;
    }

    float PathSampling::getCurveLength() const {
        // calculate curve length
        float l = 0.f;
        float r = 1.f;
        while (true) {
            auto t = getCurveParameter(r);
            if (t < 0.f || t > 1.f) break;
            r *= 2.f;
        }
        while (true) {
            float mid = (l + r) / 2.f;
            auto t = getCurveParameter(mid);
            if (t < 0.f || t > 1.f) {
                r = mid;
            } else {
                l = mid;
            }
            if (t < 1.f && (1.f - t) < 0.0001f) {
                break;
            }
        }
        return l;
    }

    std::vector<ImVec2> PathSampling::pathSamplingByTime(const PathEditor::Path& path) {
        std::vector<ImVec2> points;
        auto samplingCurve = [&]() {
            auto l = getCurveLength();
            // calculate interval by curve length
            auto interval = 1.f / (std::ceil(l / step) * 1.2f + 1.f);
            auto t = 0.f;
            while (t < 1.f) {
                points.emplace_back(getPoint(t));
                t += interval;
            }
        };
        for (size_t i = 0; i + 3 < path.points.size(); i += 3) {
            p0 = path.points[i];
            p1 = path.points[i+1];
            p2 = path.points[i+2];
            p3 = path.points[i+3];
            samplingCurve();
        }
        if (path.is_closed) {
            p0 = path.points[path.points.size() - 3];
            p1 = path.points[path.points.size() - 2];
            p2 = path.points[path.points.size() - 1];
            p3 = path.points[0];
            samplingCurve();
        }
        return points;
    }


    void PathSampling::setPoints(const std::vector<ImVec2>& points, size_t i) {
        if (i + 3 >= points.size()) return;
        p0 = points[i];
        p1 = points[i+1];
        p2 = points[i+2];
        p3 = points[i+3];
    }

    inline ImVec2 PathSampling::getPoint(float t) const {
        const float s = 1.f - t;
        return {
                p0.x * s * s * s + 3.f*p1.x * s * s * t + 3.f*p2.x * s * t * t + p3.x * t * t * t,
                p0.y * s * s * s + 3.f*p1.y * s * s * t + 3.f*p2.y * s * t * t + p3.y * t * t * t
        };

    }

    inline float PathSampling::speed(float t) const {
        auto dx = 3.f * (-p0.x + 3.f*p1.x - 3.f*p2.x + p3.x) * t*t + 6.f*(p0.x - 2.f*p1.x + p2.x)*t + 3.f*(p1.x-p0.x);
        auto dy = 3.f * (-p0.y + 3.f*p1.y - 3.f*p2.y + p3.y) * t*t + 6.f*(p0.y - 2.f*p1.y + p2.y)*t + 3.f*(p1.y-p0.y);
        return std::hypot(dx, dy);
    }

    inline float PathSampling::getCurveParameter(float s) const {
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

    void PathSampling::setStep(float theStep) {
        step = theStep;
    }

    void PathSampling::setImax(int theImax) {
        imax = theImax;
    }

} // CFSUI