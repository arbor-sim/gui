#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/label_parse.hpp>
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
        trim(definition);
        if (definition.empty() || !definition[0]) {
            data = {}; state = def_state::empty; message = "Empty.";
            return;
        }

        try {
            data = {definition}; state = def_state::good;  message = "Ok.";
        } catch (const arb::label_parse_error &e) {
            set_error(e.what());
        }
    }
};

using ls_def = loc_def<arb::locset>;
using rg_def = loc_def<arb::region>;
