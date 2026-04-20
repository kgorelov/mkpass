#pragma once

enum class Algorithm {
    Argon2 = 1,
    SlowSha512 = 2,
    Old = 3,
    Passphrase_Diceware_EFF_Large = 4,
    Passphrase_Wordnet_Pattern = 5
};

enum class PassphraseSeparator {
    CamelCase = 1,
    KebabCase = 2
};
