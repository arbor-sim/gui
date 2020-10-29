#include "location.hpp"

loc_def::loc_def(const std::string_view n, const std::string_view d): name{n}, definition{d} {
    name.resize(512, '\0');
    definition.resize(512, '\0');
}

reg_def::reg_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
reg_def::reg_def(): loc_def{"", ""} {}

void reg_def::update() {
    if (definition.empty() || !definition[0]) {
        data = {};
        state = def_state::empty;
        message = "Empty.";
    } else {
        try {
            data = arb::region{definition};
            state = def_state::good;
            message = "Ok.";
        } catch (const arb::label_parse_error& e) {
            data = {};
            state = def_state::error;
            message = e.what();
            auto colon = message.find(':') + 1; colon = message.find(':', colon) + 1;
            message = message.substr(colon, message.size() - 1);
        }
    }
}

void reg_def::set_renderable(geometry& renderer, cell_builder& builder, renderable& render) {
    render.active = false;
    if (!data) return;
    log_info("Making frustrums for region {} '{}'", name, definition);
    try {
        auto points = builder.make_segments(data.value());
        render = renderer.make_region(points, render.color);
    } catch (arb::morphology_error& e) {
        error(e.what());
    }
}

ls_def::ls_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
ls_def::ls_def(): loc_def{"", ""} {}

void ls_def::update() {
    if (definition.empty() || !definition[0]) {
        data = {};
        state = def_state::empty;
        message = "Empty.";
    } else {
        try {
            data = arb::locset{definition};
            state = def_state::good;
            message = "Ok.";
        } catch (const arb::label_parse_error& e) {
            data = {};
            state = def_state::error;
            message = e.what();
            auto colon = message.find(':') + 1; colon = message.find(':', colon) + 1;
            message = message.substr(colon, message.size() - 1);
        }
    }
}

void ls_def::set_renderable(geometry& renderer, cell_builder& builder, renderable& render) {
    render.active = false;
    if (!data) return;
    log_info("Making markers for locset {} '{}'", name, definition);
    try {
        auto points = builder.make_points(data.value());
        render = renderer.make_marker(points, render.color);
    } catch (arb::morphology_error& e) {
        error(e.what());
    }
}
