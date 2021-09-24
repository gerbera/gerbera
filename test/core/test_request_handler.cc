/*GRB*
  Gerbera - https://gerbera.io/

  test_request_handler.cc - this file is part of Gerbera.

  Copyright (C) 2021 Gerbera Contributors

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

#include "request_handler.h"

#include <gtest/gtest.h>

using namespace testing;

class RequestHandlerTest : public ::testing::Test {

public:
    RequestHandlerTest() = default;
    ~RequestHandlerTest() override = default;
};

TEST_F(RequestHandlerTest, SplitUrlTest)
{
    EXPECT_THROW(RequestHandler::splitUrl("123X456", 'X'), std::runtime_error);

    auto&& [path, parameters] = RequestHandler::splitUrl("content/media?object_id=12345&transcode=wav", '?');
    EXPECT_EQ(path, "content/media");
    EXPECT_EQ(parameters, "object_id=12345&transcode=wav");

    std::tie(path, parameters) = RequestHandler::splitUrl("a/very/long/path", '/');
    EXPECT_EQ(path, "a/very/long");
    EXPECT_EQ(parameters, "path");
}

TEST_F(RequestHandlerTest, JoinUrlTest)
{
    auto components = std::vector<std::string> {
        "A", "B", "C", "D"
    };
    auto url = RequestHandler::joinUrl(components, false);
    EXPECT_EQ(url, "/A/B/C/D");

    url = RequestHandler::joinUrl(components, false, "&");
    EXPECT_EQ(url, "&A&B&C&D");

    url = RequestHandler::joinUrl(components, true);
    EXPECT_EQ(url, "/A/B/C/D/");

    url = RequestHandler::joinUrl({}, true);
    EXPECT_EQ(url, "/");
}

TEST_F(RequestHandlerTest, ParseParametersTest)
{
    auto m = RequestHandler::parseParameters("url/paramA/value%201/paramB/2", "url/");
    ASSERT_EQ(m.size(), 2);
    EXPECT_EQ(m["paramA"], "value 1");
    EXPECT_EQ(m["paramB"], "2");
}
