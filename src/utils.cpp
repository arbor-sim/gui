#include "utils.hpp"

// Check string suffix
bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// Logging
void log_init() { spdlog::set_level(spdlog::level::debug); }

ImVec4 to_imvec(const glm::vec4& v) { return ImVec4{v.x, v.y, v.z, v.w}; }
