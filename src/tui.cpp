#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <limits>

#if defined(_WIN32)
#include "win32_term.h"
#else
#include "posix_term.h"
#endif

#include "mkpass.h"
#include "tui.h"
#include "compose.h"
#include "context.h"
#include "db.h"
#include "linenoise.h"

Algorithm AskForAlgorithm() {
    std::map<char, Algorithm> choices = {
        {'1', Algorithm::Argon2},
        {'2', Algorithm::Modern},
        {'3', Algorithm::Old}
    };

    std::cerr << "Choose algorithm:\n";
    std::cerr << "1. Password (Argon2)\n";
    std::cerr << "2. Password (SHA512 HMAC)\n";
    std::cerr << "3. OldPassword\n";
    std::cerr << "Your choice (e.g. 1) [1]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        choice = "1";
    }

    if (choices.count(choice[0])) {
        return choices[choice[0]];
    }

    return Algorithm::Argon2;
}

std::vector<std::string> AskForCharClasses() {
    struct Choice {
        std::string name;
        std::string value;
    };
    std::map<char, Choice> choices = {
        {'1', {"Lowercase Letters", LowercaseLetters}},
        {'2', {"Uppercase Letters", UppercaseLetters}},
        {'3', {"Digits", Digits}},
        {'4', {"Symbols", Symbols}}
    };

    std::cerr << "Choose character classes:\n";
    for (auto const& [key, val] : choices) {
        std::cerr << key << ". " << val.name << "\n";
    }
    std::cerr << "Your choice (e.g. 123) [1234]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        choice = "1234";
    }

    std::vector<std::string> result;
    for (char c : choice) {
        if (choices.count(c)) {
            result.push_back(choices[c].value);
        }
    }
    return result;
}

#include "db.h"

#include "linenoise.h"
#include <fstream>

std::vector<std::string> sname_keys;

void completion(const char *buf, linenoiseCompletions *lc) {
    for (const auto &s : sname_keys) {
        if (s.rfind(buf, 0) == 0) {
            linenoiseAddCompletion(lc, s.c_str());
        }
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
    std::string service(service_c_str);
    free(service_c_str);

    auto algorithm = AskForAlgorithm();
    std::vector<std::string> char_classes;
    if (algorithm == Algorithm::Modern) {
        char_classes = AskForCharClasses();
    }

    // Input length
    unsigned length = 0;
    if (snames.count(service)) {
        length = snames[service];
        std::string prompt = "Length [" + std::to_string(length) + "]: ";
        char *length_c_str = linenoise(prompt.c_str());
        std::string length_str(length_c_str);
        free(length_c_str);
        if (!length_str.empty()) {
            length = std::stoul(length_str);
        }
    } else {
        char *length_c_str = linenoise("Length: ");
        std::string length_str(length_c_str);
        free(length_c_str);
        length = std::stoul(length_str);
    }

    // Get the result to stdout
    std::cout << MkPass({
            .password = pwd,
            .service = service,
            .char_classes = char_classes,
            .algorithm = algorithm,
            .length = length
        }) << std::endl;
    return 0;
}
