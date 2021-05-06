#include "cv_policy.hpp"

#include "gui.hpp"

#include <arbor/cv_policy_parse.hpp>

void cv_def::set_error(const std::string& m) {
    data = {};
    auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
    state = def_state::error; message = m.substr(colon, m.size() - 1);
}

void cv_def::update() {
    auto def = trim_copy(definition);
    if (def.empty() || !def[0]) {
        data  = {arb::default_cv_policy()};
        state = def_state::empty; message = "Empty. Using default.";
        return;
    }
    try {
        auto p = arb::cv::parse_expression(def);
        if (p) {
            data = p.value();
            state = def_state::good;
            message = "Ok.";
        } else {
            set_error(p.error().what());
        }
    } catch (const arb::cv::parse_error &e) {
        set_error(e.what());
    }
}

void gui_cv_policy(cv_def& item, renderable& render, event_queue& evts) {
    auto  open   = gui_tree("CV policy");
    ImGui::SameLine();
    ImGui::ColorEdit3("##cv-bounds-color", &render.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    gui_toggle(icon_show, icon_hide, render.active);
    ImGui::SameLine();
    gui_check_state(item);

    gui_right_margin();
    if (ImGui::Button(icon_refresh)) {
        item.definition = "";
        evts.push_back(evt_upd_cv{});
    }

    if (open) {
        with_item_width iw(-50.0f);
        auto indent = gui_tree_indent();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
        if (ImGui::InputTextMultiline("##cv-definition", &item.definition)) evts.push_back(evt_upd_cv{});
        ImGui::PopTextWrapPos();
        ImGui::TreePop();
    }
}
