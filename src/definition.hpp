#pragma once

#include <utils.hpp>
#include <string>

enum class def_state { changed, erase, good, error, empty };

struct definition {
    def_state state = def_state::empty;
    std::string message = "";

    void erase()  { state = def_state::erase;   message = ""; }
    void change() { state = def_state::changed; message = ""; }
    void good()   { state = def_state::good;    message = "Ok."; }
    void empty()  { state = def_state::empty;   message = "Empty."; }
    void error(const std::string& m) { state = def_state::error; message = m; }
};
