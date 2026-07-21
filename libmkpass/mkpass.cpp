#include "mkpass.h"
#include "compose_password.h"
#include "compose_passphrase.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "wordlist.h"
#include "character_classes.h"
#include "wordlists/eff_large_words.h"
#include "wordlists/wordnet_nouns.h"
#include "wordlists/wordnet_verbs.h"
#include "wordlists/wordnet_adjs.h"
#include "wordlists/wordnet_advs.h"
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
        if (cc == CharacterClass::CUSTOM
            && ctx.custom_chars
            && !ctx.custom_chars->empty()) {
            active_char_classes.push_back(*ctx.custom_chars);
        } else {
            active_char_classes.push_back(GetCharClassString(cc));
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
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    return ComposePassPhrase(g, wordlist, ctx.length, ctx.separator, ctx.char_classes, ctx.allow_substitutions, ctx.capitalize_words);
}

template <typename GeneratorType>
std::string GenerateAndComposePassphraseWordnetPattern(const Context& ctx)
{
    GeneratorType g(ctx.password, ctx.service);

    Wordlist wordlist_nouns(wordnet_nouns_get_word, wordnet_nouns_get_word_count);
    Wordlist wordlist_verbs(wordnet_verbs_get_word, wordnet_verbs_get_word_count);
    Wordlist wordlist_adjs(wordnet_adjs_get_word, wordnet_adjs_get_word_count);
    Wordlist wordlist_advs(wordnet_advs_get_word, wordnet_advs_get_word_count);

    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, wordlist_nouns},
        {WordClasses::Verb, wordlist_verbs},
        {WordClasses::Adj, wordlist_adjs},
        {WordClasses::Adv, wordlist_advs},};

    return ComposePassPhrase(
        g, std::move(wordlists), ctx.passphrase_pattern, ctx.length, ctx.separator, ctx.char_classes, ctx.allow_substitutions, ctx.capitalize_words);
}

std::string TrimWhitespace(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

} // namespace


std::string MkPass(const Context& ctx) {
    Context local_ctx = ctx;
    local_ctx.service = TrimWhitespace(local_ctx.service);

    if (local_ctx.password.empty()) {
        throw std::runtime_error("Master password must not be empty");
    }
    if (local_ctx.service.empty()) {
        throw std::runtime_error("Service must not be empty");
    }
    switch (local_ctx.algorithm) {
        case Algorithm::Argon2:
            return GenerateAndComposePassword<Generator<HKDF_Argon2>>(local_ctx);
        case Algorithm::SlowSha512:
            return GenerateAndComposePassword<Generator<HKDF_HMAC<SLOW_SHA512>>>(local_ctx);
        case Algorithm::Old:
            return ComposeOldMkpass1Password(local_ctx.password, local_ctx.service, local_ctx.length);
        case Algorithm::Passphrase_Diceware_EFF_Large:
            return GenerateAndComposePassphraseDiceware<Generator<HKDF_Argon2>>(local_ctx);
        case Algorithm::Passphrase_Wordnet_Pattern:
            return GenerateAndComposePassphraseWordnetPattern<Generator<HKDF_Argon2>>(local_ctx);
    }
    throw std::runtime_error("Unsupported algorithm");
}
