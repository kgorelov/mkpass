#pragma once

#include <cstdlib>
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>
#include <io.h>
#include <stdexcept>

namespace {

inline bool IsTerminal() {
    return _isatty(_fileno(stdin)) != 0;
}

inline std::string GetConfigDBPath() {
    if (const char* db_path_env = std::getenv("MKPASS_DB_PATH")) {
        return std::string(db_path_env);
    }

    PWSTR path = NULL;
    if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path))) {
        throw std::runtime_error("Can't make DB path");
    }
    std::wstring wpath(path);
    CoTaskMemFree(path);
    std::string db_path(wpath.begin(), wpath.end());
    db_path += "\\.mkpass.db";
    return db_path;
}

inline std::string GetConfigFilePath() {
    if (const char* config_path_env = std::getenv("MKPASS_CONFIG_PATH")) {
        return std::string(config_path_env);
    }

    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path))) {
        std::wstring wpath(path);
        CoTaskMemFree(path);
        std::string config_path(wpath.begin(), wpath.end());
        config_path += "\\mkpass\\mkpass.conf";
        return config_path;
    }

    if (const char* appdata = std::getenv("APPDATA")) {
        return std::string(appdata) + "\\mkpass\\mkpass.conf";
    }

    throw std::runtime_error("Can't make config file path");
}

inline std::string GetTmpDir() {
    char *tmpdir = getenv("TEMP");
    if (!tmpdir) {
        tmpdir = getenv("TMP");
    }
    if (tmpdir) {
        return std::string(tmpdir);
    }
    return "C:\\Windows\\Temp";
}

}
