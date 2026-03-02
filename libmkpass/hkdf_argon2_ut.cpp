#include "generator.h"
#include "hkdf_argon2.h"
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>

TEST(HKDF_Argon2Test, TestLE) {
    Generator<HKDF_Argon2> g("Key", "Info");

    // These are pre-computed values for the given key and info
    EXPECT_EQ(g(), 0x76734720);
    EXPECT_EQ(g(), 0xeb52b812);
    EXPECT_EQ(g(), 0x98360f10);
    EXPECT_EQ(g(), 0x8dbb8852);
}
