#include "db.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>
#else
#include <wordexp.h>
#endif

#include <cstdlib>

namespace mkpass {

namespace {

int CharClassesToBitmask(const std::vector<CharacterClass>& char_classes) {
    int mask = 0;
    for (const auto& cc : char_classes) {
        mask |= (1 << static_cast<int>(cc));
    }
    return mask;
}

std::vector<CharacterClass> BitmaskToCharClasses(int mask) {
    std::vector<CharacterClass> char_classes;
    if (mask & (1 << static_cast<int>(CharacterClass::LOWERCASE))) {
        char_classes.push_back(CharacterClass::LOWERCASE);
    }
    if (mask & (1 << static_cast<int>(CharacterClass::UPPERCASE))) {
        char_classes.push_back(CharacterClass::UPPERCASE);
    }
    if (mask & (1 << static_cast<int>(CharacterClass::DIGITS))) {
        char_classes.push_back(CharacterClass::DIGITS);
    }
    if (mask & (1 << static_cast<int>(CharacterClass::SYMBOLS))) {
        char_classes.push_back(CharacterClass::SYMBOLS);
    }
    return char_classes;
}

} // namespace

void ConfigDB::open_db(const std::string &db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

ConfigDB::ConfigDB() : db(nullptr) {
    if (const char* db_path_env = std::getenv("MKPASS_DB_PATH")) {
        open_db(db_path_env);
    } else {
#ifdef _WIN32
        PWSTR path = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path))) {
            std::wstring wpath(path);
            CoTaskMemFree(path);
            std::string db_path(wpath.begin(), wpath.end());
            db_path += "\\.mkpass.db";
            open_db(db_path);
        }
#else
        wordexp_t p;
        if (wordexp("~/.mkpass.db", &p, 0) == 0) {
            std::string db_path = p.we_wordv[0];
            wordfree(&p);
            open_db(db_path);
        }
#endif
    }

    if (!db) {
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS service_entries (name TEXT PRIMARY KEY, algorithm INTEGER, length INTEGER, char_classes INTEGER);";
    char *err_msg = nullptr;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}


ConfigDB::ConfigDB(const std::string &db_path) : db(nullptr) {
    open_db(db_path);
}

ConfigDB::~ConfigDB() {
    if (db) {
        sqlite3_close(db);
    }
}

std::map<std::string, int> ConfigDB::get_all_snames() {
    std::map<std::string, int> snames;
    if (!db) {
        return snames;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT name, length FROM snames";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        // Table probably doesn't exist, just return empty map.
        return snames;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        int length = sqlite3_column_int(stmt, 1);
        snames[reinterpret_cast<const char*>(name)] = length;
    }

    sqlite3_finalize(stmt);
    return snames;
}

std::optional<ServiceEntry> ConfigDB::get_service_entry(const std::string& service_name) {
    if (!db) {
        return std::nullopt;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT algorithm, length, char_classes FROM service_entries WHERE name = ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, service_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ServiceEntry entry;
        entry.service_name = service_name;
        entry.algorithm = static_cast<Algorithm>(sqlite3_column_int(stmt, 0));
        entry.length = sqlite3_column_int(stmt, 1);
        entry.char_classes = BitmaskToCharClasses(sqlite3_column_int(stmt, 2));
        sqlite3_finalize(stmt);
        return entry;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

void ConfigDB::save_service_entry(const ServiceEntry& entry) {
    if (!db) {
        return;
    }

    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR REPLACE INTO service_entries (name, algorithm, length, char_classes) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    sqlite3_bind_text(stmt, 1, entry.service_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(entry.algorithm));
    sqlite3_bind_int(stmt, 3, entry.length);
    sqlite3_bind_int(stmt, 4, CharClassesToBitmask(entry.char_classes));

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<std::string> ConfigDB::get_all_service_names() {
    std::vector<std::string> names;
    if (!db) {
        return names;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT name FROM service_entries";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return names;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        names.push_back(reinterpret_cast<const char*>(name));
    }

    sqlite3_finalize(stmt);
    return names;
}

} // namespace mkpass