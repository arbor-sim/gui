#include "gui_state.hpp"

#include "icons.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <arbornml/arbornml.hpp>
#include <arbornml/nmlexcept.hpp>

#include <sstream>

extern float delta_phi;
extern float delta_zoom;
extern glm::vec2 delta_pos;

gui_state::gui_state(): builder{} {}

void load_swc(gui_state &state,
              const std::string &fn,
              std::function<arb::morphology(const std::vector<arborio::swc_record> &)> swc_to_morph) {
  log_debug("Reading {}", fn);
  std::ifstream in(fn.c_str());
  auto swc = arborio::parse_swc(in).records();
  auto morph = swc_to_morph(swc);

  // Clean-up & Re-build ourself
  state.reset();
  state.builder = cell_builder{morph};
  state.renderer = geometry{morph};

  state.region_defs.emplace_back("soma", "(tag 1)");
  state.region_defs.emplace_back("axon", "(tag 2)");
  state.region_defs.emplace_back("dend", "(tag 3)");
  state.region_defs.emplace_back("apic", "(tag 4)");

  state.locset_defs.emplace_back("center", "(location 0 0)");
}

template <typename T> std::string to_string(const T &r) {
  std::stringstream ss;
  ss << r;
  return ss.str();
}

void gui_state::reset() {
  render_regions.clear();
  render_locsets.clear();
  locset_defs.clear();
  region_defs.clear();
  ion_defs.clear();
  mechanism_defs.clear();
}

void gui_state::load_allen_swc(const std::string &fn) {
  load_swc(*this, fn, [](const auto &d) { return arborio::load_swc_allen(d); });
}
void gui_state::load_neuron_swc(const std::string &fn) {
  load_swc(*this, fn, [](const auto &d) { return arborio::load_swc_neuron(d); });
}
void gui_state::load_arbor_swc(const std::string &fn) {
  load_swc(*this, fn, [](const auto &d) { return arborio::load_swc_arbor(d); });
}

void gui_state::load_neuroml(const std::string &fn) {
  // Read in morph
  std::ifstream fd(fn.c_str());
  std::string xml(std::istreambuf_iterator<char>(fd), {});
  arbnml::neuroml nml(xml);
  // Extract segment tree
  auto cell_ids = nml.cell_ids();
  auto id = cell_ids.front();
  auto morph_data = nml.cell_morphology(id).value();
  auto morph = morph_data.morphology;

  // Clean-up & Re-build ourself
  reset();
  builder = cell_builder{morph};
  renderer = geometry{morph};

  // Copy over locations
  for (const auto &[k, v]: morph_data.groups.regions()) {
    log_debug("NML region {}\n {}", k, to_string(v));
    region_defs.push_back({k, to_string(v)});
  }
  for (const auto &[k, v]: morph_data.groups.locsets()) {
    log_debug("NML locset {}\n {}", k, to_string(v));
    locset_defs.push_back({k, to_string(v)});
  }
}

