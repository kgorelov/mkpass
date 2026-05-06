#pragma once
#include <string>

const std::string LowercaseLetters = "abcdefghijklmnopqrstuvwxyz";
const std::string UppercaseLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string Digits = "0123456789";
const std::string Symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";

#include <string>
#include <map>

enum class CharacterClass {
    LOWERCASE = 0,
    UPPERCASE = 1,
    DIGITS = 2,
    SYMBOLS = 3,
    CUSTOM = 4
};

std::string GetCharClassString(CharacterClass cls);

using CharMap = std::map<char, char>;

CharMap GetCharClassSubstitutions(CharacterClass cls);
