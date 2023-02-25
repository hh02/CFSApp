//
// Created by huihao on 2023/1/24.
//

#ifndef CFSAPP_SVG_H
#define CFSAPP_SVG_H
#include "PathEditor.h"
namespace CFSUI::svg {

    void load_path(const char* filename, std::vector<PathEditor::Path>& paths);
    void save_path(const char* filename, const std::vector<PathEditor::Path>& paths);

}
#endif //CFSAPP_SVG_H
