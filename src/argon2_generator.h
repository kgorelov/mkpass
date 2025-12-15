#pragma once

#include "generator.h"
#include "argon2/argon2.h"
#include <vector>
#include <cstring>
#include <stdexcept>
#include "digest.h"

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

class Argon2Generator : public GeneratorInterface {
public:
    Argon2Generator(std::span<const uint8_t> key, std::span<const uint8_t> info)
        : index_(0)
    {
        // 1. Derive a salt from key and info
        std::vector<uint8_t> salt_material;
        salt_material.insert(salt_material.end(), key.begin(), key.end());
        salt_material.insert(salt_material.end(), info.begin(), info.end());

        auto salt_full = CalcHash<SHA512>(salt_material);
        std::vector<uint8_t> salt(salt_full.begin(), salt_full.begin() + 16); // Use a 16-byte salt

        // Argon2 parameters
        const uint32_t t_cost = 3;
        const uint32_t m_cost = 1 << 16; // 65536 KiB = 64 MiB
        const uint32_t parallelism = 4;
        const uint32_t hash_len = 256;

        digest_.resize(hash_len);

        argon2_context context;
        context.out = digest_.data();
        context.outlen = digest_.size();
        context.pwd = const_cast<uint8_t*>(key.data());
        context.pwdlen = key.size();
        context.salt = salt.data();
        context.saltlen = salt.size();
        context.ad = const_cast<uint8_t*>(info.data());
        context.adlen = info.size();
        context.secret = nullptr;
        context.secretlen = 0;
        context.t_cost = t_cost;
        context.m_cost = m_cost;
        context.lanes = parallelism;
        context.threads = parallelism;
        context.version = ARGON2_VERSION_NUMBER;
        context.allocate_cbk = nullptr;
        context.free_cbk = nullptr;
        context.flags = ARGON2_DEFAULT_FLAGS;

        int result = argon2_ctx(&context, Argon2_id);

        if (result != ARGON2_OK) {
            throw std::runtime_error(argon2_error_message(result));
        }
    }

    Argon2Generator(const std::string& key, const std::string& info)
        : Argon2Generator(
            {reinterpret_cast<const uint8_t*>(key.data()), key.size()},
            {reinterpret_cast<const uint8_t*>(info.data()), info.size()})
    {
    }

    result_type operator()() override {
        return htole32(Get<result_type>());
    }

private:
    void Get(uint8_t* dest, size_t len) {
        size_t dest_index = 0;
        while (len > 0) {
            if (index_ >= digest_.size()) {
                // Should not happen with a large enough hash
                // but as a fallback, just wrap around.
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
        Get(reinterpret_cast<uint8_t*>(&result), sizeof(result));
        return result;
    }

private:
    size_t index_;
    std::vector<uint8_t> digest_;
};

// Concept check
static_assert(std::uniform_random_bit_generator<Argon2Generator>,
              "Argon2Generator does not meet UniformRandomBitGenerator requirements");
