#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "character_classes.h"
#include <stdexcept>
#include <vector>
#include <algorithm>

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

template <typename GeneratorType>
std::string GenerateAndComposePassword(const Context& ctx) {
    GeneratorType g(ctx.password, ctx.service);
    if (ctx.char_classes.empty()) {
        return ComposePassword(g, {LowercaseLetters, UppercaseLetters, Digits, Symbols}, ctx.length);
    }
    auto sorted_char_classes = ctx.char_classes;
    std::sort(sorted_char_classes.begin(), sorted_char_classes.end());
    std::vector<std::string> char_classes;
    for (auto cc : sorted_char_classes) {
        char_classes.push_back(CharClassToString(cc));
    }
    return ComposePassword(g, std::move(char_classes), ctx.length);
}

} // namespace

std::string MkPass(const Context& ctx) {
    switch (ctx.algorithm) {
        case Algorithm::Argon2:
            return GenerateAndComposePassword<Generator<HKDF_Argon2>>(ctx);
        case Algorithm::SlowSha512:
            return GenerateAndComposePassword<Generator<HKDF_HMAC<SLOW_SHA512>>>(ctx);
        case Algorithm::Old:
            return ComposeOldMkpass1Password(ctx.password, ctx.service, ctx.length);
    }
    throw std::runtime_error("Unsupported algorithm");
}