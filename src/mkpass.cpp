#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "sha512.h"

std::string MkPass(const context& ctx) {
    Generator<SHA512> g(ctx.password, ctx.service);
    if (ctx.char_classes.empty()) {
        return ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, ctx.length);
    }
    return ComposePassword(g, {ctx.char_classes.begin(), ctx.char_classes.end()}, ctx.length);
}
