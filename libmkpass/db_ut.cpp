#include "db.h"
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <algorithm>

#include "platform_utils.h"


class ConfigDBTest : public ::testing::Test {
protected:
    std::string db_path;

    void SetUp() override {
        std::string tmpl = GetTmpDir() + "/mkpass-test-XXXXXX";
        int fd = mkstemp((char*)tmpl.c_str());
        if (fd != -1) {
	    db_path = tmpl;
            close(fd);
        }

        // Create a dummy database for testing
        sqlite3 *db;
        sqlite3_open(db_path.c_str(), &db);
        const char *sql =
            "CREATE TABLE snames (name TEXT PRIMARY KEY, length INTEGER);"
            "INSERT INTO snames VALUES ('github.com', 10);"
            "INSERT INTO snames VALUES ('google.com', 12);";
        char *err_msg = 0;
        sqlite3_exec(db, sql, 0, 0, &err_msg);

        const char *sql2 =
            "CREATE TABLE service_entries (name TEXT PRIMARY KEY, algorithm INTEGER, length INTEGER, char_classes INTEGER, custom_chars TEXT);"
            "INSERT INTO service_entries VALUES ('user@github.com', 1, 11, 1, NULL);"
            "INSERT INTO service_entries VALUES ('gitlab.com', 1, 12, 1, NULL);";
        sqlite3_exec(db, sql2, 0, 0, &err_msg);

        sqlite3_close(db);
    }

    void TearDown() override {
        if (!db_path.empty()) {
            remove(db_path.c_str());
        }
    }
};

TEST_F(ConfigDBTest, GetAllServiceNames) {
    mkpass::ConfigDB db(db_path);
    auto service_names = db.get_all_service_names();
    ASSERT_EQ(service_names.size(), 4);
    EXPECT_EQ(service_names.count("github.com"), 1);
    EXPECT_EQ(service_names.count("google.com"), 1);
    EXPECT_EQ(service_names.count("user@github.com"), 1);
    EXPECT_EQ(service_names.count("gitlab.com"), 1);
}

TEST_F(ConfigDBTest, GetServiceRecordOld) {
    mkpass::ConfigDB db(db_path);

    auto rec = db.get_service_entry("github.com");
    EXPECT_EQ(static_cast<bool>(rec), true);
    EXPECT_EQ(rec->service_name, "github.com");
    EXPECT_EQ(rec->algorithm, Algorithm::Old);
    EXPECT_EQ(rec->length, 10);
    std::vector<CharacterClass> cs{};
    EXPECT_EQ(rec->char_classes, cs);
}

TEST_F(ConfigDBTest, GetServiceRecordNew) {
    mkpass::ConfigDB db(db_path);

    auto rec = db.get_service_entry("user@github.com");
    EXPECT_EQ(static_cast<bool>(rec), true);
    EXPECT_EQ(rec->service_name, "user@github.com");
    EXPECT_EQ(rec->algorithm, Algorithm::Argon2);
    EXPECT_EQ(rec->length, 11);
    std::vector<CharacterClass> cs{CharacterClass::LOWERCASE};
    EXPECT_EQ(rec->char_classes, cs);
}

TEST_F(ConfigDBTest, CustomChars) {
    mkpass::ConfigDB db(db_path);

    // 1. Test with custom_chars
    mkpass::ServiceEntry entry;
    entry.service_name = "test.com";
    entry.algorithm = Algorithm::Argon2;
    entry.length = 16;
    entry.char_classes = {CharacterClass::LOWERCASE, CharacterClass::CUSTOM};
    entry.custom_chars = "!@#$";

    db.save_service_entry(entry);

    auto rec = db.get_service_entry("test.com");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->service_name, "test.com");
    EXPECT_EQ(rec->algorithm, Algorithm::Argon2);
    EXPECT_EQ(rec->length, 16);

    std::vector<CharacterClass> expected_classes = {CharacterClass::LOWERCASE, CharacterClass::CUSTOM};
    EXPECT_EQ(rec->char_classes, expected_classes);

    ASSERT_TRUE(rec->custom_chars.has_value());
    EXPECT_EQ(rec->custom_chars.value(), "!@#$");

    // 2. Test without custom_chars (should be nullopt)
    auto rec_no_custom = db.get_service_entry("user@github.com");
    ASSERT_TRUE(rec_no_custom.has_value());
    EXPECT_FALSE(rec_no_custom->custom_chars.has_value());
}

