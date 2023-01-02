/*GRB*
  Gerbera - https://gerbera.io/

  test_request_handler.cc - this file is part of Gerbera.

  Copyright (C) 2021-2023 Gerbera Contributors

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

#include "util/url_utils.h"

#include <gtest/gtest.h>

class URLUtilsTest : public ::testing::Test {

public:
    URLUtilsTest() = default;
    ~URLUtilsTest() override = default;
};

TEST_F(URLUtilsTest, SplitUrlTest)
{
    EXPECT_THROW(URLUtils::splitUrl("123X456", 'X'), std::runtime_error);

    auto&& [path, parameters] = URLUtils::splitUrl("content/media?object_id=12345&transcode=wav", '?');
    EXPECT_EQ(path, "content/media");
    EXPECT_EQ(parameters, "object_id=12345&transcode=wav");

    std::tie(path, parameters) = URLUtils::splitUrl("a/very/long/path", '/');
    EXPECT_EQ(path, "a/very/long");
    EXPECT_EQ(parameters, "path");
}

TEST_F(URLUtilsTest, JoinUrlTest)
{
    auto components = std::vector<std::string> {
        "A", "B", "C", "D"
    };
    auto url = URLUtils::joinUrl(components, false);
    EXPECT_EQ(url, "/A/B/C/D");

    url = URLUtils::joinUrl(components, false, "&");
    EXPECT_EQ(url, "&A&B&C&D");

    url = URLUtils::joinUrl(components, true);
    EXPECT_EQ(url, "/A/B/C/D/");

    url = URLUtils::joinUrl({}, true);
    EXPECT_EQ(url, "/");
}

TEST_F(URLUtilsTest, ParseParametersTest)
{
    auto m = URLUtils::parseParameters("url/paramA/value%201/paramB/2", "url/");
    ASSERT_EQ(m.size(), 2);
    EXPECT_EQ(m["paramA"], "value 1");
    EXPECT_EQ(m["paramB"], "2");
}
