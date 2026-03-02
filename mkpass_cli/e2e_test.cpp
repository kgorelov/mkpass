#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
#include <algorithm>
#include <sqlite3.h>
#include "character_classes.h"
#include "algorithms.h"

struct ProcessOutput {
    std::string std_out;
    std::string std_err;
    int exit_code;
};

ProcessOutput exec_with_input(const std::string& cmd, const std::string& input) {
    std::array<int, 2> pipe_stdin;
    std::array<int, 2> pipe_stdout;
    std::array<int, 2> pipe_stderr;

    if (pipe(pipe_stdin.data()) == -1) {
        throw std::runtime_error("pipe_stdin failed");
    }
    if (pipe(pipe_stdout.data()) == -1) {
        throw std::runtime_error("pipe_stdout failed");
    }
    if (pipe(pipe_stderr.data()) == -1) {
        throw std::runtime_error("pipe_stderr failed");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) { // child process
        close(pipe_stdin[1]);
        dup2(pipe_stdin[0], STDIN_FILENO);
        close(pipe_stdin[0]);

        close(pipe_stdout[0]);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        close(pipe_stdout[1]);

        close(pipe_stderr[0]);
        dup2(pipe_stderr[1], STDERR_FILENO);
        close(pipe_stderr[1]);

        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127); // If execl fails
    }

    // parent process
    close(pipe_stdin[0]);
    close(pipe_stdout[1]);
    close(pipe_stderr[1]);

    if (!input.empty()) {
        write(pipe_stdin[1], input.c_str(), input.size());
    }
    close(pipe_stdin[1]);

    std::string std_out;
    std::string std_err;
    std::array<char, 128> buffer;

    while (true) {
        ssize_t count = read(pipe_stdout[0], buffer.data(), buffer.size());
        if (count > 0) {
            std_out.append(buffer.data(), count);
        } else {
            break;
        }
    }

    while (true) {
        ssize_t count = read(pipe_stderr[0], buffer.data(), buffer.size());
        if (count > 0) {
            std_err.append(buffer.data(), count);
        } else {
            break;
        }
    }

    close(pipe_stdout[0]);
    close(pipe_stderr[0]);

    int status;
    waitpid(pid, &status, 0);

    return {std_out, std_err, WEXITSTATUS(status)};
}

void trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


TEST(E2ETest, SimpleTest) {
    std::cout << "MKPASS_EXECUTABLE_PATH: " << MKPASS_EXECUTABLE_PATH << std::endl;
    std::string input = "master_password\nmaster_password\nservice_name\n1\n1234\n16\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 16);
}

TEST(E2ETest, DatabasePath) {
    const char* db_path = "/tmp/mkpass-e2e-test.db";
    setenv("MKPASS_DB_PATH", db_path, 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path, &db);
    const char *sql =
        "CREATE TABLE snames (name TEXT PRIMARY KEY, length INTEGER);"
        "INSERT INTO snames VALUES ('github.com', 10);"
        "INSERT INTO snames VALUES ('gitlab.com', 12);";
    char *err_msg = 0;
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_close(db);

    std::string input = "master_password\nmaster_password\ngit\t\t\n1\n1234\n16\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 16);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}

TEST(E2ETest, DatabasePathWithUsernames) {
    const char* db_path = "/tmp/mkpass-e2e-test-2.db";
    setenv("MKPASS_DB_PATH", db_path, 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path, &db);
    const char *sql =
        "CREATE TABLE snames (name TEXT PRIMARY KEY, length INTEGER);"
        "INSERT INTO snames VALUES ('user@github.com', 15);"
        "INSERT INTO snames VALUES ('user@gitlab.com', 18);";
    char *err_msg = 0;
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_close(db);

    std::string input = "master_password\nmaster_password\nuser@git\t\t\n1\n1234\n20\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 20);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}

TEST(E2ETest, AutocompleteOldSnames) {
    const char* db_path = "/tmp/mkpass-e2e-test-3.db";
    setenv("MKPASS_DB_PATH", db_path, 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path, &db);
    const char *sql =
        "CREATE TABLE snames (name TEXT PRIMARY KEY, length INTEGER);"
        "INSERT INTO snames VALUES ('user@github.com', 15);"
        "INSERT INTO snames VALUES ('user@gitlab.com', 18);"
        "INSERT INTO snames VALUES ('google.com', 20);";
    char *err_msg = 0;
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_close(db);

    std::string input = "master_password\nmaster_password\nuser@\t\t\n1\n1234\n20\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 20);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}

TEST(E2ETest, CtrlCAtServiceName) {
    std::string input = "master_password\nmaster_password\n\x03";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    EXPECT_EQ(output.exit_code, 130);
}

TEST(E2EServiceEntriesTest, DatabaseUpdate) {
    const char* db_path = "/tmp/mkpass-e2e-test2-db-update.db";
    setenv("MKPASS_DB_PATH", db_path, 1);
    remove(db_path);

    std::string input = "master_password\nmaster_password\nnew_service.com\n1\n1234\n24\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 24);

    // Check if the database was updated
    sqlite3 *db;
    sqlite3_open(db_path, &db);
    ASSERT_TRUE(db != nullptr);

    sqlite3_stmt *stmt;
    const char *sql = "SELECT algorithm, length, char_classes FROM service_entries WHERE name = 'new_service.com'";
    ASSERT_EQ(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr), SQLITE_OK);

    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), static_cast<int>(Algorithm::Argon2));
    EXPECT_EQ(sqlite3_column_int(stmt, 1), 24);
    int expected_char_classes = (1 << static_cast<int>(CharacterClass::LOWERCASE)) |
                                (1 << static_cast<int>(CharacterClass::UPPERCASE)) |
                                (1 << static_cast<int>(CharacterClass::DIGITS)) |
                                (1 << static_cast<int>(CharacterClass::SYMBOLS));
    EXPECT_EQ(sqlite3_column_int(stmt, 2), expected_char_classes);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}

TEST(E2EServiceEntriesTest, AutocompleteNewServiceEntries) {
    const char* db_path = "/tmp/mkpass-e2e2-autocomplete.db";
    setenv("MKPASS_DB_PATH", db_path, 1);
    remove(db_path);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path, &db);
    const char *sql =
        "CREATE TABLE service_entries (name TEXT PRIMARY KEY, algorithm INTEGER, length INTEGER, char_classes INTEGER);"
        "INSERT INTO service_entries VALUES ('github.com', 1, 10, 1);"
        "INSERT INTO service_entries VALUES ('gitlab.com', 1, 12, 1);";
    char *err_msg = 0;
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_close(db);

    std::string input = "master_password\nmaster_password\ngithub.com\n\n\n\n";
    // TODO FIXME autocompletion by TAB doesn't work in the testcase for some reason
    // std::string input = "master_password\nmaster_password\ngithub\t\n\n\n\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 10);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path);
}