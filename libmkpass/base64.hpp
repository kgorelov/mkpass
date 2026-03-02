/*
    base64.hpp:
        - header-only C++17 Base64 encoding/decoding library.
        - no dependencies.
        - public domain (0BSD).
*/

#ifndef CZKZ_BASE64_HPP
#define CZKZ_BASE64_HPP

#include <string>
#include <string_view>
#include <vector>

namespace czkz
{
    // encodes a string to Base64.
    std::string base64_encode(std::string_view data);

    // encodes a byte vector to Base64.
    std::string base64_encode(const std::vector<std::uint8_t>& data);

    // decodes a Base64 string.
    // returns an empty vector if the string is invalid.
    std::vector<std::uint8_t> base64_decode(std::string_view data);
}

// implementation
namespace czkz
{
    namespace
    {
        constexpr std::string_view base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        // returns the Base64 character for a 6-bit value.
        char base64_char(std::uint8_t val)
        {
            return base64_chars[val];
        }

        // returns the 6-bit value for a Base64 character.
        // returns -1 if the character is invalid.
        int base64_val(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return c - 'A';
            if (c >= 'a' && c <= 'z')
                return c - 'a' + 26;
            if (c >= '0' && c <= '9')
                return c - '0' + 52;
            if (c == '+')
                return 62;
            if (c == '/')
                return 63;
            return -1;
        }
    }

    std::string base64_encode(std::string_view data)
    {
        std::string result;
        result.reserve((data.size() + 2) / 3 * 4);

        for (std::size_t i = 0; i < data.size(); i += 3)
        {
            std::uint8_t b1 = data[i];
            std::uint8_t b2 = (i + 1 < data.size()) ? data[i + 1] : 0;
            std::uint8_t b3 = (i + 2 < data.size()) ? data[i + 2] : 0;

            std::uint8_t c1 = b1 >> 2;
            std::uint8_t c2 = ((b1 & 0x03) << 4) | (b2 >> 4);
            std::uint8_t c3 = ((b2 & 0x0f) << 2) | (b3 >> 6);
            std::uint8_t c4 = b3 & 0x3f;

            result += base64_char(c1);
            result += base64_char(c2);

            if (i + 1 < data.size())
                result += base64_char(c3);
            else
                result += '=';

            if (i + 2 < data.size())
                result += base64_char(c4);
            else
                result += '=';
        }

        return result;
    }

    std::string base64_encode(const std::vector<std::uint8_t>& data)
    {
        return base64_encode(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    std::vector<std::uint8_t> base64_decode(std::string_view data)
    {
        std::vector<std::uint8_t> result;
        result.reserve(data.size() * 3 / 4);

        for (std::size_t i = 0; i < data.size(); i += 4)
        {
            int v1 = base64_val(data[i]);
            int v2 = base64_val(data[i + 1]);
            int v3 = base64_val(data[i + 2]);
            int v4 = base64_val(data[i + 3]);

            if (v1 == -1 || v2 == -1)
                return {};

            result.push_back((v1 << 2) | (v2 >> 4));

            if (data[i + 2] != '=')
            {
                if (v3 == -1)
                    return {};
                result.push_back(((v2 & 0x0f) << 4) | (v3 >> 2));
            }

            if (data[i + 3] != '=')
            {
                if (v4 == -1)
                    return {};
                result.push_back(((v3 & 0x03) << 6) | v4);
            }
        }

        return result;
    }
}

#endif // CZKZ_BASE64_HPP
