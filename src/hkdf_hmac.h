#pragma once

#include "hmac.h"
#include "digest.h"
#include <span>
#include <cstdint>

HASH_CALCULATOR_TEMPLATE
struct HKDF_HMAC {
    using value_type = typename C::value_type;

    value_type operator()(std::span<const uint8_t> key, std::span<const uint8_t> message) const {
        return HMAC<C>(key, message);
    }
};
