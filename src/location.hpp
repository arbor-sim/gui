#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/label_parse.hpp>
#include <arbor/cv_policy_parse.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include "utils.hpp"

enum class def_state { empty, error, good };

template<typename T>
struct loc_def {
    std::string name = {}, definition = {};
    std::optional<T> data = {};

    def_state state = def_state::empty;
    std::string message = "Empty.";

    void set_error(const std::string& m) {
        data = {};
        auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
        state = def_state::error; message = m.substr(colon, m.size() - 1);
    }

    loc_def(const loc_def&) = default;
    loc_def& operator=(const loc_def&) = default;
    loc_def(const std::string_view n, const std::string_view d): name{n}, definition{d} { update(); }

    void update() {
        auto def = trim_copy(definition);
        if (def.empty() || !def[0]) {
            data = {}; state = def_state::empty; message = "Empty.";
            return;
        }
        try {
            data = {def}; state = def_state::good; message = "Ok.";
        } catch (const arb::label_parse_error &e) {
            set_error(e.what());
        }
    }
};

using ls_def = loc_def<arb::locset>;
using rg_def = loc_def<arb::region>;

struct cv_def {
    std::string definition = {};
    def_state   state      = def_state::empty;
    std::string message    = "Empty. Using default.";
    std::optional<arb::cv_policy> data = {};

    cv_def() = default;
    cv_def(const cv_def&) = default;
    cv_def& operator=(const cv_def&) = default;
    cv_def(const std::string_view d): definition{d} { update(); }

    void set_error(const std::string& m) {
        data = {};
        auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
        state = def_state::error; message = m.substr(colon, m.size() - 1);
    }

    void update() {
        auto def = trim_copy(definition);
        if (def.empty() || !def[0]) {
            data  = {arb::default_cv_policy()};
            state = def_state::empty; message = "Empty. Using default.";
            return;
        }
        try {
            auto p = arb::cv::parse_expression(def);
            if (p) {
                data = p.value();
                state = def_state::good;
                message = "Ok.";
            } else {
                set_error(p.error().what());
            }
        } catch (const arb::cv::parse_error &e) {
            set_error(e.what());
        }
    }
};
