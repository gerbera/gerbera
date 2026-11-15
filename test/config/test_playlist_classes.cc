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

#include "config/config_definition.h"
#include "config/config_val.h"
#include "config/setup/config_setup_dictionary.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <pugixml.hpp>
#include <string>

class PlaylistClassConfigTest : public ::testing::Test {
protected:
    std::map<std::string, std::string> parseMappings(const char* xml)
    {
        auto definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        auto setup = definition->findConfigSetup<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
        pugi::xml_document document;
        if (!document.load_string(xml)) {
            ADD_FAILURE() << "Invalid playlist class test configuration";
            return {};
        }
        return setup->getXmlContent(document.child("mimetype-upnpclass"), nullptr);
    }

    void expectPlaylistDefaults(const std::map<std::string, std::string>& mappings)
    {
        for (const auto* mime : { "audio/x-mpegurl", "audio/x-scpls", MIME_TYPE_ASX_PLAYLIST }) {
            SCOPED_TRACE(mime);
            auto mapping = mappings.find(mime);
            ASSERT_NE(mapping, mappings.end());
            EXPECT_EQ(mapping->second, UPNP_CLASS_PLAYLIST_ITEM);
        }
        EXPECT_EQ(mappings.at("audio/*"), UPNP_CLASS_MUSIC_TRACK);
        EXPECT_EQ(mappings.at("video/*"), UPNP_CLASS_VIDEO_ITEM);
    }
};

TEST_F(PlaylistClassConfigTest, EmptyMappingsGivePhysicalPlaylistItemsAndKeepMediaDefaults)
{
    expectPlaylistDefaults(parseMappings("<mimetype-upnpclass/>"));
}

TEST_F(PlaylistClassConfigTest, ExtendingLegacyWildcardsAddsMissingPlaylistDefaults)
{
    expectPlaylistDefaults(parseMappings(R"(<mimetype-upnpclass extend="yes">
        <map from="audio/*" to="object.item.audioItem.musicTrack"/>
        <map from="video/*" to="object.item.videoItem"/>
        </mimetype-upnpclass>)"));
}

TEST_F(PlaylistClassConfigTest, ExplicitMappingsOverrideOnlyTheirMatchingDefaults)
{
    auto mappings = parseMappings(R"(<mimetype-upnpclass extend="yes">
        <map from="audio/x-mpegurl" to="object.item.audioItem.audioBroadcast"/>
        <map from="video/x-ms-asx" to="object.item.videoItem.movie"/>
        </mimetype-upnpclass>)");
    EXPECT_EQ(mappings.at("audio/x-mpegurl"), UPNP_CLASS_AUDIO_BROADCAST);
    EXPECT_EQ(mappings.at(MIME_TYPE_ASX_PLAYLIST), UPNP_CLASS_VIDEO_MOVIE);
    EXPECT_EQ(mappings.at("audio/x-scpls"), UPNP_CLASS_PLAYLIST_ITEM);
    EXPECT_EQ(mappings.at("audio/*"), UPNP_CLASS_MUSIC_TRACK);
}

TEST_F(PlaylistClassConfigTest, NonextendingLegacyMappingsKeepOnlyExplicitEntries)
{
    const std::map<std::string, std::string> expected {
        { "audio/*", UPNP_CLASS_MUSIC_TRACK },
        { "video/*", UPNP_CLASS_VIDEO_ITEM },
    };
    for (const auto* xml : {
             R"(<mimetype-upnpclass extend="no"><map from="audio/*" to="object.item.audioItem.musicTrack"/><map from="video/*" to="object.item.videoItem"/></mimetype-upnpclass>)",
             R"(<mimetype-upnpclass><map from="audio/*" to="object.item.audioItem.musicTrack"/><map from="video/*" to="object.item.videoItem"/></mimetype-upnpclass>)" }) {
        SCOPED_TRACE(xml);
        EXPECT_EQ(parseMappings(xml), expected);
    }
}

TEST_F(PlaylistClassConfigTest, ConditionalOverridesRemainAvailableAlongsideExactDefaults)
{
    auto mappings = parseMappings(R"(<mimetype-upnpclass extend="yes">
        <map from="audio/*;location=one" to="object.item.audioItem.audioBroadcast"/>
        <map from="audio/*;dc:title=Special" to="object.item.audioItem.audioBroadcast"/>
        </mimetype-upnpclass>)");
    expectPlaylistDefaults(mappings);
    EXPECT_EQ(mappings.at("audio/*;location=one"), UPNP_CLASS_AUDIO_BROADCAST);
    EXPECT_EQ(mappings.at("audio/*;dc:title=Special"), UPNP_CLASS_AUDIO_BROADCAST);
}
