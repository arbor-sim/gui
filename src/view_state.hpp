#pragma once

#include <glm/glm.hpp>

struct view_state {
    float zoom = 45.0f;
    float phi = 0.0f;
    glm::vec2 offset = {0.0, 0.0f};
};
