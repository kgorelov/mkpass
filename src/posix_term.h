#pragma once
#include <termios.h>
#include <unistd.h>


void SetEcho(bool enable = true) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable) {
        tty.c_lflag &= ~(ECHO);
    } else {
        tty.c_lflag |= ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string InputPassword()
{
    std::string password;
    char ch;
    SetEcho(false);
    while (true) {
        ch = getchar();
        if (ch == '\n' || ch == '\r')
            break;
        if (ch == 127 || ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
            }
        } else {
            password.push_back(ch);
        }
    }
    SetEcho(true);
    return password;
}
