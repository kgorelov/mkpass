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

int run_tui() {
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
    std::string service;
    std::cerr << "Service name: ";
    std::cin >> service;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    auto char_classes = AskForCharClasses();

    // TODO Input Algorithm: Password, Passphrase, Old Password

    // Input length
    unsigned length = 0;
    std::cerr << "Length: ";
    std::cin >> length;

    // Get the result to stdout
    std::cout << MkPass({
            .password = pwd,
            .service = service,
            .char_classes = char_classes,
            .length = length
        }) << std::endl;
    return 0;
}
