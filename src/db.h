#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

#include "algorithms.h"
#include "character_classes.h"

struct sqlite3;

namespace mkpass {


struct ServiceEntry {
    std::string service_name;
    Algorithm algorithm;
    unsigned length;
    std::vector<CharacterClass> char_classes;
};


class ConfigDB {
public:
    ConfigDB();
    ConfigDB(const std::string &db_path);
    ~ConfigDB();

    std::map<std::string, int> get_all_snames();
    std::vector<std::string> get_all_service_names();
    std::optional<ServiceEntry> get_service_entry(const std::string& service_name);
    void save_service_entry(const ServiceEntry& entry);

private:
    void open_db(const std::string &db_path);
    void create_tables();
    sqlite3 *db;
};

} // namespace mkpass
