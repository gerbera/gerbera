/*GRB*

    Gerbera - https://gerbera.io/

    test_upnp_xml.cc - this file is part of Gerbera.

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

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "common.h"
#include "config/config_definition.h"
#include "config/config_setup.h"
#include "config/result/client_config.h"
#include "config/result/transcoding.h"
#include "context.h"
#include "metadata/metadata_handler.h"
#include "upnp/headers.h"
#include "upnp/client_manager.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

class MyConfigMock final : public ConfigMock {
public:
    MyConfigMock() { list = std::make_shared<ClientConfigList>(); }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(ConfigVal option) const override { return list; }
    std::string getOption(ConfigVal option) const override
    {
        switch (option) {
#ifdef HAVE_LIBEXIF
        case ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET:
#endif
#ifdef HAVE_EXIV2
        case ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET:
#endif
#ifdef HAVE_TAGLIB
        case ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET:
#endif
#ifdef HAVE_FFMPEG
        case ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET:
#endif
#ifdef HAVE_MATROSKA
        case ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET:
#endif
#ifdef HAVE_WAVPACK
        case ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET:
#endif
#ifdef HAVE_JS
        case ConfigVal::IMPORT_SCRIPTING_CHARSET:
        case ConfigVal::IMPORT_PLAYLIST_CHARSET:
#endif
        case ConfigVal::IMPORT_METADATA_CHARSET:
        case ConfigVal::IMPORT_FILESYSTEM_CHARSET:
            return DEFAULT_INTERNAL_CHARSET;
        default:
            return "";
        }
    }
    std::shared_ptr<ClientConfigList> list;
};

class UpnpXmlTest : public ::testing::Test {

public:
    void SetUp() override
    {
        config = std::make_shared<ConfigMock>();

        database = std::make_shared<DatabaseMock>(config);
        converterManager = std::make_shared<ConverterManager>(config);

        std::shared_ptr<ConfigDefinition> definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        context = std::make_shared<Context>(definition, config, nullptr, nullptr, database, nullptr, converterManager);

        std::string virtualDir = "http://server";
        std::string presentationURl = "http://someurl/";
        subject = std::make_shared<UpnpXMLBuilder>(context, virtualDir);
    }

    void TearDown() override
    {
        subject = nullptr;
    }

    std::shared_ptr<UpnpXMLBuilder> subject;
    std::shared_ptr<ConfigMock> config;
    std::shared_ptr<DatabaseMock> database;
    std::shared_ptr<Context> context;
    std::shared_ptr<ConverterManager> converterManager;

    std::shared_ptr<CdsObject> makeObjectWithRes()
    {
        auto obj = std::make_shared<CdsItem>();
        obj->setID(42);
        obj->setParentID(2);
        obj->setRestricted(false);
        obj->setTitle("Title");
        obj->setClass(UPNP_CLASS_MUSIC_TRACK);
        obj->addMetaData(MetadataFields::M_DESCRIPTION, "Description");
        obj->addMetaData(MetadataFields::M_ALBUM, "Album");
        obj->addMetaData(MetadataFields::M_TRACKNUMBER, "7");
        obj->addMetaData(MetadataFields::M_UPNP_DATE, "2002-01-01");
        obj->addMetaData(MetadataFields::M_DATE, "2022-04-01T00:00:00");

        auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, "http-get:*:audio/mpeg:*");
        resource->addAttribute(ResourceAttribute::BITRATE, "16044");
        resource->addAttribute(ResourceAttribute::DURATION, "123456");
        resource->addAttribute(ResourceAttribute::NRAUDIOCHANNELS, "2");
        resource->addAttribute(ResourceAttribute::SIZE, "4711");
        obj->addResource(resource);

        resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, ResourcePurpose::Subtitle);
        std::string type = "srt";
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo("text/" + type));
        resource->addAttribute(ResourceAttribute::RESOURCE_FILE, "/home/resource/subtitle.srt");
        resource->addParameter("type", type);
        obj->addResource(resource);

        resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, ResourcePurpose::Subtitle);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo("text/" + type));
        resource->addAttribute(ResourceAttribute::RESOURCE_FILE, "/home/resource/subtitle.srt");
        resource->addAttribute(ResourceAttribute::LANGUAGE, "fr");
        resource->addParameter("type", type);
        obj->addResource(resource);

        resource = std::make_shared<CdsResource>(ContentHandler::FANART, ResourcePurpose::Thumbnail);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo("image/jpeg"));
        resource->addAttribute(ResourceAttribute::RESOURCE_FILE, "/home/resource/cover.jpg");
        resource->addAttribute(ResourceAttribute::RESOLUTION, "200x200");
        obj->addResource(resource);

        return obj;
    }
};

TEST_F(UpnpXmlTest, RenderObjectContainer)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsContainer>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_ALBUM);
    obj->addMetaData(MetadataFields::M_ALBUMARTIST, "Creator");
    obj->addMetaData(MetadataFields::M_COMPOSER, "Composer");
    obj->addMetaData(MetadataFields::M_CONDUCTOR, "Conductor");
    obj->addMetaData(MetadataFields::M_ORCHESTRA, "Orchestra");
    obj->addMetaData(MetadataFields::M_UPNP_DATE, "2001-01-01");
    obj->addMetaData(MetadataFields::M_DATE, "2022-04-01T00:00:00");

    // albumArtURI
    auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, ResourcePurpose::Thumbnail);
    resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo("jpg"));
    resource->addAttribute(ResourceAttribute::RESOURCE_FILE, "/home/resource/cover.jpg");
    obj->addResource(resource);

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<container id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.container.album.musicAlbum</upnp:class>\n";
    expectedXml << "<dc:creator>Creator</dc:creator>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<upnp:albumArtist>Creator</upnp:albumArtist>\n";
    expectedXml << "<upnp:artist>Creator</upnp:artist>\n";
    expectedXml << "<upnp:composer>Composer</upnp:composer>\n";
    expectedXml << "<upnp:conductor>Conductor</upnp:conductor>\n";
    expectedXml << "<upnp:date>2001-01-01</upnp:date>\n";
    expectedXml << "<upnp:orchestra>Orchestra</upnp:orchestra>\n";
    expectedXml << "<upnp:albumArtURI>http://server/media/object_id/1/res_id/0/ext/file.jpg</upnp:albumArtURI>\n";
    expectedXml << "<res id=\"1.0\" protocolInfo=\"http-get:*:jpg:DLNA.ORG_OP=01;DLNA.ORG_CI=0\">http://server/media/object_id/1/res_id/0/ext/file.jpg</res>\n";
    expectedXml << "</container>\n";
    expectedXml << "</DIDL-Lite>\n";

    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItem)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->addMetaData(MetadataFields::M_DESCRIPTION, "Description");
    obj->addMetaData(MetadataFields::M_ALBUM, "Album");
    obj->addMetaData(MetadataFields::M_TRACKNUMBER, "10");
    obj->addMetaData(MetadataFields::M_DATE, "2022-04-01T00:00:00");

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<dc:description>Description</dc:description>\n";
    expectedXml << "<upnp:album>Album</upnp:album>\n";
    expectedXml << "<upnp:originalTrackNumber>10</upnp:originalTrackNumber>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItemWithEscapes)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title 'n Ticks");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->addMetaData(MetadataFields::M_DESCRIPTION, "Description < Input");
    obj->addMetaData(MetadataFields::M_ALBUM, "Album & Test");
    obj->addMetaData(MetadataFields::M_TRACKNUMBER, "10");
    obj->addMetaData(MetadataFields::M_DATE, "2022-04-01T00:00:00");

    auto clientConfig = std::make_shared<MyConfigMock>();
    auto clientManager = std::make_shared<ClientManager>(clientConfig, database, nullptr);
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto quirks = std::make_shared<Quirks>(subject, clientManager, addr, "DLNADOC/1.50", nullptr);

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title 'n Ticks</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<dc:description>Description &lt; Input</dc:description>\n";
    expectedXml << "<upnp:album>Album &amp; Test</upnp:album>\n";
    expectedXml << "<upnp:originalTrackNumber>10</upnp:originalTrackNumber>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root, quirks);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, QuirksFlag)
{
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto headers = std::make_shared<Headers>();
    auto profile = ClientProfile();
    profile.flags = ClientConfig::getFlag(Quirk::Transcoding1);
    auto client = ClientObservation(addr, "", std::chrono::seconds(0), std::chrono::seconds(0), headers, &profile);
    auto quirk = Quirks(&client);
    EXPECT_TRUE(quirk.hasFlag(Quirk::Transcoding1));
    EXPECT_FALSE(quirk.hasFlag(Quirk::Transcoding2));
    profile.flags = 0;
    EXPECT_FALSE(quirk.hasFlag(Quirk::Transcoding1));
    EXPECT_FALSE(quirk.hasFlag(Quirk::Transcoding2));
    profile.flags = ClientConfig::makeFlags("Transcoding1");
    EXPECT_TRUE(quirk.hasFlag(Quirk::Transcoding1));
    EXPECT_FALSE(quirk.hasFlag(Quirk::Transcoding2));
    EXPECT_TRUE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding1"), false));
    EXPECT_FALSE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding2"), false));
    EXPECT_TRUE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding1"), true));
    EXPECT_FALSE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding2"), true));
    profile.flags = 0;
    EXPECT_FALSE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding1"), false));
    EXPECT_FALSE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding2"), false));
    EXPECT_TRUE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding1"), true));
    EXPECT_TRUE(quirk.hasFlag(ClientConfig::makeFlags("Transcoding2"), true));
}

TEST_F(UpnpXmlTest, RenderObjectItemWithStrictXmlQuirks)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title 'n Ticks");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->addMetaData(MetadataFields::M_DESCRIPTION, "Description < Input");
    obj->addMetaData(MetadataFields::M_ALBUM, "Album & Test");
    obj->addMetaData(MetadataFields::M_TRACKNUMBER, "10");
    obj->addMetaData(MetadataFields::M_DATE, "2022-04-01T00:00:00");

    auto clientConfig = std::make_shared<MyConfigMock>();
    auto clientManager = std::make_shared<ClientManager>(clientConfig, database, nullptr);
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto quirks = std::make_shared<Quirks>(subject, clientManager, addr, "Allegro-Software-WebClient/5.40b1 DLNADOC/1.50", nullptr);

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title &apos;n Ticks</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<dc:description>Description &lt; Input</dc:description>\n";
    expectedXml << "<upnp:album>Album &amp; Test</upnp:album>\n";
    expectedXml << "<upnp:originalTrackNumber>10</upnp:originalTrackNumber>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root, quirks);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", pugi::format_no_escapes);
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItemWithResources)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = makeObjectWithRes();

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"42\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<dc:description>Description</dc:description>\n";
    expectedXml << "<upnp:album>Album</upnp:album>\n";
    expectedXml << "<upnp:date>2002-01-01</upnp:date>\n";
    expectedXml << "<upnp:originalTrackNumber>7</upnp:originalTrackNumber>\n";
    expectedXml << "<upnp:albumArtURI xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0\" dlna:profileID=\"JPEG_TN\">http://server/media/object_id/42/res_id/3/ext/file.jpg</upnp:albumArtURI>\n";
    expectedXml << "<sec:CaptionInfoEx sec:type=\"srt\">http://server/media/object_id/42/res_id/1/type/srt/ext/file.srt</sec:CaptionInfoEx>\n";
    expectedXml << "<res id=\"42.0\" size=\"4711\" duration=\"123456\" bitrate=\"16044\" nrAudioChannels=\"2\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\">http://server/media/object_id/42/res_id/0/group/default/ext/file.mp3</res>\n";
    expectedXml << "<res id=\"42.1\" protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\">http://server/media/object_id/42/res_id/1/type/srt/ext/file.srt</res>\n";
    expectedXml << "<res id=\"42.2\" protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\" dc:language=\"fr\">http://server/media/object_id/42/res_id/2/type/srt/ext/file.fr.srt</res>\n";
    expectedXml << "<res id=\"42.3\" resolution=\"200x200\" protocolInfo=\"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_TN;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00800000000000000000000000000000\">http://server/media/object_id/42/res_id/3/ext/file.jpg</res>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItemWithResourcesWithQuirks)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = makeObjectWithRes();

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"42\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:date>2022-04-01T00:00:00</dc:date>\n";
    expectedXml << "<dc:description>Description</dc:description>\n";
    expectedXml << "<upnp:album>Album</upnp:album>\n";
    expectedXml << "<upnp:date>2002-01-01</upnp:date>\n";
    expectedXml << "<upnp:originalTrackNumber>7</upnp:originalTrackNumber>\n";
    expectedXml << "<upnp:albumArtURI xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0\" dlna:profileID=\"JPEG_TN\">http://server/media/object_id/42/res_id/3/ext/file.jpg</upnp:albumArtURI>\n";
    expectedXml << "<sec:CaptionInfoEx protocolInfo=\"http-get:*:text/srt:*\" sec:type=\"srt\">http://server/media/object_id/42/res_id/1/type/srt/ext/file.srt</sec:CaptionInfoEx>\n";
    expectedXml << "<res id=\"42.0\" size=\"4711\" duration=\"123456\" bitrate=\"16044\" nrAudioChannels=\"2\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\">http://server/media/object_id/42/res_id/0/group/default/ext/file.mp3</res>\n";
    expectedXml << "<res id=\"42.1\" protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\">http://server/media/object_id/42/res_id/1/type/srt/ext/file.srt</res>\n";
    expectedXml << "<res id=\"42.2\" protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\" dc:language=\"fr\">http://server/media/object_id/42/res_id/2/type/srt/ext/file.fr.srt</res>\n";
    expectedXml << "<res id=\"42.3\" resolution=\"200x200\" protocolInfo=\"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_TN;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00800000000000000000000000000000\">http://server/media/object_id/42/res_id/3/ext/file.jpg</res>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    ClientProfile pInfo;
    pInfo.flags = ClientConfig::getFlags({ Quirk::CaptionProtocol });
    auto addr = std::make_shared<GrbNet>("127.0.0.1");
    struct ClientObservation client(addr, "ua", std::chrono::seconds(0), std::chrono::seconds(0), nullptr, &pInfo);
    auto quirks = std::make_shared<Quirks>(&client);
    // act
    subject->renderObject(obj, { "*" }, std::string::npos, root, quirks);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, CreatesEventPropertySet)
{
    auto result = subject->createEventPropertySet();
    auto root = result->document_element();

    EXPECT_NE(root, nullptr);
    EXPECT_STREQ(root.name(), "e:propertyset");
    EXPECT_STREQ(root.attribute("xmlns:e").as_string(), "urn:schemas-upnp-org:event-1-0");
    EXPECT_NE(root.child("e:property"), nullptr);
}

TEST_F(UpnpXmlTest, CreateResponse)
{
    std::string actionName = "action";
    std::string serviceType = "urn:schemas-upnp-org:service:ContentDirectory:1";

    auto result = subject->createResponse(actionName, serviceType);
    EXPECT_NE(result, nullptr);

    auto root = result->document_element();
    EXPECT_STREQ(root.name(), "u:actionResponse");
    EXPECT_STREQ(root.attribute("xmlns:u").value(), "urn:schemas-upnp-org:service:ContentDirectory:1");
}

TEST_F(UpnpXmlTest, FirstResourceRendersEmptyWhenNoResource)
{
    auto obj = std::make_shared<CdsItemExternalURL>();
    obj->setLocation("http://localhost/external/url");

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_EQ(result, "");
}

TEST_F(UpnpXmlTest, FirstResourceRendersPureWhenExternalUrl)
{
    auto obj = std::make_shared<CdsItemExternalURL>();
    obj->setLocation("http://localhost/external/url");

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
    resource->addAttribute(ResourceAttribute::PROTOCOLINFO, "http-get:*:audio/mpeg:*");
    resource->addAttribute(ResourceAttribute::DURATION, "123456");
    resource->addAttribute(ResourceAttribute::SIZE, "4711");
    obj->addResource(resource);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://localhost/external/url");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToExternalUrlWhenWithProxy)
{
    auto obj = std::make_shared<CdsItemExternalURL>();
    obj->setLocation("http://localhost/external/url");
    obj->setID(12345);
    obj->setFlag(OBJECT_FLAG_PROXY_URL);

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
    resource->addAttribute(ResourceAttribute::DURATION, "123456");
    resource->addAttribute(ResourceAttribute::SIZE, "4711");
    obj->addResource(resource);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://server/online/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToItem)
{
    auto obj = std::make_shared<CdsItem>();
    obj->setLocation("local/content");
    obj->setID(12345);

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
    resource->addAttribute(ResourceAttribute::DURATION, "123456");
    resource->addAttribute(ResourceAttribute::SIZE, "4711");
    obj->addResource(resource);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://server/media/object_id/12345/res_id/0");
}
