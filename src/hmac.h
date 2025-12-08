#pragma once
#include <vector>
#include "digest.h"

// HMAC(key, msg) = H((key ⊕ opad) || H((key ⊕ ipad) || msg))
template <HashCalculator C>
C::value_type HMAC(
    std::span<const uint8_t> key,
    std::span<const uint8_t> message) {
    constexpr auto block_size = C::block_size();

    std::vector<uint8_t> k(key.begin(), key.end());

    // Step 1: Key preprocessing
    if (k.size() > block_size) {
        auto d = CalcHash<C>(k);
        k.assign(d.begin(), d.end());
    }
    if (k.size() < block_size) {
        k.resize(block_size, 0x00);
    }

    // Step 2: XOR with ipad and opad
    std::vector<uint8_t> ipad(block_size), opad(block_size);
    for (size_t i = 0; i < block_size; ++i) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }

    // Step 3: Inner hash = H(ipad || message)
    std::vector<uint8_t> inner_data(ipad);
    inner_data.insert(inner_data.end(), message.begin(), message.end());
    auto inner_hash = CalcHash<C>(inner_data);

    // Step 4: Outer hash = H(opad || inner_hash)
    std::vector<uint8_t> outer_data(opad);
    outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
    return CalcHash<C>(outer_data);
}
