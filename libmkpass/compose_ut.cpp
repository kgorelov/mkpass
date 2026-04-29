#include <iostream>

#include <set>

#include "compose.h"
#include "generator.h"
#include "hkdf_hmac.h"
#include "hkdf_argon2.h"
#include "character_classes.h"
#include "wordlists/eff_large_words.h"
#include <gtest/gtest.h>


bool check_char_class(const std::string& str, const std::string& strclass) {
    std::set<std::string::value_type> chs(strclass.begin(), strclass.end());
    return std::find_if(str.begin(), str.end(),
                        [&chs](auto& c){return chs.count(c);}) != str.end();
}


TEST(ComposeTest, TestCharClasses4) {
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    // Test that even the minimum possible length password
    // contains all requested character classes
    auto p = ComposePassword(g, classes, 4);
    for (auto& c: classes) {
        EXPECT_TRUE(check_char_class(p, c));
    }
}


TEST(ComposeTest, TestCharClasses16) {
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    // Test that the password
    // contains all requested character classes
    auto p = ComposePassword(g, classes, 16);
    for (auto& c: classes) {
        EXPECT_TRUE(check_char_class(p, c));
    }
}


TEST(ComposeTest, TestCompose8) {
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 8);
    EXPECT_EQ(p, "xn/T-d#4");
}

TEST(ComposeTest, TestComposeArgon2) {
    Generator<HKDF_Argon2> g("password", "service");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 16);
    EXPECT_EQ(p, "ijR-U#=/+S>[-F0,");
}

TEST(ComposeTest, TestComposeArgon2_Impossible) {
    Generator<HKDF_Argon2> g("Key", "Info");
    // It's impossible to generate a 3 character long password
    // using 4 character classes
    EXPECT_THROW(
        ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, 3),
        std::runtime_error);
}

TEST(ComposeTest, TestComposeArgon2_ZeroLength) {
    // Zero length password is an empty string
    Generator<HKDF_Argon2> g("Key", "Info");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 0);
    EXPECT_EQ(p, "");
}

TEST(ComposeTest, TestComposeArgon2_EmptyKey) {
    Generator<HKDF_Argon2> g("", "service");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 16);
    EXPECT_EQ(p, "Rt!ON9:MG0TeocZn");
}

TEST(ComposeTest, TestComposeArgon2_EmptyInfo) {
    Generator<HKDF_Argon2> g("password", "");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 16);
    EXPECT_EQ(p, "3I*LD/kG1ZYKbU)*");
}

TEST(ComposeTest, TestComposeArgon2_EmptyKeyAndInfo) {
    Generator<HKDF_Argon2> g("", "");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 16);
    EXPECT_EQ(p, "fQcBCq).Fd3+@rQ4");
}


TEST(ComposeTest, Impossible) {
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");
    // It's impossible to generate a 3 character long password
    // using 4 character classes
    EXPECT_THROW(
        ComposePassword(g, {UppercaseLetters, LowercaseLetters, Digits, Symbols}, 3),
        std::runtime_error);
}


TEST(ComposeTest, ZeroLength) {
    // Zero length password is an empty string
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 0);
    EXPECT_EQ(p, "");
}


TEST(ComposeTest, ComposeFrequency) {
    Generator<HKDF_HMAC<SHA512>> g("test", "");
    auto classes = {UppercaseLetters, LowercaseLetters, Digits, Symbols};
    auto p = ComposePassword(g, classes, 10*1024);

    typedef struct {
        std::set<char> chrs;
        unsigned cnt;
    } cls_cnt_t;

    std::map<std::string, cls_cnt_t> cls_counts = {
        {"upper", {{UppercaseLetters.begin(), UppercaseLetters.end()}, 0}},
        {"lower", {{LowercaseLetters.begin(), LowercaseLetters.end()}, 0}},
        {"digits", {{Digits.begin(), Digits.end()}, 0}},
        {"symbols", {{Symbols.begin(), Symbols.end()}, 0}}
    };

    for (auto& ch: p) {
        for (auto& [name, cls]: cls_counts) {
            if (cls.chrs.count(ch)) {
                ++cls.cnt;
            }
        }
    }

    // Count totals
    unsigned total_chars = 0;
    unsigned total_cnt = 0;
    for (auto& [name, cls]: cls_counts) {
        total_chars += cls.chrs.size();
        total_cnt += cls.cnt;
    }

    // Check that frequency of each character class in within 10% range
    // from theoretical ideal distribution
    for (auto& [name, cls]: cls_counts) {
        float r1 = 1.0 * cls.chrs.size() / total_chars;
        float r2 = 1.0 * cls.cnt / total_cnt;
        ASSERT_TRUE(100*abs(r1-r2)/r1 < 10);
    }
}

TEST(ComposeTest, TestOldAlgo) {
    auto p = ComposeOldMkpass1Password("master", "service", 8);
    EXPECT_EQ(p, "Zcxtvpqv");
}

TEST(ComposeTest, TestOldAlgo1) {
    auto p = ComposeOldMkpass1Password("password", "gmail.com", 10);
    EXPECT_EQ(p, "JZ2kMUREH3");
}

TEST(ComposeTest, TestOldAlgo2) {
    auto p = ComposeOldMkpass1Password("12345", "facebook", 12);
    EXPECT_EQ(p, "zRmHkPRuDvPW");
}

TEST(ComposeTest, TestOldAlgo3) {
    auto p = ComposeOldMkpass1Password("a", "b", 6);
    EXPECT_EQ(p, "2iNhTg");
}

TEST(ComposeTest, TestOldAlgo4) {
    auto p = ComposeOldMkpass1Password("Secret", "Service", 10);
    EXPECT_EQ(p, "RDeard32Oz");
}

TEST(ComposeTest, TestPassphraseKebabCase) {
    Generator<HKDF_Argon2> g("password", "service");
    Wordlist wordlist(eff_large_get_word, eff_large_get_word_count);
    std::vector<CharacterClass> char_classes;
    auto p = ComposePassPhrase(g, wordlist, 3, PassphraseSeparator::KebabCase, char_classes, false);
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
    auto p = ComposePassPhrase(g, wordlist, 3, PassphraseSeparator::CamelCase, char_classes, false);
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
