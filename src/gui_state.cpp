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
    auto data = arborio::parse_swc(in, arborio::swc_mode::relaxed);
    auto tree = arborio::load_swc_allen(data);
    builder = cell_builder(std::move(tree));
    renderer = geometry{};
    renderer.load_geometry(builder.tree);

    region_defs.emplace_back("all",    "(all)");
    region_defs.emplace_back("soma",   "(tag 1)");
    region_defs.emplace_back("axon",   "(tag 2)");
    region_defs.emplace_back("dend",   "(tag 3)");
    region_defs.emplace_back("apic",   "(tag 4)");
    locset_defs.emplace_back("center", "(location 0 0)");
}

void gui_state::load_neuron_swc(const std::string& swc_fn) {
    log_debug("Reading {}", swc_fn);
    render_regions.clear();
    render_locsets.clear();
    locset_defs.clear();
    region_defs.clear();
    std::ifstream in(swc_fn.c_str());
    auto data = arborio::parse_swc(in, arborio::swc_mode::relaxed);
    auto tree = arborio::load_swc_neuron(data);
    builder = cell_builder(std::move(tree));
    renderer = geometry{};
    renderer.load_geometry(builder.tree);

    region_defs.emplace_back("all",    "(all)");
    region_defs.emplace_back("soma",   "(tag 1)");
    region_defs.emplace_back("axon",   "(tag 2)");
    region_defs.emplace_back("dend",   "(tag 3)");
    region_defs.emplace_back("apic",   "(tag 4)");
    locset_defs.emplace_back("center", "(location 0 0)");
}

void gui_state::load_arbor_swc(const std::string& swc_fn) {
    log_debug("Reading {}", swc_fn);
    render_regions.clear();
    render_locsets.clear();
    locset_defs.clear();
    region_defs.clear();
    std::ifstream in(swc_fn.c_str());
    auto data = arborio::parse_swc(in, arborio::swc_mode::strict);
    auto tree = arborio::as_segment_tree(data);
    builder = cell_builder(std::move(tree));
    renderer = geometry{};
    renderer.load_geometry(builder.tree);

    region_defs.emplace_back("all",    "(all)");
    region_defs.emplace_back("soma",   "(tag 1)");
    region_defs.emplace_back("axon",   "(tag 2)");
    region_defs.emplace_back("dend",   "(tag 3)");
    region_defs.emplace_back("apic",   "(tag 4)");
    locset_defs.emplace_back("center", "(location 0 0)");
}

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
std::unordered_map<std::string, Item> update_defs(std::vector<Item>& defs, std::vector<renderable>& renderables, cell_builder& builder, geometry& renderer) {
    for (auto& def: defs) {
        auto& lnk = def.lnk_renderable;
        if (lnk < 0) {
            lnk = renderables.size();
            renderables.emplace_back();
        }
        if (def.lnk_renderable < 0) log_fatal("no linked renderable");
        auto& render = renderables[def.lnk_renderable];
        if (def.state == def_state::changed) {
            update_def(def);
            def_set_renderable(renderer, builder, render, def);
        }
        if (def.state == def_state::erase) render.state = def_state::erase;
    }
    defs.erase(std::remove_if(defs.begin(), defs.end(), [](const auto& p) { return p.state == def_state::erase; }), defs.end());
    renderables.erase(std::remove_if(renderables.begin(), renderables.end(), [](const auto& p) { return p.state == def_state::erase; }), renderables.end());

    std::unordered_map<std::string, Item> result;
    for (const auto& def: defs) result[def.name] = def;
    return result;
}

template<typename Item>
void update_placables(std::unordered_map<std::string, ls_def>& locsets, std::vector<Item>& defs) {
    defs.erase(std::remove_if(defs.begin(), defs.end(), [](const auto& p) { return p.state == def_state::erase; }), defs.end());
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
    auto regions = update_defs(region_defs, render_regions, builder, renderer);
    auto locsets = update_defs(locset_defs, render_locsets, builder, renderer);
    update_placables(locsets, probe_defs);
    update_placables(locsets, iclamp_defs);
}

unsigned long gui_state::render_cell(float width, float height) {
    return renderer.render(zoom, phi, width, height, render_regions, render_locsets);
}
