#include "spike_detector.hpp"

#include "gui.hpp"
#include "icons.hpp"

void gui_detector(id_type id, detector_def& data, event_queue& evts) {
    with_id guard{id};
    ImGui::Bullet();
    ImGui::SameLine();
    with_item_width width {120.f};
    gui_input_double("Threshold", data.threshold, "mV");
    gui_right_margin();
    if (ImGui::Button(icon_delete)) evts.push_back(evt_del_detector{id});
}
