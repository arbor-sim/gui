#include "simulation.hpp"

#include "gui.hpp"
#include "icons.hpp"

void gui_sim(simulation& sim) {
    with_item_width width(120.0f);

    sim.should_run = ImGui::Button(fmt::format("{} Run", icon_start).c_str());
    gui_input_double("End time",  sim.until, "ms");
    gui_input_double("Time step", sim.dt,    "ms");
}
