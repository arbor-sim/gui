#include "utils.hpp"

void log_init() { spdlog::set_level(spdlog::level::debug); }

ImVec4 to_imvec(const glm::vec4& v) { return {v.x, v.y, v.z, v.w}; }
ImVec4 to_imvec(const glm::vec3& v) { return {v.x, v.y, v.z, 1.0f}; }

glm::vec2 to_glmvec(const ImVec2& v) { return glm::vec2{v.x, v.y}; }

std::string slurp(const std::filesystem::path& fn) {
    std::ifstream fd(fn);
    if (!fd.good()) log_fatal("Could not find file {}", fn.c_str());
    return {std::istreambuf_iterator<char>(fd), {}};
}

glm::vec3 hsv2rgb(glm::vec3 in) {
    if(in.s <= 0.0) return {in.z, in.z, in.z};
    float hh = ((in.x >= 360.0f) ? 0.0f : in.x/60.0f) ;
    float ff = hh - (long)hh;
    float p  = in.x * (1.0 -  in.y);
    float q  = in.x * (1.0 - (in.y * ff));
    float t  = in.x * (1.0 - (in.y * (1.0 - ff)));
    switch((long)hh) {
      case 0:  return {in.z, t, p};
      case 1:  return {q, in.z, p};
      case 2:  return {p, in.z, t};
      case 3:  return {p, q, in.z};
      case 4:  return {t, p, in.z};
      default: return {in.z, p, q};
    }
}

glm::vec3 next_color() {
  static glm::vec3 cur{};
  cur.z += 0.618033988749895;
  if (cur.z >= 1.0f) cur.z -= 1.0f;
  return hsv2rgb(cur);
}
