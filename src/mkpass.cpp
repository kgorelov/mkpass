#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "character_classes.h"
#include <stdexcept>
#include <vector>

namespace {
std::string CharClassToString(CharacterClass cc) {
    switch (cc) {
        case CharacterClass::LOWERCASE:
            return LowercaseLetters;
        case CharacterClass::UPPERCASE:
            return UppercaseLetters;
        case CharacterClass::DIGITS:
            return Digits;
        case CharacterClass::SYMBOLS:
            return Symbols;
    }
    throw std::runtime_error("Unknown character class");
}
}

std::string MkPass(const Context& ctx) {
    switch (ctx.algorithm) {
        case Algorithm::Argon2:
        {
            Generator<HKDF_Argon2> g(ctx.password, ctx.service);
            if (ctx.char_classes.empty()) {
                return ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, ctx.length);
            }
            std::vector<std::string> char_classes;
            for (auto cc : ctx.char_classes) {
                char_classes.push_back(CharClassToString(cc));
            }
            return ComposePassword(g, std::move(char_classes), ctx.length);
        }
        case Algorithm::SlowSha512:
        {
            Generator<HKDF_HMAC<SLOW_SHA512>> g(ctx.password, ctx.service);
            if (ctx.char_classes.empty()) {
                return ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, ctx.length);
            }
            std::vector<std::string> char_classes;
            for (auto cc : ctx.char_classes) {
                char_classes.push_back(CharClassToString(cc));
            }
            return ComposePassword(g, std::move(char_classes), ctx.length);
        }
        case Algorithm::Old:
            return ComposeOldMkpass1Password(ctx.password, ctx.service, ctx.length);
    }
    throw std::runtime_error("Unsupported algorithm");
}
