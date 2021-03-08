#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include "utils.hpp"

enum class def_state { empty, error, good };

struct definition {
    void good()                      { state = def_state::good;  message = "Ok."; }
    void empty()                     { state = def_state::empty; message = "Empty."; }
    void error(const std::string& m) { state = def_state::error; message = m; }
    def_state state = def_state::empty;
    std::string message = "New.";
};

struct loc_def: definition {
    std::string name, definition;

    loc_def(const loc_def&) = default;
    loc_def& operator=(const loc_def&) = default;
    loc_def(const std::string_view n, const std::string_view d);
};

struct reg_def: loc_def {
    std::optional<arb::region> data = {};

    reg_def(const reg_def&) = default;
    reg_def& operator=(const reg_def&) = default;
    reg_def(const std::string_view n, const std::string_view d);
    reg_def();
};

struct ls_def: loc_def {
    std::optional<arb::locset> data = {};

    ls_def(const ls_def&) = default;
    ls_def& operator=(const ls_def&) = default;
    ls_def(const std::string_view n, const std::string_view d);
    ls_def();
};
