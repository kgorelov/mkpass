#include "algorithms.h"
#include <algorithm>
#include <cctype>

namespace {
std::string to_lower_trimmed(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
} // namespace

std::string AlgorithmToIdentifier(Algorithm algo) {
    switch (algo) {
        case Algorithm::Argon2:
            return "password/argon2";
        case Algorithm::SlowSha512:
            return "password/sha512";
        case Algorithm::Old:
            return "password/old";
        case Algorithm::Passphrase_Diceware_EFF_Large:
            return "passphrase/diceware";
        case Algorithm::Passphrase_Wordnet_Pattern:
            return "passphrase/wordnet";
    }
    return "password/argon2";
}

std::optional<Algorithm> ParseAlgorithm(const std::string& str) {
    std::string s = to_lower_trimmed(str);
    if (s.empty()) {
        return std::nullopt;
    }

    if (s == "password/argon2" || s == "argon2" || s == "1") {
        return Algorithm::Argon2;
    }
    if (s == "password/sha512" || s == "sha512" || s == "2") {
        return Algorithm::SlowSha512;
    }
    if (s == "password/old" || s == "old" || s == "3") {
        return Algorithm::Old;
    }
    if (s == "passphrase/diceware" || s == "diceware" || s == "4") {
        return Algorithm::Passphrase_Diceware_EFF_Large;
    }
    if (s == "passphrase/wordnet" || s == "wordnet" || s == "5") {
        return Algorithm::Passphrase_Wordnet_Pattern;
    }

    return std::nullopt;
}
