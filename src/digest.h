#pragma once

#include <span>
#include <string_view>
#include <string>
#include <array>
#include <cstdint>


template<typename T>
concept HashCalculator =
requires {
    // static constexpr size_t block_size()
    { T::block_size() } -> std::convertible_to<std::size_t>;
    // static constexpr size_t hash_size()
    { T::hash_size() } -> std::convertible_to<std::size_t>;
    requires (T::hash_size() > 0);
}
    &&
    requires {
    // value_type = std::array<uint8_t, hash_size()>
    typename T::value_type;
    requires std::same_as<
        typename T::value_type,
        std::array<uint8_t, T::hash_size()>
        >;
}
&&
requires(T h, std::span<const uint8_t> s) {
    // Update(span<const uint8_t>)
    { h.Update(s) } -> std::same_as<void>;

    // Finalize() -> value_type
    { h.Finalize() } -> std::same_as<typename T::value_type>;
};


template<HashCalculator C>
C::value_type CalcHash(std::span<const uint8_t> inp) {
    C calculator;
    calculator.Update(inp);
    return calculator.Finalize();
}

////////////////////////////////////////////////////////////////////////////////

struct SHA512
{
    static constexpr std::size_t block_size() { return 128; }
    static constexpr std::size_t hash_size()  { return 64; }
    using value_type = std::array<uint8_t, 64>;

    void Update(std::span<const uint8_t> inp);
    value_type Finalize();

    void* state_ptr_ = nullptr;
};
