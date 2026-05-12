#pragma once

#include <conio.h>


std::string InputPassword(bool* was_eof = nullptr)
{
    if (was_eof) *was_eof = false;
    std::string password;
    char ch;
    while (true) {
        int r = _getch();
        if (r == 0 || r == 0xE0) { // Special key
            _getch(); // Skip
            continue;
        }
        if (r == '\r' || r == '\n') break;
        if (r == 3) { // Ctrl+C
            throw std::exception();
        }
        if (r == 26) { // Ctrl+Z (EOF on Windows)
            if (was_eof) *was_eof = true;
            break;
        }
        ch = static_cast<char>(r);
        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
            }
        } else {
            password.push_back(ch);
        }
    }
    return password;
}
