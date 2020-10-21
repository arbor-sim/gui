const static std::unordered_map<std::string, arb::mechanism_catalogue>
catalogues = {{"default", arb::global_default_catalogue()},
              {"allen",   arb::global_allen_catalogue()}};

const std::vector<const char*>& all_mechanisms() {
    static std::vector<const char*> names = {};
    if (names.empty()) {
        for (const auto& [cat_name, cat]: catalogues) {
            for (const auto& name: cat.mechanism_names()) {
                names.push_back(strdup(name.c_str()));
            }
        }
    }
    return names;
}

auto get_mechanism_info(const std::string& name) {
    for (const auto& [cat_name, cat]: catalogues) {
        if (cat.has(name)) return cat[name];
    }
    log_error("Unknown mechanism {}", name);
}

void gui_mechanism(arb::mechanism_desc& mech) {
    auto name = mech.name();
    auto info = get_mechanism_info(name);
    if (!info.globals.empty()) {
        if (ImGui::TreeNode("Globals")) {
            for (const auto& [key, desc]: info.globals) {
                ImGui::BulletText("%s", key.c_str());
                // auto tmp = mech.get(key);
                // if (ImGui::InputDouble(key.c_str(), &tmp)) mech.set(key, tmp);
            }
            ImGui::TreePop();
        }
    }
    if (!info.parameters.empty()) {
        if (ImGui::TreeNode("Parameters")) {
            for (const auto& [key, desc]: info.parameters) {
                auto tmp = mech.get(key);
                if (ImGui::InputDouble(key.c_str(), &tmp)) mech.set(key, tmp);
            }
            ImGui::TreePop();
        }
    }
    if (!info.state.empty()) {
        if (ImGui::TreeNode("State variables")) {
            for (const auto& [key, desc]: info.state) {
                ImGui::BulletText("%s", key.c_str());
                // auto tmp = mech.get(key);
                // if (ImGui::InputDouble(key.c_str(), &tmp)) mech.set(key, tmp);
            }
            ImGui::TreePop();
        }
    }
    if (!info.ions.empty()) {
        if (ImGui::TreeNode("Ion dependencies")) {
            for (const auto& [key, desc]: info.ions) {
                ImGui::BulletText("%s", key.c_str());
                // auto tmp = mech.get(key);
                // if (ImGui::InputDouble(key.c_str(), &tmp)) mech.set(key, tmp);
            }
            ImGui::TreePop();
        }
    }
}

void gui_iclamp(arb::locset& location, arb::i_clamp& iclamp, float dt, std::vector<float>& ic) {
    ImGui::BulletText("%s", to_string(location).c_str());
    ImGui::InputDouble("From (ms)",      &iclamp.delay);
    ImGui::InputDouble("Length (ms)",    &iclamp.duration);
    ImGui::InputDouble("Amplitude (mA)", &iclamp.amplitude);
    // Make a nice sparkline (NB std::transform_on_index where are you?)
    for (auto ix = 0ul; ix < ic.size(); ++ix) {
        auto t = ix*dt;
        ic[ix] = ((t >= iclamp.delay) && (t < (iclamp.duration + iclamp.delay))) ? iclamp.amplitude : 0.0f;
    }
    ImGui::PlotLines("IClamp", ic.data(), ic.size(), 0);
}

void gui_probe(probe& p) {
    ImGui::BulletText("%s", to_string(p.location).c_str());
    ImGui::InputDouble("Frequency (Hz)", &p.frequency);
    ImGui::LabelText("Variable", "%s", p.variable.c_str());
}

