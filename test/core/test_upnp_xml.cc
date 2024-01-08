/*GRB*

Gerbera - https://gerbera.io/

    test_upnp_xml.cc - this file is part of Gerbera.

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
*/

#include <gtest/gtest.h>

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "common.h"
#include "config/client_config.h"
#include "config/config_setup.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"
#include "upnp_xml.h"
#include "util/grb_net.h"
#include "util/tools.h"

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"

using ::testing::_;
using ::testing::Return;

class MyConfigMock final : public ConfigMock {
public:
    MyConfigMock() { list = std::make_shared<ClientConfigList>(); }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override { return list; }
    std::shared_ptr<ClientConfigList> list;
};

class UpnpXmlTest : public ::testing::Test {

public:
    void SetUp() override
    {
        config = std::make_shared<ConfigMock>();

        database = std::make_shared<DatabaseMock>(config);
        context = std::make_shared<Context>(config, nullptr, nullptr, database, nullptr);

        std::string virtualDir = "http://server";
        std::string presentationURl = "http://someurl/";
        subject = std::make_shared<UpnpXMLBuilder>(context, virtualDir, presentationURl);
    }

    void TearDown() override
    {
        subject = nullptr;
    }

    std::shared_ptr<UpnpXMLBuilder> subject;
    std::shared_ptr<ConfigMock> config;
    std::shared_ptr<DatabaseMock> database;
    std::shared_ptr<Context> context;
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
    obj->addMetaData(M_ALBUMARTIST, "Creator");
    obj->addMetaData(M_COMPOSER, "Composer");
    obj->addMetaData(M_CONDUCTOR, "Conductor");
    obj->addMetaData(M_ORCHESTRA, "Orchestra");
    obj->addMetaData(M_UPNP_DATE, "2001-01-01");
    obj->addMetaData(M_DATE, "2022-04-01T00:00:00");

