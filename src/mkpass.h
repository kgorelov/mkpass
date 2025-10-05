#pragma once

#include "compose.h"


std::string MkPass(const context& ctx) {
    Generator g(ctx.password + ctx.service);
    // TODO character classes shall not be hardcoded
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    return ComposePassword(g, classes, ctx.length);
}
