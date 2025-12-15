#include "argon2_generator.h"
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>

TEST(Argon2GeneratorTest, TestLE) {
    Argon2Generator g("Key", "Info");

    // These are pre-computed values for the given key and info
    EXPECT_EQ(g(), 0xa28366f);
    EXPECT_EQ(g(), 0xf42e8de6);
    EXPECT_EQ(g(), 0xb5cc0f3a);
    EXPECT_EQ(g(), 0x3322b652);
}
