#include "gui_state.hpp"

#include <cmath>

#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/cable_cell_param.hpp>
#include <arborio/neurolucida.hpp>
#include <arborio/neuroml.hpp>
#include <arborio/swcio.hpp>
#include <arborio/cableio.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "utils.hpp"
#include "icons.hpp"
#include "events.hpp"
#include "loader.hpp"

extern float delta_phi;
extern float delta_gamma;
extern float delta_zoom;
extern float mouse_x;
extern float mouse_y;
extern glm::vec2 delta_pos;

namespace {
  inline void gui_read_morphology(gui_state& state, bool& open);
  inline void gui_tooltip(const std::string&);
  inline void gui_dir_view(file_chooser_state& state);
  inline void gui_debug(bool&);
  inline void gui_style(bool&);

  inline void gui_right_margin(float delta=40.0f) { ImGui::SameLine(ImGui::GetWindowWidth() - delta); }

  inline bool gui_tree(const std::string& label) {
    ImGui::AlignTextToFramePadding();
    return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
  }

  template<typename F> bool gui_tree_add(const std::string& label, F action) {
    auto open = gui_tree(label);
    gui_right_margin();
    if (ImGui::Button(icon_add)) action();
    return open;
  }

  struct with_item_width {
    with_item_width(float px) { ImGui::PushItemWidth(px); }
    ~with_item_width() { ImGui::PopItemWidth(); }
  };

  struct with_indent {
    with_indent(float px_=0.0f): px{px_} { ImGui::Indent(px); }
    ~with_indent() { ImGui::Unindent(px); }
    float px;
  };

  with_indent gui_tree_indent() { return { 24.0f - ImGui::GetStyle().IndentSpacing }; } // fix alignment under trees

  struct with_style {
    template<typename V> with_style(ImGuiStyleVar var, V val) { ImGui::PushStyleVar(var, val); }
    ~with_style() { ImGui::PopStyleVar(); }
  };

  struct with_id {
    with_id(size_t val)             { ImGui::PushID(val); }
    with_id(id_type val)            { ImGui::PushID(val.value); }
    with_id(const char* ptr)        { ImGui::PushID(ptr); }
    with_id(const std::string& str) { ImGui::PushID(str.c_str()); }
    ~with_id() { ImGui::PopID(); }
  };

  inline bool gui_select(const std::string& item, std::string& current) {
    if (ImGui::Selectable(item.c_str(), item == current)) {
      current = item;
      return true;
    }
    return false;
  }

  template<typename Container>
  inline void gui_choose(const std::string& lbl, std::string& current, const Container& items) {
    if (ImGui::BeginCombo(lbl.c_str(), current.c_str())) {
      for (const auto& item: items) gui_select(item, current);
      ImGui::EndCombo();
    }
  }

  inline bool gui_input_double(const std::string& lbl, double& v, const std::string& unit="", const std::string& fmt="%8g") {
    auto format = fmt;
    if (!unit.empty()) format += " " + unit;
    return ImGui::InputDouble(lbl.c_str(),& v, 0.0, 0.0, format.c_str(), ImGuiInputTextFlags_CharsScientific);
  }

  inline void gui_defaulted_double(const std::string& label, const std::string& unit, std::optional<double>& value, const double fallback) {
    auto tmp = value.value_or(fallback);
    if (gui_input_double(label, tmp, unit)) value = {tmp};
    gui_right_margin();
    if (ImGui::Button(icon_refresh)) value = {};
    gui_tooltip("Reset to default");
  }

  auto gui_defaulted_double(const std::string& label,
                            const std::string& unit,
                            std::optional<double>& value,
                            const std::optional<double>& fallback) {
    if (fallback)  return gui_defaulted_double(label, unit, value, fallback.value());
    throw std::runtime_error{""};
  }

  auto gui_defaulted_double(const std::string& label,
                            const std::string& unit,
                            std::optional<double>& value,
                            const std::optional<double>& fallback,
                            const std::optional<double>& fallback2) {
    if (fallback)  return gui_defaulted_double(label, unit, value, fallback.value());
    if (fallback2) return gui_defaulted_double(label, unit, value, fallback2.value());
    throw std::runtime_error{""};
  }

  inline void gui_tooltip(const std::string& message) {
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(message.c_str());
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }

  template<typename T>
  inline void gui_check_state(const loc_def<T>& def) {
    static std::unordered_map<def_state, const char*> tags{{def_state::empty, icon_question},
                                                           {def_state::error, icon_error},
                                                           {def_state::good,  icon_ok}};
    ImGui::Text("%s", tags[def.state]);
    gui_tooltip(def.message);
  }


  inline bool gui_toggle(const char *on, const char *off, bool& flag) {
    if (ImGui::Button(flag ? on : off)) {
      flag = !flag;
      return true;
    }
    return false;
  }

  inline bool gui_menu_item(const char* text, const char* icon) { return ImGui::MenuItem(fmt::format("{} {}", icon, text).c_str()); }

  inline void gui_save_acc(gui_state& state, bool& open) {
    ZoneScopedN(__FUNCTION__);
    with_id id{"writing acc"};
    ImGui::OpenPopup("Save");
    static std::vector<std::string> suffixes{"acc"};
    if (ImGui::BeginPopupModal("Save")) {
      gui_dir_view(state.acc_chooser);
      {
        with_item_width id{-210.0f};
        auto file = state.acc_chooser.file.filename().string();
        if (ImGui::InputText("File", &file)) state.acc_chooser.file = state.acc_chooser.file.replace_filename(file);
      }
      ImGui::SameLine();
      {
        with_item_width id{120.0f};
        auto lbl = state.acc_chooser.filter.value_or("all");
        if (ImGui::BeginCombo("Filter", lbl.c_str())) {
          if (ImGui::Selectable("all", "all" == lbl)) state.acc_chooser.filter = {};
          for (const auto& k: suffixes) {
            if (ImGui::Selectable(k.c_str(), k == lbl)) state.acc_chooser.filter = {k};
          }
          ImGui::EndCombo();
        }
      }
      auto ok = ImGui::Button("Save");
      ImGui::SameLine();
      auto ko = ImGui::Button("Cancel");

      if (ok) state.serialize(state.acc_chooser.file);
      if (ok || ko) open = false;
      ImGui::EndPopup();
    }
  }

