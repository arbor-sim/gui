#pragma once

#include <utils.hpp>
#include <string>

enum class def_state { empty, error, good, erase, changed };

struct definition {
    void erase()  { state = def_state::erase;   message = "Deleting."; }
    void change() { state = def_state::changed; message = "Updating."; }
    void good()   { state = def_state::good;    message = "Ok."; }
    void empty()  { state = def_state::empty;   message = "Empty."; }
    void error(const std::string& m) { log_debug("called error {}!", m); state = def_state::error; message = m; }
    def_state state = def_state::empty;
    std::string message = "New.";
};
