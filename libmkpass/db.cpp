#include "db.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

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
    if (mask & (1 << static_cast<int>(CharacterClass::CUSTOM))) {
        char_classes.push_back(CharacterClass::CUSTOM);
    }
    return char_classes;
}

std::string PatternToString(const std::vector<WordClasses>& pattern) {
    std::string s;
    for (auto wc : pattern) {
        switch (wc) {
        case WordClasses::Noun: s += 'n'; break;
        case WordClasses::Verb: s += 'v'; break;
        case WordClasses::Adj:  s += 'a'; break;
        case WordClasses::Adv:  s += 'r'; break;
        }
    }
    return s;
}

std::vector<WordClasses> StringToPattern(const std::string& s) {
    std::vector<WordClasses> pattern;
    for (char c : s) {
        switch (c) {
        case 'n': pattern.push_back(WordClasses::Noun); break;
        case 'v': pattern.push_back(WordClasses::Verb); break;
        case 'a': pattern.push_back(WordClasses::Adj);  break;
        case 'r': pattern.push_back(WordClasses::Adv);  break;
        }
    }
    return pattern;
}

bool column_exists(sqlite3 *db, const std::string& table_name, const std::string& column_name) {
    sqlite3_stmt *stmt;
    std::string sql = "PRAGMA table_info(" + table_name + ")";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (column_name == reinterpret_cast<const char*>(name)) {
            sqlite3_finalize(stmt);
            return true;
        }
    }
    sqlite3_finalize(stmt);
    return false;
}

} // namespace

void ConfigDB::open_db(const std::string &db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

void ConfigDB::create_tables() {
    if (!db) {
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS service_entries (name TEXT PRIMARY KEY, algorithm INTEGER, length INTEGER, char_classes INTEGER);";
    char *err_msg = nullptr;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    if (!column_exists(db, "service_entries", "custom_chars")) {
        const char *alter_sql = "ALTER TABLE service_entries ADD COLUMN custom_chars TEXT;";
        if (sqlite3_exec(db, alter_sql, 0, 0, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to alter table: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }

    if (!column_exists(db, "service_entries", "separator")) {
        const char *alter_sql = "ALTER TABLE service_entries ADD COLUMN separator TEXT DEFAULT '';";
        if (sqlite3_exec(db, alter_sql, 0, 0, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to alter table: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }

    if (!column_exists(db, "service_entries", "passphrase_pattern")) {
        const char *alter_sql = "ALTER TABLE service_entries ADD COLUMN passphrase_pattern TEXT;";
        if (sqlite3_exec(db, alter_sql, 0, 0, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to alter table: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }

    if (!column_exists(db, "service_entries", "allow_substitutions")) {
        const char *alter_sql = "ALTER TABLE service_entries ADD COLUMN allow_substitutions INTEGER DEFAULT 0;";
        if (sqlite3_exec(db, alter_sql, 0, 0, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to alter table: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }

    if (!column_exists(db, "service_entries", "capitalize_words")) {
        const char *alter_sql = "ALTER TABLE service_entries ADD COLUMN capitalize_words INTEGER DEFAULT 0;";
        if (sqlite3_exec(db, alter_sql, 0, 0, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to alter table: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }
}

ConfigDB::ConfigDB(const std::string &db_path) : db(nullptr) {
    open_db(db_path);
    create_tables();
}

ConfigDB::~ConfigDB() {
    if (db) {
        sqlite3_close(db);
    }
}

std::set<std::string> ConfigDB::get_service_names(const std::string& table_name) {
    std::set<std::string> names;

    if (!db) {
        return names;
    }

    sqlite3_stmt *stmt;
    std::string sql = "SELECT name FROM " + table_name;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        // Table probably doesn't exist, just return empty set.
        return names;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        names.insert(reinterpret_cast<const char*>(name));
    }

    sqlite3_finalize(stmt);
    return names;
}

std::optional<ServiceEntry> ConfigDB::get_old_service_entry(const std::string& service_name) {
    if (!db) {
        return std::nullopt;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT name, length FROM snames WHERE name = ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, service_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ServiceEntry entry;
        entry.service_name = service_name;
        entry.algorithm = Algorithm::Old;
        entry.length = sqlite3_column_int(stmt, 1);
        entry.char_classes = {};
        entry.separator = "-";
        entry.allow_substitutions = false;
        entry.capitalize_words = false;
        sqlite3_finalize(stmt);
        return entry;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<ServiceEntry> ConfigDB::get_new_service_entry(const std::string& service_name) {
    if (!db) {
        return std::nullopt;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT algorithm, length, char_classes, custom_chars, separator, passphrase_pattern, allow_substitutions, capitalize_words FROM service_entries WHERE name = ?";
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
        const unsigned char *custom_chars = sqlite3_column_text(stmt, 3);
        if (custom_chars) {
            entry.custom_chars = reinterpret_cast<const char*>(custom_chars);
        }

        int sep_type = sqlite3_column_type(stmt, 4);
        if (sep_type == SQLITE_INTEGER) {
            int sep_val = sqlite3_column_int(stmt, 4);
            if (sep_val == 1) { // CamelCase
                entry.separator = "";
                entry.capitalize_words = true;
            } else { // KebabCase
                entry.separator = "-";
                entry.capitalize_words = false;
            }
        } else {
            const unsigned char *sep_str = sqlite3_column_text(stmt, 4);
            if (sep_str) {
                entry.separator = reinterpret_cast<const char*>(sep_str);
            }
            entry.capitalize_words = sqlite3_column_int(stmt, 7) != 0;
        }

        const unsigned char *pattern = sqlite3_column_text(stmt, 5);
        if (pattern) {
            entry.passphrase_pattern = StringToPattern(reinterpret_cast<const char*>(pattern));
        }
        entry.allow_substitutions = sqlite3_column_int(stmt, 6) != 0;
        sqlite3_finalize(stmt);
        return entry;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<ServiceEntry> ConfigDB::get_service_entry(const std::string& service_name) {
    auto entry = get_new_service_entry(service_name);
    if (!entry) {
        return get_old_service_entry(service_name);
    }
    return entry;
}

void ConfigDB::save_service_entry(const ServiceEntry& entry) {
    if (!db) {
        return;
    }

    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR REPLACE INTO service_entries (name, algorithm, length, char_classes, custom_chars, separator, passphrase_pattern, allow_substitutions, capitalize_words) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    sqlite3_bind_text(stmt, 1, entry.service_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(entry.algorithm));
    sqlite3_bind_int(stmt, 3, entry.length);
    sqlite3_bind_int(stmt, 4, CharClassesToBitmask(entry.char_classes));
    if (entry.custom_chars) {
        sqlite3_bind_text(stmt, 5, entry.custom_chars->c_str(), -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    sqlite3_bind_text(stmt, 6, entry.separator.c_str(), -1, SQLITE_STATIC);
    if (!entry.passphrase_pattern.empty()) {
        sqlite3_bind_text(stmt, 7, PatternToString(entry.passphrase_pattern).c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 7);
    }
    sqlite3_bind_int(stmt, 8, entry.allow_substitutions ? 1 : 0);
    sqlite3_bind_int(stmt, 9, entry.capitalize_words ? 1 : 0);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::set<std::string> ConfigDB::get_all_service_names() {
    std::set<std::string> names = get_service_names("snames");
    names.merge(get_service_names("service_entries"));
    return names;
}

} // namespace mkpass
