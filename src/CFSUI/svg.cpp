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
                for (int i = 0; i < path->npts-1; i++) {
                    ph.points.emplace_back(path->pts[i*2], path->pts[i*2+1]);
                }
                std::cout << ph.is_closed << std::endl;
                paths.emplace_back(ph);
            }
        }
        nsvgDelete(image);
    }
    void save_path(const char* filename, const std::vector<Canvas::Path>& paths) {

    }

}