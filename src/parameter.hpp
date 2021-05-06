#pragma once

#include <optional>

#include <arbor/cable_cell_param.hpp>

struct parameter_def {
    std::optional<double> TK, Cm, Vm, RL;
};

void gui_parameter_defaults(parameter_def& to_set, const arb::cable_cell_parameter_set& defaults);

void gui_parameter(parameter_def& to_set, const parameter_def& defaults, const arb::cable_cell_parameter_set& fallback);
