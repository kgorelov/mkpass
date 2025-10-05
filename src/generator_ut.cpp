#include "generator.h"
#include <gtest/gtest.h>

TEST(GeneratorTest, TestLE) {
    Generator g("Test");

    // Expect LittleEndian
    EXPECT_EQ(g(), 0x339eeec6);
    EXPECT_EQ(g(), 0x15675ccf);

    // Test the value on the boundary of 512 bits
    for (unsigned i=0; i < (512/8/4 - 3); ++i) {
        g();
    }
    EXPECT_EQ(g(), 0x318a0b3b);

    // Test values beyond 512 boundary
    // making sure infinite generation works
    EXPECT_EQ(g(), 0x404c111d);
    EXPECT_EQ(g(), 0x8ba4a58e);
}
