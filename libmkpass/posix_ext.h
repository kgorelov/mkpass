#pragma once

#include <cstdlib>
#include <wordexp.h>
#include <unistd.h>
#include <stdexcept>

namespace {

inline bool IsTerminal() {
    return isatty(STDIN_FILENO);
}

inline std::string GetConfigDBPath() {
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

inline std::string GetConfigFilePath() {
    if (const char* config_path_env = std::getenv("MKPASS_CONFIG_PATH")) {
        return std::string(config_path_env);
    }

    if (const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdg_config_home != '\0') {
            return std::string(xdg_config_home) + "/mkpass/mkpass.conf";
        }
    }

    wordexp_t p;
    if (wordexp("~/.config/mkpass/mkpass.conf", &p, 0) != 0) {
        throw std::runtime_error("Can't make config file path");
    }
    std::string config_path = p.we_wordv[0];
    wordfree(&p);
    return config_path;
}

inline std::string GetTmpDir() {
    char *tmpdir = getenv("TMPDIR");
    if (tmpdir) {
      return std::string(tmpdir);
    }
    return "/tmp";
}

}
