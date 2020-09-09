bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void log_init() { spdlog::set_level(spdlog::level::debug); }

template <typename... T> void log_debug(T... t) { spdlog::debug(t...); }
template <typename... T> void log_info(T... t) { spdlog::info(t...); }
template <typename... T> void log_warn(T... t) { spdlog::warn(t...); }
template <typename... T> void log_error(T... t) {
    spdlog::error(t...);
    throw std::runtime_error(fmt::format(t...));
}
template <typename... T> void log_fatal(T... t) {
    spdlog::error(t...);
    abort();
}