TEST_F(ConfigDBTest, Separator) {
    mkpass::ConfigDB db(db_path);

    // 1. Test with empty separator and capitalize_words
    mkpass::ServiceEntry entry;
    entry.service_name = "camel.com";
    entry.algorithm = Algorithm::Passphrase_Diceware_EFF_Large;
    entry.length = 6;
    entry.char_classes = {};
    entry.separator = "";
    entry.capitalize_words = true;

    db.save_service_entry(entry);

    auto rec = db.get_service_entry("camel.com");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->separator, "");
    EXPECT_TRUE(rec->capitalize_words);

    // 2. Test with KebabCase (-)
    entry.service_name = "kebab.com";
    entry.separator = "-";
    entry.capitalize_words = false;
    db.save_service_entry(entry);

    auto rec2 = db.get_service_entry("kebab.com");
    ASSERT_TRUE(rec2.has_value());
    EXPECT_EQ(rec2->separator, "-");
    EXPECT_FALSE(rec2->capitalize_words);

    // 3. Test default (existing record without separator column should default to empty/false if no mapping found)
    auto rec_old = db.get_service_entry("user@github.com");
    ASSERT_TRUE(rec_old.has_value());
}

TEST_F(ConfigDBTest, AllowSubstitutions) {
    mkpass::ConfigDB db(db_path);

    // 1. Test with allow_substitutions = true
    mkpass::ServiceEntry entry;
    entry.service_name = "subst.com";
    entry.algorithm = Algorithm::Passphrase_Diceware_EFF_Large;
    entry.length = 6;
    entry.char_classes = {};
    entry.separator = "";
    entry.capitalize_words = true;
    entry.allow_substitutions = true;

    db.save_service_entry(entry);

    auto rec = db.get_service_entry("subst.com");
    ASSERT_TRUE(rec.has_value());
    EXPECT_TRUE(rec->allow_substitutions);

    // 2. Test with allow_substitutions = false
    entry.service_name = "nosubst.com";
    entry.allow_substitutions = false;
    db.save_service_entry(entry);

    auto rec2 = db.get_service_entry("nosubst.com");
    ASSERT_TRUE(rec2.has_value());
    EXPECT_FALSE(rec2->allow_substitutions);
}

TEST(ConfigDB, NonExistentDB) {
    mkpass::ConfigDB db("non-existent-db.db");
    auto names = db.get_all_service_names();
    ASSERT_TRUE(names.empty());
}

TEST(ConfigDB, EnvVariable) {
    std::string db_path = GetTmpDir() + "/mkpass-test-env.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);

    mkpass::ConfigDB db(GetConfigDBPath());
    auto snames = db.get_all_service_names();
    ASSERT_TRUE(snames.empty());

    // Check that the database file was created
    ASSERT_EQ(access(db_path.c_str(), F_OK), 0);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST_F(ConfigDBTest, DeleteServiceEntry) {
    mkpass::ConfigDB db(db_path);

    // Initial count: 4
    ASSERT_EQ(db.get_all_service_names().size(), 4);

    // Delete from service_entries (new table)
    db.delete_service_entry("gitlab.com");
    auto names = db.get_all_service_names();
    ASSERT_EQ(names.size(), 3);
    EXPECT_EQ(names.count("gitlab.com"), 0);

    // Delete from snames (old table)
    db.delete_service_entry("google.com");
    names = db.get_all_service_names();
    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names.count("google.com"), 0);

    // Delete non-existent
    db.delete_service_entry("non-existent.com");
    ASSERT_EQ(db.get_all_service_names().size(), 2);
}

