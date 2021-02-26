#include "utils.hpp"

void log_init() { spdlog::set_level(spdlog::level::debug); }

ImVec4 to_imvec(const glm::vec4& v) { return ImVec4{v.x, v.y, v.z, v.w}; }

glm::vec2 to_glmvec(const ImVec2& v) { return glm::vec2{v.x, v.y}; }

std::string slurp(const std::filesystem::path& fn) {
    std::ifstream fd(fn);
    if (!fd.good()) log_fatal("Could not find file {}", fn.c_str());
    return {std::istreambuf_iterator<char>(fd), {}};
}
