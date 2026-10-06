#include "semver.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace mkpass {

namespace {

std::string_view TrimWhitespace(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::vector<std::string_view> SplitByDot(std::string_view s) {
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find('.', start);
        if (end == std::string_view::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

bool IsAllDigits(std::string_view s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

bool IsValidIdentifier(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<SemVer> SemVer::Parse(std::string_view str) {
    str = TrimWhitespace(str);
    if (str.empty()) {
        return std::nullopt;
    }

    // Optional leading 'v' or 'V'
    if (str.front() == 'v' || str.front() == 'V') {
        str.remove_prefix(1);
        str = TrimWhitespace(str);
    }
    if (str.empty()) {
        return std::nullopt;
    }

    std::string_view build_part;
    size_t plus_pos = str.find('+');
    if (plus_pos != std::string_view::npos) {
        build_part = str.substr(plus_pos + 1);
        str = str.substr(0, plus_pos);

        auto build_ids = SplitByDot(build_part);
        if (build_ids.empty()) {
            return std::nullopt;
        }
        for (const auto& id : build_ids) {
            if (!IsValidIdentifier(id)) {
                return std::nullopt;
            }
        }
    }

    std::string_view prerelease_part;
    size_t dash_pos = str.find('-');
    if (dash_pos != std::string_view::npos) {
        prerelease_part = str.substr(dash_pos + 1);
        str = str.substr(0, dash_pos);

        auto prerelease_ids = SplitByDot(prerelease_part);
        if (prerelease_ids.empty()) {
            return std::nullopt;
        }
        for (const auto& id : prerelease_ids) {
            if (!IsValidIdentifier(id)) {
                return std::nullopt;
            }
            if (IsAllDigits(id) && id.size() > 1 && id.front() == '0') {
                return std::nullopt; // Leading zero not permitted in numeric identifier
            }
        }
    }

    auto core_parts = SplitByDot(str);
    if (core_parts.size() < 2 || core_parts.size() > 3) {
        return std::nullopt;
    }

    for (const auto& part : core_parts) {
        if (!IsAllDigits(part)) {
            return std::nullopt;
        }
        if (part.size() > 1 && part.front() == '0') {
            return std::nullopt; // Leading zeros not permitted in core version numbers
        }
    }

    try {
        int maj = std::stoi(std::string(core_parts[0]));
        int min = std::stoi(std::string(core_parts[1]));
        int pat = 0;
        if (core_parts.size() == 3) {
            pat = std::stoi(std::string(core_parts[2]));
        }
        if (maj < 0 || min < 0 || pat < 0) {
            return std::nullopt;
        }

        SemVer res;
        res.major = maj;
        res.minor = min;
        res.patch = pat;
        res.prerelease = std::string(prerelease_part);
        res.build = std::string(build_part);
        return res;
    } catch (...) {
        return std::nullopt;
    }
}

std::string SemVer::ToString() const {
    std::string s = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (!prerelease.empty()) {
        s += "-" + prerelease;
    }
    if (!build.empty()) {
        s += "+" + build;
    }
    return s;
}

bool SemVer::operator==(const SemVer& other) const {
    return major == other.major &&
           minor == other.minor &&
           patch == other.patch &&
           prerelease == other.prerelease;
}

bool SemVer::operator<(const SemVer& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;

    // Normal release > pre-release
    if (prerelease.empty() && !other.prerelease.empty()) {
        return false;
    }
    if (!prerelease.empty() && other.prerelease.empty()) {
        return true;
    }
    if (prerelease.empty() && other.prerelease.empty()) {
        return false;
    }

    // Both have pre-release versions; compare dot-separated identifiers
    auto parts_a = SplitByDot(prerelease);
    auto parts_b = SplitByDot(other.prerelease);

    size_t count = std::min(parts_a.size(), parts_b.size());
    for (size_t i = 0; i < count; ++i) {
        std::string_view a = parts_a[i];
        std::string_view b = parts_b[i];

        bool a_num = IsAllDigits(a);
        bool b_num = IsAllDigits(b);

        if (a_num && b_num) {
            long long a_val = std::stoll(std::string(a));
            long long b_val = std::stoll(std::string(b));
            if (a_val != b_val) {
                return a_val < b_val;
            }
        } else if (a_num != b_num) {
            // Numeric identifiers have lower precedence than alphanumeric
            return a_num;
        } else {
            // Both alphanumeric
            if (a != b) {
                return a < b;
            }
        }
    }

    return parts_a.size() < parts_b.size();
}

} // namespace mkpass
