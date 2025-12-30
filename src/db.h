#pragma once

#include <string>
#include <vector>
#include <map>

struct sqlite3;

namespace mkpass {


struct ServiceEntry {
    std::string service_name;
    unsigned algorithm_id;
    unsigned length;
};


class ConfigDB {
public:
    ConfigDB();
    ConfigDB(const std::string &db_path);
    ~ConfigDB();

    std::map<std::string, int> get_all_snames();

private:
    void open_db(const std::string &db_path);
    sqlite3 *db;
};

} // namespace mkpass
