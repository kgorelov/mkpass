#pragma once

#include <cstdlib>
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>

namespace {

std::string GetConfigDBPath() {
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

}
