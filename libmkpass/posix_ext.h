#pragma once

#include <cstdlib>
#include <wordexp.h>

#include <unistd.h>

namespace {

inline bool IsTerminal() {
    return isatty(STDIN_FILENO);
}

std::string GetConfigDBPath() {
    if (const char* db_path_env = std::getenv("MKPASS_DB_PATH")) {
        return std::string(db_path_env);
    }

    wordexp_t p;
    if (wordexp("~/.mkpass.db", &p, 0) != 0) {
        throw std::runtime_error("Can't make DB path");
    }
    std::string db_path = p.we_wordv[0];
    wordfree(&p);
    return db_path;
}

std::string GetTmpDir() {
    char *tmpdir = getenv("TMPDIR");
    if (tmpdir) {
      return std::string(tmpdir);
    }
    return "/tmp";
}

}
