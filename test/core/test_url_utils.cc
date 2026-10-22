/*GRB*

    Gerbera - https://gerbera.io/

    test_request_handler.cc - this file is part of Gerbera.

    Copyright (C) 2021-2026 Gerbera Contributors

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
#include <map>

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

TEST_F(URLUtilsTest, dictEncodeTest)
{
    std::map<std::string, std::string> values;
    values.emplace("Key 1", "Value 1");
    values.emplace("Key2", "Value 2");
    EXPECT_EQ(URLUtils::dictEncode(values), "Key%201=Value%201&Key2=Value%202");
}

TEST_F(URLUtilsTest, dictDecodeTest)
{
    std::map<std::string, std::string> values = URLUtils::dictDecode("Key%201=Value%201&Key2=Value%202");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key2"], "Value 2");

    values = URLUtils::dictDecode("");
    EXPECT_EQ(values.size(), 0);
}

TEST_F(URLUtilsTest, pathToMapTest)
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

TEST_F(URLUtilsTest, ResolvesRfc3986ReferencesAndPreservesQueryAndFragment)
{
    const std::map<std::string, std::string> examples {
        { "g", "http://a/b/c/g" },
        { "./g", "http://a/b/c/g" },
        { "g/", "http://a/b/c/g/" },
        { "/g", "http://a/g" },
        { "//g", "http://g" },
        { "?y", "http://a/b/c/d;p?y" },
        { "g?y", "http://a/b/c/g?y" },
        { "#s", "http://a/b/c/d;p?q#s" },
        { "g#s", "http://a/b/c/g#s" },
        { "g?y#s", "http://a/b/c/g?y#s" },
        { ";x", "http://a/b/c/;x" },
        { "g;x", "http://a/b/c/g;x" },
        { "g;x?y#s", "http://a/b/c/g;x?y#s" },
        { ".", "http://a/b/c/" },
        { "./", "http://a/b/c/" },
        { "..", "http://a/b/" },
        { "../", "http://a/b/" },
        { "../g", "http://a/b/g" },
        { "../..", "http://a/" },
        { "../../", "http://a/" },
        { "../../g", "http://a/g" },
        { "../../../g", "http://a/g" },
        { "/./g", "http://a/g" },
        { "/../g", "http://a/g" },
        { "g/./h", "http://a/b/c/g/h" },
        { "g/../h", "http://a/b/c/h" },
        { "g;x=1/../y", "http://a/b/c/y" },
        { "g?y/./x", "http://a/b/c/g?y/./x" },
        { "g#s/../x", "http://a/b/c/g#s/../x" },
        { "http://g/a/../b", "http://g/b" },
        { "http://g//a/./b", "http://g//a/b" },
    };
    for (const auto& [reference, expected] : examples) {
        SCOPED_TRACE(reference);
        EXPECT_EQ(URLUtils::resolveUriReference(reference, "http://a/b/c/d;p?q"),
            expected);
    }
}

TEST_F(URLUtilsTest, NativeMediaPathsPreserveLiteralReservedCharacters)
{
    const auto folder = fs::absolute(fs::temp_directory_path());
    const auto base = URLUtils::filePathToUri(folder / "list with % and ?.xspf");
    EXPECT_EQ(URLUtils::resolveMediaReference("a%20?b#c.wav", base, true),
        (folder / "a%20?b#c.wav").lexically_normal().string());
}

TEST_F(URLUtilsTest, FileUriReferencesRejectInvalidEncodingAndRemoteAuthorities)
{
    const auto base = URLUtils::filePathToUri(fs::temp_directory_path() / "list.xspf");
    for (const auto* reference :
        { "file:///a%00b.wav", "file:///a%GG.wav", "file://remote/a.wav" }) {
        SCOPED_TRACE(reference);
        EXPECT_FALSE(URLUtils::resolveMediaReference(reference, base, false));
    }
}

TEST_F(URLUtilsTest, HttpMediaReferencesKeepEncodedQueryAndFragment)
{
    EXPECT_EQ(URLUtils::resolveMediaReference(
                  "../a.wav?token=a%26b+c#part",
                  "https://example.invalid/music/list.xspf", false),
        "https://example.invalid/a.wav?token=a%26b+c#part");
}

TEST_F(URLUtilsTest, SharedDecoderKeepsFormAndFilePathSemanticsSeparate)
{
    EXPECT_EQ(URLUtils::urlUnescape("a+b%20c%2Fd"), "a b c/d");
    EXPECT_EQ(URLUtils::urlUnescape("a%"), "a");
    EXPECT_EQ(URLUtils::urlUnescape("a%2"), "a");
    EXPECT_EQ(URLUtils::urlUnescape("%GG"), std::string(1, '\0'));
    EXPECT_EQ(URLUtils::urlUnescape("a+b%20c%2fd", true), "a+b c/d");
    EXPECT_EQ(URLUtils::urlUnescape("%C3%A4", true), "\xC3\xA4");
    for (const auto* value : { "%", "%2", "%GG", "%00", "%C3%28", "%FF" }) {
        SCOPED_TRACE(value);
        EXPECT_FALSE(URLUtils::urlUnescape(value, true));
    }
}

TEST_F(URLUtilsTest, SharedEncoderPreservesLegacyTextAndEncodesUriPaths)
{
    EXPECT_EQ(URLUtils::urlEscape("a/b~c:d+e%?\xC3\xA4"),
        "a%2Fb%7Ec%3Ad%2Be%25%3F\xC3\xA4");
    EXPECT_EQ(URLUtils::urlEscape("a/b~c:d+e%?\xC3\xA4", true),
        "a/b~c:d%2Be%25%3F%C3%A4");
}

TEST_F(URLUtilsTest, UriSplittingPreservesExplicitEmptyQueryAndFragment)
{
    EXPECT_EQ(URLUtils::resolveUriReference("?", "https://a/b?old#old"), "https://a/b?");
    EXPECT_EQ(URLUtils::resolveUriReference("#", "https://a/b?old#old"), "https://a/b?old#");
    EXPECT_EQ(URLUtils::resolveUriReference("?#", "https://a/b?old#old"), "https://a/b?#");
    EXPECT_EQ(URLUtils::resolveUriReference("//b?x/y#z", "https://a/file"), "https://b?x/y#z");
}
