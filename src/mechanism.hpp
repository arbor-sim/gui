#pragma once

#include <unordered_map>
#include <string>
#include <compare>

#include <arbor/mechcat.hpp>

#include "events.hpp"

struct mechanism_def {
    std::string name = "";
    std::string cat  = "";
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

static const std::unordered_map<std::string, arb::mechanism_catalogue> default_catalogues = {{"default", arb::global_default_catalogue()},
                                                                                             {"allen",   arb::global_allen_catalogue()},
                                                                                             {"BBP",     arb::global_bbp_catalogue()}};
extern std::unordered_map<std::string, arb::mechanism_catalogue> catalogues;

void make_mechanism(mechanism_def& data,
                    const std::string& cat_name, const std::string& name,
                    const std::unordered_map<std::string, double>& values={});

void gui_mechanism(id_type, mechanism_def&, event_queue&);