TEST_F(ConfigDBTest, ServiceWhitespaceStripping) {
    mkpass::ConfigDB db(db_path);

    mkpass::ServiceEntry entry;
    entry.service_name = "test_whitespace_service   \n\t ";
    entry.algorithm = Algorithm::Argon2;
    entry.length = 16;
    entry.char_classes = {CharacterClass::LOWERCASE};

    db.save_service_entry(entry);

    auto rec = db.get_service_entry("test_whitespace_service");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->service_name, "test_whitespace_service");

    auto rec2 = db.get_service_entry("test_whitespace_service  \t\n ");
    ASSERT_TRUE(rec2.has_value());
    EXPECT_EQ(rec2->service_name, "test_whitespace_service");

    db.delete_service_entry("test_whitespace_service \t ");
    auto rec3 = db.get_service_entry("test_whitespace_service");
    EXPECT_FALSE(rec3.has_value());
}

TEST(ServiceDetailsTest, HelperFunctions) {
    EXPECT_EQ(mkpass::GetAlgorithmName(Algorithm::Argon2), "Password (Argon2)");
    EXPECT_EQ(mkpass::GetAlgorithmName(Algorithm::SlowSha512), "Password (SHA512 HMAC)");
    EXPECT_EQ(mkpass::GetAlgorithmName(Algorithm::Old), "OldPassword");
    EXPECT_EQ(mkpass::GetAlgorithmName(Algorithm::Passphrase_Diceware_EFF_Large), "Passphrase Diceware (Argon2)");
    EXPECT_EQ(mkpass::GetAlgorithmName(Algorithm::Passphrase_Wordnet_Pattern), "Passphrase Wordnet Pattern (Argon2)");

    EXPECT_EQ(mkpass::GetSeparatorName(""), "None");
    EXPECT_EQ(mkpass::GetSeparatorName("-"), "Hyphen (-)");
    EXPECT_EQ(mkpass::GetSeparatorName(" "), "Space ( )");
    EXPECT_EQ(mkpass::GetSeparatorName("/"), "Slash (/)");
    EXPECT_EQ(mkpass::GetSeparatorName("_"), "_");

    std::vector<CharacterClass> cc = {
        CharacterClass::LOWERCASE,
        CharacterClass::UPPERCASE,
        CharacterClass::DIGITS,
        CharacterClass::SYMBOLS,
        CharacterClass::CUSTOM
    };
    EXPECT_EQ(mkpass::GetCharacterClassesString(cc, "!@#"),
              "Lowercase Letters, Uppercase Letters, Digits, Symbols, Custom (!@#)");
    EXPECT_EQ(mkpass::GetCharacterClassesString({}), "None");
}

