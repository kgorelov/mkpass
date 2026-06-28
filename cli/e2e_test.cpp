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
#include "platform_utils.h"

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
    std::string db_path = GetTmpDir() + "/mkpass-e2e-test.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path.c_str(), &db);
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
    remove(db_path.c_str());
}

TEST(E2ETest, DatabasePathWithUsernames) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-test.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path.c_str(), &db);
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
    remove(db_path.c_str());
}

TEST(E2ETest, AutocompleteOldSnames) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-test-3.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path.c_str(), &db);
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
    remove(db_path.c_str());
}

TEST(E2ETest, CtrlCAtServiceName) {
    std::string input = "master_password\nmaster_password\n\x03";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    EXPECT_EQ(output.exit_code, 130);
}

TEST(E2ETest, InfiniteMode) {
    // 1st iter: p1, p1, s1
    // 2nd iter: \n (pwd default), \n (service default)
    // then many \n to answer any possible questions and then EOF
    std::string input = "p1\np1\ns1\n\n\n\n\n\n\n\n\n\n";
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -i -dd";
    ProcessOutput output = exec_with_input(cmd, input);

    // We don't check exit_code because it might vary depending on how EOF is handled

    std::vector<std::string> passwords;
    std::stringstream ss(output.std_out);
    std::string line;
    while (std::getline(ss, line)) {
        trim(line);
        if (line.length() == 16) { // Argon2 default length
            passwords.push_back(line);
        }
    }
    EXPECT_GE(passwords.size(), 2);
    if (passwords.size() >= 2) {
        EXPECT_EQ(passwords[0], passwords[1]);
    }
}

TEST(E2EServiceEntriesTest, DatabaseUpdate) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-test2-db-update.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    std::string input = "master_password\nmaster_password\nnew_service.com\n1\n1234\n24\n";
    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 24);

    // Check if the database was updated
    sqlite3 *db;
    sqlite3_open(db_path.c_str(), &db);
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
    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EEnvVarsTest, AllEnvVarsSet) {
    setenv("MKPASS_PASSWORD", "test_master", 1);
    setenv("MKPASS_SERVICE", "test_service", 1);
    setenv("MKPASS_ALGORITHM", "1", 1);
    setenv("MKPASS_CHAR_CLASSES", "123", 1);
    setenv("MKPASS_LENGTH", "20", 1);

    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 20);

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_CHAR_CLASSES");
    unsetenv("MKPASS_LENGTH");
}

TEST(E2EEnvVarsTest, PassphraseDiceware) {
    setenv("MKPASS_PASSWORD", "test_master", 1);
    setenv("MKPASS_SERVICE", "test_service_diceware", 1);
    setenv("MKPASS_ALGORITHM", "4", 1);
    setenv("MKPASS_LENGTH", "4", 1);
    setenv("MKPASS_DIGITS", "y", 1);
    setenv("MKPASS_SYMBOLS", "n", 1);
    setenv("MKPASS_SUBSTITUTIONS", "y", 1);
    setenv("MKPASS_CAPITALIZE", "n", 1);
    setenv("MKPASS_SEPARATOR", "2", 1); // Hyphen

    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_FALSE(output.std_out.empty());

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_LENGTH");
    unsetenv("MKPASS_DIGITS");
    unsetenv("MKPASS_SYMBOLS");
    unsetenv("MKPASS_SUBSTITUTIONS");
    unsetenv("MKPASS_CAPITALIZE");
    unsetenv("MKPASS_SEPARATOR");
}

TEST(E2EEnvVarsTest, CustomChars) {
    setenv("MKPASS_PASSWORD", "test_master", 1);
    setenv("MKPASS_SERVICE", "test_service_custom", 1);
    setenv("MKPASS_ALGORITHM", "1", 1);
    setenv("MKPASS_CHAR_CLASSES", "5", 1);
    setenv("MKPASS_CUSTOM_CHARS", "ABC", 1);
    setenv("MKPASS_LENGTH", "10", 1);

    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 10);
    for (char c : output.std_out) {
        EXPECT_TRUE(c == 'A' || c == 'B' || c == 'C');
    }

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_CHAR_CLASSES");
    unsetenv("MKPASS_CUSTOM_CHARS");
    unsetenv("MKPASS_LENGTH");
}

