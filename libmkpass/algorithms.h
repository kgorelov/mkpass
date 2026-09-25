#pragma once

#include <string>
#include <optional>

enum class Algorithm {
    Argon2 = 1,
    SlowSha512 = 2,
    Old = 3,
    Passphrase_Diceware_EFF_Large = 4,
    Passphrase_Wordnet_Pattern = 5
};

std::string AlgorithmToIdentifier(Algorithm algo);
std::optional<Algorithm> ParseAlgorithm(const std::string& str);
