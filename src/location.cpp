#include "location.hpp"

loc_def::loc_def(const std::string_view n, const std::string_view d): name{n}, definition{d} {
    name.resize(512, '\0');
    definition.resize(512, '\0');
}

reg_def::reg_def(const std::string_view n, const std::string_view d): loc_def{n, d} {
    update();
}

reg_def::reg_def(): loc_def{"", ""} {
    update();
}

void reg_def::update() {
    if (definition.empty() || !definition[0]) {
        data = {};
        bg_color = yellow;
        message = "Empty.";
    } else {
        try {
            data = arb::region{definition};
            bg_color = green;
            message = "Ok.";
        } catch (const arb::label_parse_error& e) {
            data = {};
            bg_color = red;
            message = e.what();
            auto colon = message.find(':') + 1; colon = message.find(':', colon) + 1;
            message = message.substr(colon, message.size() - 1);
        }
    }
}

ls_def::ls_def(const std::string_view n, const std::string_view d): loc_def{n, d} {
    update();
}

ls_def::ls_def(): loc_def{"", ""} {
    update();
}

void ls_def::update() {
    if (definition.empty() || !definition[0]) {
        data = {};
        bg_color = yellow;
        message = "Empty.";
    } else {
        try {
            data = arb::locset{definition};
            bg_color = green;
            message = "Ok.";
        } catch (const arb::label_parse_error& e) {
            data = {};
            bg_color = red;
            message = e.what();
            auto colon = message.find(':') + 1; colon = message.find(':', colon) + 1;
            message = message.substr(colon, message.size() - 1);
        }
    }
}
