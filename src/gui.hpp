#pragma once

#include <optional>

#include <arbor/version.hpp>

#include <fmt/format.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "config.hpp"
#include "id.hpp"
#include "icons.hpp"
#include "utils.hpp"

inline void gui_tooltip(const std::string& message) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

template<typename T>
inline void gui_check_state(const T& def) {
    static std::unordered_map<def_state, const char*> tags{{def_state::empty, icon_question},
                                                           {def_state::error, icon_error},
                                                           {def_state::good,  icon_ok}};
    ImGui::Text("%s", tags[def.state]);
    gui_tooltip(def.message);
}


inline void gui_right_margin(float delta=40.0f) { ImGui::SameLine(ImGui::GetWindowWidth() - delta); }

inline bool gui_tree(const std::string& label, bool& force) {
    if (force) {
        ImGui::SetNextItemOpen(force);
        force = false;
    }
    ImGui::AlignTextToFramePadding();
    return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
}

inline bool gui_tree(const std::string& label) {
    ImGui::AlignTextToFramePadding();
    return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
}

template<typename F>
bool gui_tree_add(const std::string& label, F action) {
    static bool force_open = false;
    auto open = gui_tree(label, force_open);
    gui_right_margin();
    if (ImGui::Button(icon_add)) {
        action();
        force_open = true;
    }
    return open;
}

struct with_item_width {
    with_item_width(float px) { ImGui::PushItemWidth(px); }
    ~with_item_width() { ImGui::PopItemWidth(); }
};

struct with_indent {
    with_indent(float px_=0.0f): px{px_} { ImGui::Indent(px); }
    ~with_indent() { ImGui::Unindent(px); }
    float px;
};

inline with_indent gui_tree_indent() { return { 24.0f - ImGui::GetStyle().IndentSpacing }; } // fix alignment under trees

struct with_style {
    template<typename V> with_style(ImGuiStyleVar var, V val) { ImGui::PushStyleVar(var, val); }
    ~with_style() { ImGui::PopStyleVar(); }
};

struct with_id {
    with_id(size_t val)             { ImGui::PushID(val); }
    with_id(id_type val)            { ImGui::PushID(val.value); }
    with_id(const char* ptr)        { ImGui::PushID(ptr); }
    with_id(const std::string& str) { ImGui::PushID(str.c_str()); }
    ~with_id() { ImGui::PopID(); }
};

inline bool gui_select(const std::string& item, std::string& current) {
    if (ImGui::Selectable(item.c_str(), item == current)) {
        current = item;
        return true;
    }
    return false;
}

template<typename Container>
inline void gui_choose(const std::string& lbl, std::string& current, const Container& items) {
    if (ImGui::BeginCombo(lbl.c_str(), current.c_str())) {
        for (const auto& item: items) gui_select(item, current);
        ImGui::EndCombo();
    }
}

inline bool gui_input_double(const std::string& lbl, double& v, const std::string& unit="", const std::string& fmt="%8g") {
    auto format = fmt;
    if (!unit.empty()) format += " " + unit;
    return ImGui::InputDouble(lbl.c_str(), &v, 0.0, 0.0, format.c_str(), ImGuiInputTextFlags_CharsScientific);
}

inline bool gui_input_double(const std::string& lbl, std::optional<double>& v, const std::string& unit="", const std::string& fmt="%8g") {
    auto format = fmt;
    if (!unit.empty()) format += " " + unit;
    double tmp;
    if (v) tmp = v.value();
    auto result = ImGui::InputDouble(lbl.c_str(), &tmp, 0.0, 0.0, format.c_str(), ImGuiInputTextFlags_CharsScientific);
    if (result) v = tmp;
    return result;
}


inline void gui_defaulted_double(const std::string& label, const std::string& unit, std::optional<double>& value, const double fallback) {
    auto tmp = value.value_or(fallback);
    if (gui_input_double(label, tmp, unit)) value = {tmp};
    gui_right_margin();
    if (ImGui::Button(icon_refresh)) value = {};
    gui_tooltip("Reset to default");
}

inline auto
gui_defaulted_double(const std::string& label,
                     const std::string& unit,
                     std::optional<double>& value,
                     const std::optional<double>& fallback) {
    if (fallback) return gui_defaulted_double(label, unit, value, fallback.value());
    throw std::runtime_error{""};
}

inline auto
gui_defaulted_double(const std::string& label,
                          const std::string& unit,
                          std::optional<double>& value,
                          const std::optional<double>& fallback,
                          const std::optional<double>& fallback2) {
    if (fallback)  return gui_defaulted_double(label, unit, value, fallback.value());
    if (fallback2) return gui_defaulted_double(label, unit, value, fallback2.value());
    throw std::runtime_error{""};
}

inline bool gui_toggle(const char *on, const char *off, bool& flag) {
    if (ImGui::Button(flag ? on : off)) {
        flag = !flag;
        return true;
    }
    return false;
}

inline bool gui_menu_item(const char* text, const char* icon) { return ImGui::MenuItem(fmt::format("{} {}", icon, text).c_str()); }
inline bool gui_menu_item(const char* text, const char* icon, const char* key) { return ImGui::MenuItem(fmt::format("{} {}", icon, text).c_str(), key); }

inline void gui_debug(bool& open) {  ImGui::ShowMetricsWindow(&open); }

inline void gui_style(bool& open) {
    if (ImGui::Begin("Style", &open)) ImGui::ShowStyleEditor();
    ImGui::End();
}

inline void gui_about(bool& open) {
    if (ImGui::Begin("About", &open)) {
        ImGui::Text("Version: %s", gui_git_commit);
        ImGui::Text("Webpage: %s", gui_web_page);
        ImGui::Text("Arbor:   %s (%s)", arb::version, arb::source_id);
    }
    ImGui::End();
}

inline void gui_demo(bool& open) {
    if (ImGui::Begin("Demo", &open)) ImGui::ShowDemoWindow();
    ImGui::End();
}
