#include "generator.h"
#include "hkdf_argon2.h"
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>

TEST(HKDF_Argon2Test, TestLE) {
    Generator<HKDF_Argon2> g("Key", "Info");

    // These are pre-computed values for the given key and info
    EXPECT_EQ(g(), 0x4c44b2c);
    EXPECT_EQ(g(), 0x4a9ec479);
    EXPECT_EQ(g(), 0x9edf3054);
    EXPECT_EQ(g(), 0x9103f2c0);
}
