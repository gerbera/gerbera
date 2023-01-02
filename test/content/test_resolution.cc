/*GRB*
    Gerbera - https://gerbera.io/

    test_resolution.cc - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/
#include <gtest/gtest.h>

#include "metadata//resolution.h"

TEST(ResolutionTest, parse) {
    auto res = Resolution("122586668x448448485");

    EXPECT_EQ(res.x(), 122586668);
    EXPECT_EQ(res.y(), 448448485);
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
