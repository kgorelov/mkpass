#include "mkpass.h"
#include "compose.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "wordlist.h"
#include "character_classes.h"
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace {

template <typename GeneratorType>
std::string GenerateAndComposePassword(const Context& ctx) {
    GeneratorType g(ctx.password, ctx.service);

    std::vector<std::string> active_char_classes;

    auto char_classes_to_use = ctx.char_classes;
    if (char_classes_to_use.empty()) {
        char_classes_to_use = {CharacterClass::LOWERCASE,
                               CharacterClass::UPPERCASE,
                               CharacterClass::DIGITS,
                               CharacterClass::SYMBOLS};
    }
    std::sort(char_classes_to_use.begin(), char_classes_to_use.end());

    for (auto cc : char_classes_to_use) {
        switch (cc) {
        case CharacterClass::LOWERCASE:
            active_char_classes.push_back(LowercaseLetters);
            break;
        case CharacterClass::UPPERCASE:
            active_char_classes.push_back(UppercaseLetters);
            break;
        case CharacterClass::DIGITS:
            active_char_classes.push_back(Digits);
            break;
        case CharacterClass::SYMBOLS:
            active_char_classes.push_back(Symbols);
            break;
        case CharacterClass::CUSTOM:
            if (ctx.custom_chars && !ctx.custom_chars->empty()) {
                active_char_classes.push_back(*ctx.custom_chars);
            }
            break;
        }
    }

    if (active_char_classes.empty()) {
        // Fallback
        return ComposePassword(
            g, {LowercaseLetters, UppercaseLetters, Digits, Symbols},
            ctx.length);
    }

    return ComposePassword(g, std::move(active_char_classes), ctx.length);
}

template <typename GeneratorType>
std::string GenerateAndComposePassphraseDiceware(const Context& ctx) 
{
    GeneratorType g(ctx.password, ctx.service);
    Wordlist wordlist;
    return ComposePassPhrase(g, wordlist, ctx.length);
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
	case Algorithm::Passphrase_Diceware_EFF_Large:
	    return GenerateAndComposePassphraseDiceware<Generator<HKDF_Argon2>>(ctx);
    }
    throw std::runtime_error("Unsupported algorithm");
}
