#include "cell_builder.hpp"

cell_builder::cell_builder(): tree{}, morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
cell_builder::cell_builder(const arb::segment_tree& t): tree{t}, morph{tree}, pwlin{morph}, labels{}, provider{morph, labels} {};

std::vector<arb::msegment> cell_builder::make_segments(const arb::region& region) {
    auto concrete = thingify(region, provider);
    return pwlin.all_segments(concrete);
}

std::vector<glm::vec3> cell_builder::make_points(const arb::locset& locset) {
    auto concrete = thingify(locset, provider);
    std::vector<glm::vec3> points;
    for (const auto& loc: concrete) {
        for (const auto p: pwlin.all_at(loc)) {
            points.emplace_back((float) p.x, (float) p.y, (float) p.z);
        }
    }
    return points;
}

arb::cable_cell cell_builder::make_cell() { return {morph, labels}; }
