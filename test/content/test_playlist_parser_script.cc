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

#include "content/scripting/xml_playlist.h"
#include "util/tools.h"

#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>
#include <map>
#include <pugixml.hpp>

class XmlPlaylistTest : public ::testing::TestWithParam<std::string> { };

static std::vector<XmlPlaylistEntry> readXmlPlaylist(const fs::path& path,
    XmlPlaylistType type)
{
    auto contents = GrbFile(path).readBinaryFile();
    if (!contents)
        throw std::runtime_error("Cannot open XML fixture");
    auto playlist = XmlPlaylist::create(type);
    pugi::xml_document doc;
    auto result = doc.load_buffer(contents->data(), contents->size(),
        playlist->getParseFlags());
    if (!result)
        throw std::runtime_error(result.description());
    playlist->read(doc, path);
    std::vector<XmlPlaylistEntry> entries;
    while (auto entry = playlist->readPlaylistEntry())
        entries.push_back(entry->get());
    return entries;
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
        EXPECT_THROW(readXmlPlaylist(folder / GetParam(), type),
            std::runtime_error);
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
    [](const ::testing::TestParamInfo<std::string>& info) {
        return info.param.substr(0, 3);
    });

TEST(XmlPlaylistReaderTest,
    RetainsAlternativeLocationsWithoutCreatingExtraTracks)
{
    auto entries = readXmlPlaylist(fs::path(XML_PLAYLIST_FIXTURES) / "x09.xspf",
        XmlPlaylistType::Xspf);
    ASSERT_EQ(entries.size(), 2);
    ASSERT_EQ(entries[0].locations.size(), 3);
    EXPECT_EQ(fs::path(entries[0].locations[0]).filename(), "missing.wav");
    EXPECT_EQ(fs::path(entries[0].locations[1]).filename(), "a.wav");
    EXPECT_EQ(fs::path(entries[0].locations[2]).filename(), "b.wav");
}

TEST(XmlPlaylistReaderTest, ReusingReaderReplacesPreviousPositions)
{
    WplPlaylist playlist;
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string("<smil><body><seq><media src='a.wav'/></seq></body></smil>"));
    playlist.read(doc, fs::temp_directory_path() / "list.wpl");
    ASSERT_EQ(playlist.entries.size(), 1);
    ASSERT_TRUE(doc.load_string("<smil><body><seq/></body></smil>"));
    playlist.read(doc, fs::temp_directory_path() / "list.wpl");
    EXPECT_TRUE(playlist.entries.empty());
    ASSERT_TRUE(doc.load_string("<invalid/>"));
    EXPECT_THROW(playlist.read(doc, fs::temp_directory_path() / "list.wpl"), std::runtime_error);
    EXPECT_TRUE(playlist.entries.empty());
}

static AsxPlaylistDocument skipAsxEntryRef(const std::string&, const fs::path&)
{
    return { {}, {}, "ENTRYREF intentionally unavailable in this reader fixture" };
}

