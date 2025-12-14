#pragma once
#include <string>
#include <vector>

// TODO: an option to omit known info (from DB, cmd options or env)
// cmd and ENV always ovverride unless cmd option "explicit" is given

enum class Algorithm {
    Modern,
    Old
};

struct context {
    std::string password;
    std::string service;
    std::vector<std::string> char_classes;
    Algorithm algorithm = Algorithm::Modern;

    bool is_gui = false;
    //std::optional<unsigned> length;
    unsigned length = 0;
};
