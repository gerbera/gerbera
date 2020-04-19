#include "util/tools.h"

#include <gtest/gtest.h>

using namespace ::testing;

TEST(ToolsTest, secondsToHMS)
{
    EXPECT_EQ(secondsToHMS(0), "00:00:00");
    EXPECT_EQ(secondsToHMS(1), "00:00:01");
    EXPECT_EQ(secondsToHMS(62), "00:01:02");
    EXPECT_EQ(secondsToHMS(3662), "01:01:02");
}

TEST(ToolsTest, secondsToHMSOverflow)
{
    EXPECT_EQ(secondsToHMS(1000 * 3600 + 1), "999:00:01");
}
