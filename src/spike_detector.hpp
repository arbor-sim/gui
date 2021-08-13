#pragma once

#include "events.hpp"

struct detector_def {
    double threshold = 0;  // [mV]
    std::string tag;
};

void gui_detector(id_type id, detector_def& data, event_queue& evts);
