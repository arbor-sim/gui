#pragma once

#include <string>
#include <array>
#include <optional>

#include <arbor/cable_cell_param.hpp>

#include "events.hpp"

struct ion_def {
    std::string name = "";
    int charge = 0;

    ion_def() = default;
    ion_def(const ion_def&) = default;
    ion_def& operator=(const ion_def&) = default;
    ion_def(const std::string_view n, int c): name{n}, charge{c} {}
};

struct ion_parameter {
    std::optional<double> Xi, Xo, Er;
};

struct ion_default {
    double Xi = 0, Xo = 0, Er = 0;
    std::string method = methods.front();
    constexpr static std::array<const char*, 2> methods{"const.", "Nernst"};
};

void gui_ion_parameter(id_type, const ion_def&, ion_parameter&, const ion_default&, const std::unordered_map<std::string, arb::cable_cell_ion_data>&);
void gui_ion_default(id_type, ion_def&, ion_default&, const std::unordered_map<std::string, arb::cable_cell_ion_data>&, event_queue&);
