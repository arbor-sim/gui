#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <arbor/cable_cell.hpp>
#include <arbor/morph/place_pwlin.hpp>
#include <arbor/morph/mprovider.hpp>
#include <arbor/morph/primitives.hpp>
#include <arbor/morph/morphexcept.hpp>
#include <arbor/morph/label_parse.hpp>
#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/mechcat.hpp>
#include <arbor/swcio.hpp>
#include <arbor/mechinfo.hpp>

#include <location.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>

struct region {
    std::unordered_map<std::string, arb::mechanism_desc> mechanisms;
    arb::cable_cell_parameter_set values;
    arb::region location;
};

struct probe {
    double frequency;
    std::string variable;
};

struct stimulus {
    arb::i_clamp clamp;
};

struct gui_state {
    geometry renderer;

    // Main loop

    std::vector<renderable> render_regions = {};
    std::vector<renderable> render_locsets = {};

    std::vector<reg_def> region_defs = {};
    std::vector<ls_def>  locset_defs = {};

    cell_builder builder;

    std::filesystem::path cwd = std::filesystem::current_path();

    gui_state(const gui_state&) = delete;
    gui_state();

    void load_allen_swc(const std::string& swc_fn);

    void add_region(std::string_view l, std::string_view d);
    void add_region();
    void add_locset(std::string_view l, std::string_view d);
    void add_locset();

    unsigned long render_cell(float width, float height);
};
