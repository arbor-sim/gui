#include "mechanism.hpp"

#include <fmt/format.h>

#include "utils.hpp"
#include "gui.hpp"

std::unordered_map<std::string, arb::mechanism_catalogue> catalogues = default_catalogues;

void make_mechanism(mechanism_def& data,
                    const std::string& cat_name, const std::string& name,
                    const std::unordered_map<std::string, double>& values) {
    if (name.empty()) log_error("Empty mechanism name. Selector must be 'cat::mech'.");
    data.name = name;
    data.cat  = cat_name;
    if (!catalogues.contains(data.cat)) log_error(fmt::format("Unknown catalogue: {}", data.cat));
    auto cat = catalogues[data.cat];
    if (!cat.has(data.name)) log_error(fmt::format("Unknown mechanism {} in catalogue {}", data.name, data.cat));
    auto info = cat[data.name];
    data.globals.clear();
    data.parameters.clear();
    data.states.clear();
    log_debug("Setting values");
    for (const auto& [k, v]: info.globals)    data.globals[k]    = values.contains(k) ? values.at(k) : v.default_value;
    for (const auto& [k, v]: info.parameters) data.parameters[k] = values.contains(k) ? values.at(k) : v.default_value;
    for (const auto& [k, v]: info.state)      data.states[k]     = values.contains(k) ? values.at(k) : v.default_value;
}

void gui_mechanism(id_type id, mechanism_def& data, const std::vector<ie_def>& ies, event_queue& evts) {
    with_id mech_guard{id};
    auto open = gui_tree("##mechanism-tree");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##mechanism-choice", data.name.c_str())) {
        for (const auto& [cat_name, cat]: catalogues) {
            ImGui::Selectable(cat_name.c_str(), false);
            with_indent ind{};
            for (const auto& name: cat.mechanism_names()) {
                if (cat[name].kind != arb_mechanism_kind_density) continue;
                if (gui_select(fmt::format("  {}##{}", name, cat_name), data.name)) make_mechanism(data, cat_name, name);
            }
        }
        ImGui::EndCombo();
    }
    gui_right_margin();
    if (ImGui::Button(icon_delete)) evts.push_back(evt_del_mechanism{id});
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
            for (auto& [k, v]: data.parameters) {
                {
                    with_id id(k);
                    with_item_width iw(80.0f);
                    auto none = data.scales.contains(k);
                    auto lbl  = data.scales[k].value_or("-");
                    if (ImGui::BeginCombo("Scale", lbl.c_str())) {
                        if (ImGui::Selectable("-", lbl == "-")) data.scales[k] = {};
                        for (const auto& ie: ies) {
                            if (ie.state != def_state::good) continue;
                            auto& nm = ie.name;
                            if (ImGui::Selectable(nm.c_str(), nm == lbl)) data.scales[k] = nm;
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::SameLine();
                gui_input_double(k, v);
            }
        }
        ImGui::TreePop();
    }
}
