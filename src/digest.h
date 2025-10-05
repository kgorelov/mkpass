#pragma once

#include <string>
#include <openssl/evp.h>

struct Digest {
    Digest()
        : length(0)
    {
    }
    unsigned char data[EVP_MAX_MD_SIZE];
    unsigned int length;
};

void CalcDigest(const EVP_MD *hash_type, const char* inp, size_t len, Digest& out);
std::string DigestString(const Digest& d);
std::string sha512(const std::string& input);
