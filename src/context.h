#pragma once
#include <string>
#include <vector>
#include "algorithms.h"
#include "character_classes.h"

// TODO: an option to omit known info (from DB, cmd options or env)
// cmd and ENV always ovverride unless cmd option "explicit" is given

struct Context {
    std::string password;
    std::string service;
    std::vector<CharacterClass> char_classes;
    Algorithm algorithm = Algorithm::Argon2;

    bool is_gui = false;
    //std::optional<unsigned> length;
    unsigned length = 0;
};
