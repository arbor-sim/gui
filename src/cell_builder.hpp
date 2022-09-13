#pragma once

#include <vector>
#include <unordered_map>

#include <arbor/cable_cell.hpp>
#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>

#include <glm/glm.hpp>

#include "location.hpp"

struct cell_builder {
    arb::morphology  morph;
    arb::place_pwlin pwlin;
    arb::label_dict  labels;
    arb::mprovider   provider;
    arb::cv_policy   policy = arb::default_cv_policy();

    cell_builder();
    cell_builder(const arb::morphology& t);

    std::vector<arb::msegment> make_segments(const arb::region&);
    std::vector<glm::vec3>     make_points(const arb::locset&);
    std::vector<glm::vec3>     make_boundary(const arb::cv_policy&);
    iexpr_info                 make_iexpr(const arb::iexpr&);
    void                       make_label_dict(std::vector<ls_def>& locsets,
                                               std::vector<rg_def>& regions,
                                               std::vector<ie_def>& iexprs);
};
