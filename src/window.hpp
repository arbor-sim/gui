#pragma once

#include <string>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <utils.hpp>

static float phi  = 0.0f;
static float zoom = 45.0f;

struct Window {
    Window();
    ~Window();

    bool running();
    void begin_frame();
    void end_frame();

    private:
        void load_font();
        GLFWwindow* handle = nullptr;
        std::string glsl_version = "";
};
