#pragma once

// constexpr?
const std::string LowercaseLetters = "abcdefghijklmnopqrstuvwxyz";
const std::string UppercaseLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string Digits = "0123456789";
const std::string Symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";

#include <string>

enum class CharacterClass {
    LOWERCASE = 0,
    UPPERCASE = 1,
    DIGITS = 2,
    SYMBOLS = 3,
    CUSTOM = 4
};

// TODO: define an enum here
