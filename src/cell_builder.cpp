#include "cell_builder.hpp"

#include "utils.hpp"

cell_builder::cell_builder()
    : morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
cell_builder::cell_builder(const arb::morphology &t)
    : morph{t}, pwlin{morph}, labels{}, provider{morph, labels} {};

void cell_builder::make_label_dict(std::vector<ls_def>& locsets, std::vector<rg_def>& regions) {
  labels = {};
  for (auto& item: locsets) {
    if (item.data) {
      if (labels.locset(item.name)) {
        item.state   = def_state::error;
        item.message = "Duplicate name; ignored.";
      } else {
        try {
          labels.set(item.name, item.data.value());
        } catch (const arb::arbor_exception& e) {
          item.state   = def_state::error;
          item.message = e.what();
        }
      }
    }
  }
  for (auto& item: regions) {
    if (item.data) {
      if (labels.region(item.name)) {
        item.state   = def_state::error;
        item.message = "Duplicate name; ignored.";
      } else {
        try {
          labels.set(item.name, item.data.value());
        } catch (const arb::arbor_exception& e) {
          item.state   = def_state::error;
          item.message = e.what();
        }
      }
    }
  }
  provider = {morph, labels};
}

std::vector<arb::msegment> cell_builder::make_segments(const arb::region& region) {
  auto concrete = thingify(region, provider);
  auto result = pwlin.all_segments(concrete);
  std::remove_if(result.begin(), result.end(), [](const auto& s) { return s.dist == s.prox; });
  return result;
}

std::vector<glm::vec3> cell_builder::make_points(const arb::locset& locset) {
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

std::vector<glm::vec3> cell_builder::make_boundary(const arb::cv_policy& cv) {
  auto cell = arb::cable_cell(morph, labels, {});
  return make_points(cv.cv_boundary_points(cell));
}
