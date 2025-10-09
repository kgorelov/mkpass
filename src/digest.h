#pragma once

#include <string>
#include <array>

using Digest = std::array<unsigned char, 64>;

void Sha512(const char* inp, size_t len, Digest& out);
std::string DigestToString(const Digest& d);
std::string Sha512(const std::string& input);
