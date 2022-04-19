#include <gtest/gtest.h>

#include "metadata//resolution.h"

TEST(ResolutionTest, parse) {
    auto res = Resolution("122586668x54589448448485");

    EXPECT_EQ(res.x(), 122586668);
    EXPECT_EQ(res.y(), 54589448448485);
}

TEST(ResolutionTest, parseWithSpace) {
    auto res = Resolution("1x1     ");

    EXPECT_EQ(res.x(), 1);
    EXPECT_EQ(res.y(), 1);
}

TEST(ResolutionTest, parseWithSpace2) {
    auto res = Resolution("            1             x          1     ");

    EXPECT_EQ(res.x(), 1);
    EXPECT_EQ(res.y(), 1);
}

TEST(ResolutionTest, fromNumbers) {
    auto res = Resolution(123, 456);

    EXPECT_EQ(res.x(), 123);
    EXPECT_EQ(res.y(), 456);
    EXPECT_EQ(res.string(), "123x456");
}

TEST(ResolutionTest, throwOnBad) {
    EXPECT_THROW(Resolution("123"), std::runtime_error);
    EXPECT_THROW(Resolution("123x"), std::runtime_error);
    EXPECT_THROW(Resolution("           x123"), std::runtime_error);
    EXPECT_THROW(Resolution("0x"), std::runtime_error);
    EXPECT_THROW(Resolution("0x1"), std::runtime_error);
    EXPECT_THROW(Resolution("1x0"), std::runtime_error);
}