#pragma once

#include <string>
#include <cstdint>

namespace mkpass {

struct UpdateState {
    int64_t last_check_timestamp = 0;
    std::string last_etag;
    std::string latest_known_version;
    std::string skipped_version;

    std::string path;

    explicit UpdateState(std::string file_path = "");

    bool load();
    bool save();

    bool should_check(int interval_days, int64_t current_time_sec = 0) const;

    static std::string GetDefaultStatePath();
};

} // namespace mkpass
