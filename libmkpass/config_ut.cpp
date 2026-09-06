#include "config.h"
#include "algorithms.h"
#include "character_classes.h"
#include "platform_utils.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <cstdlib>

class ConfigTest : public ::testing::Test {
protected:
    std::string test_config_path;

    void SetUp() override {
        std::string tmpl = GetTmpDir() + "/mkpass-cfg-test-XXXXXX";
        int fd = mkstemp((char*)tmpl.c_str());
        if (fd != -1) {
            test_config_path = tmpl;
            close(fd);
            unlink(test_config_path.c_str());
        }
    }

    void TearDown() override {
        if (!test_config_path.empty()) {
            unlink(test_config_path.c_str());
        }
        unsetenv("MKPASS_ENABLE_OLD_ALGORITHM");
        unsetenv("enable_old_algorithm");
    }
};

TEST(AlgorithmIdentifierTest, ConversionAndParsing) {
    EXPECT_EQ(AlgorithmToIdentifier(Algorithm::Argon2), "password/argon2");
    EXPECT_EQ(AlgorithmToIdentifier(Algorithm::SlowSha512), "password/sha512");
    EXPECT_EQ(AlgorithmToIdentifier(Algorithm::Old), "password/old");
    EXPECT_EQ(AlgorithmToIdentifier(Algorithm::Passphrase_Diceware_EFF_Large), "passphrase/diceware");
    EXPECT_EQ(AlgorithmToIdentifier(Algorithm::Passphrase_Wordnet_Pattern), "passphrase/wordnet");

    // Canonical parsing
    EXPECT_EQ(ParseAlgorithm("password/argon2"), Algorithm::Argon2);
    EXPECT_EQ(ParseAlgorithm("password/sha512"), Algorithm::SlowSha512);
    EXPECT_EQ(ParseAlgorithm("password/old"), Algorithm::Old);
    EXPECT_EQ(ParseAlgorithm("passphrase/diceware"), Algorithm::Passphrase_Diceware_EFF_Large);
    EXPECT_EQ(ParseAlgorithm("passphrase/wordnet"), Algorithm::Passphrase_Wordnet_Pattern);

    // Aliases
    EXPECT_EQ(ParseAlgorithm("argon2"), Algorithm::Argon2);
    EXPECT_EQ(ParseAlgorithm("sha512"), Algorithm::SlowSha512);
    EXPECT_EQ(ParseAlgorithm("old"), Algorithm::Old);
    EXPECT_EQ(ParseAlgorithm("diceware"), Algorithm::Passphrase_Diceware_EFF_Large);
    EXPECT_EQ(ParseAlgorithm("wordnet"), Algorithm::Passphrase_Wordnet_Pattern);

    // Legacy numeric values
    EXPECT_EQ(ParseAlgorithm("1"), Algorithm::Argon2);
    EXPECT_EQ(ParseAlgorithm("2"), Algorithm::SlowSha512);
    EXPECT_EQ(ParseAlgorithm("3"), Algorithm::Old);
    EXPECT_EQ(ParseAlgorithm("4"), Algorithm::Passphrase_Diceware_EFF_Large);
    EXPECT_EQ(ParseAlgorithm("5"), Algorithm::Passphrase_Wordnet_Pattern);

    // Case-insensitivity and whitespace
    EXPECT_EQ(ParseAlgorithm("  Password/Argon2  "), Algorithm::Argon2);
    EXPECT_EQ(ParseAlgorithm("PASSPHRASE/DICEWARE"), Algorithm::Passphrase_Diceware_EFF_Large);

    // Invalid values
    EXPECT_EQ(ParseAlgorithm(""), std::nullopt);
    EXPECT_EQ(ParseAlgorithm("invalid"), std::nullopt);
    EXPECT_EQ(ParseAlgorithm("6"), std::nullopt);
    EXPECT_EQ(ParseAlgorithm("password/unknown"), std::nullopt);
}

