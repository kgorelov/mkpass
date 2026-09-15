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

TEST(E2EInfoTest, InfoOptionWithService) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-info.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Create entry
    std::string input1 = "master\nmaster\ninfo_service\n1\n1234\n18\n";
    exec_with_input(MKPASS_EXECUTABLE_PATH, input1);

    // 2. Query info with -I -s
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I -s info_service";
    ProcessOutput output = exec_with_input(cmd, "");
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_TRUE(output.std_out.find("Service name: info_service") != std::string::npos);
    EXPECT_TRUE(output.std_out.find("Algorithm: Password (Argon2)") != std::string::npos);
    EXPECT_TRUE(output.std_out.find("Password length: 18") != std::string::npos);
    EXPECT_TRUE(output.std_out.find("Character classes: Lowercase Letters, Uppercase Letters, Digits, Symbols") != std::string::npos);

    // 3. Query info with --info -s
    std::string cmd2 = MKPASS_EXECUTABLE_PATH;
    cmd2 += " --info -s info_service";
    ProcessOutput output2 = exec_with_input(cmd2, "");
    EXPECT_EQ(output2.exit_code, 0);
    EXPECT_TRUE(output2.std_out.find("Service name: info_service") != std::string::npos);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EInfoTest, InfoInteractivePrompt) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-info-interactive.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Create entry
    std::string input1 = "master\nmaster\nprompt_service\n1\n123\n14\n";
    exec_with_input(MKPASS_EXECUTABLE_PATH, input1);

    // 2. Run -I interactively
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I";
    std::string input2 = "prompt_service\n";
    ProcessOutput output = exec_with_input(cmd, input2);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_TRUE(output.std_out.find("Service name: prompt_service") != std::string::npos);
    EXPECT_TRUE(output.std_out.find("Password length: 14") != std::string::npos);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EInfoTest, InfoNotFound) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-info-notfound.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I -s nonexistent_service";
    ProcessOutput output = exec_with_input(cmd, "");
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_TRUE(output.std_err.find("Service 'nonexistent_service' not found.") != std::string::npos);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EInfoTest, InfoPassphraseDicewareAndWordnet) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-info-passphrase.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Create Diceware entry
    setenv("MKPASS_PASSWORD", "test_master", 1);
    setenv("MKPASS_SERVICE", "dice_service", 1);
    setenv("MKPASS_ALGORITHM", "4", 1);
    setenv("MKPASS_LENGTH", "4", 1);
    setenv("MKPASS_DIGITS", "y", 1);
    setenv("MKPASS_SYMBOLS", "n", 1);
    setenv("MKPASS_SUBSTITUTIONS", "y", 1);
    setenv("MKPASS_CAPITALIZE", "n", 1);
    setenv("MKPASS_SEPARATOR", "2", 1); // Hyphen
    exec_with_input(MKPASS_EXECUTABLE_PATH, "");

    // 2. Create Wordnet entry
    setenv("MKPASS_SERVICE", "wordnet_service", 1);
    setenv("MKPASS_ALGORITHM", "5", 1);
    setenv("MKPASS_LENGTH", "3", 1);
    setenv("MKPASS_PASSPHRASE_PATTERN", "nav", 1);
    setenv("MKPASS_DIGITS", "n", 1);
    setenv("MKPASS_SYMBOLS", "n", 1);
    setenv("MKPASS_CAPITALIZE", "y", 1);
    setenv("MKPASS_SEPARATOR", "3", 1); // Space
    exec_with_input(MKPASS_EXECUTABLE_PATH, "");

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_LENGTH");
    unsetenv("MKPASS_PASSPHRASE_PATTERN");
    unsetenv("MKPASS_DIGITS");
    unsetenv("MKPASS_SYMBOLS");
    unsetenv("MKPASS_SUBSTITUTIONS");
    unsetenv("MKPASS_CAPITALIZE");
    unsetenv("MKPASS_SEPARATOR");

    // 3. Test Diceware info
    std::string cmd1 = MKPASS_EXECUTABLE_PATH;
    cmd1 += " -I -s dice_service";
    ProcessOutput out1 = exec_with_input(cmd1, "");
    EXPECT_EQ(out1.exit_code, 0);
    EXPECT_TRUE(out1.std_out.find("Algorithm: Passphrase Diceware (Argon2)") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Words count: 4") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Separator: Hyphen (-)") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Capitalize words: No") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Include digits: Yes") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Include symbols: No") != std::string::npos);
    EXPECT_TRUE(out1.std_out.find("Allow substitutions: Yes") != std::string::npos);

    // 4. Test Wordnet info
    std::string cmd2 = MKPASS_EXECUTABLE_PATH;
    cmd2 += " -I -s wordnet_service";
    ProcessOutput out2 = exec_with_input(cmd2, "");
    EXPECT_EQ(out2.exit_code, 0);
    EXPECT_TRUE(out2.std_out.find("Algorithm: Passphrase Wordnet Pattern (Argon2)") != std::string::npos);
    EXPECT_TRUE(out2.std_out.find("Words count: 3") != std::string::npos);
    EXPECT_TRUE(out2.std_out.find("Passphrase pattern: nav") != std::string::npos);
    EXPECT_TRUE(out2.std_out.find("Separator: Space ( )") != std::string::npos);
    EXPECT_TRUE(out2.std_out.find("Capitalize words: Yes") != std::string::npos);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EConfigSubcommandTest, SetAndGet) {
    std::string config_path = GetTmpDir() + "/mkpass-e2e-config-setget.conf";
    setenv("MKPASS_CONFIG_PATH", config_path.c_str(), 1);
    remove(config_path.c_str());

    // 1. Set algorithm
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set algorithm password/argon2";
    ProcessOutput out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    // 2. Get algorithm
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get algorithm";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "password/argon2");

    // 3. Set char_classes
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set char_classes lowercase,uppercase,digits,symbols";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get char_classes";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "lowercase,uppercase,digits,symbols");

    // 4. Set length
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set length 24";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get length";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "24");

    // 5. Set separator
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set separator -";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get separator";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "-");

    // 6. Set digits
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set digits true";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get digits";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "true");

    // 7. Set enable_old_algorithm
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set enable_old_algorithm false";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get enable_old_algorithm";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    trim(out.std_out);
    EXPECT_EQ(out.std_out, "false");

    unsetenv("MKPASS_CONFIG_PATH");
    remove(config_path.c_str());
}

