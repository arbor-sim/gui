#include "utils.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "spdlog/sinks/basic_file_sink.h"

std::filesystem::path get_resource_path(const std::filesystem::path& fn) {
#ifdef ARBORGUI_RESOURCES_BASE
  return ARBORGUI_RESOURCES_BASE / fn;
#else
  CFURLRef appUrlRef = CFBundleCopyBundleURL( CFBundleGetMainBundle() );
  CFStringRef macPath = CFURLCopyFileSystemPath( appUrlRef, kCFURLPOSIXPathStyle );
  CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
  const char* path = CFStringGetCStringPtr(macPath, encodingMethod);
  CFRelease(appUrlRef);
  CFRelease(macPath);
  return std::filesystem::path{path} / "Contents/Resources" / fn;
#endif
}

void log_init() {
  // auto logger = spdlog::basic_logger_mt("basic_logger", "logs/basic-log.txt");
  spdlog::set_level(spdlog::level::debug);
}


ImVec4 to_imvec(const glm::vec4& v) { return {v.x, v.y, v.z, v.w}; }
ImVec4 to_imvec(const glm::vec3& v) { return {v.x, v.y, v.z, 1.0f}; }

glm::vec2 to_glmvec(const ImVec2& v) { return glm::vec2{v.x, v.y}; }

std::string slurp(const std::filesystem::path& fn) {
    std::ifstream fd(fn);
    if (!fd.good()) log_fatal("Could not find file {}", fn.c_str());
    return {std::istreambuf_iterator<char>(fd), {}};
}


glm::vec4 hsv2rgb(const glm::vec4& hsv) {
  glm::vec4 rgb{hsv.z, hsv.z, hsv.z, hsv.w};
  if (hsv.y == 0.0f) return rgb;

  float h = std::fmod(hsv.x, 1.0f) / (60.0f / 360.0f);
  int   i = (int)h;
  float f = h - (float)i;
  float p = hsv.z * (1.0f - hsv.y);
  float q = hsv.z * (1.0f - hsv.y * f);
  float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

  switch (i) {
    case 0: rgb.x = hsv.z; rgb.y = t; rgb.z = p; break;
    case 1: rgb.x = q; rgb.y = hsv.z; rgb.z = p; break;
    case 2: rgb.x = p; rgb.y = hsv.z; rgb.z = t; break;
    case 3: rgb.x = p; rgb.y = q; rgb.z = hsv.z; break;
    case 4: rgb.x = t; rgb.y = p; rgb.z = hsv.z; break;
    case 5: default: rgb.x = hsv.z; rgb.y = p; rgb.z = q; break;
  }
  return rgb;
}

glm::vec4 next_color() {
  static size_t idx = 0;
  static glm::vec4 cur{0.27, 0.5f, 0.95f, 1.0f};
  static std::vector<glm::vec4> colours{
      glm::vec4{166,206,227, 255.0f}/256.0f,
      glm::vec4{31, 120,180, 255.0f}/256.0f,
      glm::vec4{178,223,138, 255.0f}/256.0f,
      glm::vec4{ 51,160, 44, 255.0f}/256.0f,
      glm::vec4{251,154,153, 255.0f}/256.0f,
      glm::vec4{227, 26, 28, 255.0f}/256.0f,
      glm::vec4{253,191,111, 255.0f}/256.0f,
      glm::vec4{255,127,  0, 255.0f}/256.0f,
      glm::vec4{202,178,214, 255.0f}/256.0f,
      glm::vec4{106, 61,154, 255.0f}/256.0f};

  if (idx < colours.size()) {
    return colours[idx++];
  } else {
    cur.x += 0.618033988749895;
    if (cur.x >= 1.0f) cur.x -= 1.0f;
    return hsv2rgb(cur);
  }
}