template <typename D> void update_def(D &def) {
  if (def.state == def_state::erase)
    return;
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

template <typename Item>
void update_placables(std::unordered_map<std::string, ls_def> &locsets,
                      std::vector<Item> &defs) {
  std::erase_if(defs,
                [](const auto &p) { return p.state == def_state::erase; });
  for (auto &def: defs) {
    if (def.locset_name.empty()) {
      def.empty();
    } else {
      if (locsets.contains(def.locset_name)) {
        const auto &ls = locsets[def.locset_name];
        switch (ls.state) {
        case def_state::error:
          def.error("Linked locset malformed.");
          break;
        case def_state::empty:
          def.error("Linked locset empty.");
          break;
        case def_state::good:
          def.good();
          break;
        default:
          log_fatal("invalid code path: unknown state {}", def.locset_name);
        }
      } else {
        def.error("Linked locset absent.");
      }
    }
  }
}

void gui_state::update() {
  {
    if (render_locsets.size() < locset_defs.size())
      render_locsets.resize(locset_defs.size());
    auto idx = 0;
    for (auto &def: locset_defs) {
      auto &render = render_locsets[idx];
      if (def.state == def_state::changed) {
        update_def(def);
        def_set_renderable(renderer, builder, render, def);
      }
      if (def.state == def_state::erase) {
        render_locsets[idx].state = def_state::erase;
      }
      idx++;
    }
    std::erase_if(locset_defs,
                  [](const auto &p) { return p.state == def_state::erase; });
    std::erase_if(render_locsets,
                  [](const auto &p) { return p.state == def_state::erase; });
    // collect what is left
    std::unordered_map<std::string, ls_def> locsets;
    for (const auto &def: locset_defs)
      locsets[def.name] = def;
    // finally update individual lists of placeables
    update_placables(locsets, probe_defs);
    update_placables(locsets, iclamp_defs);
    update_placables(locsets, detector_defs);
  }
  // Update paintables
  {
    if (render_regions.size() < region_defs.size()) render_regions.resize(region_defs.size());
    if (par_defs.size() < region_defs.size())       par_defs.resize(region_defs.size());
    if (mechanism_defs.size() < region_defs.size()) mechanism_defs.resize(region_defs.size());
    if (ion_defs.size() < ion_defaults.size())      ion_defs.resize(ion_defaults.size());
    for (auto &ion: ion_defs)
      if (ion.size() < region_defs.size()) ion.resize(region_defs.size());
    auto idx = 0;
    for (auto &def: region_defs) {
      auto &render = render_regions[idx];
      if (def.state == def_state::changed) {
        update_def(def);
        def_set_renderable(renderer, builder, render, def);
      }
      if (def.state == def_state::erase) {
        render_regions[idx].state = def_state::erase;
        par_defs[idx].state = def_state::erase;
        for (auto &ion: ion_defs)
          ion.erase(ion.begin() + idx);
        mechanism_defs.erase(mechanism_defs.begin() + idx);
        render_regions[idx].state = def_state::erase;
      }
      idx++;
    }
    std::erase_if(region_defs,
                  [](const auto &p) { return p.state == def_state::erase; });
    std::erase_if(render_regions,
                  [](const auto &p) { return p.state == def_state::erase; });
    std::erase_if(par_defs,
                  [](const auto &p) { return p.state == def_state::erase; });
  }
}

void gui_main(gui_state &state);
void gui_menu_bar(gui_state &state);
void gui_read_morphology(gui_state &state, bool &open);
void gui_locations(gui_state &state);
void gui_cell(gui_state &state);
void gui_place(gui_state &state);
void gui_tooltip(const std::string &);
void gui_check_state(def_state);
void gui_painting(gui_state &state);
void gui_trash(definition &);
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
  template<typename V> with_id(V val) { ImGui::PushID(val); }
  ~with_id() { ImGui::PopID(); }
};

void gui_right_margin() { ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f); }

void gui_select(const std::string& item, std::string& current) {
  if (ImGui::Selectable(item.c_str(), item == current)) current = item;
}

template<typename Container>
void gui_choose(const std::string& lbl, std::string& current, Container items) {
  if (ImGui::BeginCombo(lbl.c_str(), current.c_str())) {
    for (auto &item: items) gui_select(item, current);
    ImGui::EndCombo();
  }
}

bool gui_input_double(const std::string& lbl, double& v, const char* fmt) {
  return ImGui::InputDouble(lbl.c_str(), &v, 0.0, 0.0, fmt, ImGuiInputTextFlags_CharsScientific);
}

void gui_trash(definition &def) { if (ImGui::Button(icon_delete)) def.erase(); }

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
  const char* tags[] = { icon_question, icon_error, icon_ok };
  ImGui::Text("%s", tags[(int) def.state]);
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
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  } else {
    dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
  }

  // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render
  // our background and handle the pass-thru hole, so we ask Begin() to not
  // render a background.
  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

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
    log_debug("Show hidden files: {}", state.show_hidden);
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

struct loader_state {
  std::string message;
  const char *icon;
  std::optional<std::function<void(gui_state &, const std::string &)>> load;
};

std::unordered_map<
    std::string,
    std::unordered_map<std::string,
                       std::function<void(gui_state &, const std::string &)>>>
    loaders{
        {".swc",
         {{"Arbor",
           [](gui_state &s, const std::string &fn) { s.load_arbor_swc(fn); }},
          {"Allen",
           [](gui_state &s, const std::string &fn) { s.load_allen_swc(fn); }},
          {"Neuron", [](gui_state &s,
                        const std::string &fn) { s.load_neuron_swc(fn); }}}},
        {".nml", {{"Default", [](gui_state &s, const std::string &fn) {
                     s.load_neuroml(fn);
                   }}}}};

loader_state get_loader(const std::string &extension,
                        const std::string &flavor) {
  if (extension.empty())                    return {"Please select a file.",   icon_error, {}};
  if (!loaders.contains(extension))         return {"Unknown file type.",      icon_error, {}};
  if (flavor.empty())                       return {"Please select a flavor.", icon_error, {}};
  if (!loaders[extension].contains(flavor)) return {"Unknown flavor type.",    icon_error, {}};
  return {"Ok.", icon_ok, {loaders[extension][flavor]}};
}

