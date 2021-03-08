#include "gui_state.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "utils.hpp"
#include "icons.hpp"
#include "events.hpp"
#include "loader.hpp"

extern float delta_phi;
extern float delta_zoom;
extern float mouse_x;
extern float mouse_y;
extern glm::vec2 delta_pos;

gui_state::gui_state(): builder{} { reset(); }

void gui_state::reset() {
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

  static std::vector<std::pair<std::string, int>> species{{"na", 1}, {"k", 1}, {"ca", 2}};
  for (const auto &[k, v]: species) add_ion(k, v);
}

void gui_state::reload(const io::loaded_morphology& result) {
  reset();
  builder  = cell_builder{result.morph};
  renderer = geometry{result.morph};
  for (const auto& [k, v]: result.regions) add_region(k, v);
  for (const auto& [k, v]: result.locsets) add_locset(k, v);
}

template <typename D> void update_def(D &def) {
  if (def.definition.empty() || !def.definition[0]) {
    def.data = {};
    def.empty();
  } else {
    try {
      def.data = {def.definition};
      def.good();
    } catch (const arb::label_parse_error &e) {
      def.data = {};
      std::string m = e.what();
      auto colon = m.find(':') + 1;
      colon = m.find(':', colon) + 1;
      def.error(m.substr(colon, m.size() - 1));
    }
  }
}

void def_set_renderable(geometry &renderer,
                        cell_builder &builder,
                        renderable &render,
                        ls_def &def) {
  render.active = false;
  if (!def.data || (def.state != def_state::good)) return;
  log_info("Making markers for locset {} '{}'", def.name, def.definition);
  try {
    auto points = builder.make_points(def.data.value());
    render = renderer.make_marker(points, render.color);
  } catch (arb::morphology_error &e) {
    def.error(e.what());
  }
}

void def_set_renderable(geometry &renderer,
                        cell_builder &builder,
                        renderable &render,
                        reg_def &def) {
  render.active = false;
  if (!def.data || (def.state != def_state::good)) return;
  log_info("Making frustrums for region {} '{}'", def.name, def.definition);
  try {
    auto points = builder.make_segments(def.data.value());
    render = renderer.make_region(points, render.color);
  } catch (arb::morphology_error &e) {
    def.error(e.what());
  }
}

void gui_state::update() {
  struct event_visitor {
    gui_state* state;

    event_visitor(gui_state* state_): state{state_} {}

    void operator()(const evt_add_locdef<ls_def>& c) {
      auto ls = state->locsets.add();
      state->locset_defs.add(ls, {c.name.empty() ? fmt::format("Locset {}", ls.value) : c.name, c.definition});
      state->render_locsets.add(ls);
      state->update_locset(ls);
    }
    void operator()(const evt_upd_locdef<ls_def>& c) {
      auto& def = state->locset_defs[c.id];
      auto& red = state->render_locsets[c.id];
      update_def(def);
      def_set_renderable(state->renderer, state->builder, red, def);
    }
    void operator()(const evt_del_locdef<ls_def>& c) {
      auto id = c.id;
      log_debug("Erasing locset {}", id.value);
      state->render_locsets.del(id);
      state->locset_defs.del(id);
      state->probes.del_children(id);
      state->detectors.del_children(id);
      state->locsets.del(id);
    }
    void operator()(const evt_add_locdef<reg_def>& c) {
      auto id = state->regions.add();
      state->region_defs.add(id, {c.name.empty() ? fmt::format("Region {}", id.value) : c.name, c.definition});
      state->parameter_defs.add(id);
      state->render_regions.add(id);
      for (const auto& ion: state->ions) state->ion_par_defs.add(id, ion);
      state->update_region(id);
    }
    void operator()(const evt_upd_locdef<reg_def>& c) {
      auto& def = state->region_defs[c.id];
      auto& red = state->render_regions[c.id];
      update_def(def);
      def_set_renderable(state->renderer, state->builder, red, def);
    }
    void operator()(const evt_del_locdef<reg_def>& c) {
      auto id = c.id;
      state->render_regions.del(id);
      state->region_defs.del(id);
      state->parameter_defs.del(id);
      state->ion_par_defs.del_by_1st(id);
      state->mechanisms.del_children(id);
      state->regions.del(id);
    }
    void operator()(const evt_add_ion& c) {
      auto id = state->ions.add();
      state->ion_defs.add(id, {c.name.empty() ? fmt::format("Ion {}", id.value) : c.name, c.charge});
      state->ion_defaults.add(id);
      for (const auto& region: state->regions) state->ion_par_defs.add(region, id);
    }
    void operator()(const evt_del_ion& c) {
      auto id = c.id;
      state->ion_defs.del(id);
      state->ion_defaults.del(id);
      state->ion_par_defs.del_by_2nd(id);
      state->ions.del(id);
    }
    void operator()(const evt_add_mechanism& c) { state->mechanisms.add(c.region); }
    void operator()(const evt_del_mechanism& c) { state->mechanisms.del(c.id); }
    void operator()(const evt_add_detector& c)  { state->detectors.add(c.locset); }
    void operator()(const evt_del_detector& c)  { state->detectors.del(c.id); }
    void operator()(const evt_add_probe& c)     { state->probes.add(c.locset); }
    void operator()(const evt_del_probe& c)     { state->probes.del(c.id); }
  };

  while (!events.empty()) {
    auto evt = events.back();
    events.pop_back();
    std::visit(event_visitor{this}, evt);
  }
}

