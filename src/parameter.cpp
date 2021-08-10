#include "parameter.hpp"

#include "gui.hpp"
#include "utils.hpp"

void gui_parameter_defaults(parameter_def& to_set, const arb::cable_cell_parameter_set& defaults) {
    with_item_width item_width{120.0f};
    gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.temperature_K);
    gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.init_membrane_potential);
    gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.axial_resistivity);
    gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.membrane_capacitance);
  }

void gui_parameter(parameter_def& to_set, const parameter_def& defaults, const arb::cable_cell_parameter_set& fallback) {
    with_item_width item_width{120.0f};
    gui_defaulted_double("Temperature",          "K",    to_set.TK, defaults.TK, fallback.temperature_K);
    gui_defaulted_double("Membrane Potential",   "mV",   to_set.Vm, defaults.Vm, fallback.init_membrane_potential);
    gui_defaulted_double("Axial Resistivity",    "Ω·cm", to_set.RL, defaults.RL, fallback.axial_resistivity);
    gui_defaulted_double("Membrane Capacitance", "F/m²", to_set.Cm, defaults.Cm, fallback.membrane_capacitance);
}
