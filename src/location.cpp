#include "location.hpp"

loc_def::loc_def(const std::string_view n, const std::string_view d): name{n}, definition{d}, lnk_renderable{-1} {
    name.resize(512, '\0');
    definition.resize(512, '\0');
    change();
}

reg_def::reg_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
reg_def::reg_def(): loc_def{"", ""} {}

ls_def::ls_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
ls_def::ls_def(): loc_def{"", ""} {}
