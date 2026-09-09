/*
 * gtest_version_compat.cpp - 版本兼容模块单元测试
 *
 * Phase 5: version_compat.c 测试
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/version_compat.h"
}

/* ======================== SemVer 解析 ======================== */

TEST(SemVer, ParseBasic) {
    SemVer v;
    ASSERT_EQ(semver_parse("1.2.3", &v), 0);
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 2);
    EXPECT_EQ(v.patch, 3);
    EXPECT_STREQ(v.prerelease, "");
    EXPECT_STREQ(v.build, "");
}

TEST(SemVer, ParsePrerelease) {
    SemVer v;
    ASSERT_EQ(semver_parse("2.0.0-alpha.1", &v), 0);
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
    EXPECT_STREQ(v.prerelease, "alpha.1");
}

TEST(SemVer, ParseBuild) {
    SemVer v;
    ASSERT_EQ(semver_parse("1.0.0+build.42", &v), 0);
    EXPECT_EQ(v.major, 1);
    EXPECT_STREQ(v.build, "build.42");
}

TEST(SemVer, ParseFull) {
    SemVer v;
    ASSERT_EQ(semver_parse("3.1.4-rc.2+exp.sha.5114f85", &v), 0);
    EXPECT_EQ(v.major, 3);
    EXPECT_EQ(v.minor, 1);
    EXPECT_EQ(v.patch, 4);
    EXPECT_STREQ(v.prerelease, "rc.2");
    EXPECT_STREQ(v.build, "exp.sha.5114f85");
}

