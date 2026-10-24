/*GRB*

    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

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

#include "playlist_import_test.h"

#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "common.h"
#include "content/content.h"
#include "content/import_service.h"
#include "context.h"
#include "database/database.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"
#include "util/grb_time.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

enum class PlaylistClassMapping {
    Defaults,
    Legacy,
    ExactOverride,
    ConditionalOverride,
    DisabledDefaults,
};

class PlaylistFileClassTest : public PlaylistImportTest,
                              public ::testing::WithParamInterface<std::tuple<std::string, PlaylistClassMapping>> {
protected:
    std::vector<std::pair<const char*, const char*>> playlists {
        std::pair { "one.m3u", "audio/x-mpegurl" },
        std::pair { "two.m3u8", "audio/x-mpegurl" },
        std::pair { "three.pls", "audio/x-scpls" },
        std::pair { "four.asx", MIME_TYPE_ASX_PLAYLIST },
    };

    void configureMappings(PlaylistClassMapping mode)
    {
        auto mappings = document.select_node("/config/import/mappings/mimetype-upnpclass").node();
        ASSERT_TRUE(mappings);
        auto extend = mappings.attribute("extend");
        if (!extend)
            extend = mappings.append_attribute("extend");
        extend = mode == PlaylistClassMapping::DisabledDefaults ? "no" : "yes";
        if (mode == PlaylistClassMapping::Legacy || mode == PlaylistClassMapping::DisabledDefaults) {
            mappings.remove_children();
            setMapping(mappings, "audio/*", UPNP_CLASS_MUSIC_TRACK);
            setMapping(mappings, "video/*", UPNP_CLASS_VIDEO_ITEM);
        }
        if (mode == PlaylistClassMapping::ExactOverride) {
            setMapping(mappings, "audio/x-mpegurl", UPNP_CLASS_AUDIO_BROADCAST);
            setMapping(mappings, MIME_TYPE_ASX_PLAYLIST, UPNP_CLASS_VIDEO_MOVIE);
        }
        if (mode == PlaylistClassMapping::ConditionalOverride) {
            setMapping(mappings, "audio/*;location=one", UPNP_CLASS_AUDIO_BROADCAST);
            setMapping(mappings, "audio/*;dc:title=Special", UPNP_CLASS_AUDIO_BROADCAST);
        }
    }

    static void setMapping(pugi::xml_node mappings, const char* mime, const char* upnpClass)
    {
        auto mapping = mappings.find_child_by_attribute("map", "from", mime);
        if (!mapping) {
            mapping = mappings.append_child("map");
            mapping.append_attribute("from") = mime;
            mapping.append_attribute("to");
        }
        mapping.attribute("to") = upnpClass;
    }

    void writePlaylists()
    {
        const auto directory = source / "pl";
        fs::create_directories(directory);
        writeBytes(directory / "one.m3u", "../audio/a.wav\n../audio/b.wav\n../audio/a.wav\n");
        writeBytes(directory / "two.m3u8", "../audio/a.wav\n../audio/b.wav\n../audio/a.wav\n");
        writeBytes(directory / "three.pls", "[playlist]\nFile1=../audio/a.wav\nFile2=../audio/b.wav\nFile3=../audio/a.wav\nNumberOfEntries=3\nVersion=2\n");
        writeBytes(directory / "four.asx", "<asx version=\"3.0\"><entry><ref href=\"../audio/a.wav\"/></entry><entry><ref href=\"../audio/b.wav\"/></entry><entry><ref href=\"../audio/a.wav\"/></entry></asx>");
        if (playlists.size() > 4) {
            writeBytes(directory / "five.wpl", "<smil><body><seq><media src=\"../audio/a.wav\"/><media src=\"../audio/b.wav\"/><media src=\"../audio/a.wav\"/></seq></body></smil>");
            writeBytes(directory / "six.xspf", "<playlist xmlns=\"http://xspf.org/ns/0/\" version=\"1\"><trackList><track><location>../audio/a.wav</location></track><track><location>../audio/b.wav</location></track><track><location>../audio/a.wav</location></track></trackList></playlist>");
        }
    }

    void prepareInputs()
    {
#ifdef HAVE_JS
        // Keep the optional combined-feature coverage without requiring #4009.
        const auto parser = fs::path(GERBERA_SOURCE_DIR) / "scripts/js/playlists.js";
        if (readBytes(parser).find("function readXmlPlaylist(") != std::string::npos) {
            playlists.emplace_back("five.wpl", "application/vnd.ms-wpl");
            playlists.emplace_back("six.xspf", "application/xspf+xml");
        }
#endif
        configureMappings(std::get<1>(GetParam()));
        fs::create_directories(source / "audio");
        writeWave(source / "audio/a.wav", "Track A");
        writeWave(source / "audio/b.wav", "Track B");
        writePlaylists();
    }

    void startAndWaitForImports()
    {
        start(std::get<0>(GetParam()));
        ASSERT_TRUE(waitFor([this] { return physicalImportsReady(); }, "physical playlist and audio imports", std::chrono::seconds(20)));
#ifdef HAVE_JS
        ASSERT_TRUE(waitFor([this] { return static_cast<bool>(playlistViewsMatch()); }, "both parsed playlist views, repeated positions and media resources", std::chrono::seconds(20))) << playlistViewsMatch().message();
        ASSERT_TRUE(playlistViewsMatch());
#endif
    }

    bool physicalImportsReady()
    {
        for (const auto& [name, mime] : playlists) {
            if (!database->findObjectByPath(source / "pl" / name, DEFAULT_CLIENT_GROUP, DbFileType::File))
                return false;
        }
        for (const auto* name : { "a.wav", "b.wav" }) {
            if (!database->findObjectByPath(source / "audio" / name, DEFAULT_CLIENT_GROUP, DbFileType::File))
                return false;
        }
        return true;
    }

#ifdef HAVE_JS
    ::testing::AssertionResult playlistViewsMatch()
    {
        if (!physicalImportsReady())
            return ::testing::AssertionFailure() << "Physical imports are incomplete";
        std::vector<int> pending;
        for (const auto& object : children(CDS_ID_ROOT)) {
            if (object->getTitle() == "Playlists")
                pending.push_back(object->getID());
        }
        if (pending.size() != 1)
            return ::testing::AssertionFailure() << "Expected exactly one Playlists root";
        std::map<std::string, int> actualNames;
        while (!pending.empty()) {
            auto id = pending.back();
            pending.pop_back();
            for (const auto& object : children(id)) {
                if (object->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER)
                    ++actualNames[object->getTitle()];
                else if (object->isContainer())
                    pending.push_back(object->getID());
            }
        }
        std::map<std::string, int> expectedNames;
        for (const auto& [name, mime] : playlists)
            expectedNames[fs::path(name).stem().string()] = 2;
        if (actualNames != expectedNames)
            return ::testing::AssertionFailure() << "Playlist container names/counts differ from the input file names";
        for (const auto& [name, mime] : playlists) {
            auto physical = database->findObjectByPath(source / "pl" / name, DEFAULT_CLIENT_GROUP, DbFileType::File);
            if (!physical)
                return ::testing::AssertionFailure() << name << ": physical playlist disappeared during import";
            auto views = playlistViews(physical->getID());
            if (views.size() != 2)
                return ::testing::AssertionFailure() << name << ": expected two playlist views, got " << views.size();
            for (int id : views) {
                if (database->loadObject(id)->getTitle() != fs::path(name).stem().string())
                    return ::testing::AssertionFailure() << name << ": playlist view has the wrong title";
                auto positions = children(id);
                if (positions.size() != 3)
                    return ::testing::AssertionFailure() << name << ": expected A,B,A, got " << positions.size() << " positions";
                for (std::size_t position = 0; position < positions.size(); ++position) {
                    auto expectedPath = source / "audio" / (position == 1 ? "b.wav" : "a.wav");
                    auto audio = database->findObjectByPath(expectedPath, DEFAULT_CLIENT_GROUP, DbFileType::File);
                    auto track = std::dynamic_pointer_cast<CdsItem>(positions[position]);
                    if (!track || !audio || track->getRefID() != audio->getID()
                        || track->getLocation() != expectedPath || track->getTitle() != (position == 1 ? "Track B" : "Track A")
                        || track->getTrackNumber() != static_cast<int>(position + 1))
                        return ::testing::AssertionFailure() << name << ": wrong repeated media entry at position " << position + 1;
                    auto resource = track->getResource(ResourcePurpose::Content);
                    if (!resource || resource->getAttribute(ResourceAttribute::SIZE) != std::to_string(fs::file_size(expectedPath))
                        || std::abs(HMSFToMilliseconds(resource->getAttribute(ResourceAttribute::DURATION)) - 100) >= 2)
                        return ::testing::AssertionFailure() << name << ": missing media size/duration at position " << position + 1;
                }
            }
        }
        return ::testing::AssertionSuccess();
    }

    std::vector<int> playlistViews(int sourceId)
    {
        std::vector<int> result;
        for (auto type : { CdsEntryType::VirtualContainer, CdsEntryType::PlaylistContainer }) {
            for (int id : database->getRefObjects(sourceId, type)) {
                auto container = database->loadObject(id);
                if (container && container->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER)
                    result.push_back(id);
            }
        }
        return result;
    }

#ifdef HAVE_CURL
    ::testing::AssertionResult playlistResourceBytesMatch()
    {
        for (const auto& [name, mime] : playlists) {
            auto physical = database->findObjectByPath(source / "pl" / name, DEFAULT_CLIENT_GROUP, DbFileType::File);
            if (!physical)
                return ::testing::AssertionFailure() << name << ": physical playlist disappeared during HTTP check";
            auto views = playlistViews(physical->getID());
            if (views.size() != 2)
                return ::testing::AssertionFailure() << name << ": HTTP check requires both playlist views";
            for (int id : views) {
                auto positions = children(id);
                if (positions.size() != 3)
                    return ::testing::AssertionFailure() << name << ": HTTP check requires A,B,A positions";
                for (const auto& track : positions) {
                    if (resourceBytes(track) != readBytes(track->getLocation()))
                        return ::testing::AssertionFailure() << name << ": HTTP bytes differ for " << track->getTitle();
                }
            }
        }
        return ::testing::AssertionSuccess();
    }
#endif
#endif
};

TEST_P(PlaylistFileClassTest, PhysicalClassesKeepMimeResourcesAndParsedPlaylistPositions)
{
    auto mapping = std::get<1>(GetParam());
    ASSERT_NO_FATAL_FAILURE(prepareInputs());
    std::map<fs::path, std::string> original;
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        if (entry.is_regular_file())
            original[entry.path()] = readBytes(entry.path());
    }
    ASSERT_NO_FATAL_FAILURE(startAndWaitForImports());
    for (const auto& [name, mime] : playlists) {
        SCOPED_TRACE(name);
        auto item = std::dynamic_pointer_cast<CdsItem>(database->findObjectByPath(source / "pl" / name, DEFAULT_CLIENT_GROUP, DbFileType::File));
        ASSERT_NE(item, nullptr);
        EXPECT_TRUE(item->isItem());
        auto expectedClass = UPNP_CLASS_PLAYLIST_ITEM;
        if (mapping == PlaylistClassMapping::DisabledDefaults) {
            expectedClass = UPNP_CLASS_ITEM;
            if (std::string(mime).find("audio/") == 0)
                expectedClass = UPNP_CLASS_MUSIC_TRACK;
            if (std::string(mime).find("video/") == 0)
                expectedClass = UPNP_CLASS_VIDEO_ITEM;
        }
        if (mapping == PlaylistClassMapping::ExactOverride) {
            if (std::string(mime) == "audio/x-mpegurl")
                expectedClass = UPNP_CLASS_AUDIO_BROADCAST;
            if (std::string(mime) == MIME_TYPE_ASX_PLAYLIST)
                expectedClass = UPNP_CLASS_VIDEO_MOVIE;
        }
        if (mapping == PlaylistClassMapping::ConditionalOverride && std::string(name) == "one.m3u")
            expectedClass = UPNP_CLASS_AUDIO_BROADCAST;
        EXPECT_EQ(item->getClass(), expectedClass);
        EXPECT_EQ(item->getMimeType(), mime);
        auto resource = item->getResource(ResourcePurpose::Content);
        ASSERT_NE(resource, nullptr);
        EXPECT_EQ(resource->getAttribute(ResourceAttribute::PROTOCOLINFO), std::string("http-get:*:") + mime + ":*");
        EXPECT_EQ(resource->getAttribute(ResourceAttribute::SIZE), std::to_string(fs::file_size(source / "pl" / name)));
    }
    for (const auto* name : { "a.wav", "b.wav" }) {
        auto item = database->findObjectByPath(source / "audio" / name, DEFAULT_CLIENT_GROUP, DbFileType::File);
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(item->getClass(), UPNP_CLASS_MUSIC_TRACK);
        EXPECT_EQ(item->getMetaData(MetadataFields::M_TITLE), std::string(name) == "a.wav" ? "Track A" : "Track B");
    }
    if (mapping == PlaylistClassMapping::ConditionalOverride) {
        auto item = std::dynamic_pointer_cast<CdsItem>(database->findObjectByPath(source / "pl/two.m3u8", DEFAULT_CLIENT_GROUP, DbFileType::File));
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(item->getClass(), UPNP_CLASS_PLAYLIST_ITEM);
        item->addMetaData(MetadataFields::M_TITLE, "Special");
        auto context = content->getContext();
        ImportService service(context, context->getConverterManager());
        service.updateItemData(item, item->getMimeType());
        EXPECT_EQ(item->getClass(), UPNP_CLASS_AUDIO_BROADCAST);
    }
    stop();
    for (const auto& [path, bytes] : original) {
        SCOPED_TRACE(path.string());
        EXPECT_EQ(readBytes(path), bytes);
    }
}

TEST_P(PlaylistFileClassTest, HttpResourcesPreservePlaylistMediaBytes)
{
#if !defined(HAVE_JS)
    GTEST_SKIP() << "Parsed playlist resources require JavaScript; physical class tests remain enabled";
#elif !defined(HAVE_CURL)
    GTEST_SKIP() << "HTTP resource verification requires the existing CurlIOHandler; database class and playlist position tests remain enabled";
#else
    ASSERT_NO_FATAL_FAILURE(prepareInputs());
    ASSERT_NO_FATAL_FAILURE(startAndWaitForImports());
    ASSERT_TRUE(waitFor([this] { return static_cast<bool>(playlistResourceBytesMatch()); }, "HTTP bytes of both playlist views", std::chrono::seconds(20)));
    ASSERT_TRUE(playlistResourceBytesMatch());
#endif
}

static std::string playlistClassCaseName(const ::testing::TestParamInfo<PlaylistFileClassTest::ParamType>& info)
{
    static const std::array<const char*, 5> names { "Defaults", "Legacy", "ExactOverride", "ConditionalOverride", "DisabledDefaults" };
    return std::get<0>(info.param) + "_" + names.at(static_cast<std::size_t>(std::get<1>(info.param)));
}

INSTANTIATE_TEST_SUITE_P(ImportModes, PlaylistFileClassTest,
    ::testing::Combine(::testing::Values("mt", "grb"),
        ::testing::Values(PlaylistClassMapping::Defaults, PlaylistClassMapping::Legacy,
            PlaylistClassMapping::ExactOverride, PlaylistClassMapping::ConditionalOverride,
            PlaylistClassMapping::DisabledDefaults)),
    playlistClassCaseName);
