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
    std::vector<float> times;
    std::vector<float> values;
};


struct simulation {
    double until = 1000;
    double dt    = 0.05;

    std::unordered_map<id_type, trace> traces;
};

void gui_sim(simulation&);
