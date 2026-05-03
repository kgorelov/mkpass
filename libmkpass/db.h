#pragma once

#include <string>
#include <vector>
#include <set>
#include <optional>

#include "algorithms.h"
#include "character_classes.h"
#include "word_classes.h"

struct sqlite3;

namespace mkpass {


struct ServiceEntry {
    std::string service_name;
    Algorithm algorithm;
    unsigned length;
    std::vector<CharacterClass> char_classes;
    std::optional<std::string> custom_chars;
    std::string separator;
    std::vector<WordClasses> passphrase_pattern;
    bool allow_substitutions;
    bool capitalize_words;
};


class ConfigDB {
public:
    ConfigDB(const std::string &db_path);
    ~ConfigDB();

    std::set<std::string> get_all_service_names();
    std::optional<ServiceEntry> get_service_entry(const std::string& service_name);
    void save_service_entry(const ServiceEntry& entry);

private:
    std::set<std::string> get_service_names(const std::string& table_name);
    std::optional<ServiceEntry> get_new_service_entry(const std::string& service_name);
    std::optional<ServiceEntry> get_old_service_entry(const std::string& service_name);

    void open_db(const std::string &db_path);
    void create_tables();
    sqlite3 *db;
};

} // namespace mkpass
