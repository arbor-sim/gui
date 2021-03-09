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

#include "view_state.hpp"
#include "id.hpp"
#include "component.hpp"
#include "loader.hpp"
#include "definition.hpp"
#include "location.hpp"
#include "geometry.hpp"
#include "cell_builder.hpp"
#include "file_chooser.hpp"
#include "events.hpp"

struct gui_state {
    arb::cable_cell_parameter_set   presets = arb::neuron_parameter_defaults;
    parameter_def                   parameter_defaults = {};
    cell_builder                    builder;
    geometry                        renderer;

    entity                          locsets;
    component_unique<ls_def>        locset_defs;
    component_unique<renderable>    render_locsets;
    component_many<probe_def>       probes;
    component_many<detector_def>    detectors;

    entity                          regions;
    component_unique<rg_def>        region_defs;
    component_unique<renderable>    render_regions;
    component_unique<parameter_def> parameter_defs;
    component_many<mechanism_def>   mechanisms;

    entity                          ions;
    component_unique<ion_def>       ion_defs;
    component_unique<ion_default>   ion_defaults;

    component_join<ion_parameter>   ion_par_defs;

    file_chooser_state file_chooser;
    view_state view;

    gui_state(const gui_state&) = delete;
    gui_state();

    event_queue events;

    void reload(const io::loaded_morphology&);

    void serialize(const std::string& fn);
    void deserialize(const std::string& fn);

    void add_ion(const std::string& lbl="", int charge=0) { events.push_back(evt_add_ion{lbl, charge}); }
    template<typename Item> void add_locdef(const std::string& lbl="", const std::string& def="") { events.push_back(evt_add_locdef<Item>{lbl, def}); }
    void add_region(const std::string& lbl="", const std::string& def="") { add_locdef<rg_def>(lbl, def); }
    void add_locset(const std::string& lbl="", const std::string& def="") { add_locdef<ls_def>(lbl, def); }
    void add_mechanism(const id_type& id) { events.push_back(evt_add_mechanism{id}); }
    void add_detector(const id_type& id) { events.push_back(evt_add_detector{id}); }
    void add_probe(const id_type& id) { events.push_back(evt_add_probe{id}); }

    template<typename Item> void remove_locdef(const id_type& def) { events.push_back(evt_del_locdef<Item>{def}); }
    void remove_region(const id_type def)     { remove_locdef<rg_def>(def); }
    void remove_locset(const id_type& def)    { remove_locdef<ls_def>(def); }
    void remove_ion(const id_type& def)       { events.push_back(evt_del_ion{def}); }
    void remove_mechanism(const id_type& def) { events.push_back(evt_del_mechanism{def}); }
    void remove_detector(const id_type& id)   { events.push_back(evt_del_detector{id}); }
    void remove_probe(const id_type& id)      { events.push_back(evt_del_probe{id}); }

    template<typename Item> void update_locdef(const id_type& def) { events.push_back(evt_upd_locdef<Item>{def}); }
    void update_region(const id_type& def) { update_locdef<rg_def>(def); }
    void update_locset(const id_type& def) { update_locdef<ls_def>(def); }

    void update();
    void reset();
    void gui();
};
