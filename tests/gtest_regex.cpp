/*
 * Pony++ Regex Tests
 */

#include <gtest/gtest.h>
#include <ponypp/regex.h>
#include <cstring>

TEST(Regex, CompileValid) {
    PnyRegex *re = pny_regex_compile("[0-9]+", 0);
    ASSERT_NE(re, nullptr);
    EXPECT_EQ(pny_regex_error(re), nullptr);  /* 无错误 */
    pny_regex_free(re);
}

TEST(Regex, CompileInvalid) {
    PnyRegex *re = pny_regex_compile("[unclosed", 0);
    ASSERT_NE(re, nullptr);
    EXPECT_NE(pny_regex_error(re), nullptr);  /* 有错误信息 */
    pny_regex_free(re);
}

TEST(Regex, MatchSimple) {
    PnyRegex *re = pny_regex_compile("[0-9]+", 0);
    ASSERT_NE(re, nullptr);
    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS];
    int n = pny_regex_match(re, "abc123def", 9, matches);
    EXPECT_GT(n, 0);
    EXPECT_EQ(matches[0].so, 3);
    EXPECT_EQ(matches[0].eo, 6);
    pny_regex_free(re);
}

TEST(Regex, MatchCaptureGroups) {
    PnyRegex *re = pny_regex_compile("(\\w+)@(\\w+)\\.com", 0);
    ASSERT_NE(re, nullptr);
    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS];
    const char *email = "user@example.com";
    int n = pny_regex_match(re, email, strlen(email), matches);
    EXPECT_GE(n, 3);  /* 整体 + 2个捕获组 */
    EXPECT_EQ(matches[0].so, 0);
    EXPECT_EQ(matches[0].eo, 16);
    /* 组1: user */
    EXPECT_EQ(matches[1].so, 0);
    EXPECT_EQ(matches[1].eo, 4);
    /* 组2: example */
    EXPECT_EQ(matches[2].so, 5);
    EXPECT_EQ(matches[2].eo, 12);
    pny_regex_free(re);
}

TEST(Regex, NoMatch) {
    PnyRegex *re = pny_regex_compile("^xyz$", 0);
    ASSERT_NE(re, nullptr);
    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS];
    EXPECT_EQ(pny_regex_match(re, "hello", 5, matches), PNY_REGEX_NOMATCH);
    pny_regex_free(re);
}

TEST(Regex, IsMatch) {
    PnyRegex *re = pny_regex_compile("^hello", 0);
    ASSERT_NE(re, nullptr);
    EXPECT_TRUE(pny_regex_is_match(re, "hello world", 11));
    EXPECT_FALSE(pny_regex_is_match(re, "world hello", 11));
    pny_regex_free(re);
}

TEST(Regex, CaseInsensitive) {
    PnyRegex *re = pny_regex_compile("^hello$", PNY_REGEX_ICASE);
    ASSERT_NE(re, nullptr);
    EXPECT_TRUE(pny_regex_is_match(re, "HELLO", 5));
    EXPECT_TRUE(pny_regex_is_match(re, "HeLLo", 5));
    pny_regex_free(re);
}

TEST(Regex, ReplaceFirst) {
    PnyRegex *re = pny_regex_compile("[0-9]+", 0);
    ASSERT_NE(re, nullptr);
    char out[64];
    int n = pny_regex_replace_first(re, "a1b22c333", 9, "#", out, sizeof(out));
    EXPECT_EQ(n, 1);
    EXPECT_STREQ(out, "a#b22c333");
    pny_regex_free(re);
}

TEST(Regex, ReplaceAll) {
    PnyRegex *re = pny_regex_compile("[0-9]+", 0);
    ASSERT_NE(re, nullptr);
    char out[64];
    int n = pny_regex_replace_all(re, "a1b22c333", 9, "#", out, sizeof(out));
    EXPECT_EQ(n, 3);
    EXPECT_STREQ(out, "a#b#c#");
    pny_regex_free(re);
}

TEST(Regex, ReplaceBackreference) {
    /* $1 反向引用: 交换名和姓 */
    PnyRegex *re = pny_regex_compile("(\\w+) (\\w+)", 0);
    ASSERT_NE(re, nullptr);
    char out[64];
    int n = pny_regex_replace_first(re, "John Smith", 10, "$2 $1", out, sizeof(out));
    EXPECT_EQ(n, 1);
    EXPECT_STREQ(out, "Smith John");
    pny_regex_free(re);
}

TEST(Regex, ZeroWidthMatchSafety) {
    /* 零宽匹配不应死循环 */
    PnyRegex *re = pny_regex_compile("x*", 0);
    ASSERT_NE(re, nullptr);
    char out[64];
    int n = pny_regex_replace_all(re, "abc", 3, "-", out, sizeof(out));
    EXPECT_GT(n, 0);
    pny_regex_free(re);
}

TEST(Regex, ConvenienceMatchStr) {
    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS];
    int n = pny_regex_match_str("[a-z]+@[a-z]+", 0, "mail: bob@home here", 19, matches);
    EXPECT_GT(n, 0);
    EXPECT_EQ(matches[0].so, 6);
    EXPECT_EQ(matches[0].eo, 14);
}

TEST(Regex, NullSafety) {
    EXPECT_EQ(pny_regex_compile(nullptr, 0), nullptr);
    PnyRegex *re = pny_regex_compile("a", 0);
    ASSERT_NE(re, nullptr);
    EXPECT_FALSE(pny_regex_is_match(re, nullptr, 0));
    EXPECT_EQ(pny_regex_match(re, nullptr, 0, nullptr), PNY_REGEX_BAD_ARG);
    EXPECT_NE(pny_regex_error(nullptr), nullptr);
    pny_regex_free(nullptr);  /* 不应崩溃 */
    pny_regex_free(re);
}
