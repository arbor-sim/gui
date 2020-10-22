
struct cell_builder {
    arb::segment_tree tree;
    arb::morphology   morph;
    arb::place_pwlin  pwlin;
    arb::label_dict   labels;
    arb::mprovider    provider;

    cell_builder(): tree{}, morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};
    cell_builder(const arb::segment_tree& t): tree{t}, morph{}, pwlin{morph}, labels{}, provider{morph, labels} {};

    std::optional<renderable> make_renderable(geometry& renderer, reg_def& def) {
        if (!def.data) return {};
        log_info("Making frustrums for region {} '{}'", def.name, def.definition);
        auto region   = def.data.value();
        auto concrete = thingify(region, provider);
        auto segments = pwlin.all_segments(concrete);
        return {renderer.make_region(segments, {0.0f, 0.0f, 0.0f, 1.0f})};
    }

    std::optional<renderable> make_renderable(geometry& renderer, ls_def& def) {
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

    auto make_cell() { return arb::cable_cell(morph, labels); }
};

struct region {
    std::unordered_map<std::string, arb::mechanism_desc> mechanisms;
    arb::cable_cell_parameter_set values;
    arb::region location;
};

struct probe {
    double frequency;
    std::string variable;
};

struct stimulus {
    arb::i_clamp clamp;
};

struct gui_state {
    geometry renderer;

    std::vector<renderable> render_regions = {};
    std::vector<renderable> render_locsets = {};

    std::vector<reg_def> region_defs = {};
    std::vector<ls_def>  locset_defs = {};

    cell_builder builder;

    std::filesystem::path cwd = std::filesystem::current_path();

    gui_state(const gui_state&) = delete;

    gui_state(): builder{} {}

    void load_allen_swc(const std::string& swc_fn) {
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
        add_swc_tags();
    }

    void add_swc_tags() {
        add_region("soma", "(tag 1)");
        add_region("axon", "(tag 2)");
        add_region("dend", "(tag 3)");
        add_region("apic", "(tag 4)");
        add_locset("center", "(location 0 0)");
    }

    void add_region(std::string_view l, std::string_view d) {
        region_defs.emplace_back(l, d);
        render_regions.emplace_back();
        render_regions.back().color = next_color();
    }

    void add_region() {
        region_defs.emplace_back();
        render_regions.emplace_back();
        render_regions.back().color = next_color();
    }

    void add_locset(std::string_view l, std::string_view d) {
        locset_defs.emplace_back(l, d);
        render_locsets.emplace_back();
        render_locsets.back().color = next_color();
    }

    void add_locset() {
        locset_defs.emplace_back();
        render_locsets.emplace_back();
        render_locsets.back().color = next_color();
    }

    auto render_cell(float width, float height) {
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
};
