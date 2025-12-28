#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
#include <algorithm>

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
    std::string input = "master_password\nmaster_password\n1\nservice_name\n16\n";
    ProcessOutput output = exec_with_input("./mkpass", input);
    trim(output.std_out);
    EXPECT_EQ(output.exit_code, 0);
    EXPECT_EQ(output.std_out.length(), 16);
}
