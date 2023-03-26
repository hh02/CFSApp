//
// Created by huihao on 2023/3/17.
//

#ifndef CFSAPP_IMGUI_FONTS_TEXT_ICON_H
#define CFSAPP_IMGUI_FONTS_TEXT_ICON_H

#include <imgui.h>


// icon ranges

static const char *text_font_path = "fonts/SourceHanSansCN-Regular.otf";
static const char *icon_font_path = "fonts/icomoon.ttf";


static ImVector<ImWchar> text_range;

static ImFontConfig icon_config;
static const ImWchar icon_range[] = { 0xE900, 0xE901, 0xE902, 0xE903, 0xE904, 0xE905, 0xE906, 0xE907, 0xE908, 0xE909, 0xE90A, 0xE90B, 0xE90C, 0xE90D};

// set icon_config, and build text range
inline void init_fonts() {
    icon_config.MergeMode = true;

    ImFontGlyphRangesBuilder builder;
    ImGuiIO &io = ImGui::GetIO();

    builder.AddText(u8"文件编辑帮助撤销重做剪切复制粘贴删除关于新建打开保存插入图片路径退出在此预览模式生成取消刀具大小可视化默认类型速度播放颜色");
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.BuildRanges(&text_range);
}
#endif //CFSAPP_IMGUI_FONTS_TEXT_ICON_H
