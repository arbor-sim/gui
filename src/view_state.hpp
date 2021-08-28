#pragma once

#include <compare>

#include "glm/glm.hpp"

struct view_state {
    float zoom  = 45.0f;
    glm::vec3 camera = {0.0f, 0.0f, 2.5f};
    glm::vec3 up     = {0.0f, 1.0f, 0.0f};
    glm::mat4 rotate{1.0f};
    glm::mat4 irotate{1.0f};
    glm::vec2 size   = {0.0f, 0.0f};
    glm::vec2 offset = {0.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};

    friend bool operator==(const view_state& l, const view_state& r) {
        return (l.size == r.size) &&
            (l.zoom == r.zoom) &&
            (l.rotate == r.rotate) &&
            (l.camera == r.camera) &&
            (l.up == r.up) &&
            (l.offset == r.offset) &&
            (l.target == r.target);
    }
};
