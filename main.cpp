// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "spdlog/spdlog.h"

#include <GL/gl3w.h>

#include "utils.cpp"
#include "single-cell.cpp"
#include "parameters.cpp"
#include "geometry.cpp"
#include "glfw.cpp"
#include "gui.cpp"

int main(int, char**) {
    log_init();
    Window window{};

    auto param = parameters{};
    auto draw = geometry{};

    // Main loop
    while (window.running()) {
        window.begin_frame();
        gui_main(param, draw);
        gui_parameters(param);
        gui_locations(param);
        gui_render(param, draw);
        gui_simulation(param);
        gui_cell(draw);
        window.end_frame();
    }
}
