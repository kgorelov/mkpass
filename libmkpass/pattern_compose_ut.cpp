#include <gtest/gtest.h>
#include "compose_passphrase.h"
#include "generator.h"
#include "hkdf_argon2.h"
#include "word_classes.h"
#include "passphrase_patterns.h"

// Mock wordlists
const char* mock_get_noun(int idx) {
    static std::vector<std::string> nouns = {"bird", "cat", "dog"};
    return nouns[idx % nouns.size()].c_str();
}
int mock_get_noun_count() { return 3; }

const char* mock_get_verb(int idx) {
    static std::vector<std::string> verbs = {"fly", "run", "jump"};
    return verbs[idx % verbs.size()].c_str();
}
int mock_get_verb_count() { return 3; }

const char* mock_get_adj(int idx) {
    static std::vector<std::string> adjs = {"big", "small", "fast"};
    return adjs[idx % adjs.size()].c_str();
}
int mock_get_adj_count() { return 3; }

const char* mock_get_adv(int idx) {
    static std::vector<std::string> advs = {"quickly", "slowly", "happily"};
    return advs[idx % advs.size()].c_str();
}
int mock_get_adv_count() { return 3; }

TEST(PatternComposeTest, TestSpecificPattern) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, Wordlist(mock_get_noun, mock_get_noun_count)},
        {WordClasses::Verb, Wordlist(mock_get_verb, mock_get_verb_count)},
        {WordClasses::Adj, Wordlist(mock_get_adj, mock_get_adj_count)},
        {WordClasses::Adv, Wordlist(mock_get_adv, mock_get_adv_count)},
    };

    std::vector<WordClasses> pattern = {WordClasses::Adj, WordClasses::Noun};
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlists, pattern, 0, "-", char_classes, false, false);

    // Pattern is 2 words, so 1 hyphen
    EXPECT_TRUE(p.find('-') != std::string::npos);
    EXPECT_EQ(std::count(p.begin(), p.end(), '-'), 1);
}

TEST(PatternComposeTest, TestRandomPattern) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, Wordlist(mock_get_noun, mock_get_noun_count)},
        {WordClasses::Verb, Wordlist(mock_get_verb, mock_get_verb_count)},
        {WordClasses::Adj, Wordlist(mock_get_adj, mock_get_adj_count)},
        {WordClasses::Adv, Wordlist(mock_get_adv, mock_get_adv_count)},
    };

    std::vector<WordClasses> empty_pattern;
    std::vector<CharacterClass> char_classes;

    // length 3 should pick a pattern with 3 words
    auto p = ComposePassPhrase(g, wordlists, empty_pattern, 3, "-", char_classes, false, false);

    EXPECT_EQ(std::count(p.begin(), p.end(), '-'), 2);
}

TEST(PatternComposeTest, TestInvalidLength) {
    Generator<HKDF_Argon2> g("password", "service");
    std::map<WordClasses, Wordlist> wordlists = {
        {WordClasses::Noun, Wordlist(mock_get_noun, mock_get_noun_count)},
    };

    std::vector<WordClasses> empty_pattern;
    std::vector<CharacterClass> char_classes;

    // length 100 should throw because no patterns are defined for this length
    EXPECT_THROW(
        ComposePassPhrase(g, wordlists, empty_pattern, 100, "-", char_classes, false, false),
        std::runtime_error);
}