TEST(ServiceDetailsTest, GetServiceEntryDetails) {
    // 1. Password Argon2 entry
    mkpass::ServiceEntry pass_entry;
    pass_entry.service_name = "github.com";
    pass_entry.algorithm = Algorithm::Argon2;
    pass_entry.length = 16;
    pass_entry.char_classes = {CharacterClass::LOWERCASE, CharacterClass::UPPERCASE, CharacterClass::DIGITS};

    auto details1 = mkpass::GetServiceEntryDetails(pass_entry);
    ASSERT_EQ(details1.size(), 4);
    EXPECT_EQ(details1[0], std::make_pair(std::string("Service name"), std::string("github.com")));
    EXPECT_EQ(details1[1], std::make_pair(std::string("Algorithm"), std::string("Password (Argon2)")));
    EXPECT_EQ(details1[2], std::make_pair(std::string("Password length"), std::string("16")));
    EXPECT_EQ(details1[3], std::make_pair(std::string("Character classes"), std::string("Lowercase Letters, Uppercase Letters, Digits")));

    // 2. Old password entry
    mkpass::ServiceEntry old_entry;
    old_entry.service_name = "legacy.site";
    old_entry.algorithm = Algorithm::Old;
    old_entry.length = 8;

    auto details2 = mkpass::GetServiceEntryDetails(old_entry);
    ASSERT_EQ(details2.size(), 3);
    EXPECT_EQ(details2[0], std::make_pair(std::string("Service name"), std::string("legacy.site")));
    EXPECT_EQ(details2[1], std::make_pair(std::string("Algorithm"), std::string("OldPassword")));
    EXPECT_EQ(details2[2], std::make_pair(std::string("Password length"), std::string("8")));

    // 3. Diceware passphrase entry
    mkpass::ServiceEntry dice_entry;
    dice_entry.service_name = "bank.com";
    dice_entry.algorithm = Algorithm::Passphrase_Diceware_EFF_Large;
    dice_entry.length = 4;
    dice_entry.separator = "-";
    dice_entry.capitalize_words = true;
    dice_entry.char_classes = {CharacterClass::DIGITS};
    dice_entry.allow_substitutions = true;

    auto details3 = mkpass::GetServiceEntryDetails(dice_entry);
    ASSERT_EQ(details3.size(), 8);
    EXPECT_EQ(details3[0], std::make_pair(std::string("Service name"), std::string("bank.com")));
    EXPECT_EQ(details3[1], std::make_pair(std::string("Algorithm"), std::string("Passphrase Diceware (Argon2)")));
    EXPECT_EQ(details3[2], std::make_pair(std::string("Words count"), std::string("4")));
    EXPECT_EQ(details3[3], std::make_pair(std::string("Separator"), std::string("Hyphen (-)")));
    EXPECT_EQ(details3[4], std::make_pair(std::string("Capitalize words"), std::string("Yes")));
    EXPECT_EQ(details3[5], std::make_pair(std::string("Include digits"), std::string("Yes")));
    EXPECT_EQ(details3[6], std::make_pair(std::string("Include symbols"), std::string("No")));
    EXPECT_EQ(details3[7], std::make_pair(std::string("Allow substitutions"), std::string("Yes")));

    // 4. Wordnet Pattern entry
    mkpass::ServiceEntry wn_entry;
    wn_entry.service_name = "pattern.com";
    wn_entry.algorithm = Algorithm::Passphrase_Wordnet_Pattern;
    wn_entry.length = 3;
    wn_entry.passphrase_pattern = {WordClasses::Noun, WordClasses::Adj, WordClasses::Verb};
    wn_entry.separator = " ";
    wn_entry.capitalize_words = false;
    wn_entry.char_classes = {};
    wn_entry.allow_substitutions = false;

    auto details4 = mkpass::GetServiceEntryDetails(wn_entry);
    ASSERT_EQ(details4.size(), 9);
    EXPECT_EQ(details4[0], std::make_pair(std::string("Service name"), std::string("pattern.com")));
    EXPECT_EQ(details4[1], std::make_pair(std::string("Algorithm"), std::string("Passphrase Wordnet Pattern (Argon2)")));
    EXPECT_EQ(details4[2], std::make_pair(std::string("Words count"), std::string("3")));
    EXPECT_EQ(details4[3], std::make_pair(std::string("Passphrase pattern"), std::string("nav")));
    EXPECT_EQ(details4[4], std::make_pair(std::string("Separator"), std::string("Space ( )")));
    EXPECT_EQ(details4[5], std::make_pair(std::string("Capitalize words"), std::string("No")));
    EXPECT_EQ(details4[6], std::make_pair(std::string("Include digits"), std::string("No")));
    EXPECT_EQ(details4[7], std::make_pair(std::string("Include symbols"), std::string("No")));
    EXPECT_EQ(details4[8], std::make_pair(std::string("Allow substitutions"), std::string("No")));
}