void gui_right_margin() { ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f); }

bool gui_tree(const std::string& label) {
  ImGui::AlignTextToFramePadding();
  return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
}

template<typename F>
bool gui_tree_add(const std::string& label, F action) {
  auto open = gui_tree(label);
  gui_right_margin();
  if (ImGui::Button(icon_add)) action();
  return open;
}

void gui_menu_bar(gui_state &state);
void gui_read_morphology(gui_state &state, bool &open);
void gui_tooltip(const std::string &);
void gui_check_state(def_state);
void gui_debug(bool &);
void gui_style(bool &);

struct with_item_width {
  with_item_width(float px) { ImGui::PushItemWidth(px); }
  ~with_item_width() { ImGui::PopItemWidth(); }
};

struct with_indent {
  with_indent(float px_=0.0f): px{px_} { ImGui::Indent(px); }
  ~with_indent() { ImGui::Unindent(px); }
  float px;
};

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

bool gui_select(const std::string& item, std::string& current) {
  if (ImGui::Selectable(item.c_str(), item == current)) {
    current = item;
    return true;
  }
  return false;
}

template<typename Container>
void gui_choose(const std::string& lbl, std::string& current, const Container& items) {
  if (ImGui::BeginCombo(lbl.c_str(), current.c_str())) {
    for (const auto &item: items) gui_select(item, current);
    ImGui::EndCombo();
  }
}

bool gui_input_double(const std::string& lbl, double& v, const std::string& unit="", const std::string& fmt="% 8.3f") {
  auto format = fmt;
  if (!unit.empty()) format += " " + unit;
  return ImGui::InputDouble(lbl.c_str(), &v, 0.0, 0.0, format.c_str(), ImGuiInputTextFlags_CharsScientific);
}

