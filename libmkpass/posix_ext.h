#pragma once

#include <cstdlib>
#ifndef __ANDROID__
#include <wordexp.h>
#endif
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

#ifdef __ANDROID__
    return "/data/data/app.mkpass/databases/mkpass.db";
#else
    wordexp_t p;
    if (wordexp("~/.mkpass.db", &p, 0) != 0) {
        throw std::runtime_error("Can't make DB path");
    }
    std::string db_path = p.we_wordv[0];
    wordfree(&p);
    return db_path;
#endif
}

inline std::string GetConfigFilePath() {
    if (const char* config_path_env = std::getenv("MKPASS_CONFIG_PATH")) {
        return std::string(config_path_env);
    }

#ifdef __ANDROID__
    return "/data/data/app.mkpass/files/mkpass.conf";
#else
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
#endif
}

inline std::string GetTmpDir() {
    char *tmpdir = getenv("TMPDIR");
    if (tmpdir) {
      return std::string(tmpdir);
    }
#ifdef __ANDROID__
    return "/data/data/app.mkpass/cache";
#else
    return "/tmp";
#endif
}

}
