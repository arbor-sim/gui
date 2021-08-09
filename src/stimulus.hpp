#pragma once

#include <vector>

#include "events.hpp"

struct stimulus_def {
    double frequency = 0.0; // Hz
    double phase     = 0.0; // Radians
    std::vector<std::pair<double, double>> envelope;
};

void gui_stimulus(const id_type&, stimulus_def&, event_queue&, std::vector<float>&, double, double);
