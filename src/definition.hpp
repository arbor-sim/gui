#pragma once

#include <string>

#include "utils.hpp"

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

struct probe_def {
    double frequency = 1000; // [Hz]
    std::string variable = variables.front();
    constexpr static std::array<const char*, 2> variables{"Voltage", "Current"};
};

struct stimulus_def {
    double delay     = 0;  // [ms]
    double duration  = 0;  // [ms]
    double amplitude = 0;  // [nA]
};

struct detector_def {
    double threshold = 0;  // [mV]
};

struct parameter_def {
    std::optional<double> TK, Cm, Vm, RL;
};

struct mechanism_def {
    std::string name = "";
    std::unordered_map<std::string, double> parameters  = {};
    std::unordered_map<std::string, double> global_vars = {};
};
