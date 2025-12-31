#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <stdexcept>
//#include <fstream>


#if defined(_WIN32)
#include "win32_term.h"
#else
#include "posix_term.h"
#endif

#include "mkpass.h"
#include "tui.h"
#include "compose.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "linenoise.h"

Algorithm AskForAlgorithm(Algorithm default_algorithm) {
    std::map<char, Algorithm> choices = {
        {'1', Algorithm::Argon2},
        {'2', Algorithm::Modern},
        {'3', Algorithm::Old}
    };
    std::map<Algorithm, char> algo_to_char = {
        {Algorithm::Argon2, '1'},
        {Algorithm::Modern, '2'},
        {Algorithm::Old, '3'}
    };

    std::cerr << "Choose algorithm:\n";
    std::cerr << "1. Password (Argon2)\n";
    std::cerr << "2. Password (SHA512 HMAC)\n";
    std::cerr << "3. OldPassword\n";
    std::cerr << "Your choice (e.g. 1) [" << algo_to_char[default_algorithm] << "]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return default_algorithm;
    }

    if (choices.count(choice[0])) {
        return choices[choice[0]];
    }

    return Algorithm::Argon2;
}

std::vector<CharacterClass> AskForCharClasses(const std::vector<CharacterClass>& default_char_classes) {
    struct Choice {
        std::string name;
        CharacterClass value;
    };
    std::map<char, Choice> choices = {
        {'1', {"Lowercase Letters", CharacterClass::LOWERCASE}},
        {'2', {"Uppercase Letters", CharacterClass::UPPERCASE}},
        {'3', {"Digits", CharacterClass::DIGITS}},
        {'4', {"Symbols", CharacterClass::SYMBOLS}}
    };
    std::map<CharacterClass, char> cc_to_char = {
        {CharacterClass::LOWERCASE, '1'},
        {CharacterClass::UPPERCASE, '2'},
        {CharacterClass::DIGITS, '3'},
        {CharacterClass::SYMBOLS, '4'}
    };

    std::string default_choice_str;
    for (const auto& cc : default_char_classes) {
        default_choice_str += cc_to_char[cc];
    }

    std::cerr << "Choose character classes:\n";
    for (auto const& [key, val] : choices) {
        std::cerr << key << ". " << val.name << "\n";
    }
    std::cerr << "Your choice (e.g. 123) [" << default_choice_str << "]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return default_char_classes;
    }

    std::vector<CharacterClass> result;
    for (char c : choice) {
        if (choices.count(c)) {
            result.push_back(choices[c].value);
        }
    }
    return result;
}


std::vector<std::string> sname_keys;

void completion(const char *buf, linenoiseCompletions *lc) {
    for (const auto &s : sname_keys) {
        if (s.rfind(buf, 0) == 0) {
            linenoiseAddCompletion(lc, s.c_str());
        }
    }
}

int run_tui_safe() {
    try {
        return run_tui();
    } catch (const std::exception &e) {
        // This happens on Ctrl+C
        return 130;
    }
}

int run_tui() {
    mkpass::ConfigDB db;

    auto snames = db.get_all_snames();
    for (auto const& [name, len] : snames) {
        sname_keys.push_back(name);
    }

    // Input Master Password
    std::cerr << "Enter Master Password: ";
    std::string pwd = InputPassword();
    std::cerr << "\n";
    if (pwd.empty()) {
        throw std::runtime_error("Masster password must not be empty");
    }

    // Input second time, empty string to skip the check
    std::cerr << "Repeat Master Password: ";
    std::string pwd2 = InputPassword();
    std::cerr << "\n";

    if (pwd2.empty()) {
        std::cerr << "[NO CHECK]\n";
    } else if (pwd2 == pwd) {
        std::cerr << "[CORRECT]\n";
    } else {
        throw std::runtime_error("Passwords don't match");
    }

    // Input Service
    linenoiseSetCompletionCallback(completion);
    char *service_c_str = linenoise("Service name: ");
    if (service_c_str == nullptr) {
        return 130;
    }
    std::string service(service_c_str);
    free(service_c_str);

    auto db_entry = db.get_service_entry(service);

    Algorithm default_algorithm = Algorithm::Argon2;
    if (db_entry) {
        default_algorithm = db_entry->algorithm;
    } else if (snames.count(service)) {
        default_algorithm = Algorithm::Old;
    }

    auto algorithm = AskForAlgorithm(default_algorithm);
    std::vector<CharacterClass> char_classes;
    if (algorithm != Algorithm::Old) {
        std::vector<CharacterClass> default_char_classes = {
            CharacterClass::LOWERCASE,
            CharacterClass::UPPERCASE,
            CharacterClass::DIGITS,
            CharacterClass::SYMBOLS
        };
        if (db_entry) {
            default_char_classes = db_entry->char_classes;
        }
        char_classes = AskForCharClasses(default_char_classes);
    }

    // Input length
    unsigned length = 0;
    if (db_entry) {
        length = db_entry->length;
    } else if (snames.count(service)) {
        length = snames[service];
    }

    if (length > 0) {
        std::string prompt = "Length [" + std::to_string(length) + "]: ";
        char *length_c_str = linenoise(prompt.c_str());
        if (length_c_str == nullptr) {
            return 130;
        }
        std::string length_str(length_c_str);
        free(length_c_str);
        if (!length_str.empty()) {
            length = std::stoul(length_str);
        }
    } else {
        char *length_c_str = linenoise("Length: ");
        if (length_c_str == nullptr) {
            return 130;
        }
        std::string length_str(length_c_str);
        free(length_c_str);
        length = std::stoul(length_str);
    }

    Context ctx = {
        .password = pwd,
        .service = service,
        .char_classes = char_classes,
        .algorithm = algorithm,
        .length = length
    };

    // Get the result to stdout
    std::cout << MkPass(ctx) << std::endl;

    db.save_service_entry({service, algorithm, length, char_classes});

    return 0;
}
