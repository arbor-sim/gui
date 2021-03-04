#pragma once

#include <string>

#include "id.hpp"
#include "utils.hpp"


enum class def_state { empty, error, good };

struct definition {
    void good()                      { state = def_state::good;  message = "Ok."; }
    void empty()                     { state = def_state::empty; message = "Empty."; }
    void error(const std::string& m) { state = def_state::error; message = m; }
    def_state state = def_state::empty;
    std::string message = "New.";
};

struct ion_def: definition {
    std::string name = "";
    int charge = 0;

    ion_def() = default;
    ion_def(const ion_def&) = default;
    ion_def& operator=(const ion_def&) = default;
    ion_def(const std::string_view n, int c): name{n}, charge{c} {}
};

struct ion_parameter: definition {
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
    constexpr static std::array<const char*, 2> variables{"Voltage"};
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
