#pragma once

#include <unordered_set>
#include <string>
#include <vector>
#include <filesystem>
#include <variant>

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
#include <arborio/swcio.hpp>

#include "view_state.hpp"
#include "id.hpp"
#include "definition.hpp"
#include <location.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>

struct ion_default {
    double Xi = 0, Xo = 0, Er = 0;
    std::string method = "constant";
};

struct par_default { double TK, Cm, Vm, RL; };

struct file_chooser_state {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::optional<std::string> filter = {};
    bool show_hidden;
    bool use_filter;
    std::filesystem::path file;
};

namespace evt {
    template<typename T> struct create { T item; };
    template<typename T> struct update { id_type id; };
    template<typename T> struct remove { id_type id; };

    using event = std::variant<create<mech_def>,
                               update<mech_def>,
                               remove<mech_def>,
                               create<ls_def>,
                               update<ls_def>,
                               remove<ls_def>,
                               create<reg_def>,
                               update<reg_def>,
                               remove<reg_def>,
                               create<ion_def>,
                               update<ion_def>,
                               remove<ion_def>>;
}

struct gui_state {
    arb::cable_cell_parameter_set presets = arb::neuron_parameter_defaults;
    cell_builder builder;
    geometry renderer;
    // Location Sets
    vec_type                 locsets         = {};
    map_type<ls_def>         locset_defs     = {};
    map_type<renderable>     render_locsets  = {};
    // Regions
    vec_type                 regions         = {};
    map_type<reg_def>        region_defs     = {};
    map_type<renderable>     render_regions  = {};
    // Ions
    vec_type                 ions            = {};
    map_type<ion_def>        ion_defs        = {};
    map_type<ion_default>    ion_defaults    = {}; // Default ion settings per ion
    // Mechanisms
    vec_type                 mechanisms      = {};
    map_type<mech_def>       mechanism_defs  = {};

// std::vector<prb_def>     probe_defs     = {}; // Probes
    // std::vector<sdt_def>     detector_defs  = {}; // Spike detectors
    // std::vector<stm_def>     iclamp_defs    = {}; // Current clamps
    // paintings

    // map_type<ion_default>    ion_defaults   = {}; // Default ion settings per ion
    // std::vector<par_def>     par_defs       = {}; // Parameters per region
    join_type<ion_par_def>   ion_par_defs   = {}; // Ion settings per region and ion
    // std::vector<mech_def>    mechanism_defs = {}; // Mechanism setting per region per ion

    file_chooser_state file_chooser;
    view_state view;

    gui_state(const gui_state&) = delete;
    gui_state();

    std::vector<evt::event> events;

    void load_allen_swc(const std::string& fn);
    void load_neuron_swc(const std::string& fn);
    void load_arbor_swc(const std::string& fn);
    void load_neuroml(const std::string& fn);

    void serialize(const std::string& fn);
    void deserialize(const std::string& fn);

    void add_region(const reg_def& def={}) { events.push_back(evt::create<reg_def>{def}); }
    void add_ion(const ion_def& def={})    { events.push_back(evt::create<ion_def>{def}); }
    void add_locset(const ls_def& def={})  { events.push_back(evt::create<ls_def>{def});  }

    void remove_region(const id_type def) { events.push_back(evt::remove<reg_def>{def}); }
    void remove_ion(const id_type& def)    { events.push_back(evt::remove<ion_def>{def}); }
    void remove_mechanism(const id_type& def)    { events.push_back(evt::remove<mech_def>{def}); }
    void remove_locset(const id_type& def)  { events.push_back(evt::remove<ls_def>{def});  }


    void update();
    void reset();
    void gui();
};
