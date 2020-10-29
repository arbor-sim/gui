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

unsigned long gui_state::render_cell(float width, float height) {
    if (region_defs.size() != render_regions.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < region_defs.size(); ++ix) {
        auto& def = region_defs[ix];
        auto& render = render_regions[ix];
        if (def.state == def_state::changed) {
            def.update();
            def.set_renderable(renderer, builder, render);
        }
    }
    if (locset_defs.size() != render_locsets.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < locset_defs.size(); ++ix) {
        auto& def = locset_defs[ix];
        auto& render = render_locsets[ix];
        if (def.state == def_state::changed) {
            def.update();
            def.set_renderable(renderer, builder, render);
        }
    }
    return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
}
