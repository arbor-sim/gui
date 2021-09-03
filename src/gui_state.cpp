#include "gui_state.hpp"

#include <cmath>
#include <regex>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <implot.h>
#include <ImGuizmo.h>

#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/cable_cell_param.hpp>
#include <arbor/version.hpp>
#include <arborio/neurolucida.hpp>
#include <arborio/neuroml.hpp>
#include <arborio/swcio.hpp>
#include <arborio/cableio.hpp>
#include <arbor/simulation.hpp>
#include <arbor/context.hpp>
#include <arbor/load_balance.hpp>

#include "gui.hpp"
#include "utils.hpp"
#include "icons.hpp"
#include "events.hpp"
#include "loader.hpp"
#include "config.hpp"
#include "recipe.hpp"

extern float delta_zoom;
extern glm::vec2 mouse;

using namespace std::literals;

namespace {
  inline void gui_read_morphology(gui_state& state, bool& open);
  inline void gui_dir_view(file_chooser_state& state);
  inline void gui_debug(bool&);
  inline void gui_style(bool&);
  inline void gui_about(bool&);
  inline void gui_demo(bool&);

  inline void gui_save_acc(gui_state& state, bool& open) {

    with_id id{"writing acc"};
    ImGui::OpenPopup("Save");
    static std::vector<std::string> suffixes{".acc"};
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

    with_id id{"reading acc"};
    ImGui::OpenPopup("Load");
    static std::vector<std::string> suffixes{".acc"};
    static std::string loader_error = "";
    if (ImGui::BeginPopupModal("Load")) {
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

      try {
        if (ok) {
          state.deserialize(state.acc_chooser.file);
          open = false;
        }
      } catch (const arb::arbor_exception& e) {
        log_debug("Failed to load ACC: {}", e.what());
        loader_error = e.what();
      }

      if (!loader_error.empty()) {
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 40.0f, ImGui::GetFontSize() * 5.0f));
        ImGui::OpenPopup("Cannot Load Cable Cell");
      }
      if (ImGui::BeginPopupModal("Cannot Load Cable Cell")) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
        ImGui::TextUnformatted(loader_error.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button("Close")) {
          loader_error.clear();
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      if (ko) open = false;
      ImGui::EndPopup();
    }
  }

  inline void gui_read_cat(gui_state& state, bool& open) {

    with_id id{"loading cat"};
    ImGui::OpenPopup("Load");
    static std::vector<std::string> suffixes{".so"};
    static std::string loader_error = "";
    if (ImGui::BeginPopupModal("Load")) {
      gui_dir_view(state.cat_chooser);
      {
        with_item_width width{120.0f};
        auto lbl = state.cat_chooser.filter.value_or("all");
        if (ImGui::BeginCombo("Filter", lbl.c_str())) {
          if (ImGui::Selectable("all", "all" == lbl)) state.cat_chooser.filter = {};
          for (const auto& k: suffixes) {
            if (ImGui::Selectable(k.c_str(), k == lbl)) state.cat_chooser.filter = {k};
          }
          ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::InputText("Name", &state.open_cat_name);
      }
      auto can_load = !state.open_cat_name.empty() && !catalogues.contains(state.open_cat_name);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, can_load ? 1.0f: 0.6f);
      auto ok = ImGui::Button("Load");
      ImGui::PopStyleVar();
      ImGui::SameLine();
      ImGui::Text("%s", can_load ? icon_ok : icon_error);
      gui_tooltip("Empty or duplicate catalogue name.");

      ImGui::SameLine();
      auto ko = ImGui::Button("Cancel");

      if (ok && can_load) {
        try {
          log_debug("Adding catalogue '{}'", state.open_cat_name);
          auto cat = arb::load_catalogue(state.cat_chooser.file.string());
          catalogues[state.open_cat_name] = cat;
          open = false;
        } catch (const arb::arbor_exception& e) {
          log_debug("Failed to load catalogue: {}", e.what());
          loader_error = e.what();
        }
      }

      if (!loader_error.empty()) {
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 40.0f, ImGui::GetFontSize() * 5.0f));
        ImGui::OpenPopup("Cannot Open Mechanism Catalogue");
      }

      if (ImGui::BeginPopupModal("Cannot Open Mechanism Catalogue")) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
        ImGui::TextUnformatted(loader_error.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button("Close")) {
          loader_error.clear();
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      if (ko) open = false;
      ImGui::EndPopup();
    }
  }

  inline void gui_menu_bar(gui_state& state) {

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
      ImGui::Text("%s Morphology", icon_branch);
      {
        with_indent indent;
        state.open_morph_read = gui_menu_item("Load##morph", icon_load, "C-o");
      }
      ImGui::Separator();
      ImGui::Text("%s Cable cell", icon_cell);
      {
        with_indent indent;
        state.open_acc_read = gui_menu_item("Load##acc", icon_load);
        state.open_acc_save = gui_menu_item("Save##acc", icon_save);
      }
      ImGui::Separator();
      ImGui::Text("%s Catalogue", icon_book);
      {
        with_indent indent;
        if(gui_menu_item("Load##cat",  icon_load)) {
          state.open_cat_read = true;
          state.open_cat_name = ""; // Reset name;
        }
        if (gui_menu_item("Reset##cat", icon_clean)) catalogues = default_catalogues;
      }
      ImGui::Separator();
      state.shutdown_requested = gui_menu_item("Quit", "");
      ImGui::EndMenu();
    }
    gui_right_margin(60.0f);
    if (ImGui::BeginMenu("Help")) {
      state.open_about = gui_menu_item("About",   icon_about);
      state.open_debug = gui_menu_item("Metrics", icon_bug);
      state.open_style = gui_menu_item("Style",   icon_paint);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
    if (state.open_morph_read) gui_read_morphology(state, state.open_morph_read);
    if (state.open_acc_read)   gui_read_acc(state, state.open_acc_read);
    if (state.open_cat_read)   gui_read_cat(state, state.open_cat_read);
    if (state.open_acc_save)   gui_save_acc(state, state.open_acc_save);
    if (state.open_debug)      gui_debug(state.open_debug);
    if (state.open_style)      gui_style(state.open_style);
    if (state.open_about)      gui_about(state.open_about);
  }

  inline void gui_main(gui_state& state) {

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
    // Some well-known directories
    {
            ImGui::BeginChild("Dirs",
                              {25.0f,
                               -3.00f*ImGui::GetTextLineHeightWithSpacing()},
                              false);
            if (ImGui::Button(icon_home, {25.0f, 25.0f})) state.cwd = state.home;
            if (ImGui::Button(icon_desk, {25.0f, 25.0f})) state.cwd = state.desk;
            if (ImGui::Button(icon_docs, {25.0f, 25.0f})) state.cwd = state.docs;
            ImGui::EndChild();
    }
    ImGui::SameLine();
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
        if (ImGui::Button("Cancel")) open_file = false;
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
            } catch (const std::runtime_error& e) {
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

  inline bool gui_axes(axes& ax) {
    ImGui::Text("%s Axes", icon_axes);
    gui_right_margin();
    gui_toggle(icon_on, icon_off, ax.active);
    auto mv = ImGui::InputFloat3("Position", &ax.origin[0]);
    auto sz = ImGui::InputFloat("Size", &ax.scale, 0, 0, "%f µm");
    return mv || sz;
  }

  inline void gui_cell(gui_state& state) {
    if (ImGui::Begin("Cell")) {
      ImGui::BeginChild("Cell Render");
      auto size = ImGui::GetWindowSize(), win_pos = ImGui::GetWindowPos();
      auto& vs = state.view;
      vs.size = to_glmvec(size);
      state.renderer.render(vs, {mouse.x - win_pos.x, size.y + win_pos.y - mouse.y});
      ImGui::Image(reinterpret_cast<ImTextureID>(state.renderer.cell.tex), size, ImVec2(0, 1), ImVec2(1, 0));
      if (ImGui::IsItemHovered()) {
        auto shft = ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
        auto ctrl = ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
        if (shft || ctrl) {
          auto what = shft ? ImGuizmo::ROTATE : ImGuizmo::TRANSLATE;
          glm::vec3 shift = {vs.offset.x/vs.size.x, vs.offset.y/vs.size.y, 0.0f};
          glm::mat4 V  = glm::lookAt(vs.camera, (vs.target - state.renderer.root)/state.renderer.rescale + shift, vs.up);
          glm::mat4 P  = glm::perspective(glm::radians(vs.zoom), vs.size.x/vs.size.y, 0.1f, 100.0f);
          ImGuizmo::SetDrawlist();
          ImGuizmo::SetRect(win_pos.x, win_pos.y, size.x, size.y);
          ImGuizmo::Manipulate(glm::value_ptr(V), glm::value_ptr(P), what, ImGuizmo::LOCAL, glm::value_ptr(vs.rotate));
        } else {
          vs.zoom = std::clamp(vs.zoom + delta_zoom, 1.0f, 45.0f);
        }
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
                    state.view.target = point;
                  }
                }
                ImGui::EndMenu();
              }
            }
            ImGui::EndMenu();
          }
          if (gui_menu_item("Reset##camera", icon_refresh)) {
            state.view.offset = {0.0f, 0.0f};
            state.view.rotate = glm::mat4(1.0f);
            state.view.target = {0.0f, 0.0f, 0.0f};
            state.renderer.cell.clear_color = {214.0f/255, 214.0f/255, 214.0f/255};
          }
        }
        ImGui::Separator();
        if(gui_axes(state.renderer.ax)) state.renderer.make_ruler();
        ImGui::Separator();
        ImGui::Text("%s Model", icon_cell);
        {
          with_indent indent{};
          int tmp = state.renderer.n_faces;
          if (ImGui::DragInt("Frustrum Resolution", &tmp, 1.0f, 8, 64, "%d", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
            state.renderer.n_faces = tmp;
            state.renderer.load_geometry(state.builder.morph);
            for (const auto& rg: state.regions) state.update_region(rg);
            for (const auto& ls: state.locsets) state.update_locset(ls);
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
            ImGui::ColorButton("", to_imvec(state.renderer.regions[region].color));
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
                          entity& ids,
                          component_unique<Item>& items,
                          component_unique<renderable>& renderables,
                          event_queue& events) {

    with_id guard{name};
    auto from = -1, to = -1;
    auto open = gui_tree_add(name, [&](){ events.emplace_back(evt_add_locdef<Item>{}); });
    if (open) {
      with_item_width iw(120.0f);
      auto ix = 0;
      for (const auto& id: ids) {
        with_id guard{id};
        auto& render = renderables[id];
        auto& item   = items[id];
        auto open    = gui_tree("");
        ImGui::SameLine();
        auto beg = ImGui::GetCursorPos();
        if (ImGui::InputText("##locdef-name", &item.name, ImGuiInputTextFlags_AutoSelectAll)) events.push_back(evt_upd_locdef<Item>{id});
        ImGui::SameLine();
        ImGui::ColorEdit3("##locdef-color", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        gui_toggle(icon_show, icon_hide, render.active);
        ImGui::SameLine();
        if (ImGui::Button(icon_clone)) events.push_back(evt_add_locdef<Item>{item.name, item.definition});
        ImGui::SameLine();
        gui_check_state(item);
        gui_right_margin();
        if (ImGui::Button(icon_delete)) events.push_back(evt_del_locdef<Item>{id});

        if (open) {
          with_item_width iw(-50.0f);
          auto indent = gui_tree_indent();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
          if (ImGui::InputTextMultiline("##locdef-definition", &item.definition)) events.push_back(evt_upd_locdef<Item>{id});
          ImGui::PopTextWrapPos();
          ImGui::TreePop();
        }
        auto end = ImGui::GetCursorPos();
        ImGui::SetCursorPos(beg);
        ImGui::InvisibleButton("drag area", ImVec2(ImGui::GetContentRegionAvailWidth(), end.y - beg.y));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
          ImGui::Text("%s", item.name.c_str());
          ImGui::SetDragDropPayload(name.c_str(), &ix, sizeof(ix));
          ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
          auto data = ImGui::AcceptDragDropPayload(name.c_str(), ImGuiDragDropFlags_SourceNoHoldToOpenOthers);
          if (data) {
            from = *((int*) data->Data);
            to   = ix;
          }
          ImGui::EndDragDropTarget();
        }
        ImGui::SetCursorPos(end);
        ix++;
      }
      ImGui::TreePop();
    }

    if ((from >= 0) && (to >= 0)) {
      if (from == to) return;
      if (from < to) {
        ids.ids.insert(ids.ids.begin() + to + 1, ids.ids[from]);
        ids.ids.erase(ids.ids.begin() + from);
      }
      if (from > to) {
        ids.ids.insert(ids.ids.begin() + to, ids.ids[from]);
        ids.ids.erase(ids.ids.begin() + from + 1);
      }
    }

    float z = 0;
    for (const auto& id: ids) {
      renderables[id].zorder = z;
      z += 1.0f;
    }
  }

  inline void gui_locations(gui_state& state) {
    if (ImGui::Begin(fmt::format("{} Locations", icon_location).c_str())) {
      gui_locdefs(fmt::format("{} Regions", icon_region), state.regions, state.region_defs, state.renderer.regions, state.events);
      ImGui::Separator();
      gui_locdefs(fmt::format("{} Locsets", icon_locset), state.locsets, state.locset_defs, state.renderer.locsets, state.events);
    }
    ImGui::End();
  }

  inline void gui_ion_settings(gui_state& state) {
    with_id guard{"ion-settings"};
    if (gui_tree(fmt::format("{} Regions", icon_region))) {
      for (const auto& region: state.regions) {
        with_id guard{region.value};
        if (gui_tree(state.region_defs[region].name)) {
          for (const auto& ion: state.ions) {
            gui_ion_parameter(ion, state.ion_defs[ion], state.ion_par_defs[{region, ion}], state.ion_defaults[ion], state.presets.ion_data);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_ion_defaults(gui_state& state) {

    with_id guard{"ion-defaults"};
    auto open = gui_tree_add(fmt::format("{} Default", icon_default), [&]() { state.add_ion(); });
    if (open) {
      for (const auto& ion: state.ions) {
        gui_ion_default(ion, state.ion_defs[ion], state.ion_defaults[ion], state.presets.ion_data, state.events);
      }
      ImGui::TreePop();
    }
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
            gui_mechanism(child, state.mechanisms[child], state.events);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_parameters(gui_state& state) {

    with_id id{"parameters"};
    if (ImGui::Begin(fmt::format("{} Parameters", icon_list).c_str())) {
      if (gui_tree(fmt::format("{} Cable Cell Properties", icon_sliders))) {
        with_id id{"properties"};
        if (gui_tree(fmt::format("{} Default", icon_default))) {
          gui_parameter_defaults(state.parameter_defaults, state.presets);
          ImGui::TreePop();
        }
        if (gui_tree(fmt::format("{} Regions", icon_region))) {
          for (const auto& region: state.regions) {
            with_id id{region};
            if (gui_tree(state.region_defs[region].name)) {
              gui_parameter(state.parameter_defs[region], state.parameter_defaults, state.presets);
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
      // Ion names
      std::vector<std::string> ion_names;
      for (const auto& ion: state.ions) ion_names.push_back(state.ion_defs[ion].name);

      // Mech Variables
      std::vector<std::string> state_vars;
      for (const auto& [name, cat]: catalogues) {
        for (const auto& mech: cat.mechanism_names()) {
          const auto& info = cat[mech];
          for (const auto& [k, v]: info.state) {
            state_vars.push_back(fmt::format("{}::{}::{}", name, mech, k));
          }
        }
      }
      std::sort(state_vars.begin(), state_vars.end());
      std::unique(state_vars.begin(), state_vars.end());

      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_probe(locset); });
        if (open) {
          for (const auto& probe: state.probes.get_children(locset)) {
            gui_probe(probe, state.probes[probe], state.events, ion_names, state_vars);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_detectors(gui_state& state) {
    if (gui_tree(fmt::format("{} Spike Detectors", icon_detector))) {
      for (const auto& ls: state.locsets) {
        with_id id{ls};
        auto name = state.locset_defs[ls].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name),
                                 [&](){ state.add_detector(ls); });
        if (open) {
          for (const auto& d: state.detectors.get_children(ls)) {
            gui_detector(d, state.detectors[d], state.events);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_debug(bool& open) {  ImGui::ShowMetricsWindow(&open); }

  inline void gui_style(bool& open) {

    if (ImGui::Begin("Style", &open)) ImGui::ShowStyleEditor();
    ImGui::End();
  }

  inline void gui_about(bool& open) {

    if (ImGui::Begin("About", &open)) {
      ImGui::Text("Version: %s", gui_git_commit);
      ImGui::Text("Webpage: %s", gui_web_page);
      ImGui::Text("Arbor:   %s (%s)", arb::version, arb::source_id);
    }
    ImGui::End();
  }


  inline void gui_demo(bool& open) {

    if (ImGui::Begin("Demo", &open)) ImGui::ShowDemoWindow();
    ImGui::End();
  }

  inline void gui_stimuli(gui_state& state) {

    std::vector<float> values(state.sim.until/state.sim.dt, 0.0f);
    if (gui_tree(fmt::format("{} Stimuli", icon_stimulus))) {
      for (const auto& locset: state.locsets) {
        with_id id{locset};
        auto name = state.locset_defs[locset].name;
        auto open = gui_tree_add(fmt::format("{} {}", icon_locset, name), [&](){ state.add_stimulus(locset); });
        if (open) {
          for (const auto& stim: state.stimuli.get_children(locset)) {
            gui_stimulus(stim, state.stimuli[stim], state.events, values, state.sim.dt, state.sim.until);
          }
          ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
  }

  inline void gui_simulation(gui_state& state) {

    if (ImGui::Begin(fmt::format("{} Simulation", icon_sim).c_str())) {
      ImGui::Separator();
      gui_sim(state.sim);

      ImGui::Separator();
      gui_cv_policy(state.cv_policy_def, state.renderer.cv_boundaries, state.events);
      ImGui::Separator();
      gui_stimuli(state);
      ImGui::Separator();
      gui_probes(state);
      ImGui::Separator();
      gui_detectors(state);
    }
    ImGui::End();

    if(state.sim.should_run) {
      state.run_simulation();
      state.sim.should_run = false;
    }
  }

  inline arb::cable_cell make_cable_cell(gui_state& state) {
    arb::decor decor{};
    for (const auto& id: state.locsets) {
      auto ls = state.locset_defs[id];
      if (!ls.data) continue;
      auto locset = ls.data.value();
      for (const auto child: state.stimuli.get_children(id)) {
        auto item = state.stimuli[child];
        arb::i_clamp i_clamp;
        i_clamp.frequency = item.frequency;
        i_clamp.phase     = item.phase;
        std::sort(item.envelope.begin(), item.envelope.end());
        for (const auto& [t, i]: item.envelope) i_clamp.envelope.emplace_back(arb::i_clamp::envelope_point{t, i});
        decor.place(locset, i_clamp, item.tag);
      }
      for (const auto child: state.detectors.get_children(id)) {
        auto item = state.detectors[child];
        decor.place(locset, arb::threshold_detector{item.threshold}, item.tag);
      }
    }

    auto param = state.parameter_defaults;
    if (param.RL) decor.set_default(arb::axial_resistivity{param.RL.value()});
    if (param.Cm) decor.set_default(arb::membrane_capacitance{param.Cm.value()});
    if (param.TK) decor.set_default(arb::temperature_K{param.TK.value()});
    if (param.Vm) decor.set_default(arb::init_membrane_potential{param.Vm.value()});

    for (const auto& ion: state.ions) {
      const auto& data = state.ion_defaults[ion];
      const auto& name = state.ion_defs[ion].name;
      std::string meth;
      if (data.method == "const.") {
      } else if (data.method == "Nernst") {
        decor.set_default(arb::ion_reversal_potential_method{name, arb::mechanism_desc{fmt::format("default::nernst/{}", name)}});
      }

      if (state.presets.ion_data.contains(name)) {
        auto p = state.presets.ion_data.at(name);
        decor.set_default(arb::init_int_concentration{name,  data.Xi.value_or(p.init_int_concentration.value())});
        decor.set_default(arb::init_ext_concentration{name,  data.Xo.value_or(p.init_ext_concentration.value())});
        decor.set_default(arb::init_reversal_potential{name, data.Er.value_or(p.init_reversal_potential.value())});
      } else {
        decor.set_default(arb::init_int_concentration{name,  data.Xi.value()});
        decor.set_default(arb::init_ext_concentration{name,  data.Xo.value()});
        decor.set_default(arb::init_reversal_potential{name, data.Er.value()});
      }
    }

    for (const auto& id: state.regions) {
      auto rg = state.region_defs[id];
      if (!rg.data) continue;
      auto param  = state.parameter_defs[id];
      if (param.RL) decor.paint(rg.data.value(), arb::axial_resistivity{param.RL.value()});
      if (param.Cm) decor.paint(rg.data.value(), arb::membrane_capacitance{param.Cm.value()});
      if (param.TK) decor.paint(rg.data.value(), arb::temperature_K{param.TK.value()});
      if (param.Vm) decor.paint(rg.data.value(), arb::init_membrane_potential{param.Vm.value()});

      for (const auto& ion: state.ions) {
        const auto& data = state.ion_par_defs[{id, ion}];
        const auto& name = state.ion_defs[ion].name;
        if (data.Xi) decor.paint(rg.data.value(), arb::init_int_concentration{name,  data.Xi.value()});
        if (data.Xo) decor.paint(rg.data.value(), arb::init_ext_concentration{name,  data.Xo.value()});
        if (data.Er) decor.paint(rg.data.value(), arb::init_reversal_potential{name, data.Er.value()});
      }

      for (const auto child: state.mechanisms.get_children(id)) {
        auto item = state.mechanisms[child];
        auto name = fmt::format("{}::{}", item.cat, item.name);
        auto sep  = "/";
        for (const auto& [k, v]: item.globals) {
          name = fmt::format("{}{}{}={}", name, sep, k, v);
          sep  = ",";
        }
        auto mech = arb::mechanism_desc(name);
        for(const auto& [k, v]: item.parameters) {
          mech.set(k, v);
        }
        decor.paint(rg.data.value(), mech);
      }
    }
    return {state.builder.morph, state.builder.labels, decor};
  }

  void gui_traces(gui_state& state) {
    static std::optional<id_type> to_plot;
    if (ImGui::Begin("Traces")) {
      if (ImGui::BeginChild("TracePlot", {-180.0f, 0.0f})) {
        if (to_plot) {
          auto probe = to_plot.value();
          auto trace = state.sim.traces.at(probe);
          auto probe_def = state.probes[probe];
          auto var = fmt::format("{} {}", probe_def.kind, probe_def.variable);

          if (ImPlot::BeginPlot(fmt::format("Probe {} @ branch {} ({})", probe.value, trace.branch, trace.location).c_str(),
                                "Time (ms)",
                                var.c_str(),
                                ImVec2(-1, -20),
                                ImPlotFlags_NoLegend,
                                ImPlotAxisFlags_AutoFit,
                                ImPlotAxisFlags_AutoFit)) {
            ImPlot::PlotLine(var.c_str(), trace.times.data(), trace.values.data(), trace.values.size());
            ImPlot::EndPlot();
          }
        } else {
          if (ImPlot::BeginPlot("Please select a probe below",
                                "Time (ms)",
                                "Unknown",
                                ImVec2(-1, -20),
                                ImPlotFlags_NoLegend,
                                ImPlotAxisFlags_AutoFit,
                                ImPlotAxisFlags_AutoFit)) {
            ImPlot::EndPlot();
          }
        }
      }
      ImGui::EndChild();
      ImGui::SameLine();
      if (ImGui::BeginChild("TraceSelect", {150.0f, 0.0f})) {
        for (const auto& locset: state.locsets) {
          with_id id{locset};
          auto locset_def = state.locset_defs[locset];
          if (gui_tree(fmt::format("{} {}", icon_locset, locset_def.name))) {
            for (const auto& probe: state.probes.get_children(locset)) {
              const auto& data = state.probes[probe];
              if(ImGui::RadioButton(fmt::format("{} {}: {} {}", icon_probe, probe.value, data.kind, data.variable).c_str(),
                                    to_plot && (to_plot.value() == probe))) {
                to_plot = probe;
              }
            }
            ImGui::TreePop();
          }
        }
      }
      ImGui::EndChild();
    }
    ImGui::End();
  }
} // namespace

void gui_state::gui() {
  update();
  gui_main(*this);
  gui_locations(*this);
  gui_cell(*this);
  gui_traces(*this);
  gui_cell_info(*this);
  gui_parameters(*this);
  gui_simulation(*this);
  handle_keys();
}

void gui_state::serialize(const std::filesystem::path& fn) {
  std::ofstream fd(fn);
  auto cell = make_cable_cell(*this);
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
    std::string tag;

    ls_visitor(gui_state* s, const arb::locset& l, const std::string& t): state{s}, tag{t} {
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
      auto& data = state->detectors[id];
      data.threshold = t.threshold;
      data.tag       = tag;
    }
    void operator()(const arb::i_clamp& t)            {
      auto id        = state->stimuli.add(locset);
      auto& data     = state->stimuli[id];
      data.frequency = t.frequency;
      data.phase     = t.phase;
      data.tag       = tag;
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
      { auto sep = std::find(name.begin(), name.end(), ':'); name.erase(name.begin(), sep + 2); }
      { auto sep = std::find(cat.begin(),  cat.end(),  ':'); cat.erase(sep, cat.end()); }
      log_debug("Loading mech {} from cat {} ({})", name, cat, t.name());
      // TODO What about derived mechs?
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
      for (const auto& [k, v, t]: decor.placements()) std::visit(ls_visitor{state, k, t}, v);
      for (const auto& [k, v]: decor.paintings())  std::visit(rg_visitor{state, k}, v);
      auto defaults = decor.defaults();
      state->parameter_defaults.Cm = defaults.membrane_capacitance;
      state->parameter_defaults.RL = defaults.axial_resistivity;
      state->parameter_defaults.TK = defaults.temperature_K;
      state->parameter_defaults.Vm = defaults.init_membrane_potential;
      for (const auto& [k, v]: defaults.ion_data) {
        auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                                [&, name=k](const auto& id) { return state->ion_defs[id].name == name; });
        if (ion == state->ions.end()) log_error("Unknown ion {}.", k);
        auto& data = state->ion_defaults[*ion];
        if (v.init_reversal_potential) data.Er = v.init_reversal_potential.value();
        if (v.init_int_concentration)  data.Xi = v.init_int_concentration.value();
        if (v.init_ext_concentration)  data.Xo = v.init_ext_concentration.value();
      }
      for (const auto& [k, v]: defaults.reversal_potential_method) {
        auto ion = std::find_if(state->ions.begin(), state->ions.end(),
                                [&, name=k](const auto& id) { return state->ion_defs[id].name == name; });
        if (ion == state->ions.end()) log_error("Unknown ion {}.", k);
        state->ion_defaults[*ion].method = v.name();
      }
    }
  };

  std::visit(acc_visitor{this}, thing.value().component);
}
gui_state::gui_state(): builder{} { reset(); }

void gui_state::reset() {
  locsets.clear();
  regions.clear();
  locset_defs.clear();
  region_defs.clear();
  ions.clear();
  ion_defs.clear();
  probes.clear();
  detectors.clear();
  ion_defaults.clear();
  mechanisms.clear();
  segment_to_regions.clear();
  renderer.clear();
  const static std::vector<std::pair<std::string, int>> species{{"na", 1}, {"k", 1}, {"ca", 2}};
  for (const auto& [k, v]: species) add_ion(k, v);
}

void gui_state::reload(const io::loaded_morphology& result) {
  reset();
  builder = cell_builder{result.morph};
  renderer.load_geometry(result.morph, true);
  for (const auto& [k, v]: result.regions) add_region(k, v);
  for (const auto& [k, v]: result.locsets) add_locset(k, v);
  cv_policy_def.definition = "";
  update_cv_policy();
}

void gui_state::update() {
  struct event_visitor {
    gui_state* state;

    event_visitor(gui_state* state_): state{state_} {}

    void operator()(const evt_upd_cv&) {
      auto& def = state->cv_policy_def;
      auto& rnd = state->renderer.cv_boundaries;
      def.update();
      if (def.state != def_state::error) {
        try {
          auto points = state->builder.make_boundary(def.data.value());
          state->renderer.make_marker(points, rnd);
        } catch (const arb::arbor_exception& e) {
          def.set_error(e.what()); rnd.active = false;
        }
      }
      if (def.definition.empty()) rnd.active = false;
    }
    void operator()(const evt_add_locdef<ls_def>& c) {
      auto ls = state->locsets.add();
      state->locset_defs.add(ls, {c.name.empty() ? fmt::format("Locset {}", ls.value) : c.name, c.definition});
      state->renderer.locsets.add(ls);
      state->renderer.locsets[ls].color = next_color();
      state->update_locset(ls);
    }
    void operator()(const evt_upd_locdef<ls_def>& c) {
      auto& def = state->locset_defs[c.id];
      auto& rnd = state->renderer.locsets[c.id];
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
      auto id = c.id;
      log_debug("Erasing locset {}", id.value);
      state->renderer.locsets.del(id);
      state->locset_defs.del(id);
      state->probes.del_children(id);
      state->detectors.del_children(id);
      state->locsets.del(id);
      state->builder.make_label_dict(state->locset_defs.items, state->region_defs.items);
    }
    void operator()(const evt_add_locdef<rg_def>& c) {
      auto id = state->regions.add();
      state->region_defs.add(id, {c.name.empty() ? fmt::format("Region {}", id.value) : c.name, c.definition});
      state->parameter_defs.add(id);
      state->renderer.regions.add(id);
      state->renderer.regions[id].color = next_color();
      for (const auto& ion: state->ions) state->ion_par_defs.add(id, ion);
      state->update_region(id);
    }
    void operator()(const evt_upd_locdef<rg_def>& c) {
      auto& def = state->region_defs[c.id];
      auto& rnd = state->renderer.regions[c.id];
      for(auto& [segment, regions]: state->segment_to_regions) regions.erase(c.id);
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
      auto id = c.id;
      state->renderer.regions.del(id);
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
    void operator()(const evt_add_mechanism& c) {  state->mechanisms.add(c.region); }
    void operator()(const evt_del_mechanism& c) {  state->mechanisms.del(c.id); }
    void operator()(const evt_add_detector& c)  {
      auto id = state->detectors.add(c.locset);
      auto& data = state->detectors[id];
      if (data.tag.empty()) data.tag = fmt::format("Detector {}", id.value);
    }
    void operator()(const evt_del_detector& c)  {  state->detectors.del(c.id); }
    void operator()(const evt_add_probe& c)     {  state->probes.add(c.locset); }
    void operator()(const evt_del_probe& c)     {  state->probes.del(c.id); }
    void operator()(const evt_add_stimulus& c)  {
      auto id = state->stimuli.add(c.locset);
      auto& data = state->stimuli[id];
      if (data.tag.empty()) data.tag = fmt::format("I Clamp {}", id.value);
    }
    void operator()(const evt_del_stimulus& c)  {  state->stimuli.del(c.id); }
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
  return !!stbi_write_png(snapshot_path.c_str(), w, h, c, pixels.data() + c*w*(h - 1), -c*w);
}

void gui_state::handle_keys() {
  if (ImGui::IsKeyPressed('O') && ImGui::GetIO().KeyCtrl) open_morph_read = true;
}

void gui_state::run_simulation() {
  auto cell  = make_cable_cell(*this);

  auto prop  = arb::cable_cell_global_properties{};
  prop.default_parameters = presets;
  if (parameter_defaults.TK) prop.default_parameters.temperature_K           = parameter_defaults.TK;
  if (parameter_defaults.RL) prop.default_parameters.axial_resistivity       = parameter_defaults.RL;
  if (parameter_defaults.Cm) prop.default_parameters.membrane_capacitance    = parameter_defaults.Cm;
  if (parameter_defaults.Vm) prop.default_parameters.init_membrane_potential = parameter_defaults.Vm;

  auto cat = arb::mechanism_catalogue{};
  for (const auto& [k, v]: catalogues) cat.import(v, k + "::");
  prop.catalogue = &cat;

  for (const auto& ion: ions) {
    const auto& data   = ion_defs[ion];
    const auto& def    = ion_defaults[ion];
    const auto& name   = data.name;

    if (presets.ion_data.contains(name)) {
      auto p = presets.ion_data.at(name);
      prop.add_ion(name,
                   data.charge,
                   def.Xi.value_or(p.init_int_concentration.value()),
                   def.Xo.value_or(p.init_ext_concentration.value()),
                   def.Er.value_or(p.init_reversal_potential.value()));
    } else {
      prop.add_ion(name,
                   data.charge,
                   def.Xi.value(),
                   def.Xo.value(),
                   def.Er.value());
    }
  }
  auto rec = make_recipe(prop, cell);
  for (const auto& ls: locsets) {
    for (const auto pb: probes.get_children(ls)) {
      const auto& data = probes[pb];
      const auto& where = locset_defs[ls];
      if (!where.data) continue;
      auto loc = where.data.value();
      if (data.kind == "Voltage") {
        rec.probes.emplace_back(arb::cable_probe_membrane_voltage{loc}, pb.value);
      } else if (data.kind == "Axial Current") {
        rec.probes.emplace_back(arb::cable_probe_axial_current{loc}, pb.value);
      } else if (data.kind == "Membrane Current") {
        rec.probes.emplace_back(arb::cable_probe_total_ion_current_density{loc}, pb.value);
      }
      // TODO Finish
    }
  }
  // Make simulation
  auto ctx = arb::make_context();
  auto dd  = arb::partition_load_balance(rec, ctx);
  auto sm  = arb::simulation(rec, dd, ctx);
  sim.traces.clear();
  sm.add_sampler(arb::all_probes,
                 arb::regular_schedule(this->sim.dt),
                 [&](arb::probe_metadata pm, std::size_t n, const arb::sample_record* samples) {
                    auto loc = arb::util::any_cast<const arb::mlocation*>(pm.meta); 
                    trace t{(size_t)pm.tag, pm.index, loc->pos, loc->branch, {}, {}};
                    for (std::size_t i = 0; i<n; ++i) {
                      const double* value = arb::util::any_cast<const double*>(samples[i].data);
                      t.times.push_back(samples[i].time);
                      t.values.push_back(*value);
                    }
                    sim.traces[t.id] = t;                    
                  },
                 arb::sampling_policy::exact);
  try {
    sm.run(sim.until, sim.dt);
  } catch (...) {
      ImGui::OpenPopup("Error");
  }
  if (ImGui::BeginPopupModal("Error")) {
    ImGui::Text("Arbor failed to run.");
    if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
  }
}
