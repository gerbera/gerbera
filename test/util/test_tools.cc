/*GRB*
    Gerbera - https://gerbera.io/

    test_tools.cc - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "util/grb_fs.h"
#include "util/grb_net.h"
#include "util/grb_time.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <gtest/gtest.h>

TEST(ToolsTest, simpleDate)
{
    std::string dt = "2009-08-22T18:51:40+0200";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-22T16:51:40Z");
    dt = "2009-08-22T18:51:40-0100";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-22T19:51:40Z");
    dt = "2009-08-22T18:51:40-0500";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-22T23:51:40Z");
    dt = "2009-08-22T18:51:40-0900";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-23T03:51:40Z");
    dt = "2009-08-22T18:51:40+0100";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-22T17:51:40Z");
    dt = "2009-08-22T00:51:40+0930";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-21T15:21:40Z");
    dt = "2009-08-22T00:01:41+0930";
    EXPECT_EQ(makeSimpleDate(dt), "2009-08-21T14:31:41Z");
}

TEST(ToolsTest, millisecondsToHMSF)
{
    EXPECT_EQ(millisecondsToHMSF(0), "0:00:00.000");
    EXPECT_EQ(millisecondsToHMSF(1 * 1000), "0:00:01.000");
    EXPECT_EQ(millisecondsToHMSF(62 * 1000), "0:01:02.000");
    EXPECT_EQ(millisecondsToHMSF((3600 + 60 + 2) * 1000 + 123), "1:01:02.123");
}

TEST(ToolsTest, readWriteBinaryFile)
{
    std::string source = "This is a test, \0, binary";
    auto testFile = fs::temp_directory_path() / "gerbera-binary-test";
    EXPECT_FALSE(fs::exists(testFile)) << "Can't test existing file";
    GrbFile(testFile).writeBinaryFile(reinterpret_cast<const std::byte*>(source.data()), source.size());
    auto result = GrbFile(testFile).readBinaryFile();
    EXPECT_TRUE(result.has_value());

    std::vector<std::byte> expected(source.size());
    std::transform(source.begin(), source.end(), expected.begin(),
        [](char c) { return static_cast<std::byte>(c); });

    EXPECT_EQ(*result, expected);
    fs::remove(testFile);
}

TEST(ToolsTest, readBinaryReturnsEmptyIfFileMissing)
{
    fs::path testFile("/some/unexisting/file");
    EXPECT_FALSE(fs::exists(testFile));
    auto res = GrbFile(testFile).readBinaryFile();
    EXPECT_FALSE(res.has_value());
}

TEST(ToolsTest, writeFileThrowsIfCantOpenFile)
{
    fs::path testFile("/some/unexisting/file");
    EXPECT_FALSE(fs::exists(testFile));

    std::vector<std::byte> data { std::byte { 1 } };

    EXPECT_THROW(GrbFile(testFile).writeBinaryFile(data.data(), data.size()), std::runtime_error);
}

TEST(ToolsTest, parseIpV6)
{
    auto addr = std::make_shared<GrbNet>("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    EXPECT_TRUE(GrbNet("2001:0db8:85a3:0000:0000:8a2e:0370:7334%eth0").equals(addr));
}

TEST(ToolsTest, renderWebUriV4)
{
    EXPECT_EQ(GrbNet::renderWebUri("192.168.5.5", 7777), "192.168.5.5:7777");
}

TEST(ToolsTest, renderWebUriV6)
{
    EXPECT_EQ(GrbNet::renderWebUri("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 7777), "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:7777");
}

TEST(ToolsTest, pathTrimTest)
{
    std::string test = "/regular/path";
    EXPECT_EQ(rtrimPath(test), test);
    std::string test2 = "/regular/path//";
    EXPECT_EQ(rtrimPath(test2), test);
}

TEST(ToolsTest, subdirTest)
{
    std::string test = "/regular/path";
    std::string test2 = "/regular/path/sub/";
    EXPECT_EQ(isSubDir(test, test2), false);
    EXPECT_EQ(isSubDir(test2, test), true);
}

TEST(ToolsTest, subdirFileTest)
{
    std::string test = "/regular/path";
    std::string test2 = "/regular/path-sub";
    EXPECT_EQ(isSubDir(test, test2), false);
    EXPECT_EQ(isSubDir(test2, test), false);
}

TEST(ToolsTest, splitStringTest)
{
    auto parts = splitString("", ',');
    EXPECT_EQ(parts.size(), 0);

    parts = splitString("", ',', true);
    EXPECT_EQ(parts.size(), 1);

    parts = splitString("A", ',');
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "A");

    parts = splitString("A,", ',', false);
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "A");

    parts = splitString("A,", ',', true);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "");

    parts = splitString(",A", ',', true);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "A");

    parts = splitString("A,B,C", ',');
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "B");
    EXPECT_EQ(parts[2], "C");

    parts = splitString("A,,C", ',', false);
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "C");

    parts = splitString("A,,C", ',', true);
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "A");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "C");

    // Test splitString on last usecase where 'empty' is in use
    auto resourceString = "0~protocolInfo=http-get%3A%2A%3Aimage%2Fjpeg%3A%2A&resolution=3327x2039&size=732150~~|1~protocolInfo=http-get%3A%2A%3Aimage%2Fjpeg%3A%2A&resolution=170x256~rct=EX_TH~";
    auto resourceParts = splitString(resourceString, '|');
    ASSERT_EQ(resourceParts.size(), 2);
    auto parts0 = splitString(resourceParts[0], '~', true);
    auto parts1 = splitString(resourceParts[1], '~', true);
    ASSERT_EQ(parts0.size(), 4);
    ASSERT_EQ(parts1.size(), 4);
}

TEST(ToolsTest, trimStringTest)
{
    EXPECT_EQ(trimString(""), "");
    EXPECT_EQ(trimString("AB"), "AB");
    EXPECT_EQ(trimString("  AB  "), "AB");
}

TEST(ToolsTest, camelCaseStringTest)
{
    EXPECT_EQ(camelCaseString(""), "");
    EXPECT_EQ(camelCaseString("AB"), "AB");
    EXPECT_EQ(camelCaseString("upnp-test"), "upnpTest");
    EXPECT_EQ(camelCaseString("upnptest-"), "upnptest");
    EXPECT_EQ(camelCaseString("-upnptest"), "Upnptest");
}

TEST(ToolsTest, startswithTest)
{
    EXPECT_EQ(startswith("AB", "AB"), true);
    EXPECT_EQ(startswith("ABCD", "AB"), true);
    EXPECT_EQ(startswith("AB", "ABC"), false);
    EXPECT_EQ(startswith("ABC", "BC"), false);
    EXPECT_EQ(startswith("ABAB", "AB"), true);
}

TEST(ToolsTest, toLowerTest)
{
    EXPECT_EQ(toLower("AbC"), "abc");

    std::string test = "TestString";
    EXPECT_EQ(toLowerInPlace(test), "teststring");
    EXPECT_EQ(test, "teststring");
}

TEST(ToolsTest, hexEncodeTest)
{
    const std::uint8_t data[] = { 0x60, 0x0D, 0xC0, 0xDE };
    EXPECT_EQ(hexEncode(data, sizeof(data)), "600dc0de");

    EXPECT_EQ(hexEncode(nullptr, 0), "");
}

TEST(ToolsTest, hexDecodeTest)
{
    EXPECT_EQ(hexDecodeString("476f6f64436f6465"), "GoodCode");
}

TEST(ToolsTest, dictEncodeTest)
{
    std::map<std::string, std::string> values;
    values.emplace("Key 1", "Value 1");
    values.emplace("Key2", "Value 2");
    EXPECT_EQ(URLUtils::dictEncode(values), "Key%201=Value%201&Key2=Value%202");
}

TEST(ToolsTest, dictDecodeTest)
{
    std::map<std::string, std::string> values = URLUtils::dictDecode("Key%201=Value%201&Key2=Value%202");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key2"], "Value 2");

    values = URLUtils::dictDecode("");
    EXPECT_EQ(values.size(), 0);
}

TEST(ToolsTest, pathToMapTest)
{
    std::map<std::string, std::string> values = URLUtils::pathToMap("Key 1/Value 1/Key 2/Value 2/Key3//");
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "Value 2");
    EXPECT_EQ(values["Key3"], "");

    values = URLUtils::pathToMap("Key 1/Value 1/Key 2/");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "");

    values = URLUtils::pathToMap("Key 1/Value 1/Key 2");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key 2"], "");

    values = URLUtils::pathToMap("Key 1/");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values["Key 1"], "");

    values = URLUtils::pathToMap("/");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[""], "");

    values = URLUtils::pathToMap("/meh");
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[""], "meh");
}

TEST(ToolsTest, parseTimeTest)
{
    std::string value = "01:01:01:01";
    int result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, 90061);
    EXPECT_EQ(value, "90061");

    value = "01:01:01";
    result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, 3661);
    EXPECT_EQ(value, "3661");

    value = "51:01";
    result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, 3061);
    EXPECT_EQ(value, "3061");

    value = "61:01";
    result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, 3661);
    EXPECT_EQ(value, "3661");

    value = "10";
    result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, 10);
    EXPECT_EQ(value, "10");

    value = "-10";
    result = -1;
    ASSERT_EQ(parseTime(result, value), true);
    EXPECT_EQ(result, -10);
    EXPECT_EQ(value, "-10");

    value = "-51:01";
    result = -1;
    ASSERT_EQ(parseTime(result, value), false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, "-51:01");

    value = "51:x";
    result = -1;
    ASSERT_EQ(parseTime(result, value), false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, "51:x");

    value = "51:x1";
    result = -1;
    ASSERT_EQ(parseTime(result, value), false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, "51:x1");

    value = "1x:1x";
    result = -1;
    ASSERT_EQ(parseTime(result, value), false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(value, "1x:1x");
}
