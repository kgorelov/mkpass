#include "semver.h"
#include "update_state.h"
#include "platform_utils.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace mkpass {

TEST(SemVerTest, BasicParsing) {
    auto v1 = SemVer::Parse("1.2.3");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 2);
    EXPECT_EQ(v1->patch, 3);
    EXPECT_TRUE(v1->prerelease.empty());
    EXPECT_TRUE(v1->build.empty());
    EXPECT_FALSE(v1->IsPrerelease());
    EXPECT_EQ(v1->ToString(), "1.2.3");

    // Leading 'v' and 'V'
    auto v2 = SemVer::Parse("v0.1.0");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->major, 0);
    EXPECT_EQ(v2->minor, 1);
    EXPECT_EQ(v2->patch, 0);
    EXPECT_EQ(v2->ToString(), "0.1.0");

    auto v3 = SemVer::Parse("V2.10.5");
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(v3->major, 2);
    EXPECT_EQ(v3->minor, 10);
    EXPECT_EQ(v3->patch, 5);

    // Two-component fallback
    auto v4 = SemVer::Parse("0.3");
    ASSERT_TRUE(v4.has_value());
    EXPECT_EQ(v4->major, 0);
    EXPECT_EQ(v4->minor, 3);
    EXPECT_EQ(v4->patch, 0);
}

TEST(SemVerTest, PrereleaseAndBuildMetadata) {
    auto v1 = SemVer::Parse("1.0.0-alpha.1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 0);
    EXPECT_EQ(v1->patch, 0);
    EXPECT_EQ(v1->prerelease, "alpha.1");
    EXPECT_TRUE(v1->IsPrerelease());
    EXPECT_EQ(v1->ToString(), "1.0.0-alpha.1");

    auto v2 = SemVer::Parse("v1.0.0-beta+20261004.sha123");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->major, 1);
    EXPECT_EQ(v2->prerelease, "beta");
    EXPECT_EQ(v2->build, "20261004.sha123");
    EXPECT_EQ(v2->ToString(), "1.0.0-beta+20261004.sha123");

    auto v3 = SemVer::Parse("2.0.0+build.1");
    ASSERT_TRUE(v3.has_value());
    EXPECT_TRUE(v3->prerelease.empty());
    EXPECT_EQ(v3->build, "build.1");
    EXPECT_FALSE(v3->IsPrerelease());
}

TEST(SemVerTest, InvalidVersions) {
    EXPECT_FALSE(SemVer::Parse("").has_value());
    EXPECT_FALSE(SemVer::Parse("v").has_value());
    EXPECT_FALSE(SemVer::Parse("1").has_value());
    EXPECT_FALSE(SemVer::Parse("1.2.3.4").has_value());
    EXPECT_FALSE(SemVer::Parse("01.2.3").has_value()); // leading zero in major
    EXPECT_FALSE(SemVer::Parse("1.02.3").has_value()); // leading zero in minor
    EXPECT_FALSE(SemVer::Parse("1.2.03").has_value()); // leading zero in patch
    EXPECT_FALSE(SemVer::Parse("-1.2.3").has_value());
    EXPECT_FALSE(SemVer::Parse("1.2.3-01").has_value()); // leading zero in numeric prerelease
    EXPECT_FALSE(SemVer::Parse("1.2.3-alpha..1").has_value()); // empty identifier
    EXPECT_FALSE(SemVer::Parse("1.2.3+").has_value()); // empty build metadata
    EXPECT_FALSE(SemVer::Parse("1.2.3-").has_value()); // empty prerelease
    EXPECT_FALSE(SemVer::Parse("abc").has_value());
}

