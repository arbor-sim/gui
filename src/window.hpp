#pragma once

#include <string>
#include <optional>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

#include <utils.hpp>

extern glm::vec2 mouse;

struct Window {
    Window();
    ~Window();

    bool running();
    bool visible();
    void begin_frame(std::optional<std::string>);
    void end_frame();

    void set_style_dark();
    void set_style_light();

    private:
        std::string appname = "arbor-gui";
        ImFont* font = nullptr;
        std::string ini_file = "";
        void load_font();
        GLFWwindow* handle = nullptr;
        std::string glsl_version = "";
};
