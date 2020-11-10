#include <gui_state.hpp>
#include <window.hpp>
#include <IconsForkAwesome.h>

void gui_main(gui_state& state);
void gui_menu_bar(gui_state& state);
void gui_read_morphology(gui_state& state, bool& open);
void gui_locations(gui_state& state);
void gui_cell(gui_state& state);
void gui_place(gui_state& state);
void gui_tooltip(const std::string&);
void gui_check_state(def_state);
void gui_painting(gui_state& state);
void gui_trash(definition&);
void gui_debug(bool&);
void gui_style(bool&);

int main(int, char**) {
    log_init();

    Window window{};
    gui_state state{};

    // Main loop
    while (window.running()) {
        state.update();
        window.begin_frame();
        gui_main(state);
        gui_locations(state);
        gui_cell(state);
        gui_place(state);
        gui_painting(state);
        window.end_frame();
    }
}

void gui_trash(definition& def) {
    if (ImGui::Button((const char*) ICON_FK_TRASH)) def.erase();
}

void gui_tooltip(const std::string& message) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize()*35.0f);
        ImGui::TextUnformatted(message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void gui_check_state(const definition& def) {
    const char* tag = "";
    switch (def.state) {
        case def_state::error: tag = (const char*) ICON_FK_EXCLAMATION_TRIANGLE; break;
        case def_state::good:  tag = (const char*) ICON_FK_CHECK;                break;
        case def_state::empty: tag = (const char*) ICON_FK_QUESTION;             break;
        default: break;
    }
    ImGui::Text("%s", tag);
    gui_tooltip(def.message);
}

void gui_toggle(const char* on, const char* off, bool& flag) {
    if (ImGui::Button(flag ? on : off)) flag = !flag;
}

void gui_main(gui_state& state) {
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding) ImGui::PopStyleVar();
    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    gui_menu_bar(state);
    ImGui::End();
}

