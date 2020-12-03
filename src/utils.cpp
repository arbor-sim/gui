#include "utils.hpp"

// Logging
void log_init() { spdlog::set_level(spdlog::level::debug); }

ImVec4 to_imvec(const glm::vec4& v) { return ImVec4{v.x, v.y, v.z, v.w}; }

glm::vec2 to_glmvec(const ImVec2& v) { return glm::vec2{v.x, v.y}; }
