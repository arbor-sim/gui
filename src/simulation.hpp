#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "id.hpp"

struct trace {
    std::string tag;
    id_type id;
    size_t index;
    double location;
    size_t branch;
    bool show = true;
    std::vector<float> times;
    std::vector<float> values;

    trace(const std::string t, const id_type i, size_t x, const double l, const size_t b):
        tag{std::move(t)}, id{i}, index{x}, location{l}, branch{b}
    {}
};


struct simulation {
    double until = 100;
    double dt    = 0.05;

    bool should_run = false;
    bool show_trace = false;

    std::unordered_map<std::string, id_type> tag_to_id;
    std::vector<trace> traces;
};

void gui_sim(simulation&);
