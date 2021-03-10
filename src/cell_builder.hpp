#pragma once

#include <arbor/cable_cell.hpp>
#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/morph/label_parse.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/mechinfo.hpp>

#include <glm/glm.hpp>

struct cell_builder {
    arb::morphology   morph;
    arb::place_pwlin  pwlin;
    arb::label_dict   labels;
    arb::mprovider    provider;

    cell_builder();
    cell_builder(const arb::morphology& t);

    std::vector<size_t> make_segments(const arb::region&);
    std::vector<glm::vec3> make_points(const arb::locset&);

    arb::cable_cell make_cell();
};
