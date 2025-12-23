#include "generator.h"
#include <gtest/gtest.h>

TEST(GeneratorTest, TestLE) {
    Generator<HKDF_HMAC<SHA512>> g("Key", "Info");

    // Expect LittleEndian
    EXPECT_EQ(g(), 0xa2ab0569/*0x339eeec6*/);
    EXPECT_EQ(g(), 0x68a443cf/*0x15675ccf*/);

    // Test the value on the boundary of 512 bits
    for (unsigned i=0; i < (512/8/4 - 3); ++i) {
        g();
    }
    EXPECT_EQ(g(), 0x73e42c36/*0x318a0b3b*/);

    // Test values beyond 512 boundary
    // making sure infinite generation works
    EXPECT_EQ(g(), 0x9bd4fede/*0x404c111d*/);
    EXPECT_EQ(g(), 0x6e4c574a);
}
