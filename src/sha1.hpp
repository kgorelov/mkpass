/*
    sha1.hpp - header-only library for SHA-1 in C++

    100% Public Domain.

    Original C Code -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code -- Volker Diels-Grabsch <v@njh.eu>
    Safety fixes -- Eugene Hopkinson <slowriot at voxelstorm dot com>
    Header-only library -- Zlatko Michailov <zlatko@michailov.org>
*/

#ifndef SHA1_HPP
#define SHA1_HPP


#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>


// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

class SHA1
{
public:
    SHA1() {
        reset();
    }
    /// Process a string.
    void update(const std::string &s);
    /// Process a byte vector.
    void update(const std::vector<uint8_t> &data);
    /// Process an array of bytes.
    void update(const void *data, size_t len);
    /// Process an input stream.
    void update(std::istream &is);
    /// Get the hash as a string of 40 hexadecimal characters.
    std::string final();
    /// Get the hash as a vector of 20 bytes.
    std::vector<uint8_t> final_bytes();

private:
    uint32_t digest[5];
    std::string buffer;
    uint64_t transforms;

    void reset();
    void transform(uint32_t block[16]);
    static void buffer_to_block(const std::string &buffer, uint32_t block[16]);
    void read(std::istream &is, std::string &s, int max);
};


// -----------------------------------------------------------------------------
// Private methods
// -----------------------------------------------------------------------------

#define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define SHA1_BLK(i) (block[i&15] = SHA1_ROL(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i&15],1))

// (R0+R1), R2, R3, R4 are the different operations used in SHA1
#define SHA1_R0(v,w,x,y,z,i) z += ((w&(x^y))^y)     + block[i] + 0x5A827999 + SHA1_ROL(v,5); w = SHA1_ROL(w,30);
#define SHA1_R1(v,w,x,y,z,i) z += ((w&(x^y))^y)     + SHA1_BLK(i) + 0x5A827999 + SHA1_ROL(v,5); w = SHA1_ROL(w,30);
#define SHA1_R2(v,w,x,y,z,i) z += (w^x^y)           + SHA1_BLK(i) + 0x6ED9EBA1 + SHA1_ROL(v,5); w = SHA1_ROL(w,30);
#define SHA1_R3(v,w,x,y,z,i) z += (((w|x)&y)|(w&x)) + SHA1_BLK(i) + 0x8F1BBCDC + SHA1_ROL(v,5); w = SHA1_ROL(w,30);
#define SHA1_R4(v,w,x,y,z,i) z += (w^x^y)           + SHA1_BLK(i) + 0xCA62C1D6 + SHA1_ROL(v,5); w = SHA1_ROL(w,30);


void SHA1::transform(uint32_t block[16])
{
    // Copy digest[] to working vars
    uint32_t a = digest[0];
    uint32_t b = digest[1];
    uint32_t c = digest[2];
    uint32_t d = digest[3];
    uint32_t e = digest[4];

    // 80 rounds of operations
    // R0
    SHA1_R0(a,b,c,d,e, 0); SHA1_R0(e,a,b,c,d, 1); SHA1_R0(d,e,a,b,c, 2); SHA1_R0(c,d,e,a,b, 3);
    SHA1_R0(b,c,d,e,a, 4); SHA1_R0(a,b,c,d,e, 5); SHA1_R0(e,a,b,c,d, 6); SHA1_R0(d,e,a,b,c, 7);
    SHA1_R0(c,d,e,a,b, 8); SHA1_R0(b,c,d,e,a, 9); SHA1_R0(a,b,c,d,e,10); SHA1_R0(e,a,b,c,d,11);
    SHA1_R0(d,e,a,b,c,12); SHA1_R0(c,d,e,a,b,13); SHA1_R0(b,c,d,e,a,14); SHA1_R0(a,b,c,d,e,15);
    // R1
    SHA1_R1(e,a,b,c,d,16); SHA1_R1(d,e,a,b,c,17); SHA1_R1(c,d,e,a,b,18); SHA1_R1(b,c,d,e,a,19);
    // R2
    SHA1_R2(a,b,c,d,e,20); SHA1_R2(e,a,b,c,d,21); SHA1_R2(d,e,a,b,c,22); SHA1_R2(c,d,e,a,b,23);
    SHA1_R2(b,c,d,e,a,24); SHA1_R2(a,b,c,d,e,25); SHA1_R2(e,a,b,c,d,26); SHA1_R2(d,e,a,b,c,27);
    SHA1_R2(c,d,e,a,b,28); SHA1_R2(b,c,d,e,a,29); SHA1_R2(a,b,c,d,e,30); SHA1_R2(e,a,b,c,d,31);
    SHA1_R2(d,e,a,b,c,32); SHA1_R2(c,d,e,a,b,33); SHA1_R2(b,c,d,e,a,34); SHA1_R2(a,b,c,d,e,35);
    SHA1_R2(e,a,b,c,d,36); SHA1_R2(d,e,a,b,c,37); SHA1_R2(c,d,e,a,b,38); SHA1_R2(b,c,d,e,a,39);
    // R3
    SHA1_R3(a,b,c,d,e,40); SHA1_R3(e,a,b,c,d,41); SHA1_R3(d,e,a,b,c,42); SHA1_R3(c,d,e,a,b,43);
    SHA1_R3(b,c,d,e,a,44); SHA1_R3(a,b,c,d,e,45); SHA1_R3(e,a,b,c,d,46); SHA1_R3(d,e,a,b,c,47);
    SHA1_R3(c,d,e,a,b,48); SHA1_R3(b,c,d,e,a,49); SHA1_R3(a,b,c,d,e,50); SHA1_R3(e,a,b,c,d,51);
    SHA1_R3(d,e,a,b,c,52); SHA1_R3(c,d,e,a,b,53); SHA1_R3(b,c,d,e,a,54); SHA1_R3(a,b,c,d,e,55);
    SHA1_R3(e,a,b,c,d,56); SHA1_R3(d,e,a,b,c,57); SHA1_R3(c,d,e,a,b,58); SHA1_R3(b,c,d,e,a,59);
    // R4
    SHA1_R4(a,b,c,d,e,60); SHA1_R4(e,a,b,c,d,61); SHA1_R4(d,e,a,b,c,62); SHA1_R4(c,d,e,a,b,63);
    SHA1_R4(b,c,d,e,a,64); SHA1_R4(a,b,c,d,e,65); SHA1_R4(e,a,b,c,d,66); SHA1_R4(d,e,a,b,c,67);
    SHA1_R4(c,d,e,a,b,68); SHA1_R4(b,c,d,e,a,69); SHA1_R4(a,b,c,d,e,70); SHA1_R4(e,a,b,c,d,71);
    SHA1_R4(d,e,a,b,c,72); SHA1_R4(c,d,e,a,b,73); SHA1_R4(b,c,d,e,a,74); SHA1_R4(a,b,c,d,e,75);
    SHA1_R4(e,a,b,c,d,76); SHA1_R4(d,e,a,b,c,77); SHA1_R4(c,d,e,a,b,78); SHA1_R4(b,c,d,e,a,79);

    // Add the working vars back into digest[]
    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;

    // Count the number of transformations
    transforms++;
}


