#pragma once
#include <string>

// TODO: an option to omit known info (from DB, cmd options or env)
// cmd and ENV always ovverride unless cmd option "explicit" is given

struct context {
    std::string password;
    std::string service;

    bool is_gui = false;
    //std::optional<unsigned> length;
    // TODO character classes
    unsigned length = 0;
};
