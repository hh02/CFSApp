#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include "canvas.h"

namespace CFSUI::svg {
    void load_path(const char* filename, std::vector<Canvas::Path>& paths) {
        auto image = nsvgParseFromFile(filename, "px", 96);
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                Canvas::Path ph;
                ph.is_closed = path->closed;
                const int npts = path->closed ? path->npts-4 : path->npts - 1 ;
                for (int i = 0; i < npts; i += 3) {
                    float* p = &path->pts[i*2];
                    ph.points.emplace_back(p[0], p[1]);
                    ph.points.emplace_back(p[2],p[3]);
                    ph.points.emplace_back(p[4], p[5]);
                }
                paths.emplace_back(ph);
            }
        }
    }

}