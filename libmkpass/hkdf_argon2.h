#pragma once

#include <vector>
#include <span>
#include <cstdint>

struct HKDF_Argon2 {
    using value_type = std::vector<uint8_t>;

    value_type operator()(std::span<const uint8_t> key, std::span<const uint8_t> info) const;
};
