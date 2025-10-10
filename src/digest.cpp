#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "digest.h"


#ifndef OPENSSL_SHA512
#include "sha512.h"

void Sha512(const char* inp, size_t len, Digest& out) {
    sha512((unsigned char*)inp, static_cast<int>(len), out.data());
}

#else
#include <openssl/evp.h>

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

std::string DigestToString(const Digest& d) {
    std::ostringstream oss;
    for (unsigned int i = 0; i < d.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)(d[i]);
    }
    return oss.str();
}

std::string Sha512(const std::string& input) {
    Digest d;
    Sha512(input.c_str(), input.length(), d);
    return DigestToString(d);
}
