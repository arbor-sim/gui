#include "probe.hpp"

#include "gui.hpp"

void gui_probe(const id_type& id, probe_def& data, event_queue& evts,
               const std::vector<std::string>& ion_names,
               const std::vector<std::string>& state_variables) {
    with_id guard{id};
    ImGui::Bullet();
    ImGui::SameLine();
    with_item_width width {180.f};
    gui_input_double("Frequency", data.frequency, "Hz");
    gui_right_margin();
    if (ImGui::Button(icon_delete)) evts.push_back(evt_del_probe{id});
    with_indent indent{ImGui::GetTreeNodeToLabelSpacing()};
    gui_choose("Kind", data.kind, probe_def::kinds);
    if ((data.kind == "Membrane Current") || (data.kind == "Internal Concentration") || (data.kind == "External Concentration")) {
        if (ImGui::BeginCombo("Ion Species", data.variable.c_str())) {
            for (const auto& name: ion_names) {
                if (ImGui::Selectable(name.c_str(), name == data.variable)) data.variable = name;
            }
            ImGui::EndCombo();
        }
    } if (data.kind == "Mechanism State") {
        if (ImGui::BeginCombo("State Variable", data.variable.c_str())) {
            for (const auto& label: state_variables) {
                if (ImGui::Selectable(label.c_str(), label == data.variable)) data.variable = label;
            }
            ImGui::EndCombo();
        }
    } else {
        // data.variable = "";
    }
}