TEST(E2EEnvVarsTest, PassphraseWordnetPattern) {
    setenv("MKPASS_PASSWORD", "test_master", 1);
    setenv("MKPASS_SERVICE", "test_service_wordnet", 1);
    setenv("MKPASS_ALGORITHM", "5", 1);
    setenv("MKPASS_LENGTH", "3", 1);
    setenv("MKPASS_PASSPHRASE_PATTERN", "nav", 1);
    setenv("MKPASS_DIGITS", "n", 1);
    setenv("MKPASS_SYMBOLS", "n", 1);
    setenv("MKPASS_CAPITALIZE", "y", 1);
    setenv("MKPASS_SEPARATOR", "3", 1); // Space

    ProcessOutput output = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_FALSE(output.std_out.empty());

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_LENGTH");
    unsetenv("MKPASS_PASSPHRASE_PATTERN");
    unsetenv("MKPASS_DIGITS");
    unsetenv("MKPASS_SYMBOLS");
    unsetenv("MKPASS_CAPITALIZE");
    unsetenv("MKPASS_SEPARATOR");
}

TEST(E2ECommandLineOptionsTest, AllOptionsSet) {
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p test_master -s test_service_cmd -a 1 -c 123 -l 25";

    ProcessOutput output = exec_with_input(cmd, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 25);
}

TEST(E2ECommandLineOptionsTest, QrCodeOption) {
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p test_master -s test_service_cmd -a 1 -c 123 -l 25 -q";

    ProcessOutput output = exec_with_input(cmd, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    // QR code should be much longer than 25 characters
    EXPECT_GT(output.std_out.length(), 100);
    // Should contain some block characters
    EXPECT_TRUE(output.std_out.find("\u2588") != std::string::npos ||
                output.std_out.find("\u2580") != std::string::npos ||
                output.std_out.find("\u2584") != std::string::npos);
}

TEST(E2ECommandLineOptionsTest, MixedEnvAndCmd) {
    setenv("MKPASS_PASSWORD", "test_master", 1);
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -s test_service_mixed -a 1 -c 123 -l 15";

    ProcessOutput output = exec_with_input(cmd, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 15);

    unsetenv("MKPASS_PASSWORD");
}

TEST(E2EDefaultsTest, KnownServiceWithD) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-defaults.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Create entry
    std::string input1 = "master\nmaster\nservice1\n1\n123\n10\n";
    exec_with_input(MKPASS_EXECUTABLE_PATH, input1);

    // 2. Run with -d, should only ask for password and service (if not provided)
    // We provide password and service via CMD to see if it finishes without input
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s service1 -d";
    ProcessOutput output = exec_with_input(cmd, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 10);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EDefaultsTest, NewServiceWithDShouldAsk) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-defaults-new.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // Run with -d for a NEW service. It should still ask for parameters.
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s new_service -d";
    // We provide input for Algorithm(1), CharClasses(1234), Length(15)
    std::string input = "1\n1234\n15\n";
    ProcessOutput output = exec_with_input(cmd, input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 15);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EDefaultsTest, NewServiceWithBigD) {
    // Run with -dd for a NEW service. It should NOT ask for parameters, using program defaults.
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s new_service -dd";
    ProcessOutput output = exec_with_input(cmd, "");
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 16); // Default length for Argon2 is 16
}

TEST(E2EDeletionTest, SimpleDelete) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-delete.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Create entry
    std::string input1 = "master\nmaster\nservice_to_delete\n1\n123\n10\n";
    exec_with_input(MKPASS_EXECUTABLE_PATH, input1);

    // 2. Delete entry
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -D -s service_to_delete";
    std::string input2 = "y\n";
    ProcessOutput output = exec_with_input(cmd, input2);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_TRUE(output.std_err.find("Deleted") != std::string::npos);

    // 3. Verify it's gone - run again without -D, should ask for everything
    std::string input3 = "master\nmaster\nservice_to_delete\n1\n123\n12\n";
    ProcessOutput output2 = exec_with_input(MKPASS_EXECUTABLE_PATH, input3);
    trim(output2.std_out);
    EXPECT_EQ(output2.std_out.length(), 12);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EServiceEntriesTest, AutocompleteNewServiceEntries) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e2-autocomplete.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // Create a dummy database for testing
    sqlite3 *db;
    sqlite3_open(db_path.c_str(), &db);
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
    remove(db_path.c_str());
}

TEST(E2ECommandLineOptionsTest, EmptyPasswordError) {
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p \"\" -s test_service -a 1 -c 123 -l 25";

    ProcessOutput output = exec_with_input(cmd, "");
    EXPECT_EQ(output.exit_code, 1);
    EXPECT_TRUE(output.std_err.find("Master password must not be empty") != std::string::npos);
}

TEST(E2ECommandLineOptionsTest, EmptyServiceError) {
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p test_master -s \"\" -a 1 -c 123 -l 25";

    ProcessOutput output = exec_with_input(cmd, "");
    EXPECT_EQ(output.exit_code, 1);
    EXPECT_TRUE(output.std_err.find("Service must not be empty") != std::string::npos);
}
