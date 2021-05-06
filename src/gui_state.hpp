#pragma once

#include <string>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include <arbor/cable_cell.hpp>

#include "cell_builder.hpp"
#include "component.hpp"
#include "events.hpp"
#include "file_chooser.hpp"
#include "geometry.hpp"
#include "loader.hpp"
#include "location.hpp"

#include "ion.hpp"
#include "cv_policy.hpp"
#include "parameter.hpp"
#include "probe.hpp"
#include "mechanism.hpp"
#include "spike_detector.hpp"
#include "stimulus.hpp"
#include "simulation.hpp"

struct gui_state {
    arb::cable_cell_parameter_set   presets = arb::neuron_parameter_defaults;
    parameter_def                   parameter_defaults = {};
    cell_builder                    builder;
    geometry                        renderer;

    entity                          locsets;
    component_unique<ls_def>        locset_defs;
    component_many<probe_def>       probes;
    component_many<detector_def>    detectors;
    component_many<stimulus_def>    stimuli;

    entity                          regions;
    component_unique<rg_def>        region_defs;

    component_unique<parameter_def> parameter_defs;
    component_many<mechanism_def>   mechanisms;

    entity                          ions;
    component_unique<ion_def>       ion_defs;
    component_unique<ion_default>   ion_defaults;
    component_join<ion_parameter>   ion_par_defs;

    bool open_morph_read = false;
    bool open_acc_read   = false;
    bool open_acc_save   = false;
    bool open_debug      = false;
    bool open_style      = false;
    bool open_demo       = false;
    bool open_about      = false;

    simulation sim;

    cv_def      cv_policy_def;

    // TODO This probably belongs into geometry, but that does not know about regions (yet).
    std::unordered_map<size_t, std::unordered_set<id_type>> segment_to_regions;
    std::optional<object_id> object;

    bool shutdown_requested = false;

    file_chooser_state file_chooser;
    file_chooser_state acc_chooser;
    view_state view;

    gui_state(const gui_state&) = delete;
    gui_state();

    event_queue events;

    std::string snapshot_path = std::filesystem::current_path() / "snapshot.png";

    void reload(const io::loaded_morphology&);

    void serialize(const std::filesystem::path& fn);
    void deserialize(const std::filesystem::path& fn);

    void add_ion(const std::string& lbl="", int charge=0) { events.push_back(evt_add_ion{lbl, charge}); }
    template<typename Item> void add_locdef(const std::string& lbl="", const std::string& def="") { events.push_back(evt_add_locdef<Item>{lbl, def}); }
    void add_region(const std::string& lbl="", const std::string& def="") { add_locdef<rg_def>(lbl, def); }
    void add_locset(const std::string& lbl="", const std::string& def="") { add_locdef<ls_def>(lbl, def); }
    void add_mechanism(const id_type& id) { events.push_back(evt_add_mechanism{id}); }
    void add_detector(const id_type& id) { events.push_back(evt_add_detector{id}); }
    void add_probe(const id_type& id) { events.push_back(evt_add_probe{id}); }
    void add_stimulus(const id_type& id) { events.push_back(evt_add_stimulus{id}); }

    template<typename Item> void remove_locdef(const id_type& def) { events.push_back(evt_del_locdef<Item>{def}); }
    void remove_region(const id_type def)     { remove_locdef<rg_def>(def); }
    void remove_locset(const id_type& def)    { remove_locdef<ls_def>(def); }
    void remove_ion(const id_type& def)       { events.push_back(evt_del_ion{def}); }
    void remove_mechanism(const id_type& def) { events.push_back(evt_del_mechanism{def}); }
    void remove_detector(const id_type& id)   { events.push_back(evt_del_detector{id}); }
    void remove_probe(const id_type& id)      { events.push_back(evt_del_probe{id}); }
    void remove_stimulus(const id_type& id) { events.push_back(evt_del_stimulus{id}); }

    template<typename Item> void update_locdef(const id_type& def) { events.push_back(evt_upd_locdef<Item>{def}); }
    void update_region(const id_type& def) { update_locdef<rg_def>(def); }
    void update_locset(const id_type& def) { update_locdef<ls_def>(def); }
    void update_cv_policy() { events.push_back(evt_upd_cv{}); }

    bool store_snapshot();
    void update();
    void reset();
    void gui();
    void handle_keys();
};
