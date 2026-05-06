#include <iostream>
#include <sstream>
#include <set>

#include "compose_passphrase.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "character_classes.h"
#include "wordlists/eff_large_words.h"
#include "wordlists/wordnet_nouns.h"
#include "wordlists/wordnet_verbs.h"
#include "wordlists/wordnet_adjs.h"
#include "wordlists/wordnet_advs.h"
#include <gtest/gtest.h>


TEST(ComposePassphraseTest, TestPassphraseKebabCase) {
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

TEST(ComposePassphraseTest, TestPassphraseCamelCase) {
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
    int uppers = 0;
    for (char c : p) {
        if (isupper(c)) {
            uppers++;
        }
    }
    EXPECT_EQ(uppers, 3);
}

TEST(ComposePassphraseTest, TestPassphraseSubstitutionsDigits) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::DIGITS};
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

TEST(ComposePassphraseTest, TestPassphraseSubstitutionsSymbols) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::SYMBOLS};
    bool found_symbol = false;
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

TEST(ComposePassphraseTest, TestPassphraseNoSubstitutions) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes = {CharacterClass::DIGITS};
    auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, false, false);

    bool found_digit = false;
    for (size_t i = 0; i < p.length(); ++i) {
        if (isdigit(p[i])) {
            found_digit = true;
            bool at_end_of_word = (i + 1 == p.length()) || (p[i+1] == '-');
            EXPECT_TRUE(at_end_of_word) << "Digit " << p[i] << " found at index " << i << " in " << p << " is not at the end of a word";
        }
    }
    EXPECT_TRUE(found_digit);
}

TEST(ComposePassphraseTest, TestPassphrasePattern) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, Wordlist(wordnet_nouns_get_word, wordnet_nouns_get_word_count)},
        {WordClasses::Verb, Wordlist(wordnet_verbs_get_word, wordnet_verbs_get_word_count)},
        {WordClasses::Adj, Wordlist(wordnet_adjs_get_word, wordnet_adjs_get_word_count)},
        {WordClasses::Adv, Wordlist(wordnet_advs_get_word, wordnet_advs_get_word_count)}
    };
    std::vector<WordClasses> pattern = {WordClasses::Adj, WordClasses::Noun};
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlists, pattern, 0, "-", char_classes, false, false);
    int hyphens = 0;
    for (char c : p) {
        if (c == '-') hyphens++;
    }
    EXPECT_EQ(hyphens, 1);
}

TEST(ComposePassphraseTest, TestPassphraseAutoPattern) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, Wordlist(wordnet_nouns_get_word, wordnet_nouns_get_word_count)},
        {WordClasses::Verb, Wordlist(wordnet_verbs_get_word, wordnet_verbs_get_word_count)},
        {WordClasses::Adj, Wordlist(wordnet_adjs_get_word, wordnet_adjs_get_word_count)},
        {WordClasses::Adv, Wordlist(wordnet_advs_get_word, wordnet_advs_get_word_count)}
    };
    std::vector<WordClasses> pattern; // Empty pattern
    std::vector<CharacterClass> char_classes;
    // Patterns for length 3 exist in passphrase_patterns.cpp
    auto p = ComposePassPhrase(g, wordlists, pattern, 3, "-", char_classes, false, false);
    int hyphens = 0;
    for (char c : p) {
        if (c == '-') hyphens++;
    }
    EXPECT_EQ(hyphens, 2);
}

TEST(ComposePassphraseTest, TestPassphraseUniqueWords) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    int num_words = 10;
    auto p = ComposePassPhrase(g, wordlist, num_words, " ", char_classes, false, false);

    std::stringstream ss(p);
    std::string word;
    std::set<std::string> words;
    while (ss >> word) {
        words.insert(word);
    }
    EXPECT_EQ(words.size(), num_words);
}

TEST(ComposePassphraseTest, TestPassphraseLongSeparator) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlist, 3, "###", char_classes, false, false);
    EXPECT_TRUE(p.find("###") != std::string::npos);
    size_t count = 0;
    size_t pos = p.find("###");
    while (pos != std::string::npos) {
        count++;
        pos = p.find("###", pos + 3);
    }
    EXPECT_EQ(count, 2);
}

TEST(ComposePassphraseTest, TestPassphraseEmptyCharClasses) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, false, false);
    // Result should only contain letters and hyphens (assuming eff_large words are letters only)
    for (char c : p) {
        if (c != '-') {
            EXPECT_TRUE(isalpha(c));
        }
    }
}

TEST(ComposePassphraseTest, TestPassphraseCapitalizeAndSubstitute) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    // CharacterClass::DIGITS usually includes substitutions like 'a' -> '4', 'e' -> '3', etc.
    std::vector<CharacterClass> char_classes = {CharacterClass::DIGITS};

    bool found_upper = false;
    bool found_digit = false;

    for (int i = 0; i < 20; ++i) {
        auto p = ComposePassPhrase(g, wordlist, 3, "-", char_classes, true, true);
        for (char c : p) {
            if (isupper(c)) found_upper = true;
            if (isdigit(c)) found_digit = true;
        }
        if (found_upper && found_digit) break;
    }

    EXPECT_TRUE(found_upper);
    EXPECT_TRUE(found_digit);
}

TEST(ComposePassphraseTest, TestPassphraseNoPatternException) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists;
    std::vector<WordClasses> pattern;
    std::vector<CharacterClass> char_classes;
    // Length 10 has no patterns defined
    EXPECT_THROW(
        ComposePassPhrase(g, wordlists, pattern, 10, "-", char_classes, false, false),
        std::runtime_error);
}
