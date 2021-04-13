#pragma once

#include <string>
#include <compare>

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
    double frequency     = 1000.0; // [Hz]
    std::string kind     = kinds.front();
    std::string variable = "";
    constexpr static std::array kinds{"Voltage", "Axial Current", "Membrane Current", "Internal Concentration", "External Concentration", "Mechanism State"};
};

struct stimulus_def {
    double frequency = 0.0; // Hz
    double phase     = 0.0; // Radians
    std::vector<std::pair<double, double>> envelope = {{0.0, 0.0}}; // [(t_start, current)]
};

struct detector_def {
    double threshold = 0;  // [mV]
};

struct parameter_def {
    std::optional<double> TK, Cm, Vm, RL;
};

struct simulation {
    double until = 1000;
    double dt    = 0.05;
};

struct mechanism_def {
    std::string name = "";
    std::unordered_map<std::string, double> parameters = {};
    std::unordered_map<std::string, double> states     = {};
    std::unordered_map<std::string, double> globals    = {};
};

inline std::strong_ordering operator<=>(const mechanism_def& l, const mechanism_def& r) {
    return (l.name.c_str() <=> r.name.c_str());
}

inline bool operator==(const mechanism_def& l, const mechanism_def& r) {
    return (l <=> r) == std::strong_ordering::equal;
}
