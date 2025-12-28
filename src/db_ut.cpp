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
        sqlite3_close(db);
    }

    void TearDown() override {
        if (!db_path.empty()) {
            remove(db_path.c_str());
        }
    }
};

TEST_F(ConfigDBTest, GetAllSnames) {
    mkpass::ConfigDB db(db_path);
    auto snames = db.get_all_snames();
    ASSERT_EQ(snames.size(), 2);
    EXPECT_EQ(snames["github.com"], 10);
    EXPECT_EQ(snames["google.com"], 12);
}

TEST(ConfigDB, NonExistentDB) {
    mkpass::ConfigDB db("non-existent-db.db");
    auto snames = db.get_all_snames();
    ASSERT_TRUE(snames.empty());
}

TEST(ConfigDB, EnvVariable) {
    const char* db_path = "/tmp/mkpass-test-env.db";
    setenv("MKPASS_DB_PATH", db_path, 1);

    mkpass::ConfigDB db;
    auto snames = db.get_all_snames();
    ASSERT_TRUE(snames.empty());

    // Check that the database file was created
    ASSERT_EQ(access(db_path, F_OK), 0);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}