TEST(SemVer, ParseZero) {
    SemVer v;
    ASSERT_EQ(semver_parse("0.0.0", &v), 0);
    EXPECT_EQ(v.major, 0);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(SemVer, ParseLargeNumbers) {
    SemVer v;
    ASSERT_EQ(semver_parse("100.200.300", &v), 0);
    EXPECT_EQ(v.major, 100);
    EXPECT_EQ(v.minor, 200);
    EXPECT_EQ(v.patch, 300);
}

TEST(SemVer, ParseInvalid) {
    SemVer v;
    EXPECT_EQ(semver_parse(nullptr, &v), -1);
    EXPECT_EQ(semver_parse("", &v), -1);
    EXPECT_EQ(semver_parse("abc", &v), -1);
    EXPECT_EQ(semver_parse("1", &v), -1);
    EXPECT_EQ(semver_parse("1.2", &v), -1);
    EXPECT_EQ(semver_parse("1.2.", &v), -1);
    EXPECT_EQ(semver_parse("a.b.c", &v), -1);
    EXPECT_EQ(semver_parse("1.2.3", nullptr), -1);
}

/* ======================== SemVer 格式化 ======================== */

TEST(SemVer, FormatBasic) {
    SemVer v = {1, 2, 3, "", ""};
    char buf[64];
    ASSERT_GT(semver_format(&v, buf, sizeof(buf)), 0);
    EXPECT_STREQ(buf, "1.2.3");
}

TEST(SemVer, FormatPrerelease) {
    SemVer v = {2, 0, 0, "beta.1", ""};
    char buf[64];
    semver_format(&v, buf, sizeof(buf));
    EXPECT_STREQ(buf, "2.0.0-beta.1");
}

TEST(SemVer, FormatBuild) {
    SemVer v = {1, 0, 0, "", "build.99"};
    char buf[64];
    semver_format(&v, buf, sizeof(buf));
    EXPECT_STREQ(buf, "1.0.0+build.99");
}

/* ======================== SemVer 比较 ======================== */

TEST(SemVer, CompareMajor) {
    SemVer a = {1, 0, 0, "", ""};
    SemVer b = {2, 0, 0, "", ""};
    EXPECT_LT(semver_compare(&a, &b), 0);
    EXPECT_GT(semver_compare(&b, &a), 0);
}

TEST(SemVer, CompareMinor) {
    SemVer a = {1, 1, 0, "", ""};
    SemVer b = {1, 2, 0, "", ""};
    EXPECT_LT(semver_compare(&a, &b), 0);
}

TEST(SemVer, ComparePatch) {
    SemVer a = {1, 0, 1, "", ""};
    SemVer b = {1, 0, 2, "", ""};
    EXPECT_LT(semver_compare(&a, &b), 0);
}

TEST(SemVer, CompareEqual) {
    SemVer a = {1, 2, 3, "", ""};
    SemVer b = {1, 2, 3, "", ""};
    EXPECT_EQ(semver_compare(&a, &b), 0);
}

TEST(SemVer, ComparePrerelease) {
    SemVer a = {1, 0, 0, "alpha", ""};
    SemVer b = {1, 0, 0, "", ""};
    /* 预发布 < 正式版 */
    EXPECT_LT(semver_compare(&a, &b), 0);
    EXPECT_GT(semver_compare(&b, &a), 0);
}

TEST(SemVer, ComparePrereleaseString) {
    SemVer a = {1, 0, 0, "alpha", ""};
    SemVer b = {1, 0, 0, "beta", ""};
    EXPECT_LT(semver_compare(&a, &b), 0);
}

/* ======================== 预发布检测 ======================== */

TEST(SemVer, IsPrerelease) {
    SemVer pre = {1, 0, 0, "alpha", ""};
    SemVer rel = {1, 0, 0, "", ""};
    EXPECT_TRUE(semver_is_prerelease(&pre));
    EXPECT_FALSE(semver_is_prerelease(&rel));
    EXPECT_FALSE(semver_is_prerelease(nullptr));
}

/* ======================== 兼容性检查 ======================== */

TEST(Compat, ExactMatch) {
    EXPECT_EQ(semver_check_compat_str("1.2.3", "1.2.3"), COMPAT_EXACT);
}

TEST(Compat, PatchDiff) {
    EXPECT_EQ(semver_check_compat_str("1.2.3", "1.2.4"), COMPAT_PATCH);
}

TEST(Compat, MinorDiff) {
    EXPECT_EQ(semver_check_compat_str("1.2.0", "1.3.0"), COMPAT_MINOR);
}

TEST(Compat, MajorDiff) {
    EXPECT_EQ(semver_check_compat_str("1.0.0", "2.0.0"), COMPAT_INCOMPATIBLE);
}

TEST(Compat, InvalidInput) {
    EXPECT_EQ(semver_check_compat_str("invalid", "1.0.0"), COMPAT_INCOMPATIBLE);
    EXPECT_EQ(semver_check_compat_str("1.0.0", "invalid"), COMPAT_INCOMPATIBLE);
    EXPECT_EQ(semver_check_compat(nullptr, nullptr), COMPAT_INCOMPATIBLE);
}

/* ======================== 兼容性级别名称 ======================== */

TEST(Compat, LevelNames) {
    EXPECT_STREQ(compat_level_name(COMPAT_EXACT), "exact");
    EXPECT_STREQ(compat_level_name(COMPAT_PATCH), "patch-compatible");
    EXPECT_STREQ(compat_level_name(COMPAT_MINOR), "minor-compatible");
    EXPECT_STREQ(compat_level_name(COMPAT_INCOMPATIBLE), "incompatible");
    EXPECT_STREQ(compat_level_name(COMPAT_PRERELEASE), "prerelease-diff");
}

/* ======================== API 版本 ======================== */

TEST(ApiVersion, RegisterAndQuery) {
    ASSERT_EQ(api_version_register("test_api", 1, 5), 0);
    
    int major = 0, minor = 0;
    ASSERT_EQ(api_version_query("test_api", &major, &minor), 0);
    EXPECT_EQ(major, 1);
    EXPECT_EQ(minor, 5);
}

TEST(ApiVersion, QueryNotFound) {
    int major, minor;
    EXPECT_EQ(api_version_query("nonexistent", &major, &minor), -1);
}

TEST(ApiVersion, UpdateExisting) {
    api_version_register("update_api", 1, 0);
    api_version_register("update_api", 2, 0);
    
    int major, minor;
    api_version_query("update_api", &major, &minor);
    EXPECT_EQ(major, 2);
}

TEST(ApiVersion, CheckCompat) {
    api_version_register("check_api", 2, 3);
    
    /* 主版本相同, 次版本 >= 要求 */
    EXPECT_TRUE(api_version_check("check_api", 2, 3));
    EXPECT_TRUE(api_version_check("check_api", 2, 1));
    /* 次版本不够 */
    EXPECT_FALSE(api_version_check("check_api", 2, 5));
    /* 主版本不同 */
    EXPECT_FALSE(api_version_check("check_api", 1, 0));
    /* 不存在的 API */
    EXPECT_FALSE(api_version_check("no_such_api", 1, 0));
}

/* ======================== 版本字符串 ======================== */

TEST(Version, CurrentVersion) {
    const char *v = ponypp_version_string();
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "0.5.0");
}

/* ======================== 迁移提示 ======================== */

TEST(Migration, GetHints) {
    int count = 0;
    const MigrationHint *hints = migration_get_hints(0, 1, 0, 5, &count);
    EXPECT_GT(count, 0);
    if (hints) {
        /* 第一个提示应该是 0.1 -> 0.2 */
        EXPECT_EQ(hints[0].from_major, 0);
        EXPECT_EQ(hints[0].from_minor, 1);
    }
}

TEST(Migration, NoHints) {
    int count = 0;
    const MigrationHint *hints = migration_get_hints(1, 0, 1, 0, &count);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(hints, nullptr);
}
