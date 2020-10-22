#include "gui_state.hpp"

cell_builder::cell_builder(): tree{}, morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
cell_builder::cell_builder(const arb::segment_tree& t): tree{t}, morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};

std::optional<renderable>
cell_builder::make_renderable(geometry& renderer, reg_def& def) {
    if (!def.data) return {};
    log_info("Making frustrums for region {} '{}'", def.name, def.definition);
    auto region   = def.data.value();
    auto concrete = thingify(region, provider);
    auto segments = pwlin.all_segments(concrete);
    return {renderer.make_region(segments, {0.0f, 0.0f, 0.0f, 1.0f})};
}

std::optional<renderable>
cell_builder::make_renderable(geometry& renderer, ls_def& def) {
    if (!def.data) return {};
    log_info("Making markers for locset {} '{}'", def.name, def.definition);
    auto locset   = def.data.value();
    auto concrete = thingify(locset, provider);
    std::vector<glm::vec3> points;
    for (const auto& loc: concrete) {
        for (const auto p: pwlin.all_at(loc)) {
            points.emplace_back((float) p.x, (float) p.y, (float) p.z);
        }
    }
    return {renderer.make_marker(points, {0.0f, 0.0f, 0.0f, 1.0f})};
}

arb::cable_cell cell_builder::make_cell() { return {morph, labels}; }

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

unsigned long gui_state::render_cell(float width, float height, float zoom, float phi) {
    if (region_defs.size() != render_regions.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < region_defs.size(); ++ix) {
        auto& def = region_defs[ix];
        auto& render = render_regions[ix];
        if (def.state == def_state::changed) {
            render.active = false;
            try {
                auto maybe_render = builder.make_renderable(renderer, def);
                if (maybe_render) {
                    auto tmp = maybe_render.value();
                    tmp.color = render.color;
                    render = tmp;
                }
                def.state = def_state::clean;
            } catch (arb::morphology_error& e) {
                def.error(e.what());
                continue;
            }
        }
    }
    if (locset_defs.size() != render_locsets.size()) log_fatal("Invariant!");
    for (auto ix = 0ul; ix < locset_defs.size(); ++ix) {
        auto& def = locset_defs[ix];
        auto& render = render_locsets[ix];
        if (def.state == def_state::changed) {
            render.active = false;
            try {
                auto maybe_render = builder.make_renderable(renderer, def);
                if (maybe_render) {
                    auto tmp = maybe_render.value();
                    tmp.color = render.color;
                    render = tmp;
                }
                def.state = def_state::clean;
            } catch (arb::morphology_error& e) {
                def.error(e.what());
                continue;
            }
        }
    }
    return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
}
