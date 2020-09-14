// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ImVec4 clear_color = ImVec4(214.0f/255, 214.0f/255, 214.0f/255, 1.00f);
constexpr float PI = 3.141f;

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>


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

auto load_allen_fit(parameters& p, const std::string& dyn) {
    json genome;
    {
        std::ifstream fd(dyn.c_str());
        fd >> genome;
    }

    p.set_parameter("axial_resistivity",       double{genome["passive"][0]["ra"]});
    p.set_parameter("temperature_K",           double{genome["conditions"][0]["celsius"]} + 273.15);
    p.set_parameter("init_membrane_potential", double{genome["conditions"][0]["v_init"]});

    for (auto& block: genome["conditions"][0]["erev"]) {
        const std::string& region = block["section"];
        for (auto& kv: block.items()) {
            if (kv.key() == "section") continue;
            std::string ion = kv.key();
            ion.erase(0, 1);
            p.set_ion(region, ion, "init_reversal_potential", double{kv.value()});
        }
    }

    for (auto& block: genome["genome"]) {
        std::string region = std::string{block["section"]};
        std::string name   = std::string{block["name"]};
        double value       = block["value"].is_string() ? std::stod(std::string{block["value"]}) : double{block["value"]};
        std::string mech   = std::string{block["mechanism"]};

        if (mech == "") {
            if (ends_with(name, "_pas")) {
                mech = "pas";
            } else {
                if (name == "cm") {
                    p.set_parameter(region, "membrane_capacitance", value/100.0);
                    continue;
                }
                if ((name == "ra") || (name == "Ra")) {
                    p.set_parameter(region, "axial_resistivity", value);
                    continue;
                }
                if (name == "vm") {
                    p.set_parameter(region, "init_membrane_potential", value);
                    continue;
                }
                if (name == "celsius") {
                    p.set_parameter(region, "temperature_K", value);
                    continue;
                }
                log_error("Unknown parameter {}", name);
            }
        }
        name.erase(name.size() - mech.size() - 1, mech.size() + 1);
        p.set_mech(region, mech, name, value);
    }
}

int main(int, char**) {
    log_init();
    Window window{};
    // Initialize OpenGL loader
    if (gl3wInit()) log_fatal("Failed to initialize OpenGL loader");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)) log_error("ImGui docking disabled");
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window.handle, true);
    ImGui_ImplOpenGL3_Init(window.glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    auto param = parameters{"data/full.swc"};
    load_allen_fit(param, "data/full-dyn.json");
    auto draw = geometry{param};

    // Main loop
    while (window.running()) {
        window.poll();
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        gui_main();

        ImGui::Begin("Parameters");
        if (ImGui::TreeNode("Global Parameters")) {
            for (auto& key: param.keys) {
                float tmp = param.get_parameter(key);
                ImGui::InputFloat(key.c_str(), &tmp);
                param.set_parameter(key, tmp);
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
        if (ImGui::TreeNode("Ions")) {
            for (auto& ion: param.get_ions()) {
                if (ImGui::TreeNode(ion.c_str())) {
                    for (auto& key: param.ion_keys) {
                        {
                            float tmp = param.get_ion(ion, key);
                            ImGui::InputFloat(key.c_str(), &tmp);
                            param.set_ion(ion, key, tmp);
                        }
                    }
                    {
                        int tmp = param.get_reversal_potential_method(ion);
                        ImGui::Combo("reversal_potential_method", &tmp, parameters::reversal_potential_methods, 2);
                        param.set_reversal_potential_method(ion, tmp);
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
        if (ImGui::TreeNode("Regions")) {
            for (auto& [region, region_data]: param.regions) {
                if (ImGui::TreeNode(region.c_str())) {
                    ImGui::BulletText("Location: %s", to_string(region_data.location).c_str());
                    if (ImGui::TreeNode("Parameters")) {
                        for (auto& key: param.keys) {
                            float tmp = param.get_parameter(region, key);
                            ImGui::InputFloat(key.c_str(), &tmp);
                            param.set_parameter(region, key, tmp);
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Mechanisms")) {
                        for (auto& [mech_name, mech_data]: region_data.mechanisms) {
                            if (ImGui::TreeNode(mech_name.c_str())) {
                                gui_mechanism(mech_data);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Ions")) {
                        for (auto& [ion_name, ion_data]: region_data.values.ion_data) {
                            if (ImGui::TreeNode(ion_name.c_str())) {
                                for (auto& key: param.ion_keys) {
                                    float tmp = param.get_ion(region, ion_name, key);
                                    ImGui::InputFloat(key.c_str(), &tmp);
                                    param.set_ion(region, ion_name, key, tmp);
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::End();
        gui_render(param, draw);
        gui_simulation(param);
        gui_cell(draw);

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swap_buffers();
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
