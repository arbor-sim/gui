#pragma once

#include <string>
#include <vector>
#include <array>
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
#include "location.hpp"
#include "geometry.hpp"
#include "cell_builder.hpp"
#include "file_chooser.hpp"
#include "events.hpp"

struct gui_state {
    arb::cable_cell_parameter_set presets = arb::neuron_parameter_defaults;
    cell_builder builder;
    geometry renderer;
    // Location Sets
    vec_type                 locsets            = {}; // List of ion ids
    map_type<ls_def>         locset_defs        = {};
    map_type<renderable>     render_locsets     = {};
    // Regions
    vec_type                 regions            = {};
    map_type<reg_def>        region_defs        = {};
    map_type<renderable>     render_regions     = {};
    // Ions
    vec_type                 ions               = {};
    map_type<ion_def>        ion_defs           = {};
    map_type<ion_default>    ion_defaults       = {}; // Default ion settings per ion
    join_type<ion_parameter> ion_par_defs       = {}; // Ion settings per region and ion
    // Mechanisms
    vec_type                 mechanisms         = {};
    map_type<mechanism_def>  mechanism_defs     = {};
    mmap_type                region_mechanisms  = {}; // Map regions to lists of mechanisms
    // Parameters
    parameter_def            parameter_defaults = {};
    map_type<parameter_def>  parameter_defs     = {};
    // Placed items

    file_chooser_state file_chooser;
    view_state view;

    float color_under_mouse[3];

    gui_state(const gui_state&) = delete;
    gui_state();

    event_queue events;

    void load_allen_swc(const std::string& fn);
    void load_neuron_swc(const std::string& fn);
    void load_arbor_swc(const std::string& fn);
    void load_neuroml(const std::string& fn);

    void serialize(const std::string& fn);
    void deserialize(const std::string& fn);

    void add_ion(const std::string& lbl="", int charge=0) { events.push_back(evt_add_ion{lbl, charge}); }
    template<typename Item> void add_locdef(const std::string& lbl="", const std::string& def="") { events.push_back(evt_add_locdef<Item>{lbl, def}); }
    void add_region(const std::string& lbl="", const std::string& def="") { add_locdef<reg_def>(lbl, def); }
    void add_locset(const std::string& lbl="", const std::string& def="") { add_locdef<ls_def>(lbl, def); }
    void add_mechanism(const id_type& id) { events.push_back(evt_add_mechanism{id}); }

    template<typename Item> void remove_locdef(const id_type& def) { events.push_back(evt_del_locdef<Item>{def}); }
    void remove_region(const id_type def)     { remove_locdef<reg_def>(def); }
    void remove_locset(const id_type& def)    { remove_locdef<ls_def>(def); }
    void remove_ion(const id_type& def)       { events.push_back(evt_del_ion{def}); }
    void remove_mechanism(const id_type& def) { events.push_back(evt_del_mechanism{def}); }

    template<typename Item> void update_locdef(const id_type& def) { events.push_back(evt_upd_locdef<Item>{def}); }
    void update_region(const id_type& def) { update_locdef<reg_def>(def); }
    void update_locset(const id_type& def) { update_locdef<ls_def>(def); }

    void update();
    void reset();
    void gui();
};
