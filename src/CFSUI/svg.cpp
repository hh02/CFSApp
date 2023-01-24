#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include "canvas.h"
#include "tinyxml2.h"
#include <string>

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
        tinyxml2::XMLDocument svg_doc;
        const char* declaration =R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)";
        svg_doc.Parse(declaration);

        auto root = svg_doc.NewElement("svg");
        root->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
        root->SetAttribute("xmlns:svg", "http://www.w3.org/2000/svg");
        svg_doc.InsertEndChild(root);
        for (const auto& path : paths) {
            const auto& points = path.points;
            auto svg_path = svg_doc.NewElement("path");
            // set attribute fill and stroke
            svg_path->SetAttribute("fill", "none");
            svg_path->SetAttribute("stroke", "black");

            // set attribute d
            std::string cmd;
            // insert first curve
            cmd += "M";
            cmd += " " + std::to_string(points[0].x)+","+std::to_string(points[0].y);
            cmd += " C";
            for (size_t i = 1; i < points.size()-2; i++) {
                cmd += " " + std::to_string(points[i].x) + "," + std::to_string(points[i].y);
            }
            if (path.is_closed) {
                cmd += " " + std::to_string(points[points.size()-2].x) + "," + std::to_string(points[points.size()-2].y);
                cmd += " " + std::to_string(points[points.size()-1].x) + "," + std::to_string(points[points.size()-1].y);
                cmd += " " + std::to_string(points[0].x) + "," + std::to_string(points[0].y);
                cmd += " Z";
            }
            svg_path->SetAttribute("d", cmd.c_str());
            root->InsertEndChild(svg_path);
        }
        svg_doc.SaveFile(filename);
    }

}