#include "gui_state.hpp"

#include <arbornml/arbornml.hpp>
#include <arbornml/nmlexcept.hpp>

#include <sstream>

extern float phi;
extern float zoom;

gui_state::gui_state(): builder{} {}

arb::segment_tree morphology_to_segment_tree(const arb::morphology& morph) {
    arb::segment_tree tree{};
    std::vector<arb::msize_t> todo{arb::mnpos};
    for (; !todo.empty();) {
        auto branch = todo.back(); todo.pop_back();
        for (const auto& child: morph.branch_children(branch)) {
            todo.push_back(child);
            for (const auto& [id, prox, dist, tag]: morph.branch_segments(child)) {
                tree.append(branch, prox, dist, tag);
            }
        }
    }
    return tree;
}

void load_swc(gui_state& state, const std::string& fn, std::function<arb::morphology(const std::vector<arborio::swc_record>&)> swc_to_morph) {
    log_debug("Reading {}", fn);
    std::ifstream in(fn.c_str());
    auto swc   = arborio::parse_swc(in).records();
    auto morph = swc_to_morph(swc);
    auto tree  = morphology_to_segment_tree(morph);
    state.builder = cell_builder{tree};
    state.renderer = geometry{tree};

    state.region_defs.emplace_back("soma",   "(tag 1)");
    state.region_defs.emplace_back("axon",   "(tag 2)");
    state.region_defs.emplace_back("dend",   "(tag 3)");
    state.region_defs.emplace_back("apic",   "(tag 4)");

    state.locset_defs.emplace_back("center", "(location 0 0)");
}

std::string to_string(const arb::region& r) {
    std::stringstream ss;
    ss << r;
    return ss.str();
}

std::string to_string(const arb::locset& r) {
    std::stringstream ss;
    ss << r;
    return ss.str();
}

void load_neuroml(gui_state& state, const std::string& fn) {
    // Read in morph
    std::ifstream fd(fn.c_str());
    std::string xml(std::istreambuf_iterator<char>(fd), {});
    arbnml::neuroml nml(xml);
    // Extract segment tree
    auto cell_ids   = nml.cell_ids();
    auto id         = cell_ids.front();
    auto morph_data = nml.cell_morphology(id).value();
    auto morph      = morph_data.morphology;
    auto tree       = morphology_to_segment_tree(morph);

    // Clean up
    state.render_regions.clear();
    state.render_locsets.clear();
    state.locset_defs.clear();
    state.region_defs.clear();

    // Re-build ourself
    state.builder = cell_builder{tree};
    state.renderer = geometry{tree};

    // Copy over locations
    for (const auto& [k, v]: morph_data.groups.regions()) {
        log_debug("NML region {}\n {}", k, to_string(v));
        state.region_defs.push_back({k, to_string(v)});
    }
    for (const auto& [k, v]: morph_data.groups.locsets()) {
        log_debug("NML locset {}\n {}", k, to_string(v));
        state.locset_defs.push_back({k, to_string(v)});
    }
}

void gui_state::load_allen_swc(const std::string& fn)  { load_swc(*this, fn, [](auto d){ return arborio::load_swc_allen(d); }); }
void gui_state::load_neuron_swc(const std::string& fn) { load_swc(*this, fn, [](auto d){ return arborio::load_swc_neuron(d); }); }
void gui_state::load_arbor_swc(const std::string& fn)  { load_swc(*this, fn, [](auto d){ return arborio::load_swc_arbor(d); }); }
void gui_state::load_neuroml(const std::string& fn)    { ::load_neuroml(*this, fn); }

template<typename D>
void update_def(D& def) {
    if (def.state == def_state::erase) return;
    if (def.definition.empty() || !def.definition[0]) {
        def.data = {};
        def.empty();
    } else {
        try {
            def.data = {def.definition};
            def.good();
        } catch (const arb::label_parse_error& e) {
            def.data = {};
            std::string m = e.what();
            auto colon = m.find(':') + 1; colon = m.find(':', colon) + 1;
            def.error(m.substr(colon, m.size() - 1));
        }
    }
}

