#pragma once

#include <string>

#include <glm/glm.hpp>
#include <imgui.h>

#include <spdlog/spdlog.h>

// Logging
void log_init();

template <typename... T> void log_debug(T... t) { spdlog::debug(t...); }
template <typename... T> void log_info(T... t)  { spdlog::info(t...); }
template <typename... T> void log_warn(T... t)  { spdlog::warn(t...); }
template <typename... T> void log_error(T... t) { spdlog::error(t...); throw std::runtime_error(fmt::format(t...)); }
template <typename... T> void log_fatal(T... t) { spdlog::error(t...); abort(); }

ImVec4 to_imvec(const glm::vec4& v);

glm::vec2 to_glmvec(const ImVec2& v);

// And constants
constexpr float PI = 3.141f;