void gui_simulation(parameters& param) {
    ImGui::Begin("Simulation");
    auto& sim = param.sim;
    size_t n = sim.t_stop/sim.dt;
    
    if (ImGui::TreeNode("Probes")) {
        for (auto& p: sim.probes) {
            gui_probe(p);
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Stimuli")) {
        std::vector<float> ic(n, 0.0);
        for (auto& [location, iclamp]: sim.stimuli) {
            gui_iclamp(location, iclamp, sim.dt, ic);
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Execution")) {
        ImGui::InputDouble("To (ms)", &sim.t_stop);
        ImGui::InputDouble("dt (ms)", &sim.dt);
        ImGui::TreePop();
    }
    ImGui::Separator();
    static std::vector<float> vs;
    if (ImGui::BeginPopupModal("Results")) {
        ImGui::PlotLines("Voltage", vs.data(), vs.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(640, 480));
        ImGui::EndPopup();
    }
    if (ImGui::Button("Run")) {
        auto model = make_simulation(param);
        model.run(sim.t_stop, sim.dt);
        for (auto& trace: model.traces_) {
            vs.resize(trace.v.size());
            std::copy(trace.v.begin(), trace.v.end(), vs.begin());
            break;
        }
        ImGui::OpenPopup("Results");
    }
    ImGui::End();
}

void gui_menu_bar(parameters& p) {
    ImGui::BeginMenuBar();
    static auto open_file = false;
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Read morphology")) {
            open_file = true;
        }
        ImGui::EndMenu();
    }

    if (open_file) {
        ImGui::PushID("open_file");
        ImGui::OpenPopup("Open");
        const std::vector<const char*> filters{"all", ".swc", ".asc"};
        std::unordered_map<std::string, std::vector<const char*>>
            flavors{{"", {}},
                    {".swc", {"Allen", "NEURON"}},
                    {".asc", {}}};
        if (ImGui::BeginPopupModal("Open")) {
            static int current_filter = 1;
            static int current_flavor = 1;
            static std::filesystem::path current_file = "";
            auto new_cwd = p.cwd;
            ImGui::LabelText("CWD", p.cwd.c_str(), "%s");
            auto size = ImGui::GetWindowSize();
            size.x -=   ImGui::GetTextLineHeightWithSpacing();
            size.y -= 6.5*ImGui::GetTextLineHeightWithSpacing();
            ImGui::BeginChild("Files", size, true);
            for (const auto& it: std::filesystem::directory_iterator(p.cwd)) {
                if (it.is_directory()) {
                    static bool selected = false;
                    const auto& path = it.path();
                    ImGui::Selectable(path.filename().c_str(), selected);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        new_cwd = path;
                        selected = !selected;
                    }
                }
            }
            for (const auto& it: std::filesystem::directory_iterator(p.cwd)) {
                if (it.is_regular_file()) { // TODO add symlink, hardlink?
                    const auto& path = it.path();
                    bool selected = (path == current_file);
                    if (!current_filter || (path.extension() == filters[current_filter])) {
                        if (ImGui::Selectable(path.filename().c_str(), selected)) {
                            current_file = path;
                        }
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PushItemWidth(80.0f); // Args this is pixels
            ImGui::Combo("Filter", &current_filter, filters.data(), filters.size());
            auto extension = current_file.extension();
            if (!extension.empty()) {
                if (flavors.find(extension) != flavors.end()) {
                    auto& flavor = flavors[extension];
                    ImGui::SameLine();
                    ImGui::Combo("Flavor", &current_flavor, flavor.data(), flavor.size());
                } else {
                    ImGui::Text("Unknown Extension: %s", extension.c_str());
                }
            }
            ImGui::PopItemWidth();
            if (ImGui::Button("Load!")) {
                if (extension == ".swc") {
                    p.load_allen_swc(current_file);
                }
                open_file = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                open_file = false;
            }
            p.cwd = new_cwd;
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::EndMenuBar();
}

void gui_main(parameters& p) {
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
    gui_menu_bar(p);
    ImGui::End();
}

void gui_cell(parameters& p) {
    if (ImGui::Begin("Cell")) {
        ImGui::BeginChild("Cell Render");
        auto size = ImGui::GetWindowSize();
        auto image = p.render_cell(size.x, size.y);
        ImGui::Image((ImTextureID) image, size, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::EndChild();
    }
    ImGui::End();
}

void gui_values(arb::cable_cell_parameter_set& p, arb::cable_cell_parameter_set& defaults) {
    {
        auto tmp = p.axial_resistivity.value_or(defaults.axial_resistivity.value());
        if (ImGui::InputDouble("axial resistivity", &tmp)) {
            p.axial_resistivity = tmp;
        }
    }
    {
        auto tmp = p.temperature_K.value_or(defaults.temperature_K.value());
        if (ImGui::InputDouble("Temperature (K)", &tmp)) {
            p.temperature_K = tmp;
        }
    }
    {
        auto tmp = p.init_membrane_potential.value_or(defaults.init_membrane_potential.value());
        if (ImGui::InputDouble("Membrane Potential", &tmp)) {
            p.init_membrane_potential = tmp;
        }
    }
    {
        auto tmp = p.membrane_capacitance.value_or(defaults.membrane_capacitance.value());
        if (ImGui::InputDouble("Membrane Capacitance", &tmp)) {
            p.membrane_capacitance = tmp;
        }
    }
}

void gui_ion(arb::cable_cell_ion_data& ion, arb::cable_cell_ion_data& defaults) {
    {
        auto tmp = ion.init_int_concentration.value_or(defaults.init_int_concentration.value());
        if (ImGui::InputDouble("int. Concentration", &tmp)) {
            ion.init_int_concentration = tmp;
        }
    }
    {
        auto tmp = ion.init_ext_concentration.value_or(defaults.init_ext_concentration.value());
        if (ImGui::InputDouble("ext. Concentration", &tmp)) {
            ion.init_ext_concentration = tmp;
        }
    }
    {
        auto tmp = ion.init_reversal_potential.value_or(defaults.init_reversal_potential.value());
        if (ImGui::InputDouble("Reversal Potential", &tmp)) {
            ion.init_reversal_potential = tmp;
        }
    }
}

void gui_locdef(loc_def& def, renderable& render) {
    ImGui::Bullet();
    ImGui::SameLine();
    if (ImGui::Button("X")) def.state = def_state::erase;
    ImGui::SameLine();
    ImGui::Checkbox("Visible", &render.active);
    ImGui::Indent();
    ImGui::InputText("Name", def.name.data(), def.name.size());
    ImGui::ColorEdit4("Color", &render.color.x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, to_imvec(def.bg_color));
    if (ImGui::InputText("Definition", def.definition.data(), def.definition.size())) {
        def.update();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(def.message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor();
    ImGui::Unindent();
}

void gui_locations(parameters& p) {
    if (ImGui::Begin("Locations")) {
        {
            ImGui::PushID("region");
            ImGui::AlignTextToFramePadding();
            auto open = ImGui::TreeNodeEx("Regions", ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine();
            if (ImGui::SmallButton("+")) p.add_region();
            if (open) {
                for (auto idx = 0; idx < p.region_defs.size(); ++idx) {
                    ImGui::PushID(fmt::format("{}", idx).c_str());
                    gui_locdef(p.region_defs[idx], p.render_regions[idx]);
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        {
            ImGui::PushID("locset");
            ImGui::AlignTextToFramePadding();
            auto open = ImGui::TreeNodeEx("Locset", ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine();
            if (ImGui::SmallButton("+")) p.add_locset();
            if (open) {
                for (auto idx = 0; idx < p.locset_defs.size(); ++idx) {
                    ImGui::PushID(fmt::format("{}", idx).c_str());
                    gui_locdef(p.locset_defs[idx], p.render_locsets[idx]);
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void gui_region(region& r, arb::cable_cell_parameter_set& defaults) {
    ImGui::BulletText("Location: %s", to_string(r.location).c_str());
    if (ImGui::TreeNode("Parameters")) {
        gui_values(r.values, defaults);
        ImGui::TreePop();
    }
    // Mechanisms
    {
        static int selection = 0;
        ImGui::PushID("mechanism");
        ImGui::AlignTextToFramePadding();
        auto open = ImGui::TreeNodeEx("Mechanisms", ImGuiTreeNodeFlags_AllowItemOverlap);
        ImGui::SameLine();
        if (ImGui::SmallButton("+")) {
            ImGui::OpenPopup("Add mechanism");
        }
        if (ImGui::BeginPopupModal("Add mechanism")) {
            const auto& mechanisms = all_mechanisms();
            // TODO: Make this into sections by catalogue.
            // TODO: Grey out already present stuff.
            ImGui::Combo("Name", &selection, mechanisms.data(), mechanisms.size());
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            if (ImGui::Button("Ok")) {
                auto name = mechanisms[selection];
                auto info = get_mechanism_info(name);
                auto desc = arb::mechanism_desc{name};
                for (const auto& [k, v]: info.parameters) {
                    desc.set(k, v.default_value);
                }
                r.mechanisms[name] = desc;
                log_debug("Adding mechanism {}", name);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (open) {
            for (auto& [mech_name, mech_data]: r.mechanisms) {
                if (ImGui::TreeNode(mech_name.c_str())) {
                    gui_mechanism(mech_data);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (ImGui::TreeNode("Ions")) {
        for (auto& [ion, ion_data]: r.values.ion_data) {
            if (ImGui::TreeNode(ion.c_str())) {
                gui_ion(ion_data, defaults.ion_data[ion]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}

void gui_erev(parameters& p, const std::string& ion) {
    int tmp = p.get_reversal_potential_method(ion);
    ImGui::Combo("reversal_potential_method", &tmp, parameters::reversal_potential_methods, 2);
    p.set_reversal_potential_method(ion, tmp);
}

void gui_parameters(parameters& p) {
    ImGui::Begin("Parameters");
    if (ImGui::TreeNode("Global Parameters")) {
        gui_values(p.values, p.values);
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Ions")) {
        for (auto& [ion, ion_data]: p.values.ion_data) {
            if (ImGui::TreeNode(ion.c_str())) {
                gui_ion(ion_data, ion_data);
                gui_erev(p, ion);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Regions")) {
        for (auto& [region, region_data]: p.regions) {
            if (ImGui::TreeNode(region.c_str())) {
                gui_region(region_data, p.values);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::End();
}

auto gui(parameters& p) {
        gui_main(p);
        gui_parameters(p);
        gui_locations(p);
        gui_simulation(p);
        gui_cell(p);
}
