#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "argon2_generator.h"
#include "sha512.h"
#include <stdexcept>

std::string MkPass(const context& ctx) {
    switch (ctx.algorithm) {
        case Algorithm::Argon2:
        {
            Argon2Generator g(ctx.password, ctx.service);
            if (ctx.char_classes.empty()) {
                return ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, ctx.length);
            }
            return ComposePassword(g, {ctx.char_classes.begin(), ctx.char_classes.end()}, ctx.length);
        }
        case Algorithm::Modern:
        {
            Generator<SHA512> g(ctx.password, ctx.service);
            if (ctx.char_classes.empty()) {
                return ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, ctx.length);
            }
            return ComposePassword(g, {ctx.char_classes.begin(), ctx.char_classes.end()}, ctx.length);
        }
        case Algorithm::Old:
            return ComposeOldMkpass1Password(ctx.password, ctx.service, ctx.length);
    }
    throw std::runtime_error("Unsupported algorithm");
}
