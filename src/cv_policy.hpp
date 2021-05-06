#pragma once

#include <string>
#include <optional>

#include <arbor/cv_policy.hpp>

#include "geometry.hpp"
#include "events.hpp"
#include "utils.hpp"

struct cv_def {
    std::string definition = {};
    def_state   state      = def_state::empty;
    std::string message    = "Empty. Using default.";
    std::optional<arb::cv_policy> data = {};

    cv_def() = default;
    cv_def(const cv_def&) = default;
    cv_def& operator=(const cv_def&) = default;
    cv_def(const std::string_view d): definition{d} { update(); }

    void set_error(const std::string& m);
    void update();
};

void gui_cv_policy(cv_def& item, renderable& render, event_queue& evts);
