#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/morph/label_parse.hpp>

#include <utils.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>

// Some useful colours
const glm::vec4 red    = glm::vec4(215.0f/255.0f, 25.0f/255.0f, 28.0f/255.0f, 1.0f);
const glm::vec4 yellow = glm::vec4(255.0f/255.0f,255.0f/255.0f,191/255.0f, 1.0f);
const glm::vec4 green  = glm::vec4(26.0f/255.0f,150.0f/255.0f,65.0f/255.0f, 1.0f);

enum class def_state { changed, erase, good, error, empty };

struct loc_def {
    std::string name, definition;
    std::string message;
    def_state state = def_state::changed;
    virtual void update() = 0;

    loc_def(const loc_def&) = default;
    loc_def& operator=(const loc_def&) = default;
    loc_def(const std::string_view n, const std::string_view d);

    void error(const std::string& err) {
        state = def_state::error;
        message  = err;
    }

    virtual void set_renderable(geometry& renderer, cell_builder& builder, renderable& render) = 0;
};

struct reg_def: loc_def {
    std::optional<arb::region> data = {};

    const static std::vector<const glm::vec4> colors;
    static size_t nxt;

    reg_def(const reg_def&) = default;
    reg_def& operator=(const reg_def&) = default;

    reg_def(const std::string_view n, const std::string_view d);
    reg_def();

    virtual void update() override;
    virtual void set_renderable(geometry& renderer, cell_builder& builder, renderable& render) override;
};

struct ls_def: loc_def {
    std::optional<arb::locset> data = {};

    ls_def(const ls_def&) = default;
    ls_def& operator=(const ls_def&) = default;

    ls_def(const std::string_view n, const std::string_view d);
    ls_def();

    virtual void update() override;
    virtual void set_renderable(geometry& renderer, cell_builder& builder, renderable& render) override;
};