TEST(XmlPlaylistReaderTest, AsxIteratorOwnsDocumentAndPreservesLegacyFields)
{
    EXPECT_THROW(XmlPlaylist::create(XmlPlaylistType::Asx), std::runtime_error);
    EXPECT_THROW(XmlPlaylist::create(XmlPlaylistType::Asx, {}), std::runtime_error);
    auto playlist = XmlPlaylist::create(XmlPlaylistType::Asx, skipAsxEntryRef);
    EXPECT_EQ(playlist->getParseFlags(), pugi::parse_default);
    {
        pugi::xml_document doc;
        ASSERT_TRUE(doc.load_string(R"XML(<AsX version="3.0"><AUTHOR>Root author</AUTHOR><COPYRIGHT>Root rights</COPYRIGHT><EnTrY>
            <TiTlE>Äther 東京</TiTlE><AbStRaCt>Description</AbStRaCt>
            <ReF HrEf="https://media.example.invalid/a.mp3?token=a+b" WrItEtHrOuGh="2"/>
            <PaRaM NaMe="size" VaLuE="1672"/><param name="mimetype" value="audio/mpeg"/>
            <param name="protocol" value="http-get"/><param name="Category" value="Mix"/>
            </EnTrY></AsX>)XML"));
        playlist->read(doc, fs::path("/fixture/playlist.asx"));
    }
    auto entry = playlist->readPlaylistEntry();
    ASSERT_TRUE(entry);
    EXPECT_EQ(entry->get().locations, (std::vector<std::string> { "https://media.example.invalid/a.mp3?token=a+b" }));
    EXPECT_EQ(entry->get().title, "Äther 東京");
    EXPECT_EQ(entry->get().description, "Description");
    EXPECT_EQ(entry->get().size, "1672");
    EXPECT_EQ(entry->get().mimetype, "audio/mpeg");
    EXPECT_EQ(entry->get().protocol, "http-get");
    EXPECT_EQ(entry->get().writeThroughs, (std::vector<int> { 2 }));
    EXPECT_EQ(entry->get().defaults.at("upnp:author"), "Root author");
    EXPECT_EQ(entry->get().defaults.at("dc:rights"), "Root rights");
    EXPECT_EQ(entry->get().extra.at("Category"), "Mix");
    EXPECT_FALSE(playlist->readPlaylistEntry());
}

TEST(XmlPlaylistReaderTest, AsxIteratorKeepsEntryBaseAlternativesRepeatAndMetadata)
{
    AsxPlaylist playlist(skipAsxEntryRef);
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string(R"XML(<asx><title>First default</title>
        <base href="https://media.example.invalid/base"/>
        <entry><base href="https://ignored.example.invalid"/>
            <ref href="first.mp3" writethrough="1"/><ref HREF="ignored.mp3" href="last.mp3" writethrough="2"/>
        </entry>
        <entryref href="not-loaded.asx"/>
        <repeat count="3"><entry><ref href="not-expanded.mp3"/></entry></repeat>
        <entry><ref href="next.mp3"/></entry></asx>)XML"));
    playlist.read(doc, fs::path("/fixture/list.asx"));
    auto first = playlist.readPlaylistEntry();
    ASSERT_TRUE(first);
    EXPECT_EQ(first->get().locations, (std::vector<std::string> { "https://ignored.example.invalid/first.mp3", "https://ignored.example.invalid/last.mp3" }));
    EXPECT_TRUE(first->get().title.empty()); // The playlist title is not an entry default.
    EXPECT_EQ(first->get().writeThroughs, (std::vector<int> { 1, 2 }));
    for (int i = 0; i < 3; ++i) {
        auto repeated = playlist.readPlaylistEntry();
        ASSERT_TRUE(repeated);
        EXPECT_EQ(repeated->get().locations, (std::vector<std::string> { "https://media.example.invalid/base/not-expanded.mp3" }));
        EXPECT_TRUE(repeated->get().title.empty());
        EXPECT_EQ(repeated->get().writeThroughs, (std::vector<int> { -1 }));
    }
    auto second = playlist.readPlaylistEntry();
    ASSERT_TRUE(second);
    EXPECT_EQ(second->get().locations, (std::vector<std::string> { "https://media.example.invalid/base/next.mp3" }));
    EXPECT_TRUE(second->get().title.empty());
    EXPECT_EQ(second->get().writeThroughs, (std::vector<int> { -1 }));
    EXPECT_FALSE(playlist.readPlaylistEntry());
}

TEST(XmlPlaylistReaderTest, AsxIteratorUsesSharedUriValidationAndExternalProtocols)
{
    AsxPlaylist playlist(skipAsxEntryRef);
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string(R"XML(<asx>
        <entry><ref href="file:///fixture/Why%3F.wav"/></entry>
        <entry><ref href="file://remote/fixture/a.wav"/></entry>
        <entry><ref href="file:///fixture/Bad%GG.wav"/></entry>
        <entry><ref href="file:///fixture/Bad%00.wav"/></entry>
        <entry><ref href="https://media.example.invalid/a.mp3?name=Why%3F&amp;token=a+b"/></entry>
        <entry><ref href="mms://media.example.invalid/a.wmv?token=a+b"/></entry>
        </asx>)XML"));
    playlist.read(doc, fs::path("/fixture/list.asx"));
    auto local = playlist.readPlaylistEntry();
    ASSERT_TRUE(local);
    EXPECT_EQ(local->get().locations, (std::vector<std::string> { "/fixture/Why?.wav" }));
    auto https = playlist.readPlaylistEntry();
    ASSERT_TRUE(https);
    EXPECT_EQ(https->get().locations, (std::vector<std::string> { "https://media.example.invalid/a.mp3?name=Why%3F&token=a+b" }));
    auto mms = playlist.readPlaylistEntry();
    ASSERT_TRUE(mms);
    EXPECT_EQ(mms->get().locations, (std::vector<std::string> { "mms://media.example.invalid/a.wmv?token=a+b" }));
    EXPECT_FALSE(playlist.readPlaylistEntry());
}

TEST(XmlPlaylistReaderTest, ReusingAsxIteratorClearsStateAndRemainsAtEof)
{
    AsxPlaylist playlist(skipAsxEntryRef);
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string("<asx><entry><ref href='https://media.example.invalid/a.mp3'/></entry></asx>"));
    playlist.read(doc, fs::path("/fixture/list.asx"));
    ASSERT_TRUE(playlist.readPlaylistEntry());
    EXPECT_FALSE(playlist.readPlaylistEntry());
    EXPECT_FALSE(playlist.readPlaylistEntry());
    ASSERT_TRUE(doc.load_string("<asx/><asx/>"));
    EXPECT_THROW(playlist.read(doc, fs::path("/fixture/invalid.asx")), std::runtime_error);
    EXPECT_FALSE(playlist.readPlaylistEntry());
    ASSERT_TRUE(doc.load_string("<asx/>"));
    playlist.read(doc, fs::path("/fixture/empty.asx"));
    EXPECT_FALSE(playlist.readPlaylistEntry());
    ASSERT_TRUE(doc.load_string("<invalid/>"));
    EXPECT_THROW(playlist.read(doc, fs::path("/fixture/invalid.asx")), std::runtime_error);
    EXPECT_FALSE(playlist.readPlaylistEntry());
}
