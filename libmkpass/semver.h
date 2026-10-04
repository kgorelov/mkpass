#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace mkpass {

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::string prerelease;
    std::string build;

    static std::optional<SemVer> Parse(std::string_view str);

    std::string ToString() const;
    bool IsPrerelease() const { return !prerelease.empty(); }

    bool operator==(const SemVer& other) const;
    bool operator!=(const SemVer& other) const { return !(*this == other); }
    bool operator<(const SemVer& other) const;
    bool operator>(const SemVer& other) const { return other < *this; }
    bool operator<=(const SemVer& other) const { return !(other < *this); }
    bool operator>=(const SemVer& other) const { return !(*this < other); }
};

} // namespace mkpass