void def_set_renderable(geometry& renderer, cell_builder& builder, renderable& render, ls_def& def) {
    render.active = false;
    if (!def.data || (def.state != def_state::good)) return;
    log_info("Making markers for locset {} '{}'", def.name, def.definition);
    try {
        auto points = builder.make_points(def.data.value());
        render = renderer.make_marker(points, render.color);
    } catch (arb::morphology_error& e) {
        def.error(e.what());
    }
}

void def_set_renderable(geometry& renderer, cell_builder& builder, renderable& render, reg_def& def) {
    render.active = false;
    if (!def.data || (def.state != def_state::good)) return;
    log_info("Making frustrums for region {} '{}'", def.name, def.definition);
    try {
        auto points = builder.make_segments(def.data.value());
        render = renderer.make_region(points, render.color);
    } catch (arb::morphology_error& e) {
        def.error(e.what());
    }
}

template<typename Item>
void update_placables(std::unordered_map<std::string, ls_def>& locsets, std::vector<Item>& defs) {
    std::erase_if(defs, [](const auto& p) { return p.state == def_state::erase; });
    for (auto& def: defs) {
        if (def.locset_name.empty()) {
            def.empty();
        } else {
            if (locsets.contains(def.locset_name)) {
                const auto& ls = locsets[def.locset_name];
                switch (ls.state) {
                    case def_state::error:
                        def.error("Linked locset malformed.");
                        break;
                    case def_state::empty:
                        def.error("Linked locset empty.");
                        break;
                    case def_state::good:
                        def.good();
                        break;
                    default: log_fatal("invalid code path: unknown state {}", def.locset_name);
                }
            } else {
                def.error("Linked locset absent.");
            }
        }
    }
}

void gui_state::update() {
    {
        if (render_locsets.size() < locset_defs.size()) render_locsets.resize(locset_defs.size());
        auto idx = 0;
        for (auto& def: locset_defs) {
            auto& render = render_locsets[idx];
            if (def.state == def_state::changed) {
                update_def(def);
                def_set_renderable(renderer, builder, render, def);
            }
            if (def.state == def_state::erase) {
                render_locsets[idx].state = def_state::erase;
            }
            idx++;
        }
        std::erase_if(locset_defs,    [](const auto& p) { return p.state == def_state::erase; });
        std::erase_if(render_locsets, [](const auto& p) { return p.state == def_state::erase; });
        // collect what is left
        std::unordered_map<std::string, ls_def> locsets;
        for (const auto& def: locset_defs) locsets[def.name] = def;
        // finally update individual lists of placeables
        update_placables(locsets, probe_defs);
        update_placables(locsets, iclamp_defs);
        update_placables(locsets, detector_defs);
    }
    // Update paintables
    {
        if (render_regions.size() < region_defs.size()) render_regions.resize(region_defs.size());
        if (parameter_defs.size() < region_defs.size()) parameter_defs.resize(region_defs.size());
        for (auto& mechanism: mechanism_defs) {
            if (mechanism.size() < mechanism_defs.size()) mechanism.resize(mechanism_defs.size());
        }
        for (auto& ion: ion_defs) {
            if (ion.size() < region_defs.size()) ion.resize(region_defs.size());
        }
        auto idx = 0;
        for (auto& def: region_defs) {
            auto& render = render_regions[idx];
            if (def.state == def_state::changed) {
                update_def(def);
                def_set_renderable(renderer, builder, render, def);
            }
            if (def.state == def_state::erase) {
                render_regions[idx].state = def_state::erase;
                parameter_defs[idx].state = def_state::erase;
                for (auto& ion: ion_defs) ion[idx].state = def_state::erase;
                for (auto& mech: mechanism_defs) mech[idx].state = def_state::erase;
                render_regions[idx].state = def_state::erase;
            }
            idx++;
        }
        std::erase_if(region_defs,    [](const auto& p) { return p.state == def_state::erase; });
        std::erase_if(render_regions, [](const auto& p) { return p.state == def_state::erase; });
        std::erase_if(parameter_defs, [](const auto& p) { return p.state == def_state::erase; });
        for (auto& ion: ion_defs) std::erase_if(ion, [](const auto& p) { return p.state == def_state::erase; });
        for (auto& mech: mechanism_defs) std::erase_if(mech, [](const auto& p) { return p.state == def_state::erase; });
    }
}

unsigned long gui_state::render_cell(float width, float height) {
    return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
}
