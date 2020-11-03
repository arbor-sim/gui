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
#include <arborio/swcio.hpp>
#include <arbor/mechinfo.hpp>

#include <definition.hpp>
#include <location.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>


struct prb_def: definition {
    std::string locset_name = "";
    double frequency = 1000; // [Hz]
    std::string variable = "voltage";
};

struct stm_def: definition {
    std::string locset_name = "";
    double delay = 0;      // [ms]
    double duration = 0;   // [ms]
    double amplitude = 0;  // [nA]
};

struct gui_state {
    geometry renderer;

    // Main loop

    std::vector<renderable> render_regions = {};
    std::vector<renderable> render_locsets = {};

    std::vector<prb_def> probe_defs  = {};
    std::vector<stm_def> iclamp_defs = {};
    std::vector<reg_def> region_defs = {};
    std::vector<ls_def>  locset_defs = {};

    cell_builder builder;

    std::filesystem::path cwd = std::filesystem::current_path();

    gui_state(const gui_state&) = delete;
    gui_state();

    void load_allen_swc(const std::string& swc_fn);
    void load_neuron_swc(const std::string& swc_fn);
    void load_arbor_swc(const std::string& swc_fn);

    void update();

    unsigned long render_cell(float width, float height);
};
