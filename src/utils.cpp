#include "utils.hpp"

// Logging
void log_init() { spdlog::set_level(spdlog::level::warn); }

ImVec4 to_imvec(const glm::vec4& v) { return ImVec4{v.x, v.y, v.z, v.w}; }
