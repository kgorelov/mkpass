#pragma once

#include <conio.h>


std::string InputPassword()
{
    std::string password;
    char ch;
    while ((ch = _getch()) != '\r') {
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