void gui_defaulted_double(const std::string& label, const std::string& unit, std::optional<double>& value, const double fallback) {
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

void gui_tooltip(const std::string &message) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(message.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void gui_check_state(const definition &def) {
  auto tag = std::unordered_map<def_state, const char*>{{def_state::empty, icon_question},
                                                        {def_state::error, icon_error},
                                                        {def_state::good,  icon_ok}}[def.state];
  ImGui::Text("%s", tag);
  gui_tooltip(def.message);
}

void gui_toggle(const char *on, const char *off, bool &flag) { if (ImGui::Button(flag ? on : off)) flag = !flag; }

void gui_main(gui_state &state) {
  static bool opt_fullscreen = true;
  static bool opt_padding = false;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window
  // not dockable into, because it would be confusing to have two docking
  // targets within each others.
  ImGuiWindowFlags window_flags =
    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen) {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
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
  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

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
  ImGuiIO &io = ImGui::GetIO();

  // Swap Space and Enter key bindings
  // ImGuiKey_Space is used to "activate" menu items,
  // ImGuiKey_Enter is more intuitive.
  io.KeyMap[ImGuiKey_Space] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_SPACE;

  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
  }
  gui_menu_bar(state);
  ImGui::End();
}

void gui_save_decoration(gui_state &state, bool &open) {
  state.serialize("test");
  open = false;
}

void gui_read_decoration(gui_state &state, bool &open) {
  state.deserialize("test");
  open = false;
}

bool gui_menu_item(const char* text, const char* icon) {
  return ImGui::MenuItem(fmt::format("{} {}", icon, text).c_str());
}


void gui_menu_bar(gui_state &state) {
  ImGui::BeginMenuBar();
  static auto open_morph_read = false;
  static auto open_decor_read = false;
  static auto open_decor_save = false;
  static auto open_debug      = false;
  static auto open_style      = false;
  if (ImGui::BeginMenu("File")) {
    open_morph_read = gui_menu_item("Load morphology", icon_load);
    ImGui::Separator();
    open_decor_read = gui_menu_item("Load decoration", icon_load);
    open_decor_save = gui_menu_item("Save decoration", icon_save);
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Help")) {
    open_debug = gui_menu_item("Metrics", icon_bug);
    open_style = gui_menu_item("Style",   icon_paint);
    ImGui::EndMenu();
  }
  ImGui::EndMenuBar();
  if (open_morph_read) gui_read_morphology(state, open_morph_read);
  if (open_decor_read) gui_read_decoration(state, open_decor_read);
  if (open_decor_save) gui_save_decoration(state, open_decor_save);
  if (open_debug)      gui_debug(open_debug);
  if (open_style)      gui_style(open_style);
}

void gui_dir_view(file_chooser_state &state) {
  // Draw the current path + show hidden
  {
    auto acc = std::filesystem::path{};
    if (ImGui::Button(icon_open_dir)) state.cwd = "/";
    for (const auto &part: state.cwd) {
      acc /= part;
      if ("/" == part) continue;
      ImGui::SameLine(0.0f, 0.0f);
      if (ImGui::Button(fmt::format("/ {}", part.c_str()).c_str())) state.cwd = acc;
    }
    gui_right_margin();
    gui_toggle(icon_show, icon_hide, state.show_hidden);
    // log_debug("Show hidden files: {}", state.show_hidden);
  }
  // Draw the current dir
  {
    ImGui::BeginChild("Files",
                      {-0.35f*ImGui::GetTextLineHeightWithSpacing(),
                       -3.00f*ImGui::GetTextLineHeightWithSpacing()},
                      true);

    std::vector<std::tuple<std::string, std::filesystem::path>> dirnames;
    std::vector<std::tuple<std::string, std::filesystem::path>> filenames;

    for (const auto &it: std::filesystem::directory_iterator(state.cwd)) {
      const auto &path = it.path();
      const auto &ext = path.extension();
      std::string fn = path.filename();
      if (fn.empty() || (!state.show_hidden && (fn.front() == '.'))) continue;
      if (it.is_directory()) dirnames.push_back({fn, path});
      if (state.filter && state.filter.value() != ext) continue;
      if (it.is_regular_file()) filenames.push_back({fn, path});
    }

    std::sort(filenames.begin(), filenames.end());
    std::sort(dirnames.begin(), dirnames.end());

    for (const auto &[dn, path]: dirnames) {
      auto lbl = fmt::format("{} {}", icon_folder, dn);
      ImGui::Selectable(lbl.c_str(), false);
      if (ImGui::IsItemHovered() && (ImGui::IsMouseDoubleClicked(0) ||
                                     ImGui::IsKeyPressed(GLFW_KEY_ENTER))) {
        state.cwd = path;
        state.file.clear();
      }
    }
    for (const auto &[fn, path]: filenames) {
      if (ImGui::Selectable(fn.c_str(), path == state.file)) state.file = path;
    }
    ImGui::EndChild();
  }
}

void gui_read_morphology(gui_state &state, bool &open_file) {
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
        for (const auto &k: suffixes) {
          if (ImGui::Selectable(k.c_str(), k == lbl)) state.file_chooser.filter = {k};
        }
        ImGui::EndCombo();
      }
      if (flavors) {
        auto fs = flavors.value();
        if (fs.end() == std::find(fs.begin(), fs.end(), flavor)) flavor = fs.front();
        ImGui::SameLine();
        gui_choose("Flavor", flavor, fs);
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
          } catch (arborio::swc_error &e) {
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

void gui_cell(gui_state &state) {
  if (ImGui::Begin("Cell")) {
    ImGui::BeginChild("Cell Render");
    auto size = ImGui::GetWindowSize();
    auto pos  = ImGui::GetWindowPos();

    state.renderer.render(state.view, to_glmvec(size), state.render_regions.items, state.render_locsets.items);

    ImGui::Image((ImTextureID) state.renderer.tex, size, ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::IsItemHovered()) {
      state.view.offset -= delta_pos;
      state.view.zoom    = std::clamp(state.view.zoom + delta_zoom, 1.0f, 45.0f);
      state.view.phi     = std::fmod(state.view.phi + delta_phi + 2*PI, 2*PI);    // cyclic in [0, 2pi)
    }

    if (ImGui::BeginPopupContextWindow()) {
      ImGui::Text("%s Camera", icon_camera);
      {
        with_indent indent{};
        if (gui_menu_item("Reset", icon_refresh)) {
          state.view.offset = {0.0, 0.0};
          state.renderer.target = {0.0f, 0.0f, 0.0f};
        }
        if (ImGui::BeginMenu(fmt::format("{} Snap", icon_locset).c_str())) {
          for (const auto &id: state.locsets) {
            const auto& ls = state.locset_defs[id];
            if (ls.state != def_state::good) continue;
            if (ImGui::BeginMenu(fmt::format("{} {}", icon_locset, ls.name).c_str())) {
              auto points = state.builder.make_points(ls.data.value());
              for (const auto &point: points) {
                const auto lbl = fmt::format("({: 7.3f} {: 7.3f} {: 7.3f})", point.x, point.y, point.z);
                if (ImGui::MenuItem(lbl.c_str())) {
                  state.view.offset = {0.0, 0.0};
                  state.renderer.target = point;
                }
              }
              ImGui::EndMenu();
            }
          }
          ImGui::EndMenu();
        }
      }
      auto pop_pos  = ImGui::GetWindowPos();
      auto obj_id = state.renderer.get_id_at({pop_pos.x - pos.x, pop_pos.y - pos.y - size.y}, state.view, to_glmvec(size), state.render_regions.items);
      if (obj_id) {
        auto id = obj_id.value();
        ImGui::Separator();
        ImGui::Text("%s Selection", icon_branch);
        with_indent indent{};
        ImGui::Text("Segment: %6zu", id.segment);
        ImGui::Text("Branch:  %6zu", id.branch);
      }
      ImGui::EndPopup();
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

template<typename Item>
void gui_locdefs(const std::string& name,
                 const entity& ids,
                 component_unique<Item>& items,
                 component_unique<renderable>& renderables,
                 std::vector<event>& events) {
  with_id guard{name};
  auto open = gui_tree_add(name, [&](){ events.emplace_back(evt_add_locdef<Item>{}); });
  if (open) {
    with_item_width iw(120.0f);
    for (const auto& id: ids) {
      with_id guard{id.value};
      auto& render = renderables[id];
      auto& item   = items[id];
      auto open = gui_tree("");
      ImGui::SameLine();
      if (ImGui::InputText("", &item.name)) events.push_back(evt_upd_locdef<Item>{id});
      ImGui::SameLine();
      ImGui::ColorEdit4("", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      ImGui::SameLine();
      gui_toggle(icon_show, icon_hide, render.active);
      gui_right_margin();
      if (ImGui::Button(icon_delete)) events.push_back(evt_del_locdef<Item>{id});
      if (open) {
        if (ImGui::InputText("Definition", &item.definition)) events.push_back(evt_upd_locdef<Item>{id});
        ImGui::SameLine();
        gui_check_state(item);
        ImGui::TreePop();
      }
    }
    ImGui::TreePop();
  }
}

void gui_locations(gui_state &state) {
  if (ImGui::Begin(fmt::format("{} Locations", icon_location).c_str())) {
    gui_locdefs(fmt::format("{} Regions", icon_region), state.regions, state.region_defs, state.render_regions, state.events);
    ImGui::Separator();
    gui_locdefs(fmt::format("{} Locsets", icon_locset), state.locsets, state.locset_defs, state.render_locsets, state.events);
  }
  ImGui::End();
}

void gui_ion_settings(gui_state& state) {
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

void gui_ion_defaults(gui_state& state) {
  with_id guard{"ion-defaults"};
  auto open = gui_tree_add(fmt::format("{} Default", icon_default), [&]() { state.add_ion(); });
  if (open) {
    for (const auto &ion: state.ions) {
      with_id guard{ion.value};
      with_item_width item_width{120.0f};
      auto& definition = state.ion_defs[ion];
      auto open = gui_tree("##ion-tree");
      ImGui::SameLine();
      ImGui::InputText("##ion-name",  &definition.name);
      gui_right_margin();
      if (ImGui::Button(icon_delete)) state.remove_ion(ion);
      if (open) {
        auto& defaults = state.ion_defaults[ion];
        ImGui::InputInt("Charge", &definition.charge);
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
            gui_input_double("Reversal Potential", defaults.Er, "mV", "%.0f");
          }
        }
        ImGui::TreePop();
      }
    }
    ImGui::TreePop();
  }
}

void gui_parameter_set(parameter_def& to_set, const arb::cable_cell_parameter_set& defaults) {
  with_item_width item_width{120.0f};
  gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.temperature_K);
  gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.init_membrane_potential);
  gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.axial_resistivity);
  gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.membrane_capacitance);
}

void gui_parameter_set(parameter_def& to_set, const parameter_def& defaults, const arb::cable_cell_parameter_set& fallback) {
  with_item_width item_width{120.0f};
  gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.TK, fallback.temperature_K);
  gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.Vm, fallback.init_membrane_potential);
  gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.RL, fallback.axial_resistivity);
  gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.Cm, fallback.membrane_capacitance);
}

