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
void gui_trash(def_state&);

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
        window.end_frame();
    }
}

void gui_trash(def_state& state) {
    if (ImGui::Button((const char*) ICON_FK_TRASH)) state = def_state::erase;
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

void gui_check_state(def_state state, const std::string& message) {
    const char* tag;
    switch (state) {
        case def_state::error: tag = (const char*) ICON_FK_EXCLAMATION_TRIANGLE; break;
        case def_state::good:  tag = (const char*) ICON_FK_CHECK;                break;
        case def_state::empty: tag = (const char*) ICON_FK_QUESTION;             break;
        default: break;
    }
    ImGui::Text("%s", tag);
    gui_tooltip(message);
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
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem(fmt::format("{} Open morphology", (const char*) ICON_FK_DOWNLOAD).c_str())) {
            open_morph = true;
        }
        ImGui::EndMenu();
    }

    if (open_morph) gui_read_morphology(state, open_morph);
    ImGui::EndMenuBar();
}

void gui_read_morphology(gui_state& state, bool& open_file) {
    ImGui::PushID("open_file");
    ImGui::OpenPopup("Open");
    // TODO Fix/Add loaders
    std::unordered_map<std::string, std::unordered_map<std::string, std::function<void(gui_state&, const std::string&)>>>
        loaders{{".swc",
                 {{"Strict",  [](gui_state& s, const std::string& fn) { s.load_allen_swc(fn); }},
                  {"Relaxed", [](gui_state& s, const std::string& fn) { s.load_allen_swc(fn); }},
                  {"Allen",   [](gui_state& s, const std::string& fn) { s.load_allen_swc(fn); }},
                  {"Neuron",  [](gui_state& s, const std::string& fn) { s.load_allen_swc(fn); }}}}};

    if (ImGui::BeginPopupModal("Open")) {
        static std::string current_filter = "all";
        static std::filesystem::path current_file = "";
        static bool show_hidden = false;
        {
            auto acc = std::filesystem::path{};
            if (ImGui::Button((const char *) ICON_FK_FOLDER_OPEN)) state.cwd = "/";
            for (const auto& part: state.cwd) {
                acc = acc / part;
                if ("/" == part) continue;
                ImGui::SameLine(0.0f, 0.0f);
                if (ImGui::Button(fmt::format("/ {}", part.c_str()).c_str())) state.cwd = acc;
            }
        }
        ImGui::BeginChild("Files", {-0.35f*ImGui::GetTextLineHeightWithSpacing(), -3.0f*ImGui::GetTextLineHeightWithSpacing()}, true);
        for (const auto& it: std::filesystem::directory_iterator(state.cwd)) {
            if (it.is_directory()) {
                static bool selected = false;
                const auto& path = it.path();
                auto fn = path.filename().c_str();
                if (!show_hidden && (fn[0] == '.')) continue;
                auto lbl = fmt::format("{} {}", (const char *) ICON_FK_FOLDER, fn);
                ImGui::Selectable(lbl.c_str(), selected);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    state.cwd = path;
                    selected = false;
                }
            }
        }
        for (const auto& it: std::filesystem::directory_iterator(state.cwd)) {
            if (it.is_regular_file()) { // TODO add symlink, hardlink?
                const auto& path = it.path();
                if ((current_filter == "all") || (path.extension() == current_filter)) {
                    auto fn = path.filename().c_str();
                    if (!show_hidden && (fn[0] == '.')) continue;
                    if (ImGui::Selectable(fn, path == current_file)) current_file = path;
                }
            }
        }
        ImGui::EndChild();
        ImGui::PushItemWidth(80.0f); // this is pixels
        {
            if (ImGui::BeginCombo("Filter", current_filter.c_str())) {
                if (ImGui::Selectable("all", "all" == current_filter)) current_filter = "all";
                for (const auto& [k, v]: loaders) {
                    if (ImGui::Selectable(k.c_str(), k == current_filter)) current_filter = k;
                }
                ImGui::EndCombo();
            }
        }

        ImGui::SameLine();
        auto extension = current_file.extension();
        static std::string current_flavor = "";
        if (!loaders.contains(extension) || !loaders[extension].contains(current_flavor)) current_flavor = "";
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
        ImGui::SameLine();
        ImGui::Checkbox("Hidden", &show_hidden);
        if (loaders.contains(extension) && loaders[extension].contains(current_flavor)) {
            if (ImGui::Button("Load")) {
                loaders[extension][current_flavor](state, current_file);
                open_file = false;
            }
        } else {
            // TODO Disable Buttons? How?
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            ImGui::Button("Load!");
            ImGui::PopStyleVar();
            gui_tooltip("Unknown file type/flavor combination.");
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            open_file = false;
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
    ImGui::InputText("Name", def.name.data(), def.name.size());
    ImGui::Indent();
    if (ImGui::InputText("Definition", def.definition.data(), def.definition.size())) def.state = def_state::changed;
    ImGui::SameLine();
    gui_check_state(def.state, def.message);
    gui_trash(def.state);
    ImGui::SameLine();
    gui_toggle((const char*) ICON_FK_EYE, (const char*) ICON_FK_EYE_SLASH, render.active);
    ImGui::SameLine();
    ImGui::ColorEdit4("", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Unindent();
}

void gui_locations(gui_state& state) {
    if (ImGui::Begin("Locations")) {
        {
            ImGui::PushID("region");
            ImGui::AlignTextToFramePadding();
            auto open = ImGui::TreeNodeEx("Regions", ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);
            if (ImGui::Button((const char*) ICON_FK_PLUS_SQUARE)) state.add_region();
            if (open) {
                for (auto idx = 0ul; idx < state.region_defs.size(); ++idx) {
                    ImGui::PushID(idx);
                    gui_locdef(state.region_defs[idx], state.render_regions[idx]);
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        {
            ImGui::PushID("locset");
            ImGui::AlignTextToFramePadding();
            auto open = ImGui::TreeNodeEx("Locsets", ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);
            if (ImGui::Button((const char*) ICON_FK_PLUS_SQUARE)) state.add_locset();
            if (open) {
                for (auto idx = 0ul; idx < state.locset_defs.size(); ++idx) {
                    ImGui::PushID(idx);
                    gui_locdef(state.locset_defs[idx], state.render_locsets[idx]);
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void gui_probes(gui_state& state) {
    ImGui::PushID("probe");
    ImGui::AlignTextToFramePadding();
    auto open = ImGui::TreeNodeEx("Probes", ImGuiTreeNodeFlags_AllowItemOverlap);
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    if (ImGui::Button((const char*) ICON_FK_PLUS_SQUARE)) state.add_probe();
    if (open) {
        static std::vector<std::string> probe_variables{"voltage", "current"};
        auto ix = 0;
        for (auto& probe: state.probe_defs) {
            ImGui::PushID(ix++);
            ImGui::Bullet();
            ImGui::SameLine();
            if (ImGui::BeginCombo("Locset", probe.locset_name.c_str())) {
                for (const auto& ls: state.locset_defs) {
                    auto nm = ls.name;
                    if (ImGui::Selectable(nm.c_str(), nm == probe.locset_name)) probe.locset_name = nm;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            gui_check_state(probe.state, probe.message);
            ImGui::Indent();
            ImGui::InputDouble("Frequency (Hz)", &probe.frequency);
            if (ImGui::BeginCombo("Variable", probe.variable.c_str())) {
                for (const auto& v: probe_variables) {
                    if (ImGui::Selectable(v.c_str(), v == probe.variable)) probe.variable = v;
                }
                ImGui::EndCombo();
            }
            gui_trash(probe.state);
            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void gui_place(gui_state& state) {
    if (ImGui::Begin("Placings")) {
        gui_probes(state);
    }
    ImGui::End();
}
