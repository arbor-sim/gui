#include "file_chooser.hpp"
#include "icons.hpp"
#include "gui.hpp"

void gui_dir_view(file_chooser_state& state) {
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
    ImGui::SameLine(40.0f);
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
        if (ImGui::IsItemHovered() && (ImGui::IsMouseDoubleClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
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
