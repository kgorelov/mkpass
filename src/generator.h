#pragma once

#include <cstdint>
#include <concepts>
#include <random>
#include <cstring>
#include <string_view>

#ifdef _WIN32
// Windows-specific implementation
inline uint32_t htole32(uint32_t x) {
    // Windows is little-endian, so no conversion is needed
    return x;
}
#else
// Linux-specific implementation
#include <endian.h>
#endif

#include "digest.h"
#include "hmac.h"

class GeneratorInterface {
public:
    using result_type = std::uint32_t;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }

    virtual ~GeneratorInterface() = default;
    virtual result_type operator()() = 0;
};

/**
 * @class Generator
 * @brief This class implements a pseudo-random number generator.
 */
template <HashCalculator HC>
class Generator : public GeneratorInterface {
public:
    using result_type = std::uint32_t;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }

    /**
     * @brief C-tor
     * @param seedstr Initial input, defines the random sequence
     */
    Generator(std::span<const uint8_t> key, std::span<const uint8_t> info)
        : key_(key)
        , info_(info)
        , index_(0)
        , extend_counter_(0)
    {
        Extract();
    }

    Generator(const std::string& key, const std::string& info)
        : Generator(
            {reinterpret_cast<const uint8_t*>(key.data()), key.size()},
            {reinterpret_cast<const uint8_t*>(info.data()), info.size()})
    {
    }

    /**
     * @brief returns a pseudo-random number
     */
    result_type operator()() override {
        return htole32(Get<result_type>());
    }

private:
    void Get(char* dest, size_t len) {
        size_t dest_index = 0;
        while (len > 0) {
            if (index_ >= digest_.size()) {
                Extend();
                index_ = 0;
            }

            size_t nbytes = std::min(len, digest_.size() - index_);
            memcpy(dest + dest_index, digest_.data() + index_, nbytes);
            dest_index += nbytes;
            index_ += nbytes;
            len -= nbytes;
        }
    }

    template <typename T>
    T Get() {
        T result;
        Get((char *)&result, sizeof(result));
        return result;
    }

    void Extract() {
        std::vector<uint8_t> combined(info_.begin(), info_.end());
        combined.push_back(static_cast<uint8_t>(extend_counter_));
        digest_ = HMAC<HC>(key_, combined);
    }

    // T(0) = empty
    // T(1) = HMAC(PRK, T(0) | info | 0x01)
    // T(2) = HMAC(PRK, T(1) | info | 0x02)
    // ...
    // OKM = T(1) | T(2) | ... | T(n)
    void Extend() {
        ++extend_counter_;
        std::vector<uint8_t> combined(digest_.begin(), digest_.end());
        combined.insert(combined.end(), info_.begin(), info_.end());
        combined.push_back(static_cast<uint8_t>(extend_counter_));
        digest_ = HMAC<HC>(key_, combined);
    }

private:
    std::span<const uint8_t> key_;
    std::span<const uint8_t> info_;
    size_t index_;
    uint8_t extend_counter_;
    HC::value_type digest_;
};

#include "sha512.h"

// Concept check
static_assert(std::uniform_random_bit_generator<Generator<SHA512>>,
              "Generator does not meet UniformRandomBitGenerator requirements");
