#include "simulation.hpp"

#include "gui.hpp"
#include "icons.hpp"

void gui_sim(simulation& sim) {
    if (gui_tree(fmt::format("{} Settings", icon_gear))) {
        with_item_width width(120.0f);
        gui_input_double("End time",  sim.until, "ms");
        gui_input_double("Time step", sim.dt,    "ms");
        ImGui::TreePop();
    }
}
