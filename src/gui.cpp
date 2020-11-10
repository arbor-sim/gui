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

auto gui(gui_state& state) {
        gui_main(state);
        // gui_parameters(p);
        // gui_locations(p);
        // gui_simulation(p);
        // gui_cell(p);
}
