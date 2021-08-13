#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arborio/label_parse.hpp>
#include <arborio/cv_policy_parse.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include "utils.hpp"

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
        auto loc = arborio::parse_label_expression(def);
        if (loc) {
            data = std::any_cast<T>(loc.value());
            state = def_state::good;
            message = "Ok.";
        } else {
            set_error(loc.error().what());
        }
    }
};

using ls_def = loc_def<arb::locset>;
using rg_def = loc_def<arb::region>;
