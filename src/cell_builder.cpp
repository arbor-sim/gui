#include "cell_builder.hpp"

#include "utils.hpp"

cell_builder::cell_builder()
    : morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
cell_builder::cell_builder(const arb::morphology &t)
    : morph{t}, pwlin{morph}, labels{}, provider{morph, labels} {};

std::vector<size_t> cell_builder::make_segments(const arb::region &region) {
  auto concrete = thingify(region, provider);
  std::vector<size_t> result;
  for (const auto& segment: pwlin.all_segments(concrete)) result.push_back(segment.id);
  return result;
}

std::vector<glm::vec3> cell_builder::make_points(const arb::locset &locset) {
  auto concrete = thingify(locset, provider);
  std::vector<glm::vec3> points(concrete.size());
  std::transform(concrete.begin(), concrete.end(), points.begin(),
                 [&](const auto &loc) {
                   auto p = pwlin.at(loc);
                   return glm::vec3{p.x, p.y, p.z};
                 });
  log_debug("Made locset {} markers: {} points", to_string(locset), points.size());
  return points;
}