TEST(E2EConfigSubcommandTest, UnsetAndPrint) {
    std::string config_path = GetTmpDir() + "/mkpass-e2e-config-unset.conf";
    setenv("MKPASS_CONFIG_PATH", config_path.c_str(), 1);
    remove(config_path.c_str());

    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set algorithm password/argon2";
    ProcessOutput out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config print";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_TRUE(out.std_out.find("algorithm = 'password/argon2'") != std::string::npos ||
                out.std_out.find("algorithm = \"password/argon2\"") != std::string::npos);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config unset algorithm";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get algorithm";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);
    EXPECT_TRUE(out.std_err.find("is not set") != std::string::npos);

    unsetenv("MKPASS_CONFIG_PATH");
    remove(config_path.c_str());
}

TEST(E2EConfigSubcommandTest, ErrorHandling) {
    std::string config_path = GetTmpDir() + "/mkpass-e2e-config-err.conf";
    setenv("MKPASS_CONFIG_PATH", config_path.c_str(), 1);
    remove(config_path.c_str());

    // 1. Invalid key in get
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config get invalid_key";
    ProcessOutput out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    // 2. Invalid key in set
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set invalid_key value";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    // 3. Invalid algorithm value
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set algorithm invalid_algorithm";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    // 4. Invalid length value
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set length abc";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    // 5. Invalid char_classes value
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set char_classes invalid_class";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    // 6. Invalid boolean value
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set digits not_a_bool";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);

    unsetenv("MKPASS_CONFIG_PATH");
    remove(config_path.c_str());
}

TEST(E2EConfigDefaultsTest, GeneratorUsesConfigDefaults) {
    std::string config_path = GetTmpDir() + "/mkpass-e2e-cfg-gen.conf";
    std::string db_path = GetTmpDir() + "/mkpass-e2e-cfg-gen.db";
    setenv("MKPASS_CONFIG_PATH", config_path.c_str(), 1);
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(config_path.c_str());
    remove(db_path.c_str());

    // Set defaults in config
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set length 22";
    exec_with_input(cmd, "");

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set char_classes digits";
    exec_with_input(cmd, "");

    // Generate with -dd for a new service
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_config_service -dd";
    ProcessOutput out = exec_with_input(cmd, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 22);
    for (char c : out.std_out) {
        EXPECT_TRUE(std::isdigit(c));
    }

    // CLI option overrides config default length
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_config_service2 -l 10 -dd";
    out = exec_with_input(cmd, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 10);

    unsetenv("MKPASS_CONFIG_PATH");
    unsetenv("MKPASS_DB_PATH");
    remove(config_path.c_str());
    remove(db_path.c_str());
}

