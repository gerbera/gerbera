#include <gtest/gtest.h>

#include "cds_objects.h"
#include "common.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"
#include "upnp_xml.h"

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"

using namespace ::testing;

class UpnpXmlTest : public ::testing::Test {

public:
    UpnpXmlTest() {};
    virtual ~UpnpXmlTest() {};

    virtual void SetUp()
    {
        config = std::make_shared<ConfigMock>();

        database = std::make_shared<DatabaseMock>(config);
        context = std::make_shared<Context>(config, nullptr, nullptr, database, nullptr, nullptr);

        std::string virtualDir = "http://server/content";
        std::string presentationURl = "http://someurl/";
        subject = new UpnpXMLBuilder(context, virtualDir, presentationURl);
    }

    virtual void TearDown()
    {
        delete subject;
    };

    UpnpXMLBuilder* subject;
    std::shared_ptr<ConfigMock> config;
    std::shared_ptr<DatabaseMock> database;
    std::shared_ptr<Context> context;
};

TEST_F(UpnpXmlTest, RenderObjectContainer)
{
    // arrange
    pugi::xml_document didl_lite;
    auto root = didl_lite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsContainer>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_ALBUM);
    obj->setMetadata(M_ALBUMARTIST, "Creator");
    obj->setMetadata(M_COMPOSER, "Composer");
    obj->setMetadata(M_CONDUCTOR, "Conductor");
    obj->setMetadata(M_ORCHESTRA, "Orchestra");
    obj->setMetadata(M_UPNP_DATE, "2001-01-01");

    // albumArtURI
    auto resource = std::make_shared<CdsResource>(CH_CONTAINERART);
    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("jpg"));
    resource->addAttribute(R_RESOURCE_FILE, "/home/resource/cover.jpg");
    resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
    obj->addResource(resource);

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<container id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.container.album.musicAlbum</upnp:class>\n";
    expectedXml << "<dc:creator>Creator</dc:creator>\n";
    expectedXml << "<dc:date>2001-01-01</dc:date>\n";
    expectedXml << "<upnp:albumArtist>Creator</upnp:albumArtist>\n";
    expectedXml << "<upnp:artist>Creator</upnp:artist>\n";
    expectedXml << "<upnp:composer>Composer</upnp:composer>\n";
    expectedXml << "<upnp:conductor>Conductor</upnp:conductor>\n";
    expectedXml << "<upnp:date>2001-01-01</upnp:date>\n";
    expectedXml << "<upnp:orchestra>Orchestra</upnp:orchestra>\n";
    expectedXml << "<upnp:albumArtURI>http://server/content/media/object_id/1/res_id/0/rct/aa/rh/11</upnp:albumArtURI>\n";
    expectedXml << "</container>\n";
    expectedXml << "</DIDL-Lite>\n";

    // act
    subject->renderObject(obj, std::string::npos, &root);

    // assert
    std::ostringstream buf;
    didl_lite.print(buf, "", 0);
    std::string didl_lite_xml = buf.str();
    EXPECT_STREQ(didl_lite_xml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItem)
{
    // arrange
    pugi::xml_document didl_lite;
    auto root = didl_lite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(1);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->setMetadata(M_DESCRIPTION, "Description");
    obj->setMetadata(M_TRACKNUMBER, "10");
    obj->setMetadata(M_ALBUM, "Album");

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:description>Description</dc:description>\n";
    expectedXml << "<upnp:album>Album</upnp:album>\n";
    expectedXml << "<upnp:originalTrackNumber>10</upnp:originalTrackNumber>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, &root);

    // assert
    std::ostringstream buf;
    didl_lite.print(buf, "", 0);
    std::string didl_lite_xml = buf.str();
    EXPECT_STREQ(didl_lite_xml.c_str(), expectedXml.str().c_str());
}

TEST_F(UpnpXmlTest, RenderObjectItemWithResources)
{
    // arrange
    pugi::xml_document didl_lite;
    auto root = didl_lite.append_child("DIDL-Lite");
    auto obj = std::make_shared<CdsItem>();
    obj->setID(42);
    obj->setParentID(2);
    obj->setRestricted(false);
    obj->setTitle("Title");
    obj->setClass(UPNP_CLASS_MUSIC_TRACK);
    obj->setMetadata(M_DESCRIPTION, "Description");
    obj->setMetadata(M_TRACKNUMBER, "7");
    obj->setMetadata(M_ALBUM, "Album");

    auto resource = std::make_shared<CdsResource>(CH_DEFAULT);
    resource->addAttribute(R_PROTOCOLINFO, "http-get:*:audio/mpeg:*");
    resource->addAttribute(R_BITRATE, "16044");
    resource->addAttribute(R_DURATION, "123456");
    resource->addAttribute(R_NRAUDIOCHANNELS, "2");
    resource->addAttribute(R_SIZE, "4711");
    obj->addResource(resource);

    resource = std::make_shared<CdsResource>(CH_SUBTITLE);
    std::string type = "srt";
    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(type));
    resource->addAttribute(R_RESOURCE_FILE, "/home/resource/subtitle.srt");
    resource->addParameter(RESOURCE_CONTENT_TYPE, VIDEO_SUB);
    resource->addParameter("type", type);
    obj->addResource(resource);

    resource = std::make_shared<CdsResource>(CH_FANART);
    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("jpg"));
    resource->addAttribute(R_RESOURCE_FILE, "/home/resource/cover.jpg");
    resource->addAttribute(R_RESOLUTION, "200x200");
    resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
    obj->addResource(resource);

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<item id=\"42\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n";
    expectedXml << "<dc:description>Description</dc:description>\n";
    expectedXml << "<upnp:album>Album</upnp:album>\n";
    expectedXml << "<upnp:originalTrackNumber>7</upnp:originalTrackNumber>\n";
    expectedXml << "<res bitrate=\"16044\" duration=\"123456\" nrAudioChannels=\"2\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_OP=01;DLNA.ORG_CI=0\" size=\"4711\">http://server/content/media/object_id/42/res_id/0</res>\n";
    expectedXml << "<res protocolInfo=\"http-get:*:srt:DLNA.ORG_OP=01;DLNA.ORG_CI=0\" resFile=\"/home/resource/subtitle.srt\">http://server/content/media/object_id/42/res_id/1/rct/vs/type/srt</res>\n";
    expectedXml << "<upnp:albumArtURI xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0\" dlna:profileID=\"JPEG_TN\">http://server/content/media/object_id/42/res_id/2/rct/aa</upnp:albumArtURI>\n";
    expectedXml << "</item>\n";
    expectedXml << "</DIDL-Lite>\n";

    EXPECT_CALL(*config, getTranscodingProfileListOption(_))
        .WillRepeatedly(Return(std::make_shared<TranscodingProfileList>()));

    // act
    subject->renderObject(obj, std::string::npos, &root);

    // assert
    std::ostringstream buf;
    didl_lite.print(buf, "", 0);
    std::string didl_lite_xml = buf.str();
    EXPECT_STREQ(didl_lite_xml.c_str(), expectedXml.str().c_str());
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
    EXPECT_STREQ(result.c_str(), "content/online/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToItem)
{
    auto obj = std::make_shared<CdsItem>();
    obj->setLocation("local/content");
    obj->setID(12345);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "content/media/object_id/12345/res_id/0");
}