void gui_menu_bar(gui_state& state) {
    ImGui::BeginMenuBar();
    static auto open_morph = false;
    static auto open_debug = false;
    static auto open_style = false;
    if (ImGui::BeginMenu("File")) {
        open_morph = ImGui::MenuItem(fmt::format("{} Open morphology", (const char*) ICON_FK_DOWNLOAD).c_str());
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        open_debug = ImGui::MenuItem(fmt::format("{} Metrics", (const char*) ICON_FK_BUG).c_str());
        open_style = ImGui::MenuItem(fmt::format("{} Style",   (const char*) ICON_FK_PAINT_BRUSH).c_str());
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
    if (open_morph) gui_read_morphology(state, open_morph);
    if (open_debug) gui_debug(open_debug);
    if (open_style) gui_style(open_style);
}

void gui_dir_view(file_chooser_state& state) {
    // Draw the current path + show hidden
    {
        auto acc = std::filesystem::path{};
        if (ImGui::Button((const char *) ICON_FK_FOLDER_OPEN)) state.cwd = "/";
        for (const auto& part: state.cwd) {
            acc = acc / part;
            if ("/" == part) continue;
            ImGui::SameLine(0.0f, 0.0f);
            if (ImGui::Button(fmt::format("/ {}", part.c_str()).c_str())) state.cwd = acc;
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
        if(ImGui::Button((const char*) ICON_FK_SNAPCHAT_GHOST)) state.show_hidden = !state.show_hidden;
    }
    // Draw the current dir
    {
        ImGui::BeginChild("Files", {-0.35f*ImGui::GetTextLineHeightWithSpacing(), -3.0f*ImGui::GetTextLineHeightWithSpacing()}, true);
        for (const auto& it: std::filesystem::directory_iterator(state.cwd)) {
            if (it.is_directory()) {
                const auto& path = it.path();
                std::string fn = path.filename();
                if (fn.empty() || (!state.show_hidden && (fn.front() == '.'))) continue;
                auto lbl = fmt::format("{} {}", (const char *) ICON_FK_FOLDER, fn);
                ImGui::Selectable(lbl.c_str(), false);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    state.cwd = path;
                    state.file.clear();
                }
            }
        }
        for (const auto& it: std::filesystem::directory_iterator(state.cwd)) {
            if (it.is_regular_file()) {
                const auto& path = it.path();
                const auto& ext = path.extension();
                if (state.filter && state.filter.value() != ext) continue;
                std::string fn = path.filename();
                if (fn.empty() || (!state.show_hidden && (fn.front() == '.'))) continue;
                if (ImGui::Selectable(fn.c_str(), path == state.file)) state.file = path;
            }
        }
        ImGui::EndChild();
    }
}

struct loader_state {
    std::string message;
    const char* icon;
    std::optional<std::function<void(gui_state&, const std::string&)>> load;
};

std::unordered_map<std::string, std::unordered_map<std::string, std::function<void(gui_state&, const std::string&)>>>
loaders{{".swc",
         {{"Arbor",  [](gui_state& s, const std::string& fn) { s.load_arbor_swc(fn); }},
          {"Allen",  [](gui_state& s, const std::string& fn) { s.load_allen_swc(fn); }},
          {"Neuron", [](gui_state& s, const std::string& fn) { s.load_neuron_swc(fn); }}}}};

loader_state get_loader(const std::string& extension,
                        const std::string& flavor) {
    if (extension.empty())                    return {"Please select a file.",   (const char*) ICON_FK_EXCLAMATION_TRIANGLE, {}};
    if (!loaders.contains(extension))         return {"Unknown file type.",      (const char*) ICON_FK_EXCLAMATION_TRIANGLE, {}};
    if (flavor.empty())                       return {"Please select a flavor.", (const char*) ICON_FK_EXCLAMATION_TRIANGLE, {}};
    if (!loaders[extension].contains(flavor)) return {"Unknown flavor type.",    (const char*) ICON_FK_EXCLAMATION_TRIANGLE, {}};
    return {"Ok.", (const char*) ICON_FK_CHECK, {loaders[extension][flavor]}};
}

void gui_read_morphology(gui_state& state, bool& open_file) {
    ImGui::PushID("open_file");
    ImGui::OpenPopup("Open");
    if (ImGui::BeginPopupModal("Open")) {
        gui_dir_view(state.file_chooser);
        ImGui::PushItemWidth(120.0f); // this is pixels
        {
            auto lbl = state.file_chooser.filter.value_or("all");
            if (ImGui::BeginCombo("Filter", lbl.c_str())) {
                if (ImGui::Selectable("all", "all" == lbl)) state.file_chooser.filter = {};
                for (const auto& [k, v]: loaders) {
                    if (ImGui::Selectable(k.c_str(), k == lbl)) state.file_chooser.filter = {k};
                }
                ImGui::EndCombo();
            }
        }
        ImGui::SameLine();
        auto extension = state.file_chooser.file.extension();
        static std::string current_flavor = "";
        if (ImGui::BeginCombo("Flavor", current_flavor.c_str())) {
            if (loaders.contains(extension)) {
                auto flavors = loaders[extension];
                for (const auto& [k, v]: flavors) {
                    if (ImGui::Selectable(k.c_str(), k == current_flavor)) current_flavor = k;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        {
            auto loader = get_loader(extension, current_flavor);
            auto alpha = loader.load ? 1.0f : 0.6f;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            auto do_load = ImGui::Button("Load");
            ImGui::PopStyleVar();
            ImGui::SameLine();
            ImGui::Text("%s", loader.icon);
            gui_tooltip(loader.message);
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) open_file = false;
            {
                static std::string loader_error = "";
                if (do_load && loader.load) {
                    try {
                        loader.load.value()(state, state.file_chooser.file);
                        open_file = false;
                    } catch (arborio::swc_error& e) {
                        loader_error = e.what();
                    }
                }
                if (ImGui::BeginPopupModal("Cannot Load Morphology")) {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize()*50.0f);
                    ImGui::TextUnformatted(loader_error.c_str());
                    ImGui::PopTextWrapPos();
                    if (ImGui::Button("Close")) {
                        loader_error.clear();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (!loader_error.empty()) {
                    ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetFontSize()*50.0f));
                    ImGui::OpenPopup("Cannot Load Morphology");
                }
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void gui_cell(gui_state& state) {
    if (ImGui::Begin("Cell")) {
        ImGui::BeginChild("Cell Render");
        auto size = ImGui::GetWindowSize();
        auto image = state.render_cell(size.x, size.y);
        ImGui::Image((ImTextureID) image, size, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::EndChild();
    }
    ImGui::End();
}

void gui_locdef(loc_def& def, renderable& render) {
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::InputText("Name", &def.name);
    ImGui::Indent(24.0f);
    if (ImGui::InputText("Definition", &def.definition)) def.state = def_state::changed;
    ImGui::SameLine();
    gui_check_state(def);
    gui_trash(def);
    ImGui::SameLine();
    gui_toggle((const char*) ICON_FK_EYE, (const char*) ICON_FK_EYE_SLASH, render.active);
    ImGui::SameLine();
    ImGui::ColorEdit4("", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Unindent();
}

template<typename Item>
void gui_locdefs(const std::string& name, std::vector<Item>& items, std::vector<renderable>& render) {
    bool add = false;
    ImGui::PushID(name.c_str());
    ImGui::AlignTextToFramePadding();
    auto open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    if (ImGui::Button((const char*) ICON_FK_PLUS_SQUARE)) add = true;
    if (open) {
        ImGui::PushItemWidth(120.0f); // this is pixels
        int idx = 0;
        for (auto& def: items) {
            ImGui::PushID(idx);
            gui_locdef(def, render[idx]);
            ImGui::PopID();
            idx++;
        }
        ImGui::PopItemWidth();
        ImGui::TreePop();
    }
    ImGui::PopID();
    if (add) items.emplace_back();
}

void gui_locations(gui_state& state) {
    if (ImGui::Begin("Locations")) {
        gui_locdefs("Regions", state.region_defs, state.render_regions);
        ImGui::Separator();
        gui_locdefs("Locsets", state.locset_defs, state.render_locsets);
    }
    ImGui::End();
}

void gui_placeable(prb_def& probe) {
    ImGui::InputDouble("Frequency", &probe.frequency, 0.0, 0.0, "%.0f Hz", ImGuiInputTextFlags_CharsScientific);
    static std::vector<std::string> probe_variables{"voltage", "current"};
    if (ImGui::BeginCombo("Variable", probe.variable.c_str())) {
        for (const auto& v: probe_variables) {
            if (ImGui::Selectable(v.c_str(), v == probe.variable)) probe.variable = v;
        }
        ImGui::EndCombo();
    }
}

void gui_placeable(stm_def& iclamp) {
    ImGui::InputDouble("Delay",     &iclamp.delay,     0.0, 0.0, "%.0f ms", ImGuiInputTextFlags_CharsScientific);
    ImGui::InputDouble("Duration",  &iclamp.duration,  0.0, 0.0, "%.0f ms", ImGuiInputTextFlags_CharsScientific);
    ImGui::InputDouble("Amplitude", &iclamp.amplitude, 0.0, 0.0, "%.0f nA", ImGuiInputTextFlags_CharsScientific);
}

void gui_placeable(sdt_def& detector) {
    ImGui::InputDouble("Threshold", &detector.threshold, 0.0, 0.0, "%.0f mV", ImGuiInputTextFlags_CharsScientific);
}


template<typename Item>
void gui_placeables(const std::string& label, const std::vector<ls_def>& locsets, std::vector<Item>& items) {
    ImGui::PushID(label.c_str());
    ImGui::AlignTextToFramePadding();
    auto open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    if (ImGui::Button((const char*) ICON_FK_PLUS_SQUARE)) items.emplace_back();
    if (open) {
        ImGui::PushItemWidth(120.0f); // this is pixels
        auto idx = 0;
        for (auto& item: items) {
            ImGui::PushID(idx);
            ImGui::Bullet();
            ImGui::SameLine();
            if (ImGui::BeginCombo("Locset", item.locset_name.c_str())) {
                for (const auto& ls: locsets) {
                    auto nm = ls.name;
                    if (ImGui::Selectable(nm.c_str(), nm == item.locset_name)) item.locset_name = nm;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            gui_check_state(item);
            ImGui::Indent(24.0f);
            gui_placeable(item);
            gui_trash(item);
            ImGui::Unindent();
            ImGui::PopID();
            idx++;
        }
        ImGui::PopItemWidth();
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void gui_place(gui_state& state) {
    if (ImGui::Begin("Placings")) {
        gui_placeables("Probes",  state.locset_defs, state.probe_defs);
        ImGui::Separator();
        gui_placeables("Stimuli", state.locset_defs, state.iclamp_defs);
        ImGui::Separator();
        gui_placeables("Detectors", state.locset_defs, state.detector_defs);
    }
    ImGui::End();
}

void gui_defaulted_double(const std::string& label,
                          const char* format,
                          std::optional<double>& value,
                          const double fallback) {
    auto tmp = value.value_or(fallback);
    if (ImGui::InputDouble(label.c_str(), &tmp, 0, 0, format)) value = {tmp};
    ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
    if (ImGui::Button((const char*) ICON_FK_REFRESH)) value = {};
}

void gui_painting(gui_state& state) {
    if (ImGui::Begin("Paintings")) {
        ImGui::PushItemWidth(120.0f); // this is pixels
        {
            if (ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_AllowItemOverlap)) {
                if (ImGui::TreeNodeEx("Defaults", ImGuiTreeNodeFlags_AllowItemOverlap)) {
                    static std::string preset = "Neuron";
                    std::unordered_map<std::string, arb::cable_cell_parameter_set> presets{{"Neuron", arb::neuron_parameter_defaults}};
                    ImGui::InputDouble("Temperature",          &state.parameter_defaults.TK, 0, 0, "%.0f K");
                    ImGui::InputDouble("Membrane Potential",   &state.parameter_defaults.Vm, 0, 0, "%.0f mV");
                    ImGui::InputDouble("Axial Resistivity",    &state.parameter_defaults.RL, 0, 0, "%.0f Ω·cm");
                    ImGui::InputDouble("Membrane Capacitance", &state.parameter_defaults.Cm, 0, 0, "%.0f F/m²");
                    if (ImGui::BeginCombo("Load Preset", preset.c_str())) {
                        for (const auto& [k, v]: presets) {
                            if (ImGui::Selectable(k.c_str(), k == preset)) preset = k;
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button((const char*) ICON_FK_UPLOAD)) {
                        auto df = presets[preset];
                        state.parameter_defaults.TK = df.temperature_K.value();
                        state.parameter_defaults.Vm = df.init_membrane_potential.value();
                        state.parameter_defaults.RL = df.axial_resistivity.value();
                        state.parameter_defaults.Cm = df.membrane_capacitance.value();
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNodeEx("Regions", ImGuiTreeNodeFlags_AllowItemOverlap)) {
                    for (auto& p: state.parameter_defs) {
                        if (ImGui::TreeNodeEx(p.region_name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap)) {
                            gui_defaulted_double("Temperature",          "%.0f K",     p.TK, state.parameter_defaults.TK);
                            gui_defaulted_double("Membrane Potential",   "%.0f mV",    p.Vm, state.parameter_defaults.Vm);
                            gui_defaulted_double("Axial Resistivity",    "%.0f Ω·cm ", p.RL, state.parameter_defaults.RL);
                            gui_defaulted_double("Membrane Capacitance", "%.0f F/m²",  p.Cm, state.parameter_defaults.Cm);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void gui_debug(bool& open) { ImGui::ShowMetricsWindow(&open); }
void gui_style(bool& open) {
    if (ImGui::Begin("Style", &open)) {
        ImGui::ShowStyleEditor();
    }
    ImGui::End();
}
