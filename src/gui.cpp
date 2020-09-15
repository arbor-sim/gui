double input_double(const char* label, double variable) {
    float tmp = variable;
    ImGui::InputFloat(label, &tmp);
    return tmp;
}

void gui_mechanism(arb::mechanism_desc& mech) {
    for (auto& [key, value]: mech.values()) {
        mech.set(key, input_double(key.c_str(), value));
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
    if (ImGui::Button("Run")) {
        auto model = make_simulation(param);
        model.run(sim.t_stop, sim.dt);
        for (auto& trace: model.traces_) {
            vs.resize(trace.v.size());
            std::copy(trace.v.begin(), trace.v.end(), vs.begin());
            break;
        }
        if (ImGui::BeginPopupModal("Results")) {
            ImGui::PlotLines("Voltage", vs.data(), vs.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(640, 480));
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void gui_menu_bar(parameters& p, geometry& g) {
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open SWC+Fit")) {
            parameters q;
            q.load_allen_swc("data/full.swc");
            q.load_allen_fit("data/full-dyn.json");
            p = q;
            g = geometry{p};
        }
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

void gui_main(parameters& p, geometry& g) {
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
    gui_menu_bar(p, g);
    ImGui::End();
}

void gui_cell(geometry& geo) {
    ImGui::Begin("Cell");
    ImGui::BeginChild("Cell Render");
    // re-build fbo, if needed
    auto size = ImGui::GetWindowSize();
    auto w = size.x;
    auto h = size.y;
    // log_info("Cell window {}x{}", w, h);
    geo.maybe_make_fbo(w, h);
    // render image to texture
    // glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, geo.fbo);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    geo.render(zoom, phi, w, h);
    // draw
    ImGui::Image((ImTextureID) geo.tex, size, ImVec2(0, 1), ImVec2(1, 0));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui::EndChild();
    ImGui::End();
}

void gui_render(parameters& p, geometry& g) {
    ImGui::Begin("Colour");
    if (ImGui::Button("By region")) g.set_colour_by_tag();
    for (auto tag = 0; tag < 5; ++tag) {
        if (ImGui::Button(fmt::format("Highlight tag={}", tag).c_str())) {
            auto region   = arb::reg::tagged(tag);
            auto concrete = thingify(region, p.provider);
            auto segments = p.pwlin.all_segments(concrete);
            g.set_colour_by_region(segments);
        }
    }
    ImGui::End();
}

void gui_param(arb::cable_cell_parameter_set& p, arb::cable_cell_parameter_set& defaults) {
    {
        auto tmp = p.axial_resistivity.value_or(defaults.axial_resistivity.value());
        if (ImGui::InputDouble("axial_resistivity", &tmp)) {
            p.axial_resistivity = tmp;
        }
    }
    {
        auto tmp = p.temperature_K.value_or(defaults.temperature_K.value());
        if (ImGui::InputDouble("temperature_K", &tmp)) {
            p.temperature_K = tmp;
        }
    }
    {
        auto tmp = p.init_membrane_potential.value_or(defaults.init_membrane_potential.value());
        if (ImGui::InputDouble("init_membrane_potential", &tmp)) {
            p.init_membrane_potential = tmp;
        }
    }
    {
        auto tmp = p.membrane_capacitance.value_or(defaults.membrane_capacitance.value());
        if (ImGui::InputDouble("membrane_capacitance", &tmp)) {
            p.membrane_capacitance = tmp;
        }
    }
}

void gui_ion(arb::cable_cell_ion_data& ion, arb::cable_cell_ion_data& defaults) {
    {
        auto tmp = ion.init_int_concentration;
        if (std::isnan(tmp)) tmp = defaults.init_int_concentration;
        if (ImGui::InputDouble("init_int_concentration", &tmp)) {
            ion.init_int_concentration = tmp;
        }
    }
    {
        auto tmp = ion.init_ext_concentration;
        if (std::isnan(tmp)) tmp = defaults.init_ext_concentration;
        if (ImGui::InputDouble("init_ext_concentration", &tmp)) {
            ion.init_ext_concentration = tmp;
        }
    }
    {
        auto tmp = ion.init_reversal_potential;
        if (std::isnan(tmp)) tmp = defaults.init_reversal_potential;
        if (ImGui::InputDouble("init_reversal_potential", &tmp)) {
            ion.init_reversal_potential = tmp;
        }
    }
}
