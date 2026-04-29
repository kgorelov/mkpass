#include "character_classes.h"

std::string GetCharClassString(CharacterClass cls) {
    switch (cls) {
        case CharacterClass::LOWERCASE:
            return LowercaseLetters;
        case CharacterClass::UPPERCASE:
            return UppercaseLetters;
        case CharacterClass::DIGITS:
            return Digits;
        case CharacterClass::SYMBOLS:
            return Symbols;
        default:
            return "";
    }
}

std::map<char, char> letters_to_digits = {
    {'a','4'},
    {'b','6'},
    {'B','8'},
    {'e','3'},
    {'g','9'},
    {'G','6'},
    {'l','1'},
    {'o','0'},
    {'q','9'},
    {'r','2'},
    {'s','5'},
    {'t','7'},
    {'z','2'}
};

std::map<char, char> letters_to_symbols = {
    {'a','@'},
    {'c','('},
    {'d',')'},
    {'i','!'},
    {'s','$'},
    {'t','+'},
    {'u','_'},
    {'x','%'}
};

CharMap GetCharClassSubstitutions(CharacterClass cls)
{
    switch (cls) {
        case CharacterClass::DIGITS:
            return letters_to_digits;
        case CharacterClass::SYMBOLS:
            return letters_to_symbols;
        default:
            return {};
    }
}
