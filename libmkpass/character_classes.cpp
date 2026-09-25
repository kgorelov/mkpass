#include "character_classes.h"
#include <algorithm>
#include <cctype>

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

std::string CharacterClassesToIdentifierString(const std::vector<CharacterClass>& classes) {
    std::string result;
    for (size_t i = 0; i < classes.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        switch (classes[i]) {
            case CharacterClass::LOWERCASE:
                result += "lowercase";
                break;
            case CharacterClass::UPPERCASE:
                result += "uppercase";
                break;
            case CharacterClass::DIGITS:
                result += "digits";
                break;
            case CharacterClass::SYMBOLS:
                result += "symbols";
                break;
            case CharacterClass::CUSTOM:
                result += "custom";
                break;
        }
    }
    return result;
}

std::vector<CharacterClass> ParseCharacterClasses(const std::string& str) {
    std::vector<CharacterClass> result;
    auto add_unique = [&](CharacterClass cc) {
        if (std::find(result.begin(), result.end(), cc) == result.end()) {
            result.push_back(cc);
        }
    };

    std::string trimmed = str;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), trimmed.end());

    if (trimmed.empty()) {
        return result;
    }

    bool all_digits = std::all_of(trimmed.begin(), trimmed.end(), [](unsigned char c) {
        return std::isdigit(c);
    });

    if (all_digits) {
        for (char c : trimmed) {
            switch (c) {
                case '1': add_unique(CharacterClass::LOWERCASE); break;
                case '2': add_unique(CharacterClass::UPPERCASE); break;
                case '3': add_unique(CharacterClass::DIGITS); break;
                case '4': add_unique(CharacterClass::SYMBOLS); break;
                case '5': add_unique(CharacterClass::CUSTOM); break;
            }
        }
        return result;
    }

    size_t start = 0;
    while (start < trimmed.size()) {
        size_t comma = trimmed.find(',', start);
        std::string token = (comma == std::string::npos) ? trimmed.substr(start) : trimmed.substr(start, comma - start);
        start = (comma == std::string::npos) ? trimmed.size() : comma + 1;

        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), token.end());
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) { return std::tolower(c); });

        if (token == "lowercase" || token == "lower" || token == "1") {
            add_unique(CharacterClass::LOWERCASE);
        } else if (token == "uppercase" || token == "upper" || token == "2") {
            add_unique(CharacterClass::UPPERCASE);
        } else if (token == "digits" || token == "digit" || token == "3") {
            add_unique(CharacterClass::DIGITS);
        } else if (token == "symbols" || token == "symbol" || token == "4") {
            add_unique(CharacterClass::SYMBOLS);
        } else if (token == "custom" || token == "5") {
            add_unique(CharacterClass::CUSTOM);
        }
    }

    return result;
}