TEST(SemVerTest, PrecedenceAndComparisons) {
    auto v0_1_0 = *SemVer::Parse("0.1.0");
    auto v0_2_0 = *SemVer::Parse("0.2.0");
    auto v1_0_0 = *SemVer::Parse("1.0.0");
    auto v1_0_1 = *SemVer::Parse("1.0.1");
    auto v1_1_0 = *SemVer::Parse("1.1.0");

    EXPECT_LT(v0_1_0, v0_2_0);
    EXPECT_LT(v0_2_0, v1_0_0);
    EXPECT_LT(v1_0_0, v1_0_1);
    EXPECT_LT(v1_0_1, v1_1_0);

    EXPECT_GT(v0_2_0, v0_1_0);
    EXPECT_LE(v0_1_0, v0_1_0);
    EXPECT_GE(v0_1_0, v0_1_0);
    EXPECT_EQ(v0_1_0, v0_1_0);

    // Normal version vs prerelease
    auto v1_rc1 = *SemVer::Parse("1.0.0-rc.1");
    EXPECT_LT(v1_rc1, v1_0_0);
    EXPECT_GT(v1_0_0, v1_rc1);

    // SemVer 2.0.0 Spec Example 11
    auto a1 = *SemVer::Parse("1.0.0-alpha");
    auto a2 = *SemVer::Parse("1.0.0-alpha.1");
    auto a3 = *SemVer::Parse("1.0.0-alpha.beta");
    auto b1 = *SemVer::Parse("1.0.0-beta");
    auto b2 = *SemVer::Parse("1.0.0-beta.2");
    auto b11 = *SemVer::Parse("1.0.0-beta.11");
    auto rc1 = *SemVer::Parse("1.0.0-rc.1");
    auto rel = *SemVer::Parse("1.0.0");

    EXPECT_LT(a1, a2);
    EXPECT_LT(a2, a3);
    EXPECT_LT(a3, b1);
    EXPECT_LT(b1, b2);
    EXPECT_LT(b2, b11);
    EXPECT_LT(b11, rc1);
    EXPECT_LT(rc1, rel);

    // Build metadata is ignored in precedence
    auto with_build_1 = *SemVer::Parse("1.0.0+build.1");
    auto with_build_2 = *SemVer::Parse("1.0.0+build.2");
    EXPECT_EQ(with_build_1, with_build_2);
    EXPECT_FALSE(with_build_1 < with_build_2);
    EXPECT_FALSE(with_build_2 < with_build_1);
}

class UpdateStateTest : public ::testing::Test {
protected:
    std::string test_state_path;

    void SetUp() override {
        test_state_path = GetTmpDir() + "/mkpass-state-test-" + std::to_string(GETPID()) + ".json";
        setenv("MKPASS_UPDATE_STATE_PATH", test_state_path.c_str(), 1);
        std::remove(test_state_path.c_str());
    }

    void TearDown() override {
        std::remove(test_state_path.c_str());
        unsetenv("MKPASS_UPDATE_STATE_PATH");
    }
};

TEST_F(UpdateStateTest, DefaultInitialization) {
    UpdateState state(test_state_path);
    EXPECT_TRUE(state.load());
    EXPECT_EQ(state.last_check_timestamp, 0);
    EXPECT_TRUE(state.last_etag.empty());
    EXPECT_TRUE(state.latest_known_version.empty());
    EXPECT_TRUE(state.skipped_version.empty());

    // Should check when never checked before
    EXPECT_TRUE(state.should_check(7, 1000000));
}

TEST_F(UpdateStateTest, SaveAndLoadRoundtrip) {
    UpdateState state(test_state_path);
    state.last_check_timestamp = 1790892400;
    state.last_etag = "W/\"a1b2c3d4e5f6\"";
    state.latest_known_version = "0.2.0";
    state.skipped_version = "0.1.9";

    EXPECT_TRUE(state.save());
    EXPECT_TRUE(std::filesystem::exists(test_state_path));

    UpdateState loaded(test_state_path);
    EXPECT_TRUE(loaded.load());
    EXPECT_EQ(loaded.last_check_timestamp, 1790892400);
    EXPECT_EQ(loaded.last_etag, "W/\"a1b2c3d4e5f6\"");
    EXPECT_EQ(loaded.latest_known_version, "0.2.0");
    EXPECT_EQ(loaded.skipped_version, "0.1.9");
}

TEST_F(UpdateStateTest, ShouldCheckIntervalLogic) {
    UpdateState state(test_state_path);
    int64_t base_time = 1700000000;
    state.last_check_timestamp = base_time;

    // Zero or negative interval always triggers check
    EXPECT_TRUE(state.should_check(0, base_time));
    EXPECT_TRUE(state.should_check(-1, base_time));

    // 1 day interval: 86400 seconds
    EXPECT_FALSE(state.should_check(1, base_time + 86399));
    EXPECT_TRUE(state.should_check(1, base_time + 86400));
    EXPECT_TRUE(state.should_check(1, base_time + 90000));

    // 7 days interval: 7 * 86400 = 604800 seconds
    EXPECT_FALSE(state.should_check(7, base_time + 604799));
    EXPECT_TRUE(state.should_check(7, base_time + 604800));

    // Time moving backward (clock reset / skew) triggers check
    EXPECT_TRUE(state.should_check(7, base_time - 100));
}

TEST_F(UpdateStateTest, CorruptedFileHandling) {
    // Write invalid JSON content
    {
        std::ofstream out(test_state_path);
        out << "{ corrupted: json without quotes or values ";
        out.close();
    }

    UpdateState state(test_state_path);
    // load should safely parse without throwing or crashing
    EXPECT_TRUE(state.load());
    EXPECT_EQ(state.last_check_timestamp, 0);
    EXPECT_TRUE(state.last_etag.empty());
}

} // namespace mkpass
