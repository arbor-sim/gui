#pragma once

#include <string>

#include "utils.hpp"

struct simulation {
    double until = 1000;
    double dt    = 0.05;
};

void gui_sim(simulation&);