  inline void gui_read_acc(gui_state& state, bool& open) {
    ZoneScopedN(__FUNCTION__);
    with_id id{"writing acc"};
    ImGui::OpenPopup("Save");
    static std::vector<std::string> suffixes{"acc"};
    if (ImGui::BeginPopupModal("Save")) {
      gui_dir_view(state.acc_chooser);
      {
        with_item_width id{120.0f};
        auto lbl = state.acc_chooser.filter.value_or("all");
        if (ImGui::BeginCombo("Filter", lbl.c_str())) {
          if (ImGui::Selectable("all", "all" == lbl)) state.acc_chooser.filter = {};
          for (const auto& k: suffixes) {
            if (ImGui::Selectable(k.c_str(), k == lbl)) state.acc_chooser.filter = {k};
          }
          ImGui::EndCombo();
        }
      }
      auto ok = ImGui::Button("Load");
      ImGui::SameLine();
      auto ko = ImGui::Button("Cancel");

      if (ok) state.deserialize(state.acc_chooser.file);
      if (ok || ko) open = false;
      ImGui::EndPopup();
    }
  }

  inline void gui_menu_bar(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    ImGui::BeginMainMenuBar();
    static auto open_morph_read = false;
    static auto open_acc_read = false;
    static auto open_acc_save = false;
    static auto open_debug      = false;
    static auto open_style      = false;
    if (ImGui::BeginMenu("File")) {
      ImGui::Text("%s Morphology", icon_branch);
      {
        with_indent indent;
        open_morph_read = gui_menu_item("Load", icon_load);
      }
      ImGui::Separator();
      ImGui::Text("%s Cable cell", icon_cell);
      {
        with_indent indent;
        open_acc_read = gui_menu_item("Load", icon_load);
        open_acc_save = gui_menu_item("Save", icon_save);
      }
      ImGui::Separator();
      state.shutdown_requested = gui_menu_item("Quit", "");
      ImGui::EndMenu();
    }
    gui_right_margin(60.0f);
    if (ImGui::BeginMenu("Help")) {
      open_debug = gui_menu_item("Metrics", icon_bug);
      open_style = gui_menu_item("Style",   icon_paint);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
    if (open_morph_read) gui_read_morphology(state, open_morph_read);
    if (open_acc_read) gui_read_acc(state, open_acc_read);
    if (open_acc_save) gui_save_acc(state, open_acc_save);
    if (open_debug)      gui_debug(open_debug);
    if (open_style)      gui_style(open_style);
  }

  inline void gui_main(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window
    // not dockable into, because it would be confusing to have two docking
    // targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse             |  ImGuiWindowFlags_NoResize
                   |  ImGuiWindowFlags_NoMove     |  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
      dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render
    // our background and handle the pass-thru hole, so we ask Begin() to not
    // render a background.
    if (dockspace_flags&  ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window
    // is collapsed). This is because we want to keep our DockSpace() active. If a
    // DockSpace() is inactive, all active windows docked into it will lose their
    // parent and become undocked. We cannot preserve the docking relationship
    // between an active window and an inactive docking, otherwise any change of
    // dockspace/settings would lead to windows being stuck in limbo and never
    // being visible.
    if (!opt_padding) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)   ImGui::PopStyleVar();
    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags&  ImGuiConfigFlags_DockingEnable) {
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    gui_menu_bar(state);
    ImGui::End();
  }

  inline void gui_dir_view(file_chooser_state& state) {
    // Draw the current path + show hidden
    {
      auto acc = std::filesystem::path{};
      if (ImGui::Button(icon_open_dir)) state.cwd = "/";
      for (const auto& part: state.cwd) {
        acc /= part;
        if ("/" == part) continue;
        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::Button(fmt::format("/ {}", part.c_str()).c_str())) state.cwd = acc;
      }
      gui_right_margin();
      gui_toggle(icon_show, icon_hide, state.show_hidden);
    }
    // Draw the current dir
    {
      ImGui::BeginChild("Files",
                        {-0.35f*ImGui::GetTextLineHeightWithSpacing(),
                         -3.00f*ImGui::GetTextLineHeightWithSpacing()},
                        true);

      std::vector<std::tuple<std::string, std::filesystem::path>> dirnames;
      std::vector<std::tuple<std::string, std::filesystem::path>> filenames;

      for (const auto& it: std::filesystem::directory_iterator(state.cwd)) {
        const auto& path = it.path();
        const auto& ext = path.extension();
        std::string fn = path.filename();
        if (fn.empty() || (!state.show_hidden && (fn.front() == '.'))) continue;
        if (it.is_directory()) dirnames.push_back({fn, path});
        if (state.filter && state.filter.value() != ext) continue;
        if (it.is_regular_file()) filenames.push_back({fn, path});
      }

      std::sort(filenames.begin(), filenames.end());
      std::sort(dirnames.begin(), dirnames.end());

      for (const auto& [dn, path]: dirnames) {
        auto lbl = fmt::format("{} {}", icon_folder, dn);
        ImGui::Selectable(lbl.c_str(), false);
        if (ImGui::IsItemHovered() && (ImGui::IsMouseDoubleClicked(0) || ImGui::IsKeyPressed(GLFW_KEY_ENTER))) {
          state.cwd = path;
          state.file.clear();
        }
      }
      for (const auto& [fn, path]: filenames) {
        if (ImGui::Selectable(fn.c_str(), path == state.file)) state.file = path;
      }
      ImGui::EndChild();
    }
  }

