/*
 * SODA Player - String Utils Tests
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <gtest/gtest.h>
#include <soda/string_utils.hpp>

using namespace soda::string_utils;

TEST(StringUtilsTest, Trim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("hello"), "hello");
    EXPECT_EQ(trim(""), "");
    EXPECT_EQ(trim("  "), "");
}

TEST(StringUtilsTest, ToLower) {
    EXPECT_EQ(toLower("HELLO"), "hello");
    EXPECT_EQ(toLower("Hello World"), "hello world");
    EXPECT_EQ(toLower("123"), "123");
}

TEST(StringUtilsTest, ToUpper) {
    EXPECT_EQ(toUpper("hello"), "HELLO");
    EXPECT_EQ(toUpper("Hello World"), "HELLO WORLD");
}

TEST(StringUtilsTest, Split) {
    auto result = split("a,b,c", ',');
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST(StringUtilsTest, Join) {
    std::vector<std::string> parts = {"a", "b", "c"};
    EXPECT_EQ(join(parts, ","), "a,b,c");
    EXPECT_EQ(join(parts, " - "), "a - b - c");
}

TEST(StringUtilsTest, FormatDuration) {
    EXPECT_EQ(formatDuration(soda::Duration(0)), "0:00");
    EXPECT_EQ(formatDuration(soda::Duration(1000)), "0:01");
    EXPECT_EQ(formatDuration(soda::Duration(60000)), "1:00");
    EXPECT_EQ(formatDuration(soda::Duration(125000)), "2:05");
    EXPECT_EQ(formatDuration(soda::Duration(3661000)), "1:01:01");
}

TEST(StringUtilsTest, StartsWith) {
    EXPECT_TRUE(startsWith("hello world", "hello"));
    EXPECT_FALSE(startsWith("hello world", "world"));
    EXPECT_TRUE(startsWith("hello", "hello"));
}

TEST(StringUtilsTest, EndsWith) {
    EXPECT_TRUE(endsWith("hello world", "world"));
    EXPECT_FALSE(endsWith("hello world", "hello"));
    EXPECT_TRUE(endsWith("hello", "hello"));
}

TEST(StringUtilsTest, Contains) {
    EXPECT_TRUE(contains("hello world", "lo wo"));
    EXPECT_FALSE(contains("hello world", "xyz"));
}

TEST(StringUtilsTest, ReplaceAll) {
    EXPECT_EQ(replaceAll("aaa", "a", "b"), "bbb");
    EXPECT_EQ(replaceAll("hello world", " ", "_"), "hello_world");
}

TEST(StringUtilsTest, Slugify) {
    EXPECT_EQ(slugify("Hello World!"), "hello-world");
    EXPECT_EQ(slugify("Test  123"), "test-123");
    EXPECT_EQ(slugify("  spaces  "), "spaces");
}

TEST(StringUtilsTest, EscapeHtml) {
    EXPECT_EQ(escapeHtml("<div>"), "&lt;div&gt;");
    EXPECT_EQ(escapeHtml("\"quoted\""), "&quot;quoted&quot;");
    EXPECT_EQ(escapeHtml("a & b"), "a &amp; b");
}

TEST(StringUtilsTest, Base64) {
    std::string original = "Hello, World!";
    std::string encoded = base64Encode(original);
    std::string decoded = base64DecodeString(encoded);
    EXPECT_EQ(decoded, original);
}
