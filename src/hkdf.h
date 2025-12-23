#pragma once

#include "hmac.h"
#include "digest.h"

template <HashCalculator C>
struct HKDF_HMAC {
    using value_type = typename C::value_type;

    value_type operator()(std::span<const uint8_t> key, std::span<const uint8_t> message) const {
        return HMAC<C>(key, message);
    }
};
