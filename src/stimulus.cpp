#include "stimulus.hpp"

#include "gui.hpp"

void gui_stimulus(const id_type& id, stimulus_def& data, event_queue& evts, std::vector<float>& values, double dt, double until) {
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

        auto add_values = [&](const double I0, const double I1, const double t0, const double t1) {
            for (auto t = t0; t < t1; t += dt) {
                auto ix = t/dt;
                auto f = data.frequency ? std::sin(t*2e-3*data.frequency*PI + data.phase) : 1.0f;
                auto I = I0 + (I1 - I0)*(t - t0)/(t1 - t0);
                values[ix] = I*f;
            }
        };

        auto envelope = data.envelope;
        if (!envelope.empty() && dt > 0.0) {
            std::sort(envelope.begin(), envelope.end());
            auto it = envelope.begin();
            auto t = it->first;
            auto I = it->second;
            add_values(0.0, 0.0, 0, t);
            for (; it != envelope.end(); ++it) {
                const auto& [t1, I1] = *it;
                if (t1 <= t) continue;
                add_values(I, I1, t, t1);
                I = I1; t = t1;
            }
            add_values(I, I, t, until);
            ImGui::PlotLines("Preview", values.data(), values.size(), 0, nullptr, FLT_MAX, FLT_MAX, {0, 0});
        }
        ImGui::TreePop();
    }
}