TEST(CharacterClassesIdentifierTest, ConversionAndParsing) {
    std::vector<CharacterClass> all = {
        CharacterClass::LOWERCASE,
        CharacterClass::UPPERCASE,
        CharacterClass::DIGITS,
        CharacterClass::SYMBOLS
    };
    EXPECT_EQ(CharacterClassesToIdentifierString(all), "lowercase,uppercase,digits,symbols");

    std::vector<CharacterClass> subset = {
        CharacterClass::UPPERCASE,
        CharacterClass::DIGITS
    };
    EXPECT_EQ(CharacterClassesToIdentifierString(subset), "uppercase,digits");

    std::vector<CharacterClass> custom = { CharacterClass::CUSTOM };
    EXPECT_EQ(CharacterClassesToIdentifierString(custom), "custom");

    // Parsing comma-separated string
    auto parsed_all = ParseCharacterClasses("lowercase,uppercase,digits,symbols");
    EXPECT_EQ(parsed_all, all);

    auto parsed_subset = ParseCharacterClasses("uppercase,digits");
    EXPECT_EQ(parsed_subset, subset);

    // Aliases
    auto parsed_aliases = ParseCharacterClasses("lower,upper,digit,symbol");
    EXPECT_EQ(parsed_aliases, all);

    // Whitespace trimming & case insensitivity
    auto parsed_spaces = ParseCharacterClasses("  lowercase , Uppercase , digits ");
    std::vector<CharacterClass> expected_three = {
        CharacterClass::LOWERCASE,
        CharacterClass::UPPERCASE,
        CharacterClass::DIGITS
    };
    EXPECT_EQ(parsed_spaces, expected_three);

    // Legacy numeric strings
    auto parsed_legacy = ParseCharacterClasses("1234");
    EXPECT_EQ(parsed_legacy, all);

    auto parsed_legacy_5 = ParseCharacterClasses("5");
    EXPECT_EQ(parsed_legacy_5, custom);

    // Empty string
    EXPECT_TRUE(ParseCharacterClasses("").empty());
}

TEST_F(ConfigTest, NonExistentFileLoadsDefaults) {
    mkpass::Config cfg(test_config_path);
    EXPECT_FALSE(cfg.exists());
    EXPECT_TRUE(cfg.load());
    EXPECT_FALSE(cfg.options().algorithm.has_value());
    EXPECT_FALSE(cfg.options().length.has_value());
    EXPECT_FALSE(cfg.options().char_classes.has_value());
    EXPECT_FALSE(cfg.options().enable_old_algorithm.has_value());
}

TEST_F(ConfigTest, SaveAndLoadRoundTrip) {
    mkpass::Config cfg(test_config_path);

    cfg.set_raw("algorithm", "password/sha512");
    cfg.set_raw("char_classes", "lowercase,uppercase,digits");
    cfg.set_raw("custom_chars", "!@#$%");
    cfg.set_raw("length", "24");
    cfg.set_raw("separator", "-");
    cfg.set_raw("passphrase_pattern", "navrn");
    cfg.set_raw("digits", "true");
    cfg.set_raw("symbols", "false");
    cfg.set_raw("substitutions", "true");
    cfg.set_raw("capitalize", "false");
    cfg.set_raw("enable_old_algorithm", "true");

    EXPECT_TRUE(cfg.save());
    EXPECT_TRUE(cfg.exists());

    // Load from disk with a fresh Config instance
    mkpass::Config loaded(test_config_path);
    EXPECT_TRUE(loaded.load());

    EXPECT_EQ(loaded.get_raw("algorithm"), "password/sha512");
    EXPECT_EQ(loaded.get_raw("char_classes"), "lowercase,uppercase,digits");
    EXPECT_EQ(loaded.get_raw("custom_chars"), "!@#$%");
    EXPECT_EQ(loaded.get_raw("length"), "24");
    EXPECT_EQ(loaded.get_raw("separator"), "-");
    EXPECT_EQ(loaded.get_raw("passphrase_pattern"), "navrn");
    EXPECT_EQ(loaded.get_raw("digits"), "true");
    EXPECT_EQ(loaded.get_raw("symbols"), "false");
    EXPECT_EQ(loaded.get_raw("substitutions"), "true");
    EXPECT_EQ(loaded.get_raw("capitalize"), "false");
    EXPECT_EQ(loaded.get_raw("enable_old_algorithm"), "true");

    // Strongly typed options
    ASSERT_TRUE(loaded.options().algorithm.has_value());
    EXPECT_EQ(*loaded.options().algorithm, Algorithm::SlowSha512);

    ASSERT_TRUE(loaded.options().length.has_value());
    EXPECT_EQ(*loaded.options().length, 24u);

    ASSERT_TRUE(loaded.options().digits.has_value());
    EXPECT_TRUE(*loaded.options().digits);

    ASSERT_TRUE(loaded.options().symbols.has_value());
    EXPECT_FALSE(*loaded.options().symbols);

    ASSERT_TRUE(loaded.options().enable_old_algorithm.has_value());
    EXPECT_TRUE(*loaded.options().enable_old_algorithm);
}

