#pragma once

#include <string>
#include <vector>
#include <map>

struct sqlite3;

namespace mkpass {

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