void SHA1::buffer_to_block(const std::string &buffer, uint32_t block[16])
{
    // Convert the std::string (byte buffer) to a uint32_t array (MSB)
    for (size_t i = 0; i < 16; i++)
    {
        block[i] = (buffer[4*i+3] & 0xff)
                 | (buffer[4*i+2] & 0xff)<<8
                 | (buffer[4*i+1] & 0xff)<<16
                 | (buffer[4*i+0] & 0xff)<<24;
    }
}


void SHA1::reset()
{
    // SHA1 initialization constants
    digest[0] = 0x67452301;
    digest[1] = 0xEFCDAB89;
    digest[2] = 0x98BADCFE;
    digest[3] = 0x10325476;
    digest[4] = 0xC3D2E1F0;

    // Reset counters
    buffer    = "";
    transforms = 0;
}


// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------

void SHA1::update(const std::string &s)
{
    std::istringstream is(s);
    update(is);
}


void SHA1::update(const std::vector<uint8_t> &data)
{
    update(data.data(), data.size());
}


void SHA1::update(const void *data, size_t len)
{
    const char *s = (const char *)data;
    for (size_t i = 0; i < len; ++i) {
        buffer += s[i];
        if (buffer.size() == 64) {
            uint32_t block[16];
            buffer_to_block(buffer, block);
            transform(block);
            buffer.clear();
        }
    }
}


void SHA1::update(std::istream &is)
{
    while (true)
    {
        char s[64];
        is.read(s, 64 - buffer.size());
        buffer.append(s, is.gcount());
        if (buffer.size() == 64)
        {
            uint32_t block[16];
            buffer_to_block(buffer, block);
            transform(block);
            buffer.clear();
        }
        else
        {
            break;
        }
    }
}


std::string SHA1::final()
{
    // Total number of bits
    uint64_t total_bits = (transforms*64 + buffer.size()) * 8;

    // Add padding
    buffer += (char)0x80;
    size_t orig_size = buffer.size();
    while (buffer.size() < 56)
    {
        buffer += (char)0x00;
    }

    // Append total_bits to the end of the buffer
    buffer += (char)((total_bits >> 56) & 0xff);
    buffer += (char)((total_bits >> 48) & 0xff);
    buffer += (char)((total_bits >> 40) & 0xff);
    buffer += (char)((total_bits >> 32) & 0xff);
    buffer += (char)((total_bits >> 24) & 0xff);
    buffer += (char)((total_bits >> 16) & 0xff);
    buffer += (char)((total_bits >> 8) & 0xff);
    buffer += (char)((total_bits >> 0) & 0xff);

    // Process the last block(s)
    uint32_t block[16];
    buffer_to_block(buffer, block);
    transform(block);

    // Return the hash as a string
    std::ostringstream result;
    for (size_t i = 0; i < 5; i++)
    {
        result << std::hex << std::setfill('0') << std::setw(8) << digest[i];
    }

    // Reset for next use
    // reset();

    return result.str();
}

std::vector<uint8_t> SHA1::final_bytes()
{
    final(); // calculates the digest

    std::vector<uint8_t> result;
    result.reserve(20);
    for (size_t i = 0; i < 5; i++)
    {
        result.push_back((digest[i] >> 24) & 0xff);
        result.push_back((digest[i] >> 16) & 0xff);
        result.push_back((digest[i] >> 8) & 0xff);
        result.push_back((digest[i] >> 0) & 0xff);
    }

    return result;
}


#endif // SHA1_HPP