TEST_F(ConfigTest, GetSetUnsetAndValidation) {
    mkpass::Config cfg(test_config_path);

    // Unknown key
    EXPECT_FALSE(mkpass::Config::is_valid_key("non_existent"));
    EXPECT_THROW(cfg.get_raw("non_existent"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("non_existent", "val"), std::invalid_argument);
    EXPECT_THROW(cfg.unset_raw("non_existent"), std::invalid_argument);

    // Validation of values
    EXPECT_THROW(cfg.set_raw("algorithm", "unsupported_algo"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("length", "0"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("length", "-5"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("length", "not_a_number"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("digits", "maybe"), std::invalid_argument);
    EXPECT_THROW(cfg.set_raw("passphrase_pattern", "xyz"), std::invalid_argument);

    // Unset
    cfg.set_raw("length", "32");
    EXPECT_TRUE(cfg.is_set("length"));
    EXPECT_EQ(cfg.get_raw("length"), "32");

    EXPECT_TRUE(cfg.unset_raw("length"));
    EXPECT_FALSE(cfg.is_set("length"));
    EXPECT_FALSE(cfg.get_raw("length").has_value());

    // Unsetting again returns false (was not set)
    EXPECT_FALSE(cfg.unset_raw("length"));
}

TEST_F(ConfigTest, LegacyNumericTomlParsing) {
    // Write a TOML file using legacy numbers
    std::ofstream out(test_config_path);
    out << "algorithm = 1\n";
    out << "char_classes = \"1234\"\n";
    out << "length = 18\n";
    out.close();

    mkpass::Config cfg(test_config_path);
    EXPECT_TRUE(cfg.load());

    ASSERT_TRUE(cfg.options().algorithm.has_value());
    EXPECT_EQ(*cfg.options().algorithm, Algorithm::Argon2);

    ASSERT_TRUE(cfg.options().char_classes.has_value());
    EXPECT_EQ(cfg.options().char_classes->size(), 4u);

    // Canonical string output
    EXPECT_EQ(cfg.get_raw("algorithm"), "password/argon2");
    EXPECT_EQ(cfg.get_raw("char_classes"), "lowercase,uppercase,digits,symbols");

    // Re-saving writes canonical strings
    EXPECT_TRUE(cfg.save());
    std::string print_out = cfg.print();
    EXPECT_NE(print_out.find("password/argon2"), std::string::npos);
    EXPECT_NE(print_out.find("lowercase,uppercase,digits,symbols"), std::string::npos);
}

TEST_F(ConfigTest, OldAlgorithmEnforcementFlag) {
    mkpass::Config cfg(test_config_path);

    // Initially false by default
    EXPECT_FALSE(mkpass::IsOldAlgorithmEnabled(cfg));

    // Enabled in config
    cfg.set_raw("enable_old_algorithm", "true");
    EXPECT_TRUE(mkpass::IsOldAlgorithmEnabled(cfg));

    cfg.set_raw("enable_old_algorithm", "false");
    EXPECT_FALSE(mkpass::IsOldAlgorithmEnabled(cfg));

    // Overridden by MKPASS_ENABLE_OLD_ALGORITHM
    setenv("MKPASS_ENABLE_OLD_ALGORITHM", "true", 1);
    EXPECT_TRUE(mkpass::IsOldAlgorithmEnabled(cfg));

    setenv("MKPASS_ENABLE_OLD_ALGORITHM", "0", 1);
    EXPECT_FALSE(mkpass::IsOldAlgorithmEnabled(cfg));
    unsetenv("MKPASS_ENABLE_OLD_ALGORITHM");

    // Overridden by enable_old_algorithm
    setenv("enable_old_algorithm", "1", 1);
    EXPECT_TRUE(mkpass::IsOldAlgorithmEnabled(cfg));
}
