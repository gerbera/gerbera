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

#include "util/tools.h"
#include "util/xml_playlist.h"

#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>
#include <map>
#include <pugixml.hpp>

class XmlPlaylistTest : public ::testing::TestWithParam<std::string> { };

static std::vector<XmlPlaylistEntry> readXmlPlaylist(const fs::path& path, XmlPlaylistType type)
{
    auto contents = GrbFile(path).readBinaryFile();
    if (!contents)
        throw std::runtime_error("Cannot open XML fixture");
    pugi::xml_document doc;
    auto result = doc.load_buffer(contents->data(), contents->size(), pugi::parse_default | pugi::parse_doctype);
    if (!result)
        throw std::runtime_error(result.description());
    return readXmlPlaylist(doc, path, type);
}

static std::vector<std::string> cases()
{
    std::vector<std::string> result;
    for (int n = 1; n <= 20; ++n)
        result.push_back("w" + std::string(n < 10 ? "0" : "") + std::to_string(n) + ".wpl");
    for (int n = 1; n <= 25; ++n)
        result.push_back("x" + std::string(n < 10 ? "0" : "") + std::to_string(n) + ".xspf");
    return result;
}

TEST_P(XmlPlaylistTest, ResolvesDeclaredPositionsAndRejectsInvalidDocuments)
{
    const fs::path folder = XML_PLAYLIST_FIXTURES;
    Json::Value manifest;
    std::ifstream input(folder / "cases.json");
    ASSERT_TRUE(input.good());
    input >> manifest;
    Json::Value test;
    for (const auto& candidate : manifest["cases"]) {
        if (candidate["file"].asString() == GetParam())
            test = candidate;
    }
    ASSERT_FALSE(test.isNull());
    SCOPED_TRACE(test["purpose"].asString());
    auto type = GetParam()[0] == 'w' ? XmlPlaylistType::Wpl : XmlPlaylistType::Xspf;
    if (test["outcome"].asString() != "import") {
        EXPECT_THROW(readXmlPlaylist(folder / GetParam(), type), std::runtime_error);
        return;
    }
    std::map<std::string, std::string> known;
    for (const auto& key : manifest["media"].getMemberNames()) {
        auto path = manifest["media"][key].asString();
        known[(folder.parent_path() / path).lexically_normal().string()] = key;
        known[(fs::path("/fixture/xml") / path).string()] = key;
    }
    known["https://media.example.invalid/a.mp3?x=1&y=2"] = "HTTPS";
    known["https://media.example.invalid/a.mp3?token=one%26two&x=1"] = "HTTPS";
    known["https://media.example.invalid/media/a.wav"] = "RELATIVE_HTTPS";
    known["https://media.example.invalid/a.mp3?title=Why%3F&token=a+b"] = "HTTPS";
    auto entries = readXmlPlaylist(folder / GetParam(), type);
    std::vector<std::string> actual;
    for (const auto& entry : entries) {
        for (const auto& location : entry.locations) {
            if (auto found = known.find(location); found != known.end()) {
                actual.push_back(found->second);
                break;
            }
            // A missing local candidate is deliberately present in fallback cases.
            EXPECT_EQ(fs::path(location).filename(), "missing.wav") << location;
        }
    }
    std::vector<std::string> expected;
    for (const auto& key : test["expected"])
        expected.push_back(key.asString());
    EXPECT_EQ(actual, expected);
}

INSTANTIATE_TEST_SUITE_P(Corpus, XmlPlaylistTest, ::testing::ValuesIn(cases()),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param.substr(0, 3); });

TEST(XmlPlaylistReaderTest, RetainsAlternativeLocationsWithoutCreatingExtraTracks)
{
    auto entries = readXmlPlaylist(fs::path(XML_PLAYLIST_FIXTURES) / "x09.xspf", XmlPlaylistType::Xspf);
    ASSERT_EQ(entries.size(), 2);
    ASSERT_EQ(entries[0].locations.size(), 3);
    EXPECT_EQ(fs::path(entries[0].locations[0]).filename(), "missing.wav");
    EXPECT_EQ(fs::path(entries[0].locations[1]).filename(), "a.wav");
    EXPECT_EQ(fs::path(entries[0].locations[2]).filename(), "b.wav");
}

TEST(XmlPlaylistReaderTest, ResolvesRfc3986ReferencesAndPreservesQueryAndFragment)
{
    struct TemporaryXml {
        fs::path path = fs::temp_directory_path() / ("grb-xml-" + generateRandomId() + ".xspf");
        ~TemporaryXml()
        {
            std::error_code ec;
            fs::remove(path, ec);
        }
    } file;
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
        pugi::xml_document doc;
        auto root = doc.append_child("playlist");
        root.append_attribute("version") = "1";
        root.append_attribute("xmlns") = "http://xspf.org/ns/0/";
        root.append_attribute("xml:base") = "http://a/b/c/d;p?q";
        root.append_child("trackList").append_child("track").append_child("location").text() = reference.c_str();
        ASSERT_TRUE(doc.save_file(file.path.c_str()));
        auto entries = readXmlPlaylist(file.path, XmlPlaylistType::Xspf);
        ASSERT_EQ(entries.size(), 1);
        EXPECT_EQ(entries[0].locations, (std::vector<std::string> { expected }));
    }
}
