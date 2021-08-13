#include "spike_detector.hpp"

#include "gui.hpp"
#include "icons.hpp"

void gui_detector(id_type id, detector_def& data, event_queue& evts) {
    with_id guard{id};
    with_item_width width {120.f};
    auto open = gui_tree("##detector");
    ImGui::SameLine();
    ImGui::InputText("##detector", &data.tag, ImGuiInputTextFlags_AutoSelectAll);
    gui_right_margin();
    if (ImGui::Button(icon_delete)) evts.push_back(evt_del_detector{id});
    if (open) {
        gui_input_double("Threshold", data.threshold, "mV");
    }
}
