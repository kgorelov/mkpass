#pragma once

#include <cstdint>
#include <concepts>
#include <random>
#include <cstring>

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

/**
 * @class Generator
 * @brief This class implements a pseudo-random number generator.
 */
class Generator
{
public:
    using result_type = std::uint32_t;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }

    /**
     * @brief C-tor
     * @param seedstr Initial input, defines the random sequence
     */
    Generator(const std::string& seedstr)
        : index_(0)
    {
        Sha512(seedstr.c_str(), seedstr.length(), digest_);
    }

    /**
     * @brief returns a pseudo-random number
     */
    result_type operator()() {
        return htole32(Get<result_type>());
    }

private:
    void Get(char* dest, size_t len) {
        size_t dest_index = 0;
        while (len > 0) {
            if (index_ >= digest_.size()) {
                Sha512((const char*) digest_.data(), digest_.size(), digest_);
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

private:
    size_t index_;
    Digest digest_;
};

// Concept check
static_assert(std::uniform_random_bit_generator<Generator>,
              "Generator does not meet UniformRandomBitGenerator requirements");
