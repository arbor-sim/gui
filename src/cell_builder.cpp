#include "cell_builder.hpp"

#include "utils.hpp"

cell_builder::cell_builder()
    : morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
cell_builder::cell_builder(const arb::morphology &t)
    : morph{t}, pwlin{morph}, labels{}, provider{morph, labels} {};

void cell_builder::make_label_dict(std::vector<ls_def>& locsets,
                                   std::vector<rg_def>& regions,
                                   std::vector<ie_def>& iexprs) {
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
  for (auto& item: iexprs) {
    if (item.data) {
      if (labels.iexpr(item.name)) {
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
  std::erase_if(result, [](const auto& s) { return distance(s.dist, s.prox) <= std::numeric_limits<double>::epsilon(); });
  return result;
}

iexpr_info cell_builder::make_iexpr(const arb::iexpr& expr) {
  auto all    = pwlin.all_segments(thingify(arb::reg::all(), provider));
  auto iex    = arb::thingify(expr, provider);
  auto result = iexpr_info{};
  for (const auto& seg: all) {
    const auto& [px, py, pz, pr] = seg.prox;
    const auto& [dx, dy, dz, dr] = seg.dist;
    const auto& [plocs, pdel]    = pwlin.all_closest(px, py, pz);
    const auto& [dlocs, ddel]    = pwlin.all_closest(dx, dy, dz);
    bool found = false;
    for (const auto& dloc: dlocs) {
      for (const auto& ploc: plocs) {
        if (dloc.branch == ploc.branch) {
          float dval = iex->eval(provider, {dloc.branch, dloc.pos, dloc.pos});
          float pval = iex->eval(provider, {ploc.branch, ploc.pos, ploc.pos});
          // TODO: This is rather adhoc

          result.min = std::min(std::min(pval, dval), result.min);
          result.max = std::max(std::max(pval, dval), result.max);
          result.values[seg.id] = {pval, dval};
          found = true;
          break;
        }
      }
    }
    if (!found) log_error("no matching");
  }

  // Normalise colour lookups
  auto scal = 0.0f;
  if (result.min != result.max) scal = 1/(result.max - result.min);
  log_debug("IExpr has min {} max {} scale {}", result.min, result.max, scal);
  for (auto& [k, v]: result.values) {
    v.first  = (v.first  - result.min)*scal;
    v.second = (v.second - result.min)*scal;
  }
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
  auto cell = arb::cable_cell(morph, {}, labels);
  return make_points(cv.cv_boundary_points(cell));
}
