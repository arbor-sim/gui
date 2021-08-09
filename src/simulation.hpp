#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "id.hpp"
#include "utils.hpp"

struct trace {
    id_type id;
    size_t index;
    double location;
    size_t branch;
    bool show = true;
    std::vector<float> times;
    std::vector<float> values;
};


struct simulation {
    double until = 100;
    double dt    = 0.05;

    bool should_run = false;
    bool show_trace = false;

    std::unordered_map<id_type, trace> traces;
};

void gui_sim(simulation&);
