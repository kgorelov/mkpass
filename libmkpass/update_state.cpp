#include "update_state.h"
#include "platform_utils.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mkpass {

namespace {

std::string EscapeJsonString(const std::string& s) {
    std::string res;
    res.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            case '\b': res += "\\b"; break;
            case '\f': res += "\\f"; break;
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    res += buf;
                } else {
                    res += c;
                }
                break;
        }
    }
    return res;
}

std::string UnescapeJsonString(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case '"': res += '"'; break;
                case '\\': res += '\\'; break;
                case '/': res += '/'; break;
                case 'b': res += '\b'; break;
                case 'f': res += '\f'; break;
                case 'n': res += '\n'; break;
                case 'r': res += '\r'; break;
                case 't': res += '\t'; break;
                default: res += s[i]; break;
            }
        } else {
            res += s[i];
        }
    }
    return res;
}

std::optional<std::string> ExtractStringField(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return std::nullopt;

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return std::nullopt;
    ++pos;

    // Skip whitespace
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        return std::nullopt;
    }
    ++pos;

    size_t start = pos;
    bool escaped = false;
    while (pos < json.size()) {
        if (escaped) {
            escaped = false;
        } else if (json[pos] == '\\') {
            escaped = true;
        } else if (json[pos] == '"') {
            break;
        }
        ++pos;
    }
    if (pos >= json.size()) return std::nullopt;

    return UnescapeJsonString(std::string_view(json.data() + start, pos - start));
}

std::optional<int64_t> ExtractIntField(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return std::nullopt;

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return std::nullopt;
    ++pos;

    // Skip whitespace
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size()) return std::nullopt;

    size_t start = pos;
    if (json[pos] == '-' || json[pos] == '+') {
        ++pos;
    }
    while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos == start || (pos == start + 1 && (json[start] == '-' || json[start] == '+'))) {
        return std::nullopt;
    }

    try {
        return std::stoll(json.substr(start, pos - start));
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace

UpdateState::UpdateState(std::string file_path)
    : path(std::move(file_path)) {
    if (path.empty()) {
        path = GetDefaultStatePath();
    }
}

std::string UpdateState::GetDefaultStatePath() {
    return GetUpdateStateFilePath();
}

bool UpdateState::load() {
    if (path.empty()) {
        path = GetDefaultStatePath();
    }

    if (!std::filesystem::exists(path)) {
        last_check_timestamp = 0;
        last_etag.clear();
        latest_known_version.clear();
        skipped_version.clear();
        return true;
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    if (auto ts = ExtractIntField(content, "last_check_timestamp")) {
        last_check_timestamp = *ts;
    }
    if (auto etag = ExtractStringField(content, "last_etag")) {
        last_etag = *etag;
    }
    if (auto ver = ExtractStringField(content, "latest_known_version")) {
        latest_known_version = *ver;
    }
    if (auto skip = ExtractStringField(content, "skipped_version")) {
        skipped_version = *skip;
    }

    return true;
}

bool UpdateState::save() {
    try {
        if (path.empty()) {
            path = GetDefaultStatePath();
        }

        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream out(path);
        if (!out.is_open()) {
            return false;
        }

        out << "{\n";
        out << "  \"last_check_timestamp\": " << last_check_timestamp << ",\n";
        out << "  \"last_etag\": \"" << EscapeJsonString(last_etag) << "\",\n";
        out << "  \"latest_known_version\": \"" << EscapeJsonString(latest_known_version) << "\",\n";
        out << "  \"skipped_version\": \"" << EscapeJsonString(skipped_version) << "\"\n";
        out << "}\n";

        return true;
    } catch (...) {
        return false;
    }
}

bool UpdateState::should_check(int interval_days, int64_t current_time_sec) const {
    if (current_time_sec <= 0) {
        current_time_sec = static_cast<int64_t>(std::time(nullptr));
    }

    if (last_check_timestamp <= 0) {
        return true;
    }

    if (interval_days <= 0) {
        return true;
    }

    int64_t elapsed = current_time_sec - last_check_timestamp;
    if (elapsed < 0) {
        // Clock skew or reset backwards
        return true;
    }

    return elapsed >= (static_cast<int64_t>(interval_days) * 86400LL);
}

} // namespace mkpass
