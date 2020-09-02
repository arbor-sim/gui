// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

constexpr float PI = 3.141f;

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include "json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "arbor-interface/single-cell.cpp"
#include "arbor-interface/parameters.cpp"
#include "arbor-interface/geometry.cpp"

bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

auto load_allen_fit(parameters& p, const std::string& dyn) {

    std::unordered_map<std::string, std::string> lut = {{"cm", "membrane_capacitance"},
                                                        {"Ra", "axial_resistivity"},
                                                        {"ra", "axial_resistivity"},
                                                        {"Vm",  "init_membrane_potential"},
                                                        {"vm",  "init_membrane_potential"}};
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
                p.set_parameter(region, lut[name], value);
                continue;
            }
        }
        name.erase(name.size() - mech.size() - 1, mech.size() + 1);
        p.set_mech(region, mech, name, value);
    }
}

auto make_simulation(parameters& p) {
    auto cell = arb::cable_cell(p.morph, p.labels);

    // This takes care of
    //  * Baseline parameters: cm, Vm, Ra, T
    //  * Ion concentrations
    //  * Reversal potentials
    cell.default_parameters = p.values;

    for (const auto& region: p.regions) {
        cell.paint(region.first, arb::membrane_capacitance{p.get_parameter(region.first, "membrane_capacitance")}); // cm
        cell.paint(region.first, arb::init_membrane_potential{p.get_parameter(region.first, "init_membrane_potential")}); // Vm
        cell.paint(region.first, arb::axial_resistivity{p.get_parameter(region.first, "axial_resistivity")}); // Ra
        cell.paint(region.first, arb::temperature_K{p.get_parameter(region.first, "temperature_K")}); // T
        for (const auto& ion: region.second.values.ion_data) {
            cell.paint(region.first, {ion.first, ion.second});
        }
        for (const auto& mech: region.second.mechanisms) {
            cell.paint(arb::reg::named(region.first), mech.second);
        }
    }

    auto model = single_cell_model(cell);

    for (const auto& probe: p.probes) {
        model.probe(probe.variable, probe.site, probe.frequency);
    }

    return model;
}

float phi  = 0.0f;
float zoom = 45.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    phi += 0.1 * (float) xoffset;
    if (phi > 2.0f*PI) phi -= 2*PI;
    if (phi <    0.0f) phi += 2*PI;
    zoom -= (float) yoffset;
    if (zoom < 1.0f)  zoom =  1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}


static void glfw_error_callback(int error, const char* description) { std::cerr << "Glfw Error: " << error << "\n --> " << description << '\n'; }

int main(int, char**) {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    auto window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err) {
        std::cerr << "Failed to initialize OpenGL loader!\n";
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

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

    ImVec4 clear_color = ImVec4(214.0f/255, 214.0f/255, 214.0f/255, 1.00f);

    auto param = parameters{"full.swc"};
    load_allen_fit(param, "full-dyn.json");
    auto draw = geometry{param};

    glfwSetScrollCallback(window, scroll_callback);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Probes, Parameters, and Mechanisms");

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
            ImGui::LabelText("", "Regions");
            for (auto& region: param.regions) {
                if (ImGui::TreeNode(region.first.c_str())) {
                    if (ImGui::TreeNode("Parameters")) {
                        for (auto& key: param.keys) {
                            float tmp = param.get_parameter(region.first, key);
                            ImGui::InputFloat(key.c_str(), &tmp);
                            param.set_parameter(region.first, key, tmp);
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Mechanisms")) {
                        for (auto& mechanism: region.second.mechanisms) {
                            if (ImGui::TreeNode(mechanism.first.c_str())) {
                                for (auto& mechanism_param: mechanism.second.values()) {
                                    float tmp = mechanism_param.second;
                                    ImGui::InputFloat(mechanism_param.first.c_str(), &tmp);
                                    mechanism.second.set(mechanism_param.first, tmp);
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Ions")) {
                        for (auto& ion: region.second.values.ion_data) {
                            if (ImGui::TreeNode(ion.first.c_str())) {
                                for (auto& key: param.ion_keys) {
                                    float tmp = param.get_ion(region.first, ion.first, key);
                                    ImGui::InputFloat(key.c_str(), &tmp);
                                    param.set_ion(region.first, ion.first, key, tmp);
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::Separator();
            ImGui::LabelText("", "Probes");
            for (auto& trace: param.probes) {
                std::stringstream ss;
                ss << trace.site;
                ImGui::BulletText("Voltage @ %s %f Hz", ss.str().c_str(), trace.frequency);
            }
            ImGui::End();
        }

        static float simulation_stop  = 1000.0f;
        static float simulation_delta = 0.25f;
        size_t n = simulation_stop/simulation_delta;
        static std::vector<float> ts;
        static std::vector<float> vs;
        static std::vector<float> ic;
        ts.resize(n);
        ic.resize(n);
        for (auto ix = 0ul; ix < n; ++ix) {
            auto t = ix*simulation_delta;
            ic[ix] = ((t >= param.iclamp.delay) && (t < (param.iclamp.duration + param.iclamp.delay))) ? param.iclamp.amplitude : 0.0f;
        }

        {
            ImGui::Begin("Simulation");
            ImGui::LabelText("", "Simulation");
            ImGui::InputFloat("To (ms)",   &simulation_stop);
            ImGui::InputFloat("dt (ms)",   &simulation_delta);
            ImGui::LabelText("", "Stimulus (Rectangle)");
            {
                float tmp = param.iclamp.delay;
                ImGui::InputFloat("From (ms)", &tmp);
                param.iclamp.delay = tmp;
            }
            {
                float tmp = param.iclamp.duration;
                ImGui::InputFloat("Length (ms)", &tmp);
                param.iclamp.duration = tmp;
            }
            {
                float tmp = param.iclamp.amplitude;
                ImGui::InputFloat("Amplitude (mA)", &tmp);
                param.iclamp.amplitude = tmp;
            }
            ImGui::PlotLines("IClamp", ic.data(), ic.size(), 0);

            static bool show_results = false;
            if (ImGui::Button("Run")) {
                auto sim = make_simulation(param);
                sim.run(simulation_stop, simulation_delta);
                for (auto& trace: sim.traces_) {
                    vs.resize(trace.v.size());
                    std::copy(trace.v.begin(), trace.v.end(), vs.begin());
                    break;
                }
                show_results = true;
            }
            if (show_results) {
                ImGui::Begin("Results");
                ImGui::PlotLines("Voltage", vs.data(), vs.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(640, 480));
                ImGui::End();
            }

            ImGui::End();
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        draw.render(zoom, phi, display_w, display_h);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
