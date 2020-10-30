#pragma once

#include <string>

#include <glm/glm.hpp>

#include <arbor/morph/region.hpp>
#include <arbor/morph/locset.hpp>
#include <arbor/morph/label_parse.hpp>


#include <utils.hpp>
#include <geometry.hpp>
#include <cell_builder.hpp>
#include <definition.hpp>

struct loc_def: definition {
    std::string name, definition;
    std::string message;
    virtual void update() = 0;

    loc_def(const loc_def&) = default;
    loc_def& operator=(const loc_def&) = default;
    loc_def(const std::string_view n, const std::string_view d);

    virtual ~loc_def() {};
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

    virtual ~reg_def() = default;
    virtual void update() override;
    virtual void set_renderable(geometry& renderer, cell_builder& builder, renderable& render) override;
};

struct ls_def: loc_def {
    std::optional<arb::locset> data = {};

    ls_def(const ls_def&) = default;
    ls_def& operator=(const ls_def&) = default;

    ls_def(const std::string_view n, const std::string_view d);
    ls_def();

    virtual ~ls_def() = default;
    virtual void update() override;
    virtual void set_renderable(geometry& renderer, cell_builder& builder, renderable& render) override;
};
