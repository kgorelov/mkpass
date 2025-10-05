#pragma once

#include <iostream>
#include <string>
#include "mkpass.h"

#if defined(_WIN32)
#include "win32_term.h"
#else
#include "posix_term.h"
#endif

int run_tui() {
    // Input Master Password
    std::cerr << "Enter Master Password: ";
    std::string pwd = InputPassword();
    std::cerr << "\n";

    // Input second time, empty string to skip the check
    std::cerr << "Repeat Master Password: ";
    std::string pwd2 = InputPassword();
    std::cerr << "\n";

    if (pwd2.empty()) {
        std::cerr << "[NO CHECK]\n";
    } else if (pwd2 == pwd) {
        std::cerr << "[CORRECT]\n";
    }

    // Input Service
    std::string service;
    std::cerr << "Service name: ";
    std::cin >> service;

    // TODO read character classes (choice)
    // TODO Input Algorithm: Password, Passphrase, Old Password

    // Input length
    unsigned length;
    std::cerr << "Length: ";
    std::cin >> length;

    // Get the result to stdout
    std::cout << MkPass({
            .password = pwd,
            .service = service,
            .length = length
        }) << std::endl;
    return 0;
}
