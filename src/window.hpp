#pragma once

#include <string>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <utils.hpp>

extern float phi;
extern float zoom;

struct Window {
    Window();
    ~Window();

    bool running();
    void begin_frame();
    void end_frame();

    private:
        ImFont* font = nullptr;
        std::string ini_file = "";
        void load_font();
        GLFWwindow* handle = nullptr;
        std::string glsl_version = "";
};
