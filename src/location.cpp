#include "location.hpp"

loc_def::loc_def(const std::string_view n, const std::string_view d): name{n}, definition{d} {}

reg_def::reg_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
reg_def::reg_def(): loc_def{"", ""} {}

ls_def::ls_def(const std::string_view n, const std::string_view d): loc_def{n, d} {}
ls_def::ls_def(): loc_def{"", ""} {}
