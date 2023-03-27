#ifndef CFSAPP_MYIMGUIPLUGIN_H
#define CFSAPP_MYIMGUIPLUGIN_H

#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/ViewerPlugin.h>
#include <igl/igl_inline.h>

#include <igl/project.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include "imgui_fonts_text_icon.h"
#include <GLFW/glfw3.h>
#include "Visualization.h"
#include "PathEditor.h"

#include <igl/readOBJ.h>

// Forward declarations
struct ImGuiContext;


namespace CFSUI
{
class MyImGuiPlugin : public igl::opengl::glfw::ViewerPlugin
{
protected:
    // Hidpi scaling to be used for text rendering.
    float hidpi_scaling_;
    // Ratio between the framebuffer size and the window size.
    // May be different from the hipdi scaling!
    float pixel_ratio_;
    // ImGui Context
    ImGuiContext * context_ = nullptr;
public:
    IGL_INLINE virtual void init(igl::opengl::glfw::Viewer *_viewer) override;
    IGL_INLINE virtual void reload_font(int font_size = 19);
    IGL_INLINE virtual void shutdown() override;
    IGL_INLINE virtual bool pre_draw() override;
    IGL_INLINE virtual bool post_draw() override;
    IGL_INLINE virtual void post_resize(int width, int height) override;
    IGL_INLINE virtual bool mouse_down(int button, int modifier) override;
    IGL_INLINE virtual bool mouse_up(int button, int modifier) override;
    IGL_INLINE virtual bool mouse_move(int mouse_x, int mouse_y) override;
    IGL_INLINE virtual bool mouse_scroll(float delta_y) override;
    // Keyboard IO
    IGL_INLINE virtual bool key_pressed(unsigned int key, int modifiers) override;
    IGL_INLINE virtual bool key_down(int key, int modifiers) override;
    IGL_INLINE virtual bool key_up(int key, int modifiers) override;
    IGL_INLINE void draw_text(
        const Eigen::Vector3d pos,
        const Eigen::Vector3d normal,
        const std::string &text,
        const Eigen::Vector4f color = Eigen::Vector4f(0,0,0.04,1)); // old default color
    IGL_INLINE float pixel_ratio();
    IGL_INLINE float hidpi_scaling();
};


IGL_INLINE void MyImGuiPlugin::init(igl::opengl::glfw::Viewer *_viewer)
{
    ViewerPlugin::init(_viewer);
    // Setup ImGui binding
    if (_viewer)
    {
        IMGUI_CHECKVERSION();
        if (!context_)
        {
            // Single global context by default, but can be overridden by the user
            static ImGuiContext * __global_context = ImGui::CreateContext();
            context_ = __global_context;
        }
        const char* glsl_version = "#version 150";
        ImGui_ImplGlfw_InitForOpenGL(viewer->window, false);
        ImGui_ImplOpenGL3_Init(glsl_version);
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameRounding = 5.0f;

        init_fonts();
        reload_font();
    }
}

IGL_INLINE void MyImGuiPlugin::reload_font(int font_size)
{
    hidpi_scaling_ = hidpi_scaling();
    pixel_ratio_ = pixel_ratio();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(text_font_path, std::floor(font_size * hidpi_scaling_), nullptr, text_range.Data);
    io.Fonts->AddFontFromFileTTF(icon_font_path, std::floor(font_size * hidpi_scaling_), &icon_config, icon_range);
    io.Fonts->Build();

    io.FontGlobalScale = 1.0 / pixel_ratio_;
}

IGL_INLINE void MyImGuiPlugin::shutdown()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    // User is responsible for destroying context if a custom context is given
    // ImGui::DestroyContext(*context_);
}

IGL_INLINE bool MyImGuiPlugin::pre_draw()
{
    glfwPollEvents();

    // Check whether window dpi has changed
    float scaling = hidpi_scaling();
    if (std::abs(scaling - hidpi_scaling_) > 1e-5)
    {
        reload_font();
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    return false;
}

IGL_INLINE bool MyImGuiPlugin::post_draw()
{
    static bool show_path_editor {true};
    if (show_path_editor) {
        bool load_mesh {false};
        PathEditor::showPathEditor(&show_path_editor, &load_mesh);
        if (load_mesh) {
            viewer->load_mesh_from_file("./output.obj");
        }
    } else {
        if (ImGui::Begin("viewer tool")) {
            ImGui::Combo(u8"可视化类型", &Visualization::visualization_type, u8" default\0 under/over fill\0 3D\0\0");
            if (Visualization::visualization_type != Visualization::VisualizationType_3D) {
                show_path_editor = true;
            }
            if (ImGui::Button("open mesh")) {
                viewer->open_dialog_load_mesh();
            }
            ImGui::End();
        }
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return false;
}

IGL_INLINE void MyImGuiPlugin::post_resize(int width, int height)
{
    if (context_)
    {
        ImGui::GetIO().DisplaySize.x = float(width);
        ImGui::GetIO().DisplaySize.y = float(height);
    }
}

// Mouse IO
IGL_INLINE bool MyImGuiPlugin::mouse_down(int button, int modifier)
{
    ImGui_ImplGlfw_MouseButtonCallback(viewer->window, button, GLFW_PRESS, modifier);
    if(ImGui::GetIO().WantCaptureMouse){ return true; }
    return false;
}

IGL_INLINE bool MyImGuiPlugin::mouse_up(int button, int modifier)
{
    return false;
}

IGL_INLINE bool MyImGuiPlugin::mouse_move(int mouse_x, int mouse_y)
{
    if(ImGui::GetIO().WantCaptureMouse){ return true; }
    return false;
}

IGL_INLINE bool MyImGuiPlugin::mouse_scroll(float delta_y)
{
    ImGui_ImplGlfw_ScrollCallback(viewer->window, 0.f, delta_y);
    return ImGui::GetIO().WantCaptureMouse;
}

// Keyboard IO
IGL_INLINE bool MyImGuiPlugin::key_pressed(unsigned int key, int modifiers)
{
    ImGui_ImplGlfw_CharCallback(nullptr, key);
    if(ImGui::GetIO().WantCaptureKeyboard) { return true; }
    return false;
}

IGL_INLINE bool MyImGuiPlugin::key_down(int key, int modifiers)
{
    ImGui_ImplGlfw_KeyCallback(viewer->window, key, 0, GLFW_PRESS, modifiers);
    if(ImGui::GetIO().WantCaptureKeyboard) { return true; }
    return false;
}

IGL_INLINE bool MyImGuiPlugin::key_up(int key, int modifiers)
{
    ImGui_ImplGlfw_KeyCallback(viewer->window, key, 0, GLFW_RELEASE, modifiers);
    if(ImGui::GetIO().WantCaptureKeyboard) { return true; }
    return false;
}

IGL_INLINE float MyImGuiPlugin::pixel_ratio()
{
    // Computes pixel ratio for hidpi devices
    int buf_size[2];
    int win_size[2];
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetFramebufferSize(window, &buf_size[0], &buf_size[1]);
    glfwGetWindowSize(window, &win_size[0], &win_size[1]);
    return (float) buf_size[0] / (float) win_size[0];
}

IGL_INLINE float MyImGuiPlugin::hidpi_scaling()
{
    // Computes scaling factor for hidpi devices
    float xscale, yscale;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetWindowContentScale(window, &xscale, &yscale);
    return 0.5 * (xscale + yscale);
}
}


#endif //CFSAPP_MYIMGUIPLUGIN_H
