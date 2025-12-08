#include "generator.h"
#include "uniform.h"
#include <gtest/gtest.h>
#include <cstdlib>

TEST(UniformTest, TestDistrubution) {
    Generator g("Key", "Info");
    UniformDistribution ud(0, 25);
    EXPECT_EQ(ud(g), 19);
    EXPECT_EQ(ud(g), 15);
    EXPECT_EQ(ud(g), 1);
}

TEST(UniformTest, TestDistrubution2) {
    Generator g("Key", "Info");
    UniformDistribution ud(0, 1);
    EXPECT_EQ(ud(g), 1);
    EXPECT_EQ(ud(g), 1);
    EXPECT_EQ(ud(g), 1);
}

TEST(UniformTest, TestDistrubution3) {
    Generator g("Key", "Info");
    UniformDistribution ud(0, 0);
    EXPECT_EQ(ud(g), 0);
    EXPECT_EQ(ud(g), 0);
}

TEST(UniformTest, TestDistrubution4) {
    Generator g("Key", "Info");
    UniformDistribution ud(1, 1);
    EXPECT_EQ(ud(g), 1);
    EXPECT_EQ(ud(g), 1);
}

TEST(UniformTest, TestBaskets) {
    Generator g("Key", "Info");
    UniformDistribution ud(0, 9);
    std::map<unsigned, unsigned> baskets;
    for (unsigned i = 0; i < 100000; ++i) {
        baskets[ud(g)]++;
    }
    EXPECT_EQ(baskets.size(), 10);
    for (auto& [k, v]: baskets) {
        EXPECT_LT(std::abs(10000 - (int)v), 1000);
    }
}
