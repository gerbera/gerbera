/*GRB*
    Gerbera - https://gerbera.io/
    Copyright (C) 2026 Gerbera Contributors
    SPDX-License-Identifier: GPL-2.0-only

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "util/url_utils.h"

#include <gtest/gtest.h>
#include <map>

TEST(URLUtilsTest, dictEncodeTest)
{
    std::map<std::string, std::string> values;
    values.emplace("Key 1", "Value 1");
    values.emplace("Key2", "Value 2");
    EXPECT_EQ(URLUtils::dictEncode(values), "Key%201=Value%201&Key2=Value%202");
}

TEST(URLUtilsTest, dictDecodeTest)
{
    std::map<std::string, std::string> values = URLUtils::dictDecode("Key%201=Value%201&Key2=Value%202");
    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values["Key 1"], "Value 1");
    EXPECT_EQ(values["Key2"], "Value 2");

    values = URLUtils::dictDecode("");
    EXPECT_EQ(values.size(), 0);
}

TEST(URLUtilsTest, pathToMapTest)
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

TEST(URLUtilsTest, ResolvesRfc3986ReferencesAndPreservesQueryAndFragment)
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

TEST(URLUtilsTest, NativeMediaPathsPreserveLiteralReservedCharacters)
{
    const auto folder = fs::absolute(fs::temp_directory_path());
    const auto base = URLUtils::filePathToUri(folder / "list with % and ?.xspf");
    EXPECT_EQ(URLUtils::resolveMediaReference("a%20?b#c.wav", base, true),
        (folder / "a%20?b#c.wav").lexically_normal().string());
}

TEST(URLUtilsTest, FileUriReferencesRejectInvalidEncodingAndRemoteAuthorities)
{
    const auto base = URLUtils::filePathToUri(fs::temp_directory_path() / "list.xspf");
    for (const auto* reference :
        { "file:///a%00b.wav", "file:///a%GG.wav", "file://remote/a.wav" }) {
        SCOPED_TRACE(reference);
        EXPECT_FALSE(URLUtils::resolveMediaReference(reference, base, false));
    }
}

TEST(URLUtilsTest, HttpMediaReferencesKeepEncodedQueryAndFragment)
{
    EXPECT_EQ(URLUtils::resolveMediaReference(
                  "../a.wav?token=a%26b+c#part",
                  "https://example.invalid/music/list.xspf", false),
        "https://example.invalid/a.wav?token=a%26b+c#part");
}
