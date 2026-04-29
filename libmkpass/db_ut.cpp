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

    // 1. Test with CamelCase
    mkpass::ServiceEntry entry;
    entry.service_name = "camel.com";
    entry.algorithm = Algorithm::Passphrase_Diceware_EFF_Large;
    entry.length = 6;
    entry.char_classes = {};
    entry.separator = PassphraseSeparator::CamelCase;

    db.save_service_entry(entry);

    auto rec = db.get_service_entry("camel.com");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->separator, PassphraseSeparator::CamelCase);

    // 2. Test with KebabCase
    entry.service_name = "kebab.com";
    entry.separator = PassphraseSeparator::KebabCase;
    db.save_service_entry(entry);

    auto rec2 = db.get_service_entry("kebab.com");
    ASSERT_TRUE(rec2.has_value());
    EXPECT_EQ(rec2->separator, PassphraseSeparator::KebabCase);

    // 3. Test default (existing record without separator column should default to CamelCase)
    // Actually, create_tables adds the column with DEFAULT 1 (CamelCase)
    auto rec_old = db.get_service_entry("user@github.com");
    ASSERT_TRUE(rec_old.has_value());
    EXPECT_EQ(rec_old->separator, PassphraseSeparator::CamelCase);
}

TEST_F(ConfigDBTest, AllowSubstitutions) {
    mkpass::ConfigDB db(db_path);

    // 1. Test with allow_substitutions = true
    mkpass::ServiceEntry entry;
    entry.service_name = "subst.com";
    entry.algorithm = Algorithm::Passphrase_Diceware_EFF_Large;
    entry.length = 6;
    entry.char_classes = {};
    entry.separator = PassphraseSeparator::CamelCase;
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
