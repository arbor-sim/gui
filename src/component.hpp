#pragma once

#include <cassert>

#include "id.hpp"

struct entity {
    std::vector<id_type> ids;

    id_type add() { auto id = get_next_id(); ids.push_back(id); return id; }
    void del(const id_type& id) { std::erase_if(ids, [&] (const auto& it){ return id == it; });}

    auto begin() { return ids.begin(); }
    auto end() { return ids.end(); }
    auto begin() const { return ids.begin(); }
    auto end() const { return ids.end(); }
    auto clear() { ids.clear(); }
};

template<typename C>
struct component_unique {
    std::vector<C> items;
    std::unordered_map<id_type, size_t> parents;
    std::unordered_map<size_t, id_type> idx_to_parent;

    auto clear() { items.clear(); parents.clear(); idx_to_parent.clear();}

    void add(const id_type& parent, const C& c={}) {
        auto idx = items.size();
        items.push_back(c);
        parents[parent]    = idx;
        idx_to_parent[idx] = parent;

        assert(parents.size() == idx_to_parent.size());
        assert(items.size()   == idx_to_parent.size());
        assert(parents.contains(parent));
        assert(items.size() > parents[parent]);
        assert(idx_to_parent[parents[parent]] == parent);
    }

    C& operator[](const id_type& parent) {
        return items[parents[parent]];
    }

    void del(const id_type& id) {
        auto idx = parents[id];
        auto end = items.size() - 1;
        auto old = idx_to_parent[end];
        std::swap(items[idx], items[end]);
        items.pop_back();
        idx_to_parent.erase(end);
        parents[old] = idx;
        parents.erase(id);
    }
};

template<typename C>
struct component_many {
    std::vector<C> items;
    std::vector<id_type> ids;
    std::unordered_map<size_t, id_type> idx_to_parent;
    std::unordered_map<size_t, id_type> idx_to_id;
    std::unordered_map<id_type, size_t> lookup;
    std::unordered_map<id_type, std::vector<id_type>> parents;

    auto clear() { items.clear(); parents.clear(); idx_to_parent.clear(); idx_to_id.clear(); lookup.clear(); ids.clear(); }

    struct child_iter {
        std::vector<id_type>::iterator beg_, cur_, end_;
        auto operator++() { return cur_++; }
        auto operator*() { return *cur_; }
        auto begin() { return beg_;}
        auto end()  { return end_;}
    };

    auto& operator[](const id_type& ref) { return items[lookup[ref]]; }

    id_type add(const id_type& parent, const C& c={}) {
        auto id = get_next_id();
        auto idx = items.size();

        lookup[id] = idx;
        parents[parent].push_back(id);
        idx_to_parent[idx] = parent;
        idx_to_id[idx] = id;
        items.push_back(c);
        return id;
    }

    auto get_children(const id_type& parent) {
        auto& chunk = parents[parent];
        return child_iter{.beg_=chunk.begin(),
                          .cur_=chunk.begin(),
                          .end_=chunk.end()};
    }

    void del(const id_type& id) {
        // Find indices and swap/delete
        auto idx = lookup[id];
        auto end = items.size() - 1;
        std::swap(items[idx], items[end]);
        items.pop_back();

        // Remove delete element from parent -> children
        auto parent = idx_to_parent[idx];
        std::erase_if(parents[parent], [&] (const auto& it) { return it == id; });

        // Update mappings
        idx_to_id[idx]         = idx_to_id[end];
        idx_to_parent[idx]     = idx_to_parent[end];
        lookup[idx_to_id[end]] = idx;

        // Shave off remainder
        idx_to_id.erase(end);
        idx_to_parent.erase(end);
        lookup.erase(id);
    }

    void del_children(const id_type& id) {
        std::vector<id_type> to_remove;
        for (const auto& child: get_children(id)) to_remove.push_back(child);
        for (const auto& key: to_remove) del(key);
    }
};

template<typename C>
struct component_join {
    using key_type = std::pair<id_type, id_type>;
    std::vector<C> items;
    std::unordered_map<size_t, key_type> idx_to_parent;
    std::unordered_map<key_type, size_t> parents;

    auto& operator[](const key_type& key) { return items[parents[key]]; }

    void add(const id_type& a, const id_type& b, const C& c= {}) {
        auto idx = items.size();
        auto par = std::make_pair(a, b);
        parents[par] = idx;
        idx_to_parent[idx] = par;
        items.push_back(c);
    }

    void del(const key_type& id) {
        auto idx = parents[id];
        auto end = items.size() - 1;
        auto old = idx_to_parent[end];
        std::swap(items[idx], items[end]);
        items.pop_back();
        idx_to_parent.erase(end);
        parents[old] = idx;
        parents.erase(id);
    }

    void del_by_1st(const id_type& id) {
        std::vector<key_type> to_remove;
        for (const auto& [key, val]: parents) {
            if (key.first == id) to_remove.push_back(key);
        }
        for (const auto& key: to_remove) del(key);
    }

    void del_by_2nd(const id_type& id) {
        std::vector<key_type> to_remove;
        for (const auto& [key, val]: parents) {
            if (key.second == id) to_remove.push_back(key);
        }
        for (const auto& key: to_remove) del(key);
    }

    auto clear() { items.clear(); parents.clear(); idx_to_parent.clear();}
};
