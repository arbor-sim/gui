#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arborio/label_parse.hpp>
#include <arborio/cv_policy_parse.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include "utils.hpp"

struct ls_def {
    std::string name = {}, definition = {};
    std::optional<arb::locset> data = {};

    def_state state = def_state::empty;
    std::string message = "Empty.";

    void set_error(const std::string& m) {
        data = {};
        auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
        state = def_state::error; message = m.substr(colon, m.size() - 1);
    }

    ls_def(const ls_def&) = default;
    ls_def& operator=(const ls_def&) = default;
    ls_def(const std::string_view n, const std::string_view d): name{n}, definition{d} { update(); }

    void update() {
        auto def = trim_copy(definition);
        if (def.empty() || !def[0]) {
            data = {};
            state = def_state::empty; message = "Empty.";
            return;
        }
        if (auto loc = arborio::parse_locset_expression(def)) {
            data = loc.value();
            state = def_state::good;
            message = "Ok.";
        } else {
            set_error(loc.error().what());
        }
    }
};

struct rg_def {
    std::string name = {}, definition = {};
    std::optional<arb::region> data = {};

    def_state state = def_state::empty;
    std::string message = "Empty.";

    void set_error(const std::string& m) {
        data = {};
        auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
        state = def_state::error; message = m.substr(colon, m.size() - 1);
    }

    rg_def(const rg_def&) = default;
    rg_def& operator=(const rg_def&) = default;
    rg_def(const std::string_view n, const std::string_view d): name{n}, definition{d} { update(); }

    void update() {
        auto def = trim_copy(definition);
        if (def.empty() || !def[0]) {
            data = {};
            state = def_state::empty; message = "Empty.";
            return;
        }
        if (auto loc = arborio::parse_region_expression(def)) {
            data = loc.value();
            state = def_state::good;
            message = "Ok.";
        } else {
            set_error(loc.error().what());
        }
    }
};
