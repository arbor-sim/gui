#pragma once

#include <glm/glm.hpp>

struct view_state {
    float zoom  = 45.0f;
    float phi   = 0.0f;
    float theta = 0.0f;
    float gamma = 0.0f;
    glm::vec2 offset = {0.0f, 0.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
};
