#pragma once

#include <array>
#include <string>

#include "events.hpp"

struct probe_def {
    double frequency     = 1000.0; // [Hz]
    std::string kind     = kinds.front();
    std::string variable;
    constexpr static std::array kinds{"Voltage", "Axial Current", "Membrane Current", "Internal Concentration", "External Concentration", "Mechanism State"};
};

void gui_probe(const id_type& id, probe_def& data, event_queue& evts,
               const std::vector<std::string>&, const std::vector<std::string>&);
