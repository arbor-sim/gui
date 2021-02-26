#pragma once

#include <string>

#include "id.hpp"
#include "utils.hpp"


enum class def_state { empty, error, good };

struct definition {
    void good()   { state = def_state::good;    message = "Ok."; }
    void empty()  { state = def_state::empty;   message = "Empty."; }
    void error(const std::string& m) { log_debug("called error {}!", m); state = def_state::error; message = m; }
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

struct ion_par_def: definition {
    std::optional<double> Xi;
    std::optional<double> Xo;
    std::optional<double> Er;
};

struct prb_def: definition {
    double frequency = 1000; // [Hz]
    std::string variable = "voltage";
};

struct stm_def: definition {
    double delay = 0;      // [ms]
    double duration = 0;   // [ms]
    double amplitude = 0;  // [nA]
};

struct sdt_def: definition {
    double threshold = 0;  // [mV]
};

// definitions for paintings
struct par_def: definition {
    std::optional<double> TK, Cm, Vm, RL;
};

struct mech_def: definition {
    mech_def() = default;
    mech_def(const id_type& id): region{id} {}
    std::string name = "";
    id_type region = {};
    std::unordered_map<std::string, double> parameters  = {};
    std::unordered_map<std::string, double> global_vars = {};
};
