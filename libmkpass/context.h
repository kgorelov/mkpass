#pragma once
#include <string>
#include <vector>
#include <optional>
#include "algorithms.h"
#include "character_classes.h"
#include "word_classes.h"

// TODO: an option to omit known info (from DB, cmd options or env)
// cmd and ENV always ovverride unless cmd option "explicit" is given

struct Context {
    std::string password;
    std::string service;
    std::vector<CharacterClass> char_classes;
    Algorithm algorithm = Algorithm::Argon2;
    std::string separator; // Empty by default
    unsigned length = 0;
    std::optional<std::string> custom_chars;
    std::vector<WordClasses> passphrase_pattern;
    bool allow_substitutions;
    bool capitalize_words;
};
