// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include <gui_state.hpp>
#include <window.hpp>

int main(int, char**) {
    log_init();

    Window window{};
    gui_state state{};

    // Main loop
    while (window.running()) {
        window.begin_frame();
        window.end_frame();
    }
}
