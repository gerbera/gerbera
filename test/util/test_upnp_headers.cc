/*GRB*
    Gerbera - https://gerbera.io/

    test_upnp_headers.cc - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include "util/upnp_headers.h"

class HeadersHelperTest : public ::testing::Test {

public:
    HeadersHelperTest()
#if defined(USING_NPUPNP)
        : info(new UpnpFileInfo)
#else
        : info(UpnpFileInfo_new())
#endif
    {
    }
    ~HeadersHelperTest() override
    {
#if defined(USING_NPUPNP)
        delete info;
#else
        UpnpFileInfo_delete(info);
#endif
    }

    UpnpFileInfo* info;
    Headers subject {};
};

#if !defined(USING_NPUPNP)
TEST_F(HeadersHelperTest, TerminatesTheHeaderWithCarriageNewLine)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"";
    std::map<std::string, std::string> expected;
    expected[key] = value;

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, DoesNotAddTerminationCarriageNewLineWhenAlreadyExists)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\n";
    std::map<std::string, std::string> expected;
    expected[key] = "attachment; filename=\"file.mp3\"";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, MultipleHeaders)
{
    // arrange
    std::map<std::string, std::string> expected;
    expected["Content-Disposition"] = "attachment; filename=\"file.mp3\"";
    expected["Accept-Ranges"] = "bytes";

    // act
    for (auto&& add : expected) {
        subject.addHeader(add.first, add.second);
    }
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, MultipleHeadersSingleCarriageNewLine)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"";
    std::string key2 = "Accept-Ranges";
    std::string value2 = "bytes\r\n";
    std::map<std::string, std::string> expected;
    expected[key] = value;
    expected[key2] = "bytes";

    // act
    subject.addHeader(key, value);
    subject.addHeader(key2, value2);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, MultiBothCarriageNewLine)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\n";
    std::string key2 = "Accept-Ranges";
    std::string value2 = "bytes\r\n";
    std::map<std::string, std::string> expected;
    expected[key] = "attachment; filename=\"file.mp3\"";
    expected[key2] = "bytes";

    // act
    subject.addHeader(key, value);
    subject.addHeader(key2, value2);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, IgnoresDataAfterFirstCarriageNewLine)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r\nAccept-Ranges: bytes";
    std::map<std::string, std::string> expected;
    expected[key] = "attachment; filename=\"file.mp3\"";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HeaderIsOnlyLinebreakReturnsEmpty)
{
    // arrange
    std::string header = "\r\n";
    std::map<std::string, std::string> expected;

    // act
    subject.addHeader(header, header);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HeaderIsEmptyReturnsEmpty)
{
    // arrange
    std::string header = "";
    std::map<std::string, std::string> expected;

    // act
    subject.addHeader(header, header);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HandlesSingleCarriageReturn)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\r";
    std::map<std::string, std::string> expected;
    expected[key] = "attachment; filename=\"file.mp3\"";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HandlesSingleNewLine)
{
    // arrange
    std::string key = "Content-Disposition";
    std::string value = "attachment; filename=\"file.mp3\"\n";
    std::map<std::string, std::string> expected;
    expected[key] = "attachment; filename=\"file.mp3\"";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, EmptyValueNotAdded)
{
    // arrange
    std::string key = "a";
    std::map<std::string, std::string> expected;

    // act
    subject.addHeader(key, "");
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HandlesEmptyString)
{
    // arrange
    std::string header;
    std::map<std::string, std::string> expected;

    // act
    subject.addHeader(header, header);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HandlesExtraContent)
{
    // arrange
    std::string key = "foo";
    std::string value = "bar\r\nzoo: wombat\r\n";
    std::map<std::string, std::string> expected;
    expected[key] = "bar";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}

TEST_F(HeadersHelperTest, HandlesExtraContentTwo)
{
    // arrange
    std::string key = "foo";
    std::string value = "bar\r\nzoo: wombat\r\nwhere: somewhere";
    std::map<std::string, std::string> expected;
    expected[key] = "bar";

    // act
    subject.addHeader(key, value);
    subject.writeHeaders(info);

    // assert
    auto actual = Headers::readHeaders(info);
    EXPECT_EQ(actual, expected);
}
#endif
