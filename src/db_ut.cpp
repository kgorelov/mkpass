#include "db.h"
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdlib.h>

class ConfigDBTest : public ::testing::Test {
protected:
    std::string db_path;

    void SetUp() override {
        char tmpl[] = "/tmp/mkpass-test-XXXXXX";
        int fd = mkstemp(tmpl);
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
            "CREATE TABLE service_entries (name TEXT PRIMARY KEY, algorithm INTEGER, length INTEGER, char_classes INTEGER);"
            "INSERT INTO service_entries VALUES ('user@github.com', 1, 11, 1);"
            "INSERT INTO service_entries VALUES ('gitlab.com', 1, 12, 1);";
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

TEST(ConfigDB, NonExistentDB) {
    mkpass::ConfigDB db("non-existent-db.db");
    auto names = db.get_all_service_names();
    ASSERT_TRUE(names.empty());
}

TEST(ConfigDB, EnvVariable) {
    const char* db_path = "/tmp/mkpass-test-env.db";
    setenv("MKPASS_DB_PATH", db_path, 1);

    mkpass::ConfigDB db;
    auto snames = db.get_all_service_names();
    ASSERT_TRUE(snames.empty());

    // Check that the database file was created
    ASSERT_EQ(access(db_path, F_OK), 0);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}
