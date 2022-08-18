#pragma once

#include <string>

// #include <GL/gl3w.h>
#include <glbinding/gl/gl.h>
using namespace gl;
#define GLFW_INCLUDE_NONE
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
    void begin_frame();
    void end_frame();

    void set_style_dark();
    void set_style_light();

    private:
        ImFont* font = nullptr;
        std::string ini_file = "";
        void load_font();
        GLFWwindow* handle = nullptr;
        std::string glsl_version = "";
};
