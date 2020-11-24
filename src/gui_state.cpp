#include "gui_state.hpp"

extern float phi;
extern float zoom;

gui_state::gui_state(): builder{} {}

void load_swc(gui_state& state, const std::string& swc_fn, std::function<arb::segment_tree(const std::vector<arborio::swc_record>&)> swc_to_segment_tree) {
    log_debug("Reading {}", swc_fn);
    state.render_regions.clear();
    state.render_locsets.clear();
    state.locset_defs.clear();
    state.region_defs.clear();
    std::ifstream in(swc_fn.c_str());
    auto data = arborio::parse_swc(in).records();
    auto tree = swc_to_segment_tree(data);
    state.builder = cell_builder(std::move(tree));
    state.renderer = geometry{};
    state.renderer.load_geometry(state.builder.tree);

    state.region_defs.emplace_back("all",    "(all)");
    state.region_defs.emplace_back("soma",   "(tag 1)");
    state.region_defs.emplace_back("axon",   "(tag 2)");
    state.region_defs.emplace_back("dend",   "(tag 3)");
    state.region_defs.emplace_back("apic",   "(tag 4)");
    state.render_regions.resize(5);

    state.locset_defs.emplace_back("center", "(location 0 0)");
    state.render_locsets.resize(1);
}

void gui_state::load_allen_swc(const std::string& swc_fn)  { return load_swc(*this, swc_fn, [](auto d){ return arborio::load_swc_allen(d); }); }
void gui_state::load_neuron_swc(const std::string& swc_fn) { return load_swc(*this, swc_fn, [](auto d){ return arborio::load_swc_neuron(d); }); }
void gui_state::load_arbor_swc(const std::string& swc_fn)  { return load_swc(*this, swc_fn, [](auto d){ return arborio::load_swc_arbor(d); }); }

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
        if (mechanism_defs.size() < region_defs.size()) mechanism_defs.resize(region_defs.size());
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
