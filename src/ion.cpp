#include "ion.hpp"

#include "gui.hpp"

void gui_ion_parameter(id_type id,
                       const ion_def& def, ion_parameter& data,
                       const ion_default& fallback, const std::unordered_map<std::string, arb::cable_cell_ion_data>& preset) {
    with_id guard{id};
    auto name = def.name;
    if (gui_tree(name)) {
        with_item_width width{120.0f};
        if (preset.contains(name)) {
            auto p = preset.at(name);
            gui_defaulted_double("Int. Concentration", "mM", data.Xi, fallback.Xi, p.init_int_concentration);
            gui_defaulted_double("Ext. Concentration", "mM", data.Xo, fallback.Xo, p.init_ext_concentration);
            gui_defaulted_double("Reversal Potential", "mV", data.Er, fallback.Er, p.init_reversal_potential);
        } else {
            gui_defaulted_double("Int. Concentration", "mM", data.Xi, fallback.Xi);
            gui_defaulted_double("Ext. Concentration", "mM", data.Xo, fallback.Xo);
            gui_defaulted_double("Reversal Potential", "mV", data.Er, fallback.Er);
        }
        ImGui::TreePop();
    }
}

void gui_ion_default(id_type id, ion_def& definition, ion_default& data, const std::unordered_map<std::string, arb::cable_cell_ion_data>& preset, event_queue& evts) {
    with_id guard{id};
    with_item_width item_width{120.0f};
    auto open = gui_tree("##ion-tree");
    ImGui::SameLine();
    ImGui::InputText("##ion-name", &definition.name, ImGuiInputTextFlags_AutoSelectAll);
    gui_right_margin();
    if (ImGui::Button(icon_delete)) evts.push_back(evt_del_ion{id});
    if (open) {
        auto name = definition.name;
        if (preset.contains(name)) {
            auto p = preset.at(name);
            auto indent = gui_tree_indent();
            ImGui::InputInt("Charge", &definition.charge);
            gui_defaulted_double("Int. Concentration", "mM", data.Xi, p.init_int_concentration);
            gui_defaulted_double("Ext. Concentration", "mM", data.Xo, p.init_ext_concentration);
            gui_choose("##rev-pot-method", data.method, ion_default::methods);
            ImGui::SameLine();
            {
                with_item_width width{40.0f};
                gui_defaulted_double("Reversal Potential", "mV", data.Er, p.init_reversal_potential);
            }
        } else {
            auto indent = gui_tree_indent();
            ImGui::InputInt("Charge", &definition.charge);
            gui_input_double("Int. Concentration", data.Xi, "mM");
            gui_input_double("Ext. Concentration", data.Xo, "mM");
            gui_choose("##rev-pot-method", data.method, ion_default::methods);
            ImGui::SameLine();
            {
                with_item_width width{40.0f};
                gui_input_double("Reversal Potential", data.Er, "mV");
            }
        }
        ImGui::TreePop();
    }
}
