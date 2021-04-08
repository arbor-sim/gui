#pragma once

#include <arbor/cable_cell.hpp>
#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include <glm/glm.hpp>

#include "location.hpp"

struct cell_builder {
    arb::morphology   morph;
    arb::place_pwlin  pwlin;
    arb::label_dict   labels;
    arb::mprovider    provider;

    cell_builder();
    cell_builder(const arb::morphology& t);

    std::vector<arb::msegment> make_segments(const arb::region&);
    std::vector<glm::vec3> make_points(const arb::locset&);
    void make_label_dict(std::vector<ls_def>& locsets, std::vector<rg_def>& regions);

    arb::cable_cell make_cell();
};
