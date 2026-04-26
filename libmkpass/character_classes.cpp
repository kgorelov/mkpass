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
