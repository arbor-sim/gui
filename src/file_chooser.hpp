#pragma once

#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <variant>

#include "utils.hpp"

struct file_chooser_state {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::optional<std::string> filter = {};
    bool show_hidden = false;
    bool use_filter;
    std::filesystem::path file, home, desk, docs;

    file_chooser_state() {
        if (auto hd = getenv("HOME"); hd != nullptr) {
            home = hd;
            desk = home / "Desktop";
            docs = home / "Documents";
        } else {
            log_info("Could not retrieve $HOME");
        }

    }
};
