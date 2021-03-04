#pragma once

#include "definition.hpp"

struct evt_add_ion { std::string name; int charge; };
struct evt_del_ion { id_type id; };
struct evt_add_mechanism { id_type region; };
struct evt_del_mechanism { id_type id; };
template <typename Item> struct evt_add_locdef { std::string name, definition; };
template <typename Item> struct evt_upd_locdef { id_type id; };
template <typename Item> struct evt_del_locdef { id_type id; };

using event = std::variant<evt_add_mechanism,                            evt_del_mechanism,
                           evt_add_ion,                                  evt_del_ion,
                           evt_add_locdef<reg_def>, evt_upd_locdef<reg_def>, evt_del_locdef<reg_def>,
                           evt_add_locdef<ls_def>,  evt_upd_locdef<ls_def>,  evt_del_locdef<ls_def>>;

using event_queue = std::vector<event>;
