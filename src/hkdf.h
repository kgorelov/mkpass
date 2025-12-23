#pragma once

#include <concepts>
#include <span>
#include <cstdint>
#include <vector>

template <typename T>
concept HKDF = requires(T hkdf, std::span<const uint8_t> key, std::span<const uint8_t> info) {
    typename T::value_type;
    { hkdf(key, info) } -> std::same_as<typename T::value_type>;
};