  inline void gui_read_morphology(gui_state& state, bool& open_file) {
    ZoneScopedN(__FUNCTION__);
    with_id id{"reading morphology"};
    ImGui::OpenPopup("Open");
    if (ImGui::BeginPopupModal("Open")) {
      gui_dir_view(state.file_chooser);
      auto suffix   = state.file_chooser.file.extension();
      auto suffixes = io::get_suffixes();
      auto flavors  = io::get_flavors(suffix);
      static std::string flavor = "";
      {
        with_item_width id{120.0f};
        auto lbl = state.file_chooser.filter.value_or("all");
        if (ImGui::BeginCombo("Filter", lbl.c_str())) {
          if (ImGui::Selectable("all", "all" == lbl)) state.file_chooser.filter = {};
          for (const auto& k: suffixes) {
            if (ImGui::Selectable(k.c_str(), k == lbl)) state.file_chooser.filter = {k};
          }
          ImGui::EndCombo();
        }
        if (!flavors.empty()) {
          if (flavors.end() == std::find(flavors.begin(), flavors.end(), flavor)) flavor = flavors.front();
          ImGui::SameLine();
          gui_choose("Flavor", flavor, flavors);
        } else {
          flavor = "";
        }
      }

      {
        auto loader = io::get_loader(suffix, flavor);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, loader.load ? 1.0f: 0.6f);
        auto do_load = ImGui::Button("Load");
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::Text("%s", loader.load ? icon_ok : icon_error);
        gui_tooltip(loader.message);
        ImGui::SameLine();
        open_file = !ImGui::Button("Cancel");
        {
          static std::string loader_error = "";
          if (do_load && loader.load) {
            try {
              auto result = loader.load.value()(state.file_chooser.file);
              state.reload(result);
              open_file = false;
            } catch (const arborio::swc_error& e) {
              loader_error = e.what();
            } catch (const arborio::neuroml_exception& e) {
              loader_error = e.what();
            }
          }
          if (ImGui::BeginPopupModal("Cannot Load Morphology")) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
            ImGui::TextUnformatted(loader_error.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::Button("Close")) {
              loader_error.clear();
              ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
          }
          if (!loader_error.empty()) {
            ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetFontSize() * 50.0f));
            ImGui::OpenPopup("Cannot Load Morphology");
          }
        }
      }
      ImGui::EndPopup();
    }
  }

  inline void gui_cell(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    if (ImGui::Begin("Cell")) {
      ImGui::BeginChild("Cell Render");
      auto size = ImGui::GetWindowSize(), win_pos = ImGui::GetWindowPos();
      state.view.size = to_glmvec(size);
      state.renderer.render(state.view,
                            state.render_regions.items, state.render_locsets.items,
                            {mouse_x - win_pos.x, size.y + win_pos.y - mouse_y});
      ImGui::Image(reinterpret_cast<ImTextureID>(state.renderer.cell.tex), size, ImVec2(0, 1), ImVec2(1, 0));

      if (ImGui::IsItemHovered()) {
        state.view.offset -= delta_pos;
        state.view.zoom    = std::clamp(state.view.zoom + delta_zoom, 1.0f, 45.0f);
        state.view.phi     = std::fmod(state.view.phi   + delta_phi   + 2*PI, 2*PI); // cyclic in [0, 2pi)
        state.view.gamma   = std::fmod(state.view.gamma + delta_gamma + 2*PI, 2*PI); // cyclic in [0, 2pi)
      }
      state.object = state.renderer.get_id();

      if (ImGui::BeginPopupContextWindow()) {
        ImGui::Text("%s Camera", icon_camera);
        {
          with_indent indent{};
          ImGui::InputFloat3("Target", &state.view.target[0]);
          ImGui::ColorEdit3("Background",& (state.renderer.cell.clear_color.x), ImGuiColorEditFlags_NoInputs);
          if (ImGui::BeginMenu(fmt::format("{} Snap", icon_locset).c_str())) {
            for (const auto& id: state.locsets) {
              const auto& ls = state.locset_defs[id];
              if (ls.state != def_state::good) continue;
              if (ImGui::BeginMenu(fmt::format("{} {}", icon_locset, ls.name).c_str())) {
                auto points = state.builder.make_points(ls.data.value());
                for (const auto& point: points) {
                  const auto lbl = fmt::format("({: 7.3f} {: 7.3f} {: 7.3f})", point.x, point.y, point.z);
                  if (ImGui::MenuItem(lbl.c_str())) {
                    state.view.offset = {0.0, 0.0};
                    state.view.target = (point - state.renderer.root)/state.renderer.rescale;
                  }
                }
                ImGui::EndMenu();
              }
            }

            ImGui::EndMenu();
          }

          if (gui_menu_item("Reset##camera", icon_refresh)) {
            state.view.offset = {0.0f, 0.0f};
            state.view.phi    = 0.0f;
            state.view.target = {0.0f, 0.0f, 0.0f};
            state.renderer.cell.clear_color = {214.0f/255, 214.0f/255, 214.0f/255};
          }
        }
        ImGui::Separator();
        ImGui::Text("%s Axes", icon_axes);
        static bool show_axes = state.renderer.axes.back().active;
        gui_toggle(icon_on, icon_off, show_axes);
        for (auto& ax: state.renderer.axes) ax.active = show_axes;
        static glm::vec3 axes_pos = {0, 0, 0};
        static float axes_ext = 0.1f;
        if (ImGui::InputFloat3("Position", &axes_pos[0])) {
          state.renderer.make_ruler(axes_pos, axes_ext);
        }
        if (ImGui::InputFloat("Size", &axes_ext, 0, 0, "%f µm")) {
          state.renderer.make_ruler(axes_pos, axes_ext);
        }

        ImGui::Separator();
        ImGui::Text("%s Model", icon_cell);
        {
          with_indent indent{};
          ImGui::SliderFloat("Theta", &state.view.theta, -PI, PI);
          ImGui::SliderFloat("Gamma", &state.view.gamma, -PI, PI);
          int tmp = state.renderer.n_faces;
          if (ImGui::DragInt("Frustrum Resolution", &tmp, 1.0f, 8, 64, "%d", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
            state.renderer.n_faces = tmp;
            state.renderer.load_geometry(state.builder.morph);
            for (const auto& rg: state.regions) state.update_region(rg);
          }
        }
        ImGui::Separator();
        if (gui_menu_item("Snapshot", icon_paint)) state.store_snapshot();
        ImGui::InputText("Output",& state.snapshot_path);
        ImGui::EndPopup();
      }
      ImGui::EndChild();
    }
    ImGui::End();
  }

  inline void gui_cell_info(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    if (ImGui::Begin("Info")) {
      ImGui::Text("%s Selection", icon_branch);
      if (state.object) {
        auto& object = state.object.value();
        ImGui::BulletText("Segment %u", object.data.id);
        {
          with_indent indent;
          auto
            px = object.data.prox.x, dx = object.data.dist.x, lx = dx - px,
            py = object.data.prox.y, dy = object.data.dist.y, ly = dy - py,
            pz = object.data.prox.z, dz = object.data.dist.z, lz = dz - pz;
          ImGui::BulletText("Extent (%.1f, %.1f, %.1f) -- (%.1f, %.1f, %.1f)", px, py, pz, dx, dy, dz);
          ImGui::BulletText("Radii  %g µm %g µm", object.data.prox.radius, object.data.dist.radius);
          ImGui::BulletText("Length %g µm", std::sqrt(lx*lx + ly*ly + lz*lz));
        }
        ImGui::BulletText("Branch %zu", object.branch);
        {
          with_indent indent;
          ImGui::BulletText("Segments");
          auto count = 0ul;
          for (const auto& [lo, hi]: *object.segment_ids) {
            ImGui::SameLine();
            if (lo == hi) {
              ImGui::Text("%zu", lo);
            } else {
              ImGui::Text("%zu-%zu", lo, hi);
            }
            count += hi - lo + 1;
          }
          ImGui::BulletText("Count %zu", count);
        }
        ImGui::BulletText("Regions");
        {
          for (const auto& region: state.segment_to_regions[object.data.id]) {
            ImGui::SameLine();
            ImGui::ColorButton("", to_imvec(state.render_regions[region].color));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", state.region_defs[region].name.c_str());
          }
        }
      }
    }
    ImGui::End();
  }

  template<typename Item>
  inline void gui_locdefs(const std::string& name,
                   const entity& ids,
                   component_unique<Item>& items,
                   component_unique<renderable>& renderables,
                   std::vector<event>& events) {
    ZoneScopedN(__FUNCTION__);
    with_id guard{name};
    auto open = gui_tree_add(name, [&](){ events.emplace_back(evt_add_locdef<Item>{}); });
    if (open) {
      with_item_width iw(120.0f);
      for (const auto& id: ids) {
        with_id guard{id};
        auto& render = renderables[id];
        auto& item   = items[id];
        auto open = gui_tree("");
        ImGui::SameLine();
        if (ImGui::InputText("##locdef-name",& item.name, ImGuiInputTextFlags_AutoSelectAll)) events.push_back(evt_upd_locdef<Item>{id});
        ImGui::SameLine();
        ImGui::ColorEdit3("##locdef-color",& render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        gui_toggle(icon_show, icon_hide, render.active);
        ImGui::SameLine();
        gui_check_state(item);

        gui_right_margin();
        if (ImGui::Button(icon_delete)) events.push_back(evt_del_locdef<Item>{id});
        if (open) {
          with_item_width iw(-50.0f);
          auto indent = gui_tree_indent();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
          if (ImGui::InputTextMultiline("##locdef-definition",& item.definition)) events.push_back(evt_upd_locdef<Item>{id});
          ImGui::PopTextWrapPos();
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_locations(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    if (ImGui::Begin(fmt::format("{} Locations", icon_location).c_str())) {
      gui_locdefs(fmt::format("{} Regions", icon_region), state.regions, state.region_defs, state.render_regions, state.events);
      ImGui::Separator();
      gui_locdefs(fmt::format("{} Locsets", icon_locset), state.locsets, state.locset_defs, state.render_locsets, state.events);
    }
    ImGui::End();
  }

  inline void gui_ion_settings(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    with_id guard{"ion-settings"};
    if (gui_tree(fmt::format("{} Regions", icon_region))) {
      for (const auto& region: state.regions) {
        with_id guard{region.value};
        auto name = state.region_defs[region].name;
        if (gui_tree(name)) {
          for (const auto& ion: state.ions) {
            with_id guard{ion.value};
            auto  name = state.ion_defs[ion].name;
            auto& parm = state.ion_par_defs[{region, ion}];
            auto  dflt = state.ion_defaults[ion];
            if (gui_tree(name)) {
              with_item_width width{120.0f};
              if (state.presets.ion_data.contains(name)) {
                auto preset = state.presets.ion_data[name];
                gui_defaulted_double("Int. Concentration", "mM", parm.Xi, dflt.Xi, preset.init_int_concentration);
                gui_defaulted_double("Ext. Concentration", "mM", parm.Xo, dflt.Xo, preset.init_ext_concentration);
                gui_defaulted_double("Reversal Potential", "mV", parm.Er, dflt.Er, preset.init_reversal_potential);
              } else {
                gui_defaulted_double("Int. Concentration", "mM", parm.Xi, dflt.Xi);
                gui_defaulted_double("Ext. Concentration", "mM", parm.Xo, dflt.Xo);
                gui_defaulted_double("Reversal Potential", "mV", parm.Er, dflt.Er);
              }
              ImGui::TreePop();
            }
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_ion_defaults(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    with_id guard{"ion-defaults"};
    auto open = gui_tree_add(fmt::format("{} Default", icon_default), [&]() { state.add_ion(); });
    if (open) {
      for (const auto& ion: state.ions) {
        with_id guard{ion.value};
        with_item_width item_width{120.0f};
        auto& definition = state.ion_defs[ion];
        auto open = gui_tree("##ion-tree");
        ImGui::SameLine();
        ImGui::InputText("##ion-name", & definition.name, ImGuiInputTextFlags_AutoSelectAll);
        gui_right_margin();
        if (ImGui::Button(icon_delete)) state.remove_ion(ion);
        if (open) {
          auto indent = gui_tree_indent();
          auto& defaults = state.ion_defaults[ion];
          ImGui::InputInt("Charge",& definition.charge);
          gui_input_double("Int. Concentration", defaults.Xi, "mM");
          gui_input_double("Ext. Concentration", defaults.Xo, "mM");
          {
            {
              with_item_width width{70.0f};
              gui_choose("##rev-pot-method", defaults.method, ion_default::methods);
            }
            ImGui::SameLine();
            {
              with_item_width width{40.0f};
              gui_input_double("Reversal Potential", defaults.Er, "mV");
            }
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_parameter_set(parameter_def& to_set, const arb::cable_cell_parameter_set& defaults) {
    ZoneScopedN(__FUNCTION__);
    with_item_width item_width{120.0f};
    gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.temperature_K);
    gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.init_membrane_potential);
    gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.axial_resistivity);
    gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.membrane_capacitance);
  }

  inline void gui_parameter_set(parameter_def& to_set, const parameter_def& defaults, const arb::cable_cell_parameter_set& fallback) {
    ZoneScopedN(__FUNCTION__);
    with_item_width item_width{120.0f};
    gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.TK, fallback.temperature_K);
    gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.Vm, fallback.init_membrane_potential);
    gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.RL, fallback.axial_resistivity);
    gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.Cm, fallback.membrane_capacitance);
  }

  const static std::unordered_map<std::string, arb::mechanism_catalogue>
  catalogues = {{"default", arb::global_default_catalogue()},
                {"allen",   arb::global_allen_catalogue()},
                {"BBP",     arb::global_bbp_catalogue()}};

  inline void make_mechanism(mechanism_def& data,
                             const std::string& cat_name, const std::string& name,
                             const std::unordered_map<std::string, double>& values={}) {
    data.name = fmt::format("{}/{}", cat_name, name);
    log_debug("Fetching cat {}", cat_name);
    auto cat = catalogues.at(cat_name);
    log_debug("Fetching mach {}", name);
    auto info = cat[name];
    data.globals.clear();
    data.parameters.clear();
    data.states.clear();
    log_debug("Setting values");
    for (const auto& [k, v]: info.globals)    data.globals[k]    = values.contains(k) ? values.at(k) : v.default_value;
    for (const auto& [k, v]: info.parameters) data.parameters[k] = values.contains(k) ? values.at(k) : v.default_value;
    for (const auto& [k, v]: info.state)      data.states[k]     = values.contains(k) ? values.at(k) : v.default_value;
  }

  inline bool gui_mechanism(mechanism_def& data) {
    ZoneScopedN(__FUNCTION__);
    auto open = gui_tree("##mechanism-tree");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##mechanism-choice", data.name.c_str())) {
      for (const auto& [cat_name, cat]: catalogues) {
        ImGui::Selectable(cat_name.c_str(), false);
        with_indent ind{};
        for (const auto& name: cat.mechanism_names()) {
          if (gui_select(fmt::format("  {}##{}", name, cat_name), data.name)) make_mechanism(data, cat_name, name);
        }
      }
      ImGui::EndCombo();
    }
    gui_right_margin();
    auto remove = ImGui::Button(icon_delete);
    if (open) {
      if (!data.globals.empty()) {
        ImGui::BulletText("Global Values");
        with_indent ind{};
        for (auto& [k, v]: data.globals) gui_input_double(k, v);
      }
      if (!data.states.empty()) {
        ImGui::BulletText("State Variables");
        with_indent ind{};
        for (auto& [k, v]: data.states) gui_input_double(k, v);
      }
      if (!data.parameters.empty()) {
        ImGui::BulletText("Parameters");
        with_indent ind{};
        for (auto& [k, v]: data.parameters) gui_input_double(k, v);
      }
      ImGui::TreePop();
    }
    return remove;
  }

  inline void gui_mechanisms(gui_state& state) {
    if (gui_tree(fmt::format("{} Mechanisms", icon_gears))) {
      for (const auto& region: state.regions) {
        with_id region_guard{region.value};
        auto name = state.region_defs[region].name;
        auto open = gui_tree_add(name, [&]() { state.add_mechanism(region); });
        if (open) {
          with_item_width width{120.0f};
          for (const auto& child: state.mechanisms.get_children(region)) {
            with_id mech_guard{child};
            auto rem = gui_mechanism(state.mechanisms[child]);
            if (rem) state.remove_mechanism(child);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_parameters(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    with_id id{"parameters"};
    if (ImGui::Begin(fmt::format("{} Parameters", icon_list).c_str())) {
      if (gui_tree(fmt::format("{} Cable Cell Properties", icon_sliders))) {
        with_id id{"properties"};
        if (gui_tree(fmt::format("{} Default", icon_default))) {
          gui_parameter_set(state.parameter_defaults, state.presets);
          ImGui::TreePop();
        }
        if (gui_tree(fmt::format("{} Regions", icon_region))) {
          for (const auto& region: state.regions) {
            with_id id{region.value};
            if (gui_tree(state.region_defs[region].name)) {
              gui_parameter_set(state.parameter_defs[region], state.parameter_defaults, state.presets);
              ImGui::TreePop();
            }
          }
          ImGui::TreePop();
        }
        ImGui::TreePop();
      }
      ImGui::Separator();
      if (gui_tree(fmt::format("{} Ion Settings", icon_ion))) {
        with_id id{"ion-settings"};
        gui_ion_defaults(state);
        gui_ion_settings(state);
        ImGui::TreePop();
      }
      ImGui::Separator();
      gui_mechanisms(state);
    }
    ImGui::End();
  }

  inline void gui_probes(gui_state& state) {
    auto open = gui_tree(fmt::format("{} Probes", icon_probe));
    if (open) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_probe(locset); });
        if (open) {
          for (const auto& probe: state.probes.get_children(locset)) {
            with_id id{probe};
            ImGui::Bullet();
            ImGui::SameLine();
            with_item_width width {180.f};
            auto& data = state.probes[probe];
            gui_input_double("Frequency", data.frequency, "Hz");
            gui_right_margin();
            if (ImGui::Button(icon_delete)) state.remove_probe(probe);
            with_indent indent{ImGui::GetTreeNodeToLabelSpacing()};
            gui_choose("Kind", data.kind, probe_def::kinds);
            if ((data.kind == "Membrane Current") || (data.kind == "Internal Concentration") || (data.kind == "External Concentration")) {
              if (ImGui::BeginCombo("Ion Species", data.variable.c_str())) {
                for (const auto& ion: state.ions) {
                  auto& name = state.ion_defs[ion].name;
                  if (ImGui::Selectable(name.c_str(), name == data.variable)) data.variable = name;
                }
                ImGui::EndCombo();
              }
            } if (data.kind == "Mechanism State") {
              if (ImGui::BeginCombo("State Variable", data.variable.c_str())) {
                std::vector<mechanism_def> mechs = state.mechanisms.items;
                std::sort(mechs.begin(), mechs.end());
                std::unique(mechs.begin(), mechs.end());
                for (const auto& mech: mechs) {
                  for (const auto& [k, v]: mech.states) {
                    auto label = fmt::format("{}/{}", mech.name, k);
                    if (ImGui::Selectable(label.c_str(), label == data.variable)) data.variable = label;
                  }
                }
                ImGui::EndCombo();
              }
            } else {
              // data.variable = "";
            }
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_detectors(gui_state& state) {
    if (gui_tree(fmt::format("{} Spike Detectors", icon_detector))) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_detector(locset); });
        if (open) {
          for (const auto& detector: state.detectors.get_children(locset)) {
            with_id id{detector};
            auto data = state.detectors[detector];
            ImGui::Bullet();
            ImGui::SameLine();
            with_item_width width {120.f};
            gui_input_double("Threshold", data.threshold, "mV");
            gui_right_margin();
            if (ImGui::Button(icon_delete)) state.remove_detector(detector);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_debug(bool& open) { ZoneScopedN(__FUNCTION__); ImGui::ShowMetricsWindow(&open); }

  inline void gui_style(bool& open) {
    ZoneScopedN(__FUNCTION__);
    if (ImGui::Begin("Style",& open)) ImGui::ShowStyleEditor();
    ImGui::End();
  }

  inline void gui_stimuli(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    if (gui_tree(fmt::format("{} Stimuli", icon_stimulus))) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_stimulus(locset); });
        if (open) {
          std::vector<float> values(state.sim.until/state.sim.dt, 0.0f);
          auto n = 0;
          for (const auto& stim: state.stimuli.get_children(locset)) {
            with_id id{stim};
            auto& data = state.stimuli[stim];
            with_item_width iw{160.0f};
            auto open = gui_tree(fmt::format("I Clamp {}", n));
            if (open) {
              bool clean = false;
              double z = 0.0, pi2 = 2*PI;
              ImGui::SliderScalar("Phase", ImGuiDataType_Double, &data.phase, &z, &pi2, "%.3f");
              gui_right_margin();
              if (ImGui::Button(icon_delete)) state.remove_stimulus(stim);
              gui_input_double("Frequency", data.frequency, "Hz");
              ImGui::Text("Envelope");
              ImGui::SameLine();
              if (ImGui::Button(icon_clean)) clean = true;
              gui_right_margin();
              if (ImGui::Button(icon_add)) data.envelope.emplace_back(data.envelope.back());
              if (data.envelope.empty()) data.envelope.emplace_back(0, 0);
              {
                with_item_width iw{80.0f};
                auto idx = 0ul;
                auto del = -1;
                for (auto& [t, u]: data.envelope) {
                  with_id id{idx};
                  ImGui::Bullet();
                  gui_input_double("##t", t, "ms");
                  ImGui::SameLine();
                  gui_input_double("##u", u, "nA");
                  ImGui::SameLine();
                  if (ImGui::Button(icon_delete)) del = idx;
                  idx++;
                }
                if (del >= 0) data.envelope.erase(data.envelope.begin() + del);
              }
              if (clean) {
                std::sort(data.envelope.begin(), data.envelope.end());
                auto last = std::unique(data.envelope.begin(), data.envelope.end());
                data.envelope.erase(last, data.envelope.end());
              }
              ImGui::TreePop();
            }
            auto t = 0.0, u = 0.0;
            auto envelope = data.envelope;
            auto it = envelope.begin();
            std::sort(envelope.begin(), envelope.end());
            for (auto ix = 0; ix < values.size(); ++ix) {
              auto t = ix*state.sim.dt;
              if ((it != envelope.end()) && (t > it->first)) u = it++->second;
              auto f = data.frequency ? std::sin(t*1e-3*data.frequency + data.phase) : 1.0f;
              values[ix] += u*f;
            }
            n++;
          }
          ImGui::Text("Preview");
          ImGui::PlotLines("", values.data(), values.size(), 0, nullptr, FLT_MAX, FLT_MAX, {0, 50});
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_simulation(gui_state& state) {
    ZoneScopedN(__FUNCTION__);
    if (ImGui::Begin(fmt::format("{} Simulation", icon_sim).c_str())) {
      if (gui_tree(fmt::format("{} Settings", icon_gear))) {
        with_item_width width(120.0f);
        gui_input_double("End time",  state.sim.until, "ms");
        gui_input_double("Time step", state.sim.dt,    "ms");
        ImGui::TreePop();
      }
      ImGui::Separator();
      gui_stimuli(state);
      ImGui::Separator();
      gui_probes(state);
      ImGui::Separator();
      gui_detectors(state);
    }
    ImGui::End();
  }
}

void gui_state::gui() {
  ZoneScopedN(__FUNCTION__);
  gui_main(*this);
  gui_locations(*this);
  gui_cell(*this);
  gui_cell_info(*this);
  gui_parameters(*this);
  gui_simulation(*this);
}

void gui_state::serialize(const std::filesystem::path& fn) {
  std::ofstream fd(fn);
  arb::decor decor{};
  for (const auto& id: locsets) {
    auto ls = locset_defs[id];
    auto locset = fmt::format("(locset \"{}\")", ls.name);
    if (!ls.data) continue;
    for (const auto child: stimuli.get_children(id)) {
      auto item = stimuli[child];
      arb::i_clamp i_clamp;
      i_clamp.frequency = item.frequency;
      i_clamp.phase     = item.phase;
      for (const auto& [t, i]: item.envelope) i_clamp.envelope.emplace_back(arb::i_clamp::envelope_point{t, i});
      decor.place(locset, i_clamp);
    }
    for (const auto child: detectors.get_children(id)) {
      auto item = detectors[child];
      decor.place(locset, arb::threshold_detector{item.threshold});
    }

  }

  auto param = parameter_defaults;
  // if (param.RL) decor.set_default(arb::axial_resistivity{param.RL.value()});
  // if (param.Cm) decor.set_default(arb::membrane_capacitance{param.Cm.value()});
  // if (param.TK) decor.set_default(arb::temperature_K{param.TK.value()});
  // if (param.Vm) decor.set_default(arb::init_membrane_potential{param.Vm.value()});
  for (const auto& ion: ions) {
    const auto& data = ion_defaults[ion];
    const auto& name = ion_defs[ion];
    std::string meth;
    if (data.method == "const.") {
    } else if (data.method == "Nernst") {
      decor.set_default(arb::ion_reversal_potential_method{"default/nernst"});
    }
    // decor.set_default(arb::init_int_concentration{data.Xi});
    // decor.set_default(arb::init_ext_concentration{data.Xo});
    // decor.set_default(arb::init_reversal_potential{data.Er});
  }

  for (const auto& id: regions) {
    auto rg = region_defs[id];
    if (!rg.data) continue;
    auto region = fmt::format("(region \"{}\")",  rg.name);
    auto param  = parameter_defs[id];
    // if (param.RL) decor.paint(region, arb::axial_resistivity{param.RL.value()});
    // if (param.Cm) decor.paint(region, arb::membrane_capacitance{param.Cm.value()});
    // if (param.TK) decor.paint(region, arb::temperature_K{param.TK.value()});
    // if (param.Vm) decor.paint(region, arb::init_membrane_potential{param.Vm.value()});

    for (const auto& ion: ions) {
      const auto& data = ion_par_defs[{id, ion}];
      const auto& name = ion_defs[ion];
      // decor.paint(region, arb::init_int_concentration{data.Xi});
      // decor.paint(region, arb::init_ext_concentration{data.Xo});
      // decor.paint(region, arb::init_reversal_potential{data.Er});
    }

    for (const auto child: mechanisms.get_children(id)) {
      auto item = mechanisms[child];
      auto mech = arb::mechanism_desc(item.name);
      for(const auto& [k, v]: item.parameters) mech.set(k, v);
      for(const auto& [k, v]: item.globals)    mech.set(k, v);
      for(const auto& [k, v]: item.states)     mech.set(k, v);
      decor.paint(region, mech);
    }
  }
  auto cell = arb::cable_cell(builder.morph, builder.labels, decor);
  arborio::write_component(fd, cell);
}

void gui_state::deserialize(const std::filesystem::path& fn) {
  std::ifstream fd(fn);
  log_debug("Reading ACC from {}", fn.string());
  auto thing = arborio::parse_component(fd);
  if (!thing.has_value()) throw thing.error();

  struct ls_visitor {
    gui_state* state;
    id_type locset;

    ls_visitor(gui_state* s, const arb::locset& l): state{s} {
      auto ls = to_string(l);
      auto res = std::find_if(state->locsets.begin(), state->locsets.end(),
                              [&](const auto& id) {
                                auto it = state->locset_defs[id];
                                auto nm = fmt::format("(locset \"{}\")", it.name);
                                return ((it.definition == ls) || (nm == ls));
                              });
      if (res == state->locsets.end()) log_error("Unknown locset");
      locset = *res;
    }

    void operator()(const arb::threshold_detector& t) {
      auto id = state->detectors.add(locset);
      state->detectors[id].threshold = t.threshold;
    }
    void operator()(const arb::i_clamp& t)            {
      auto id        = state->stimuli.add(locset);
      auto& data     = state->stimuli[id];
      data.frequency = t.frequency;
      data.phase     = t.phase;
      for (const auto& [k, v]: t.envelope) data.envelope.emplace_back(k, v);
    }
    void operator()(const arb::mechanism_desc& t)     { log_error("Cannot handle mech."); }
    void operator()(const arb::gap_junction_site& t)  { log_error("Cannot handle GJ."); }
  };

  struct rg_visitor {
    gui_state* state;
    id_type region;

    rg_visitor(gui_state* s, const arb::region& l): state{s} {
      auto ls = to_string(l);
      auto res = std::find_if(state->regions.begin(), state->regions.end(),
                              [&](const auto& id) {
                                auto it = state->region_defs[id];
                                auto nm = fmt::format("(region \"{}\")", it.name);
                                return ((it.definition == ls) || (nm == ls));
                              });
      if (res == state->regions.end()) log_error("Unknown region");
      region = *res;
    }

    void operator()(const arb::init_membrane_potential& t) { state->parameter_defs[region].Vm = t.value; }
    void operator()(const arb::axial_resistivity& t)       { state->parameter_defs[region].RL = t.value; }
    void operator()(const arb::temperature_K& t)           { state->parameter_defs[region].TK = t.value; }
    void operator()(const arb::membrane_capacitance& t)    { state->parameter_defs[region].Cm = t.value; }
    void operator()(const arb::init_int_concentration& t) {
      auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                              [&](const auto& id) { return state->ion_defs[id].name == t.ion; });
      if (ion == state->ions.end()) log_error("Unknown ion");
      state->ion_par_defs[{region, *ion}].Xi = t.value;
    }
    void operator()(const arb::init_ext_concentration& t) {
      auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                              [&](const auto& id) { return state->ion_defs[id].name == t.ion; });
      if (ion == state->ions.end()) log_error("Unknown ion");
      state->ion_par_defs[{region, *ion}].Xo = t.value;
    }
    void operator()(const arb::init_reversal_potential& t) {
      auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                              [&](const auto& id) { return state->ion_defs[id].name == t.ion; });
      if (ion == state->ions.end()) log_error("Unknown ion");
      state->ion_par_defs[{region, *ion}].Er = t.value;
    }
    void operator()(const arb::mechanism_desc& t) {
      auto id    = state->mechanisms.add(region);
      auto& data = state->mechanisms[id];
      auto cat   = t.name();
      auto name  = t.name();
      { auto sep = std::find(name.begin(), name.end(), '/'); name.erase(name.begin(), sep + 1); }
      { auto sep = std::find(cat.begin(),  cat.end(),  '/'); cat.erase(sep, cat.end()); }
      log_debug("Loading mech {} from cat {} ({})", name, cat, t.name());
      make_mechanism(data, cat, name, t.values());
    }
  };

  struct df_visitor {
    gui_state* state;

    void operator()(const arb::init_membrane_potential& t)       { log_error("Cannot handle this"); }
    void operator()(const arb::axial_resistivity& t)             { log_error("Cannot handle this"); }
    void operator()(const arb::temperature_K& t)                 { log_error("Cannot handle this"); }
    void operator()(const arb::membrane_capacitance& t)          { log_error("Cannot handle this"); }
    void operator()(const arb::init_int_concentration& t)        { log_error("Cannot handle this"); }
    void operator()(const arb::init_ext_concentration& t)        { log_error("Cannot handle this"); }
    void operator()(const arb::init_reversal_potential& t)       { log_error("Cannot handle this"); }
    void operator()(const arb::ion_reversal_potential_method& t) { log_error("Cannot handle this"); }
    void operator()(const arb::cv_policy& t)                     { log_error("Cannot handle this"); }
  };

  struct acc_visitor {
    gui_state* state;

    void operator()(const arb::morphology& m) {
      log_debug("Morphology from ACC");
      arb::morphology morph = m;
      std::vector<std::pair<std::string, std::string>> regions;
      std::vector<std::pair<std::string, std::string>> locsets;
      arb::decor decor;
      state->reload({morph, regions, locsets});
    }
    void operator()(const arb::label_dict& l) {
      log_debug("Label dict from ACC");
      arb::morphology morph;
      std::vector<std::pair<std::string, std::string>> regions;
      std::vector<std::pair<std::string, std::string>> locsets;
      arb::decor decor;
      for (const auto& [k, v]: l.regions()) regions.emplace_back(k, to_string(v));
      for (const auto& [k, v]: l.locsets()) locsets.emplace_back(k, to_string(v));
      state->reload({morph, regions, locsets});
    }
    void operator()(const arb::decor& d) {
      log_debug("Decor from ACC");
      arb::decor decor;
    }
    void operator()(const arb::cable_cell& c) {
      log_debug("Cable cell from ACC");
      arb::morphology morph;
      std::vector<std::pair<std::string, std::string>> regions;
      std::vector<std::pair<std::string, std::string>> locsets;
      for (const auto& [k, v]: c.labels().regions()) regions.emplace_back(k, to_string(v));
      for (const auto& [k, v]: c.labels().locsets()) locsets.emplace_back(k, to_string(v));
      state->reload({c.morphology(), regions, locsets});
      state->update(); // process events here
      arb::decor decor = c.decorations();
      for (const auto& [k, v]: decor.placements()) std::visit(ls_visitor{state, to_string(k)}, v);
      for (const auto& [k, v]: decor.paintings())  std::visit(rg_visitor{state, to_string(k)}, v);
      auto defaults = decor.defaults();
      state->parameter_defaults.Cm = defaults.membrane_capacitance;
      state->parameter_defaults.RL = defaults.axial_resistivity;
      state->parameter_defaults.TK = defaults.temperature_K;
      state->parameter_defaults.Vm = defaults.init_membrane_potential;
      for (const auto& [k, v]: defaults.ion_data) {
        auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                                [&, name=k](const auto& id) { return state->ion_defs[id].name == name; });
        if (ion == state->ions.end()) log_error("Unknown ion");
        auto& data = state->ion_defaults[*ion];
        if (v.init_reversal_potential) data.Er = v.init_reversal_potential.value();
        if (v.init_int_concentration)  data.Xi = v.init_int_concentration.value();
        if (v.init_ext_concentration)  data.Xo = v.init_ext_concentration.value();
      }
      for (const auto& [k, v]: defaults.reversal_potential_method) {
        auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                                [&, name=k](const auto& id) { return state->ion_defs[id].name == name; });
        if (ion == state->ions.end()) log_error("Unknown ion");
        state->ion_defaults[*ion].method = v.name();
      }
    }
  };

  std::visit(acc_visitor{this}, thing.value().component);
}
gui_state::gui_state(): builder{} { reset(); }

void gui_state::reset() {
  ZoneScopedN(__FUNCTION__);
  locsets.clear();
  regions.clear();
  render_regions.clear();
  render_locsets.clear();
  locset_defs.clear();
  region_defs.clear();
  ions.clear();
  ion_defs.clear();
  probes.clear();
  detectors.clear();
  ion_defaults.clear();
  mechanisms.clear();
  segment_to_regions.clear();

  const static std::vector<std::pair<std::string, int>> species{{"na", 1}, {"k", 1}, {"ca", 2}};
  for (const auto& [k, v]: species) add_ion(k, v);
}

void gui_state::reload(const io::loaded_morphology& result) {
  ZoneScopedN(__FUNCTION__);
  reset();
  builder = cell_builder{result.morph};
  renderer.load_geometry(result.morph);
  for (const auto& [k, v]: result.regions) add_region(k, v);
  for (const auto& [k, v]: result.locsets) add_locset(k, v);
}

void gui_state::update() {
  ZoneScopedN("gui_state::update()");
  struct event_visitor {
    gui_state* state;

    event_visitor(gui_state* state_): state{state_} {}

    void operator()(const evt_add_locdef<ls_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto ls = state->locsets.add();
      state->locset_defs.add(ls, {c.name.empty() ? fmt::format("Locset {}", ls.value) : c.name, c.definition});
      state->render_locsets.add(ls);
      state->render_locsets[ls].color = next_color();
      state->update_locset(ls);
    }
    void operator()(const evt_upd_locdef<ls_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto& def = state->locset_defs[c.id];
      auto& rnd = state->render_locsets[c.id];
      def.update();
      if (def.state == def_state::good) {
        log_info("Making markers for locset {} '{}'", def.name, def.definition);
        try {
          auto points = state->builder.make_points(def.data.value());
          state->renderer.make_marker(points, rnd);
        } catch (const arb::arbor_exception& e) {
          def.set_error(e.what()); rnd.active = false;
        }
      }
      state->builder.make_label_dict(state->locset_defs.items, state->region_defs.items);
    }
    void operator()(const evt_del_locdef<ls_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto id = c.id;
      log_debug("Erasing locset {}", id.value);
      state->render_locsets.del(id);
      state->locset_defs.del(id);
      state->probes.del_children(id);
      state->detectors.del_children(id);
      state->locsets.del(id);
      state->builder.make_label_dict(state->locset_defs.items, state->region_defs.items);
    }
    void operator()(const evt_add_locdef<rg_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto id = state->regions.add();
      state->region_defs.add(id, {c.name.empty() ? fmt::format("Region {}", id.value) : c.name, c.definition});
      state->parameter_defs.add(id);
      state->render_regions.add(id);
      state->render_regions[id].color = next_color();
      for (const auto& ion: state->ions) state->ion_par_defs.add(id, ion);
      state->update_region(id);
    }
    void operator()(const evt_upd_locdef<rg_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto& def = state->region_defs[c.id];
      auto& rnd = state->render_regions[c.id];
      for(auto& [segment, regions]: state->segment_to_regions) {
        regions.erase(c.id);
      }
      def.update();
      if (def.state == def_state::good) {
        log_info("Making frustrums for region {} '{}'", def.name, def.definition);
        try {
          auto segments = state->builder.make_segments(def.data.value());
          for (const auto& segment: segments) {
            const auto cached = state->renderer.segments[state->renderer.id_to_index[segment.id]];
            if ((cached.dist != segment.prox) && (cached.prox != segment.dist)) {
              state->segment_to_regions[segment.id].insert(c.id);
            }
          }
          state->renderer.make_region(segments, rnd);
        } catch (const arb::arbor_exception& e) {
          def.set_error(e.what()); rnd.active = false;
        }
      }
      state->builder.make_label_dict(state->locset_defs.items, state->region_defs.items);
    }
    void operator()(const evt_del_locdef<rg_def>& c) {
      ZoneScopedN(__FUNCTION__);
      auto id = c.id;
      state->render_regions.del(id);
      state->region_defs.del(id);
      state->parameter_defs.del(id);
      state->ion_par_defs.del_by_1st(id);
      state->mechanisms.del_children(id);
      // TODO This is quite expensive ... see if we can keep it this way
      for(auto& [segment, regions]: state->segment_to_regions) regions.erase(id);
      state->regions.del(id);
      state->builder.make_label_dict(state->locset_defs.items, state->region_defs.items);
    }
    void operator()(const evt_add_ion& c) {
      ZoneScopedN(__FUNCTION__);
      auto id = state->ions.add();
      state->ion_defs.add(id, {c.name.empty() ? fmt::format("Ion {}", id.value) : c.name, c.charge});
      state->ion_defaults.add(id);
      for (const auto& region: state->regions) state->ion_par_defs.add(region, id);
    }
    void operator()(const evt_del_ion& c) {
      ZoneScopedN(__FUNCTION__);
      auto id = c.id;
      state->ion_defs.del(id);
      state->ion_defaults.del(id);
      state->ion_par_defs.del_by_2nd(id);
      state->ions.del(id);
    }
    void operator()(const evt_add_mechanism& c) { ZoneScopedN(__FUNCTION__); state->mechanisms.add(c.region); }
    void operator()(const evt_del_mechanism& c) { ZoneScopedN(__FUNCTION__); state->mechanisms.del(c.id); }
    void operator()(const evt_add_detector& c)  { ZoneScopedN(__FUNCTION__); state->detectors.add(c.locset); }
    void operator()(const evt_del_detector& c)  { ZoneScopedN(__FUNCTION__); state->detectors.del(c.id); }
    void operator()(const evt_add_probe& c)     { ZoneScopedN(__FUNCTION__); state->probes.add(c.locset); }
    void operator()(const evt_del_probe& c)     { ZoneScopedN(__FUNCTION__); state->probes.del(c.id); }
    void operator()(const evt_add_stimulus& c)  { ZoneScopedN(__FUNCTION__); state->stimuli.add(c.locset); }
    void operator()(const evt_del_stimulus& c)  { ZoneScopedN(__FUNCTION__); state->stimuli.del(c.id); }
  };

  while (!events.empty()) {
    auto evt = events.back();
    events.pop_back();
    std::visit(event_visitor{this}, evt);
  }
}

bool gui_state::store_snapshot() {
  auto w = renderer.cell.width;
  auto h = renderer.cell.height;
  auto c = 3;
  auto pixels = std::vector<unsigned char>(c*w*h);
  glBindFramebuffer(GL_FRAMEBUFFER, renderer.cell.post_fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  auto rc = stbi_write_png(snapshot_path.c_str(), w, h, c, pixels.data() + c*w*(h - 1), -c*w);
  return !!rc;
}
