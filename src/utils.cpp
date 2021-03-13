#include "utils.hpp"

void log_init() { spdlog::set_level(spdlog::level::debug); }

ImVec4 to_imvec(const glm::vec4& v) { return ImVec4{v.x, v.y, v.z, v.w}; }

glm::vec2 to_glmvec(const ImVec2& v) { return glm::vec2{v.x, v.y}; }

std::string slurp(const std::filesystem::path& fn) {
    std::ifstream fd(fn);
    if (!fd.good()) log_fatal("Could not find file {}", fn.c_str());
    return {std::istreambuf_iterator<char>(fd), {}};
}

glm::vec4 next_color() {
  static size_t nxt = 0;
  constexpr glm::vec4 colors[] = {{166.0f/255.0f,206.0f/255.0f,227.0f/255.0f, 1.0f},
                                  { 31.0f/255.0f,120.0f/255.0f,180.0f/255.0f, 1.0f},
                                  {178.0f/255.0f,223.0f/255.0f,138.0f/255.0f, 1.0f},
                                  { 51.0f/255.0f,160.0f/255.0f, 44.0f/255.0f, 1.0f},
                                  {251.0f/255.0f,154.0f/255.0f,153.0f/255.0f, 1.0f},
                                  {227.0f/255.0f, 26.0f/255.0f, 28.0f/255.0f, 1.0f},
                                  {253.0f/255.0f,191.0f/255.0f,111.0f/255.0f, 1.0f},
                                  {255.0f/255.0f,127.0f/255.0f,  0.0f/255.0f, 1.0f},
                                  {202.0f/255.0f,178.0f/255.0f,214.0f/255.0f, 1.0f},
                                  {106.0f/255.0f, 61.0f/255.0f,154.0f/255.0f, 1.0f},
                                  {255.0f/255.0f,255.0f/255.0f,153.0f/255.0f, 1.0f},
                                  {177.0f/255.0f, 89.0f/255.0f, 40.0f/255.0f, 1.0f}};
  constexpr size_t count = sizeof(colors)/sizeof(glm::vec4);
  log_debug("Loaded {}/{} colours.", nxt, count);
  nxt = (nxt + 1) % count;
  return colors[nxt];
}