    // albumArtURI
    auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, CdsResource::Purpose::Thumbnail);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo("jpg"));
    resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, "/home/resource/cover.jpg");
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
    expectedXml << "<upnp:albumArtURI>http://server/content/media/object_id/1/res_id/0/ext/file.jpg</upnp:albumArtURI>\n";
    expectedXml << "</container>\n";
    expectedXml << "</DIDL-Lite>\n";

    // act
    subject->renderObject(obj, std::string::npos, root);

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
    obj->addMetaData(M_DESCRIPTION, "Description");
    obj->addMetaData(M_ALBUM, "Album");
    obj->addMetaData(M_TRACKNUMBER, "10");
    obj->addMetaData(M_DATE, "2022-04-01T00:00:00");

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

    EXPECT_CALL(*config, getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, root);

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
    obj->addMetaData(M_DESCRIPTION, "Description < Input");
    obj->addMetaData(M_ALBUM, "Album & Test");
    obj->addMetaData(M_TRACKNUMBER, "10");
    obj->addMetaData(M_DATE, "2022-04-01T00:00:00");

    auto clientConfig = std::make_shared<MyConfigMock>();
    auto clientManager = std::make_shared<ClientManager>(clientConfig, database);
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto quirks = std::make_unique<Quirks>(subject, clientManager, addr, "DLNADOC/1.50");

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

    EXPECT_CALL(*config, getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, root, quirks);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "");
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
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
    obj->addMetaData(M_DESCRIPTION, "Description < Input");
    obj->addMetaData(M_ALBUM, "Album & Test");
    obj->addMetaData(M_TRACKNUMBER, "10");
    obj->addMetaData(M_DATE, "2022-04-01T00:00:00");

    auto clientConfig = std::make_shared<MyConfigMock>();
    auto clientManager = std::make_shared<ClientManager>(clientConfig, database);
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto quirks = std::make_unique<Quirks>(subject, clientManager, addr, "Allegro-Software-WebClient/5.40b1 DLNADOC/1.50");

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

    EXPECT_CALL(*config, getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    log_warning(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, root, quirks);

    // assert
    std::string didlLiteXml = UpnpXMLBuilder::printXml(didlLite, "", pugi::format_no_escapes);
    EXPECT_STREQ(didlLiteXml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItemWithResources)
{
    // arrange
    pugi::xml_document didlLite;
    auto root = didlLite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(42);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->addMetaData(M_DESCRIPTION, "Description");
    obj->addMetaData(M_ALBUM, "Album");
    obj->addMetaData(M_TRACKNUMBER, "7");
    obj->addMetaData(M_UPNP_DATE, "2002-01-01");
    obj->addMetaData(M_DATE, "2022-04-01T00:00:00");

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, CdsResource::Purpose::Content);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, "http-get:*:audio/mpeg:*");
    resource->addAttribute(CdsResource::Attribute::BITRATE, "16044");
    resource->addAttribute(CdsResource::Attribute::DURATION, "123456");
    resource->addAttribute(CdsResource::Attribute::NRAUDIOCHANNELS, "2");
    resource->addAttribute(CdsResource::Attribute::SIZE, "4711");
    obj->addResource(resource);

    resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, CdsResource::Purpose::Subtitle);
    std::string type = "srt";
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo("text/" + type));
    resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, "/home/resource/subtitle.srt");
    resource->addParameter("type", type);
    obj->addResource(resource);

    resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, CdsResource::Purpose::Subtitle);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo("text/" + type));
    resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, "/home/resource/subtitle.srt");
    resource->addAttribute(CdsResource::Attribute::LANGUAGE, "fr");
    resource->addParameter("type", type);
    obj->addResource(resource);

    resource = std::make_shared<CdsResource>(ContentHandler::FANART, CdsResource::Purpose::Thumbnail);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo("jpg"));
    resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, "/home/resource/cover.jpg");
    resource->addAttribute(CdsResource::Attribute::RESOLUTION, "200x200");
    obj->addResource(resource);

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
    expectedXml << "<upnp:albumArtURI xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0\" dlna:profileID=\"JPEG_TN\">http://server/content/media/object_id/42/res_id/3/ext/file.jpg</upnp:albumArtURI>\n";
    expectedXml << "<sec:CaptionInfoEx protocolInfo=\"http-get:*:text/srt:*\" sec:type=\"srt\">http://server/content/media/object_id/42/res_id/1/type/srt/ext/file.srt</sec:CaptionInfoEx>\n";
    expectedXml << "<res size=\"4711\" duration=\"123456\" bitrate=\"16044\" nrAudioChannels=\"2\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\">http://server/content/media/object_id/42/res_id/0/group/default/ext/file.mp3</res>\n";
    expectedXml << "<res protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\">http://server/content/media/object_id/42/res_id/1/type/srt/ext/file.srt</res>\n";
    expectedXml << "<res protocolInfo=\"http-get:*:text/srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=00d00000000000000000000000000000\" dc:language=\"fr\">http://server/content/media/object_id/42/res_id/2/type/srt/ext/file.fr.srt</res>\n";
    expectedXml << "<res resolution=\"200x200\" protocolInfo=\"http-get:*:jpg:DLNA.ORG_OP=01;DLNA.ORG_CI=0\">http://server/content/media/object_id/42/res_id/3/ext/file.jpg</res>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP))
        .WillRepeatedly(Return(" / "));
    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, root);

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

TEST_F(UpnpXmlTest, FirstResourceRendersPureWhenExternalUrl)
{
    auto obj = std::make_shared<CdsItemExternalURL>();
    obj->setLocation("http://localhost/external/url");

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://localhost/external/url");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToExternalUrlWhenOnlineWithProxy)
{
    auto obj = std::make_shared<CdsItemExternalURL>();
    obj->setLocation("http://localhost/external/url");
    obj->setID(12345);
    obj->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
    obj->setFlag(OBJECT_FLAG_PROXY_URL);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://server/content/online/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToItem)
{
    auto obj = std::make_shared<CdsItem>();
    obj->setLocation("local/content");
    obj->setID(12345);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://server/content/media/object_id/12345/res_id/0");
}
