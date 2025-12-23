#pragma once

#include "hmac.h"
#include "digest.h"
#include "argon2/argon2.h"
#include "sha512.h"
#include <stdexcept>

template <HashCalculator C>
struct HKDF_HMAC {
    using value_type = typename C::value_type;

    value_type operator()(std::span<const uint8_t> key, std::span<const uint8_t> message) const {
        return HMAC<C>(key, message);
    }
};

struct HKDF_Argon2 {
    using value_type = std::vector<uint8_t>;

    value_type operator()(std::span<const uint8_t> key, std::span<const uint8_t> info) const {
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

        std::vector<uint8_t> digest(hash_len);

        argon2_context context;
        context.out = digest.data();
        context.outlen = digest.size();
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
        return digest;
    }
};