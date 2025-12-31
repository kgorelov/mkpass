#pragma once

// constexpr?
const std::string LowercaseLetters = "abcdefghijklmnopqrstuvwxyz";
const std::string UppercaseLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string Digits = "0123456789";
const std::string Symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";

#include <string>

enum class CharacterClass {
    LOWERCASE,
    UPPERCASE,
    DIGITS,
    SYMBOLS
};

// TODO: define an enum here
