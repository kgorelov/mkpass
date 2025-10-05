#include <iostream>

#include "digest.h"
#include <gtest/gtest.h>


TEST(DigestTest, Sha512Test) {
    EXPECT_EQ(sha512("Test"), "c6ee9e33cf5c6715a1d148fd73f7318884b41adcb916021e2bc0e800a5c5dd97f5142178f6ae88c8fdd98e1afb0ce4c8d2c54b5f37b30b7da1997bb33b0b8a31");
}

TEST(DigestTest, Sha512Empty) {
    EXPECT_EQ(sha512(""), "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
}
