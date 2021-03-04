#pragma once

#include <compare>
#include <unordered_map>
#include <vector>

struct id_type {
    size_t value;
    auto operator<=>(const id_type&) const = default;
};

namespace std {
    namespace {
        template<typename  T>
        inline size_t combine(std::size_t seed, T const& v) {
            return std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2); // From boost hash
        }

        inline size_t combine_all(std::size_t seed) { return seed; }

        template<typename T, typename... Ts>
        inline size_t combine_all(std::size_t seed, T const& t, Ts... ts) {
            return combine_all(combine(seed, t), ts...);
        }

        template<typename... Ts>
        inline size_t combine_all0(Ts... ts) {
            return combine_all(0, ts...);
        }
    }

    template<> struct hash<id_type> {
        std::size_t operator()(const id_type& k) const { return std::hash<decltype(k.value)>{}(k.value); }
    };

    template<typename A, typename B> struct hash<std::pair<A, B>> {
        std::size_t operator()(const std::pair<A, B>& p) const { return combine_all0(p.first, p.second); }
    };

    template<typename... As> struct hash<std::tuple<As...>> {
        std::size_t operator()(const std::tuple<As...>& t) const {
            return std::apply([](auto... as){ return combine_all0(as...); }, t);
        }
    };
}

id_type get_next_id();

using vec_type = std::vector<id_type>;
using mmap_type  = std::unordered_map<id_type, std::vector<id_type>>;
template<typename V> using map_type  = std::unordered_map<id_type, V>;
template<typename V> using join_type = std::unordered_map<std::tuple<id_type, id_type>, V>;
