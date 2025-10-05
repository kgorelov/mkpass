#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "digest.h"

void CalcDigest(const EVP_MD *hash_type, const char* inp, size_t len, Digest& out) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(context, hash_type, nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (EVP_DigestUpdate(context, inp, len) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestUpdate failed");
    }

    if (EVP_DigestFinal_ex(context, out.data, &out.length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(context);
}

std::string DigestString(const Digest& d) {
    std::ostringstream oss;
    for (unsigned int i = 0; i < d.length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)(d.data[i]);
    }
    return oss.str();
}

std::string sha512(const std::string& input) {
    Digest d;
    CalcDigest(EVP_sha512(), input.c_str(), input.length(), d);
    return DigestString(d);
}
