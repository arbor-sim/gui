#include "gui_state.hpp"

extern float phi;
extern float zoom;

gui_state::gui_state(): builder{} {}

void gui_state::load_allen_swc(const std::string& swc_fn) {
    log_debug("Reading {}", swc_fn);
    render_regions.clear();
    render_locsets.clear();
    locset_defs.clear();
    region_defs.clear();
    std::ifstream in(swc_fn.c_str());
    auto tree = arb::as_segment_tree(arb::parse_swc(in, arb::swc_mode::relaxed));
    builder = cell_builder(std::move(tree));
    renderer = geometry{};
    renderer.load_geometry(builder.tree);
    //
    add_region("all", "(all)");
    add_region("soma", "(tag 1)");
    add_region("axon", "(tag 2)");
    add_region("dend", "(tag 3)");
    add_region("apic", "(tag 4)");
    add_locset("center", "(location 0 0)");
}

void gui_state::add_region(std::string_view l, std::string_view d) {
    region_defs.emplace_back(l, d);
    render_regions.emplace_back();
    render_regions.back().color = next_color();
}

void gui_state::add_region() {
    region_defs.emplace_back();
    render_regions.emplace_back();
    render_regions.back().color = next_color();
}

void gui_state::add_probe() {
    probe_defs.emplace_back();
}

void gui_state::add_locset(std::string_view l, std::string_view d) {
    locset_defs.emplace_back(l, d);
    render_locsets.emplace_back();
    render_locsets.back().color = next_color();
}

void gui_state::add_locset() {
    locset_defs.emplace_back();
    render_locsets.emplace_back();
    render_locsets.back().color = next_color();
}

void gui_state::update() {
    // Scan regions
    if (region_defs.size() != render_regions.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < region_defs.size(); ++ix) {
        auto& def = region_defs[ix];
        auto& render = render_regions[ix];
        if (def.state == def_state::changed) {
            def.update();
            def.set_renderable(renderer, builder, render);
        }
        if (def.state == def_state::erase) {
            render.state = def_state::erase;
        }
    }
    region_defs.erase(std::remove_if(region_defs.begin(), region_defs.end(), [](const auto& p) { return p.state == def_state::erase; }), region_defs.end());
    render_regions.erase(std::remove_if(render_regions.begin(), render_regions.end(), [](const auto& p) { return p.state == def_state::erase; }), render_regions.end());
    std::unordered_map<std::string, reg_def> regions;
    for (const auto& ls: region_defs) regions[ls.name] = ls;

    if (locset_defs.size() != render_locsets.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < locset_defs.size(); ++ix) {
        auto& def = locset_defs[ix];
        auto& render = render_locsets[ix];
        if (def.state == def_state::changed) {
            def.update();
            def.set_renderable(renderer, builder, render);
        }
        if (def.state == def_state::erase) {
            render.state = def_state::erase;
        }
    }
    locset_defs.erase(std::remove_if(locset_defs.begin(), locset_defs.end(), [](const auto& p) { return p.state == def_state::erase; }), locset_defs.end());
    render_locsets.erase(std::remove_if(render_locsets.begin(), render_locsets.end(), [](const auto& p) { return p.state == def_state::erase; }), render_locsets.end());
    std::unordered_map<std::string, ls_def> locsets;
    for (const auto& ls: locset_defs) locsets[ls.name] = ls;

    probe_defs.erase(std::remove_if(probe_defs.begin(), probe_defs.end(), [](const auto& p) { return p.state == def_state::erase; }), probe_defs.end());
    for (auto& probe: probe_defs) {
        if (probe.locset_name.empty()) {
            probe.empty();
        } else {
            if (locsets.contains(probe.locset_name)) {
                const auto& def = locsets[probe.locset_name];
                switch (def.state) {
                    case def_state::error:
                        probe.error("Linked locset malformed.");
                        break;
                    case def_state::empty:
                        probe.error("Linked locset empty.");
                        break;
                    case def_state::good:
                        probe.good();
                        break;
                    default: log_fatal("invalid code path: unknown state {}", probe.locset_name);
                }
            } else {
                probe.error("Linked locset absent.");
            }
        }
    }
}

unsigned long gui_state::render_cell(float width, float height) {
    return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
}