void gui_read_morphology(gui_state &state, bool &open_file) {
  with_id id{"open_file"};
  ImGui::OpenPopup("Open");
  if (ImGui::BeginPopupModal("Open")) {
    gui_dir_view(state.file_chooser);
    auto extension = state.file_chooser.file.extension();
    static std::string current_flavor = "";
    {
      with_item_width id{120.0f};
      auto lbl = state.file_chooser.filter.value_or("all");
      if (ImGui::BeginCombo("Filter", lbl.c_str())) {
        if (ImGui::Selectable("all", "all" == lbl)) state.file_chooser.filter = {};
        for (const auto &[k, v]: loaders) {
          if (ImGui::Selectable(k.c_str(), k == lbl)) state.file_chooser.filter = {k};
        }
        ImGui::EndCombo();
      }
      if (loaders.contains(extension)) {
        auto flavors = loaders[extension];
        if (!flavors.contains(current_flavor)) current_flavor = flavors.begin()->first;
        ImGui::SameLine();
        if (ImGui::BeginCombo("Flavor", current_flavor.c_str())) {
          for (const auto &[k, v]: flavors) gui_select(k, current_flavor);
          ImGui::EndCombo();
        }
      } else {
        current_flavor = "";
      }
    }

    {
      auto loader = get_loader(extension, current_flavor);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, loader.load ? 1.0f: 0.6f);
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
  static float zoom = 45.0f;
  static float phi = 0.0f;
  static glm::vec2 offset = {0.0, 0.0f};

  if (ImGui::Begin("Cell")) {
    ImGui::BeginChild("Cell Render");
    auto size = ImGui::GetWindowSize();
    auto image = state.renderer.render(zoom,
                                       phi,
                                       to_glmvec(size),
                                       offset,
                                       state.render_regions,
                                       state.render_locsets);
    ImGui::Image((ImTextureID) image, size, ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::IsItemHovered()) {
      offset -= delta_pos;
      zoom    = std::clamp(zoom + delta_zoom, 1.0f, 45.0f);
      phi     = std::fmod(phi + delta_phi + 2*PI, 2*PI);    // cyclic in [0, 2pi)
    }

    if (ImGui::BeginPopupContextWindow()) {
      if (gui_menu_item("Reset camera", icon_camera)) {
        offset = {0.0, 0.0};
        state.renderer.target = {0.0f, 0.0f, 0.0f};
      }
      if (ImGui::BeginMenu("Snap to locset")) {
        for (const auto &ls: state.locset_defs) {
          if (ls.state != def_state::good) continue;
          auto name = ls.name.c_str();
          if (ImGui::BeginMenu(name)) {
            auto points = state.builder.make_points(ls.data.value());
            for (const auto &point: points) {
              const auto lbl = fmt::format("({: 7.3f} {: 7.3f} {: 7.3f})", point.x, point.y, point.z);
              if (ImGui::MenuItem(lbl.c_str())) {
                offset = {0.0, 0.0};
                state.renderer.target = point;
              }
            }
            ImGui::EndMenu();
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndPopup();
    }
    ImGui::EndChild();
  }
  ImGui::End();

  delta_pos = {0.0f, 0.0f};
  delta_phi = 0.0f;
  delta_zoom = 0.0f;
}

void gui_locdef(loc_def &def, renderable &render) {
  with_id id(def.name.c_str());
  ImGui::Bullet();
  ImGui::SameLine();
  ImGui::InputText("Name", &def.name);
  with_indent ind(24.0f);
  if (ImGui::InputText("Definition", &def.definition)) def.state = def_state::changed;
  ImGui::SameLine();
  gui_check_state(def);
  gui_trash(def);
  ImGui::SameLine();
  gui_toggle(icon_show, icon_hide, render.active);
  ImGui::SameLine();
  ImGui::ColorEdit4("", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
}

template <typename Item>
void gui_locdefs(const std::string &name,
                 std::vector<Item> &items,
                 std::vector<renderable> &render) {
  with_id id(name.c_str());
  ImGui::AlignTextToFramePadding();
  auto open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
  gui_right_margin();
  auto add = ImGui::Button(icon_add);
  if (open) {
    with_item_width iw(120.0f);
    for (auto idx = 0; idx < items.size(); ++idx) gui_locdef(items[idx], render[idx]);
    ImGui::TreePop();
  }
  if (add) items.emplace_back();
}

void gui_locations(gui_state &state) {
  if (ImGui::Begin("Locations")) {
    gui_locdefs("Regions", state.region_defs, state.render_regions);
    ImGui::Separator();
    gui_locdefs("Locsets", state.locset_defs, state.render_locsets);
  }
  ImGui::End();
}

void gui_placeable(prb_def &probe) {
  gui_input_double("Frequency", probe.frequency, "%.0f Hz");
  gui_choose("Variable", probe.variable, std::vector<std::string>{"voltage", "current"});
}

void gui_placeable(stm_def &iclamp) {
  gui_input_double("Delay",     iclamp.delay,     "%.0f ms");
  gui_input_double("Duration",  iclamp.duration,  "%.0f ms");
  gui_input_double("Amplitude", iclamp.amplitude, "%.0f nA");
}

void gui_placeable(sdt_def &detector) {
  gui_input_double("Threshold", detector.threshold, "%.0f mV");
}

template <typename Item>
void gui_placeables(const std::string &label,
                    const std::vector<ls_def> &locsets,
                    std::vector<Item> &items) {
  with_id id(label.c_str());
  ImGui::AlignTextToFramePadding();
  auto open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
  gui_right_margin();
  if (ImGui::Button(icon_add)) items.emplace_back();
  if (open) {
    with_item_width iw(120.0f);
    for (auto &item: items) {
      with_id id(item.locset_name.c_str());
      ImGui::Bullet();
      ImGui::SameLine();
      if (ImGui::BeginCombo("Locset", item.locset_name.c_str())) {
        for (const auto &ls: locsets) gui_select(ls.name, item.locset_name);
        ImGui::EndCombo();
      }
      ImGui::SameLine();
      gui_check_state(item);
      with_indent ind{24.0f};
      gui_placeable(item);
      gui_trash(item);
    }
    ImGui::TreePop();
  }
}

void gui_place(gui_state &state) {
  if (ImGui::Begin("Placings")) {
    gui_placeables("Probes", state.locset_defs, state.probe_defs);
    ImGui::Separator();
    gui_placeables("Stimuli", state.locset_defs, state.iclamp_defs);
    ImGui::Separator();
    gui_placeables("Detectors", state.locset_defs, state.detector_defs);
  }
  ImGui::End();
}

void gui_defaulted_double(const std::string &label, const char *format,
                          std::optional<double> &value, const double fallback) {
  auto tmp = value.value_or(fallback);
  if (gui_input_double(label, tmp, format)) value = {tmp};
  gui_right_margin();
  if (ImGui::Button(icon_refresh)) value = {};
}

void gui_painting_preset(gui_state &state) {
  static std::string preset = "Neuron";
  static std::unordered_map<std::string, arb::cable_cell_parameter_set> presets{{"Neuron", arb::neuron_parameter_defaults}};
  if (ImGui::BeginCombo("Load Preset", preset.c_str())) {
    for (const auto &[k, v]: presets) gui_select(k, preset);
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button(icon_save)) {
    auto df = presets[preset];
    state.par_defaults.TK = df.temperature_K.value();
    state.par_defaults.Vm = df.init_membrane_potential.value();
    state.par_defaults.RL = df.axial_resistivity.value();
    state.par_defaults.Cm = df.membrane_capacitance.value();
    state.ion_defaults.clear();
    state.ion_defs.clear();
    auto blank = std::vector<ion_def>(state.region_defs.size(), ion_def{});
    for (const auto &[k, v]: df.ion_data) {
      state.ion_defaults.push_back({.name = k,
                                    .Xi = v.init_int_concentration.value(),
                                    .Xo = v.init_ext_concentration.value(),
                                    .Er = v.init_reversal_potential.value()});
      state.ion_defs.push_back(blank);
    }
  }
}

void gui_painting_parameter(gui_state &state) {
  if (ImGui::TreeNodeEx("Parameters")) {
    if (ImGui::TreeNodeEx("Defaults")) {
      gui_input_double("Temperature",          state.par_defaults.TK, "%.0f K");
      gui_input_double("Membrane Potential",   state.par_defaults.Vm, "%.0f mV");
      gui_input_double("Axial Resistivity",    state.par_defaults.RL, "%.0f Ω·cm");
      gui_input_double("Membrane Capacitance", state.par_defaults.Cm, "%.0f F/m²");
      ImGui::TreePop();
    }
    auto region = state.region_defs.begin();
    for (auto &p: state.par_defs) {
      if (ImGui::TreeNodeEx(region->name.c_str())) {
        gui_defaulted_double("Temperature",          "%.0f K",     p.TK, state.par_defaults.TK);
        gui_defaulted_double("Membrane Potential",   "%.0f mV",    p.Vm, state.par_defaults.Vm);
        gui_defaulted_double("Axial Resistivity",    "%.0f Ω·cm ", p.RL, state.par_defaults.RL);
        gui_defaulted_double("Membrane Capacitance", "%.0f F/m²",  p.Cm, state.par_defaults.Cm);
        ImGui::TreePop();
      }
      ++region;
    }
    ImGui::TreePop();
  }
}

void gui_painting_ion(std::vector<ion_default> &ion_defaults,
                      std::vector<std::vector<ion_def>> &ion_defs,
                      const std::vector<reg_def>
                          &region_defs) { // These could be name strings only
  if (ImGui::TreeNodeEx("Ions")) {
    static std::vector<std::string> methods{"constant", "Nernst"};
    auto defaults = ion_defaults.begin();
    for (auto &ion: ion_defs) {
      auto name = defaults->name;
      if (ImGui::TreeNodeEx(name.c_str())) {
        if (ImGui::TreeNodeEx("Defaults")) {
          gui_input_double("Internal Concentration", defaults->Xi, "%.0f F/m²");
          gui_input_double("External Concentration", defaults->Xo, "%.0f F/m²");
          gui_input_double("Reversal Potential",     defaults->Er, "%.0f F/m²");
          gui_choose("Method", defaults->method, methods);
          ImGui::TreePop();
        }
        auto region = region_defs.begin();
        for (auto &def: ion) {
          if (ImGui::TreeNodeEx(region->name.c_str())) {
            gui_defaulted_double("Internal Concentration", "%.0f F/m²", def.Xi, defaults->Xi);
            gui_defaulted_double("External Concentration", "%.0f F/m²", def.Xo, defaults->Xo);
            gui_defaulted_double("Reversal Potential",     "%.0f F/m²", def.Er, defaults->Er);
            ImGui::TreePop();
          }
          ++region;
        }
        ImGui::TreePop();
      }
      ++defaults;
    }
    ImGui::TreePop();
  }
}

const static std::unordered_map<std::string, arb::mechanism_catalogue>
    catalogues = {{"default", arb::global_default_catalogue()},
                  {"allen", arb::global_allen_catalogue()}};

void gui_mechanism(mech_def &mech) {
  with_id id{mech.name.c_str()};
  ImGui::Bullet();
  ImGui::SameLine();
  if (ImGui::BeginCombo("Mechanism", mech.name.c_str())) {
    for (const auto &[cat_name, cat]: catalogues) {
      ImGui::Selectable(cat_name.c_str(), false);
      with_indent ind{};
      for (const auto &mech_name: cat.mechanism_names()) {
        if (ImGui::Selectable(mech_name.c_str(), mech_name == mech.name)) {
          mech.name = mech_name;
          auto info = cat[mech_name];
          mech.global_vars.clear();
          for (const auto &[k, v]: info.globals) mech.global_vars[k] = v.default_value;
          mech.parameters.clear();
          for (const auto &[k, v]: info.parameters) mech.parameters[k] = v.default_value;
        }
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  gui_trash(mech);
  with_indent ind{};
  if (!mech.global_vars.empty()) {
    ImGui::BulletText("Global Values");
    with_indent ind{};
    for (auto &[k, v]: mech.global_vars) gui_input_double(k, v, "%.3f");
  }
  if (!mech.parameters.empty()) {
    ImGui::BulletText("Parameters");
    with_indent ind{};
    for (auto &[k, v]: mech.parameters) gui_input_double(k, v, "%.3f");
  }
}

void gui_painting_mechanism(gui_state &state) {
  with_id id("mechanims");
  if (ImGui::TreeNodeEx("Mechanisms", ImGuiTreeNodeFlags_AllowItemOverlap)) {
    auto region = state.region_defs.begin();
    auto ridx = 0;
    for (auto &mechanisms: state.mechanism_defs) {
      with_id id{ridx++};
      ImGui::AlignTextToFramePadding();
      auto open = ImGui::TreeNodeEx(region->name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
      gui_right_margin();
      if (ImGui::Button(icon_add)) mechanisms.emplace_back();
      if (open) {
        for (auto &mechanism: mechanisms) gui_mechanism(mechanism);
        ImGui::TreePop();
      }
      ++region;
    }
    ImGui::TreePop();
  }
}

void gui_painting(gui_state &state) {
  if (ImGui::Begin("Paintings")) {
    with_item_width iw(120.0f);
    gui_painting_preset(state);
    ImGui::Separator();
    gui_painting_parameter(state);
    ImGui::Separator();
    gui_painting_ion(state.ion_defaults, state.ion_defs, state.region_defs);
    ImGui::Separator();
    gui_painting_mechanism(state);
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
  gui_place(*this);
  gui_painting(*this);
}

void gui_state::serialize(const std::string &dn) {}

void gui_state::deserialize(const std::string &dn) {}
