#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "digest.h"


#ifndef OPENSSL_SHA512
#include "sha512.h"

void SHA512::Update(std::span<const uint8_t> inp) {
    if (state_ptr_ == nullptr) {
        state_ptr_ = sha512_init();
        if (state_ptr_ == nullptr) {
            throw std::runtime_error("Error initializing sha512");
        }
    }
    sha512_update(state_ptr_, (unsigned char*)inp.data(), inp.size());
}


SHA512::value_type SHA512::Finalize() {
    value_type result;
    sha512_finalize(state_ptr_, result.data());
    return result;
}

void SLOW_SHA512::Update(std::span<const uint8_t> inp) {
    for (unsigned i = 0; i < 1000000; ++i) {
        SHA512::Update(inp);
    }
}


#else
#include <openssl/evp.h>

void SHA512::Update(std::span<const uint8_t> inp) {
    throw std::runtime_error("not implemented");
}

value_type SHA512::Finalize() {
    throw std::runtime_error("not implemented");
}

void SLOW_SHA512::Update(std::span<const uint8_t> inp) {
    throw std::runtime_error("not implemented");
}

void Sha512(const char* inp, size_t len, Digest& out) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if (context == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(context, /*hash_type*/ EVP_sha512(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (EVP_DigestUpdate(context, inp, len) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestUpdate failed");
    }

    unsigned int length = 0;
    if (EVP_DigestFinal_ex(context, out.data(), &length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    if (length != out.size()) {
        throw std::runtime_error("EVP_DigestFinal_ex: enexpected length");
    }

    EVP_MD_CTX_free(context);
}
#endif