TEST(E2EHumanReadableStringsTest, CommandLineOptions) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-cmd-strings.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. --algorithm password/sha512
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_sha512 --algorithm password/sha512 -dd";
    ProcessOutput out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    // Verify in db info
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I -s test_sha512";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_TRUE(out.std_out.find("Password (SHA512 HMAC)") != std::string::npos);

    // 2. --char-classes digits,symbols
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_chars --char-classes digits,symbols -l 20 -dd";
    out = exec_with_input(cmd, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 20);
    for (char c : out.std_out) {
        EXPECT_TRUE(std::isdigit(c) || Symbols.find(c) != std::string::npos);
    }

    // 3. Passphrase Diceware with string algo
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_dice --algorithm passphrase/diceware -l 4 --separator - --digits y --symbols n --capitalize y -dd";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I -s test_dice";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_TRUE(out.std_out.find("Algorithm: Passphrase Diceware (Argon2)") != std::string::npos);
    EXPECT_TRUE(out.std_out.find("Words count: 4") != std::string::npos);

    // 4. Passphrase Wordnet with string algo
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s test_wordnet --algorithm passphrase/wordnet -l 3 --pattern nav --separator / -dd";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -I -s test_wordnet";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_TRUE(out.std_out.find("Algorithm: Passphrase Wordnet Pattern (Argon2)") != std::string::npos);
    EXPECT_TRUE(out.std_out.find("Words count: 3") != std::string::npos);

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EHumanReadableStringsTest, EnvironmentVariables) {
    std::string db_path = GetTmpDir() + "/mkpass-e2e-env-strings.db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    // 1. Test MKPASS_ALGORITHM="password/argon2" and MKPASS_CHAR_CLASSES="lowercase,digits"
    setenv("MKPASS_PASSWORD", "master", 1);
    setenv("MKPASS_SERVICE", "test_env_pwd", 1);
    setenv("MKPASS_ALGORITHM", "password/argon2", 1);
    setenv("MKPASS_CHAR_CLASSES", "lowercase,digits", 1);
    setenv("MKPASS_LENGTH", "16", 1);

    ProcessOutput out = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 16);
    for (char c : out.std_out) {
        EXPECT_TRUE(std::islower(c) || std::isdigit(c));
    }

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_CHAR_CLASSES");
    unsetenv("MKPASS_LENGTH");

    // 2. Test MKPASS_ALGORITHM="passphrase/diceware"
    setenv("MKPASS_PASSWORD", "master", 1);
    setenv("MKPASS_SERVICE", "test_env_dice", 1);
    setenv("MKPASS_ALGORITHM", "passphrase/diceware", 1);
    setenv("MKPASS_LENGTH", "4", 1);
    setenv("MKPASS_DIGITS", "y", 1);
    setenv("MKPASS_SYMBOLS", "n", 1);
    setenv("MKPASS_SUBSTITUTIONS", "y", 1);
    setenv("MKPASS_CAPITALIZE", "n", 1);
    setenv("MKPASS_SEPARATOR", "-", 1);

    out = exec_with_input(MKPASS_EXECUTABLE_PATH, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_FALSE(out.std_out.empty());

    unsetenv("MKPASS_PASSWORD");
    unsetenv("MKPASS_SERVICE");
    unsetenv("MKPASS_ALGORITHM");
    unsetenv("MKPASS_LENGTH");
    unsetenv("MKPASS_DIGITS");
    unsetenv("MKPASS_SYMBOLS");
    unsetenv("MKPASS_SUBSTITUTIONS");
    unsetenv("MKPASS_CAPITALIZE");
    unsetenv("MKPASS_SEPARATOR");

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST(E2EOldAlgorithmGatingTest, GatingAndEnforcement) {
    std::string config_path = GetTmpDir() + "/mkpass-e2e-oldgating.conf";
    std::string db_path = GetTmpDir() + "/mkpass-e2e-oldgating.db";
    setenv("MKPASS_CONFIG_PATH", config_path.c_str(), 1);
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(config_path.c_str());
    remove(db_path.c_str());

    // 1. Run interactive prompt for a new service without enable_old_algorithm -> verify OldPassword is omitted
    std::string cmd = MKPASS_EXECUTABLE_PATH;
    std::string input = "master\nmaster\ninteractive_new_service\n1\n1234\n16\n";
    ProcessOutput out = exec_with_input(cmd, input);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_err.find("OldPassword"), std::string::npos);

    // 2. Attempt to create a new service with -a password/old when disabled -> verify failure with descriptive error
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s illegal_service -a password/old -dd";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);
    EXPECT_TRUE(out.std_err.find("Legacy algorithm 'password/old' is disabled. Set 'enable_old_algorithm = true' in config or environment to enable.") != std::string::npos);

    // Also with legacy numeric ID -a 3
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s illegal_service -a 3 -dd";
    out = exec_with_input(cmd, "");
    EXPECT_NE(out.exit_code, 0);
    EXPECT_TRUE(out.std_err.find("Legacy algorithm 'password/old' is disabled. Set 'enable_old_algorithm = true' in config or environment to enable.") != std::string::npos);

    // 3. Set enable_old_algorithm = true in config
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set enable_old_algorithm true";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    // Creating service with -a password/old now succeeds
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s allowed_old_service -a password/old -dd";
    out = exec_with_input(cmd, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 8);

    // 4. Disable enable_old_algorithm again in config
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " config set enable_old_algorithm false";
    out = exec_with_input(cmd, "");
    EXPECT_EQ(out.exit_code, 0);

    // Generating password for the existing service using Old algorithm in DB still works
    cmd = MKPASS_EXECUTABLE_PATH;
    cmd += " -p master -s allowed_old_service -dd";
    out = exec_with_input(cmd, "");
    trim(out.std_out);
    EXPECT_EQ(out.exit_code, 0);
    EXPECT_EQ(out.std_out.length(), 8);

    unsetenv("MKPASS_CONFIG_PATH");
    unsetenv("MKPASS_DB_PATH");
    remove(config_path.c_str());
    remove(db_path.c_str());
}
