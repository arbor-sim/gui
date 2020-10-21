// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <filesystem>

#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/morph/label_parse.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <GL/gl3w.h>

#include <utils.hpp>
#include <location.hpp>
#include <geometry.hpp>
#include "single-cell.cpp"
#include "glfw.cpp"
#include "parameters.cpp"
#include "gui.cpp"

int main(int, char**) {
    log_init();
    Window window{};

    parameters param{};

    // Main loop
    while (window.running()) {
        window.begin_frame();
        gui(param);
        window.end_frame();
    }
}
