#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "sha512.h"

std::string MkPass(const context& ctx) {
    Generator<SHA512> g(ctx.password, ctx.service);
    // TODO character classes shall not be hardcoded
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    return ComposePassword(g, classes, ctx.length);
}
