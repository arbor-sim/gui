#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/morph/label_parse.hpp>


#include <utils.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>
#include <definition.hpp>

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
