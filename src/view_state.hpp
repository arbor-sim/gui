#pragma once

#include <compare>

#include "glm/glm.hpp"

struct view_state {
    float zoom  = 45.0f;
    float phi   = 0.0f;
    float theta = 0.0f;
    float gamma = 0.0f;

    glm::vec2 size   = {0.0f, 0.0f};
    glm::vec2 offset = {0.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};

    friend bool operator==(const view_state& l, const view_state& r) {
        return (l.size == r.size) &&
            (l.zoom == r.zoom) &&
            (l.phi == r.phi) &&
            (l.theta == r.theta) &&
            (l.gamma == r.gamma) &&
            (l.offset == r.offset) &&
            (l.target == r.target);

    }
};
