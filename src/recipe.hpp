#pragma once

#include <cassert>
#include <vector>

#include <arbor/recipe.hpp>
#include <arbor/cable_cell.hpp>

#include "probe.hpp"

struct recipe: arb::recipe {
    arb::cell_size_type n = 1;
    arb::cell_kind kind = arb::cell_kind::cable;
    std::vector<arb::probe_info> probes;
    arb::cable_cell_global_properties properties;
    arb::cable_cell cell;

    arb::cell_size_type num_cells() const override { return n; }
    arb::cell_kind get_cell_kind(arb::cell_gid_type) const override { return kind; }
    arb::util::unique_any get_cell_description(arb::cell_gid_type) const override { return {cell}; }
    std::vector<arb::probe_info> get_probes(arb::cell_gid_type gid) const override { return probes; }
    std::any get_global_properties(arb::cell_kind) const override { return properties; }
};

inline
recipe make_recipe(const arb::cable_cell_global_properties& properties,
                   const arb::cable_cell& cell) {
    recipe result;
    result.properties = properties;
    result.cell       = cell;
    return result;
}
