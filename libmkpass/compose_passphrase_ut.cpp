#include <iostream>

#include "compose_passphrase.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "character_classes.h"
#include "wordlists/eff_large_words.h"
#include <gtest/gtest.h>


TEST(ComposeTest, TestPassphraseKebabCase) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, false, false);
    // Should have 2 hyphens for 3 words
    int hyphens = 0;
    for (char c : p) {
        if (c == '-') {
            hyphens++;
        }
    }
    EXPECT_EQ(hyphens, 2);
    // Should be all lowercase (assuming wordlist is lowercase)
    for (char c : p) {
        if (isalpha(c)) {
            EXPECT_TRUE(islower(c));
        }
    }
}

TEST(ComposeTest, TestPassphraseCamelCase) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    bool capitalize_words = true;
    auto p = ComposePassPhrase(g, wordlist, 3, "", char_classes, false, capitalize_words);
    // Should have no hyphens
    for (char c : p) {
        EXPECT_NE(c, '-');
    }
    // Each word should start with uppercase
    // This is a bit hard to test without knowing the words, but we can check if there are any uppercase letters
    int uppers = 0;
    for (char c : p) {
        if (isupper(c)) {
            uppers++;
        }
    }
    EXPECT_EQ(uppers, 3);
}

TEST(ComposeTest, TestPassphraseSubstitutionsDigits) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::DIGITS};
    // We want to check if any of the substituted characters appear in the output
    // letters_to_digits contains '4' for 'a', '3' for 'e', etc.
    // By allowing substitutions, we expect to see digits instead of some letters OR appended.
    // To be sure, we can run it multiple times or check if the result contains digits.
    bool found_digit = false;
    for (int i = 0; i < 10; ++i) {
        auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, true, false);
        for (char c : p) {
            if (isdigit(c)) {
                found_digit = true;
                break;
            }
        }
        if (found_digit) break;
    }
    EXPECT_TRUE(found_digit);
}

TEST(ComposeTest, TestPassphraseSubstitutionsSymbols) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::SYMBOLS};
    bool found_symbol = false;
    std::string symbols = "!@#$%^&*()-_=+[]{}|;:,.<>?/"; // more than we have but safe
    for (int i = 0; i < 10; ++i) {
        auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, true, false);
        for (char c : p) {
            if (Symbols.find(c) != std::string::npos) {
                found_symbol = true;
                break;
            }
        }
        if (found_symbol) break;
    }
    EXPECT_TRUE(found_symbol);
}

TEST(ComposeTest, TestPassphraseNoSubstitutions) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::DIGITS};
    // When allow_substitutions is false, digits should ONLY be appended at the end of some word.
    // They should not replace any letter.
    auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, false, false);

    // In KebabCase, it's easy: words are separated by '-'.
    // If a digit is present, it must be either before a '-' or at the end of the string.
    bool found_digit = false;
    for (size_t i = 0; i < p.length(); ++i) {
        if (isdigit(p[i])) {
            found_digit = true;
            // Check if it's at the end of a word
            bool at_end_of_word = (i + 1 == p.length()) || (p[i+1] == '-');
            EXPECT_TRUE(at_end_of_word) << "Digit " << p[i] << " found at index " << i << " in " << p << " is not at the end of a word";
        }
    }
    EXPECT_TRUE(found_digit);
}
