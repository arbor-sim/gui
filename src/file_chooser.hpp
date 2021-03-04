#pragma once

#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <variant>

struct file_chooser_state {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::optional<std::string> filter = {};
    bool show_hidden;
    bool use_filter;
    std::filesystem::path file;
};
