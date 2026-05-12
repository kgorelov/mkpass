#pragma once
#include <termios.h>
#include <unistd.h>


void SetEcho(bool enable = true) {
    if (!isatty(STDIN_FILENO)) return;
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) != 0) return;
    if (!enable) {
        tty.c_lflag &= ~(ECHO);
    } else {
        tty.c_lflag |= ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string InputPassword(bool* was_eof = nullptr)
{
    if (was_eof) *was_eof = false;
    std::string password;
    char ch;
    SetEcho(false);
    while (true) {
        int r = getchar();
        if (r == EOF) {
            if (was_eof) *was_eof = true;
            break;
        }
        if (r == '\n' || r == '\r')
            break;
        ch = static_cast<char>(r);
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
