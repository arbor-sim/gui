#include "stimulus.hpp"

#include "gui.hpp"

void gui_stimulus(const id_type& id, stimulus_def& data, event_queue& evts) {
    with_id guard{id};
    with_item_width iw{160.0f};
    auto open = gui_tree("I Clamp");
    if (open) {
        bool clean = false;
        double z = 0.0, pi2 = 2*PI;
        ImGui::SliderScalar("Phase", ImGuiDataType_Double, &data.phase, &z, &pi2, "%.3f");
        gui_right_margin();
        if (ImGui::Button(icon_delete)) evts.push_back(evt_del_stimulus{id});
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
}