void gui_parameters(gui_state& state) {
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
  }
  ImGui::End();
}

const static std::unordered_map<std::string, arb::mechanism_catalogue>
catalogues = {{"default", arb::global_default_catalogue()},
              {"allen", arb::global_allen_catalogue()}};

bool gui_mechanism(mechanism_def& data) {
  auto open = gui_tree("##mechanism-tree");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##mechanism-choice", data.name.c_str())) {
    for (const auto &[cat_name, cat]: catalogues) {
      ImGui::Selectable(cat_name.c_str(), false);
      with_indent ind{};
      for (const auto &name: cat.mechanism_names()) {
        if (gui_select(name, data.name)) {
          auto info = cat[data.name];
          data.global_vars.clear();
          for (const auto &[k, v]: info.globals) data.global_vars[k] = v.default_value;
          data.parameters.clear();
          for (const auto &[k, v]: info.parameters) data.parameters[k] = v.default_value;
        }
      }
    }
    ImGui::EndCombo();
  }
  gui_right_margin();
  auto remove = ImGui::Button(icon_delete);
  if (open) {
    if (!data.global_vars.empty()) {
      ImGui::BulletText("Global Values");
      with_indent ind{};
      for (auto &[k, v]: data.global_vars) gui_input_double(k, v);
    }
    if (!data.parameters.empty()) {
      ImGui::BulletText("Parameters");
      with_indent ind{};
      for (auto &[k, v]: data.parameters) gui_input_double(k, v);
    }
    ImGui::TreePop();
  }
  return remove;
}

