#include "db.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <wordexp.h>
#include <iostream>

namespace mkpass {

#include <iostream>

void ConfigDB::open_db(const std::string &db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

ConfigDB::ConfigDB() : db(nullptr) {
    wordexp_t p;
    if (wordexp("~/.mkpass.db", &p, 0) != 0) {
        // Don't throw, just ignore.
        return;
    }
    std::string db_path = p.we_wordv[0];
    wordfree(&p);
    open_db(db_path);
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

} // namespace mkpass