void gui_mechanisms(gui_state &state) {
  if (ImGui::Begin(fmt::format("{} Mechanisms", icon_gears).c_str())) {
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
  }
  ImGui::End();
}

bool gui_measurement(probe_def& data) {
  ImGui::Bullet();
  ImGui::SameLine();
  with_item_width width {120.f};
  gui_input_double("Frequency", data.frequency, "Hz");
  gui_right_margin();
  auto remove = ImGui::Button(icon_delete);
  with_indent indent{};
  gui_choose("Variable", data.variable, probe_def::variables);
  return remove;
}

bool gui_measurement(detector_def& data) {
  ImGui::Bullet();
  ImGui::SameLine();
  with_item_width width {120.f};
  gui_input_double("Threshold", data.threshold, "mV");
  gui_right_margin();
  auto remove = ImGui::Button(icon_delete);
  return remove;
}

void gui_measurements(gui_state& state) {
  if (ImGui::Begin(fmt::format("{} Measurements", icon_meter).c_str())) {
    auto open = gui_tree(fmt::format("{} Probes", icon_probe));
    if (open) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_probe(locset); });
        if (open) {
          for (const auto& probe: state.probes.get_children(locset)) {
            with_id id{probe};
            auto rem = gui_measurement(state.probes[probe]);
            if (rem) state.remove_probe(probe);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
    ImGui::Separator();
    if (gui_tree(fmt::format("{} Spike Detectors", icon_detector))) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_detector(locset); });
        if (open) {
          for (const auto& detector: state.detectors.get_children(locset)) {
            with_id id{detector};
            auto rem = gui_measurement(state.detectors[detector]);
            if (rem) state.remove_detector(detector);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }
  ImGui::End();
}

void gui_debug(bool &open) { ImGui::ShowMetricsWindow(&open); }

void gui_style(bool &open) {
  if (ImGui::Begin("Style", &open)) ImGui::ShowStyleEditor();
  ImGui::End();
}

void gui_state::gui() {
  gui_main(*this);
  gui_locations(*this);
  gui_cell(*this);
  gui_mechanisms(*this);
  gui_parameters(*this);
  gui_measurements(*this);
}

void gui_state::serialize(const std::string &dn) {}
void gui_state::deserialize(const std::string &dn) {}
