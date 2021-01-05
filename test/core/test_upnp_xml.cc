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
        std::string virtualDir = "http://server/content";
        std::string presentationURl = "http://someurl/";
        subject = new UpnpXMLBuilder(config, database, virtualDir, presentationURl);
    }

    virtual void TearDown()
    {
        delete subject;
    };

    UpnpXMLBuilder* subject;
    std::shared_ptr<ConfigMock> config;
    std::shared_ptr<DatabaseMock> database;
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
    database->findFolderImageMap.clear();
    database->findFolderImageMap[std::to_string(obj->getID())] = "10";

    std::ostringstream expectedXml;
    expectedXml << "<DIDL-Lite>\n";
    expectedXml << "<container id=\"1\" parentID=\"2\" restricted=\"0\">\n";
    expectedXml << "<dc:title>Title</dc:title>\n";
    expectedXml << "<upnp:class>object.container.album.musicAlbum</upnp:class>\n";
    expectedXml << "<dc:creator>Creator</dc:creator>\n";
    expectedXml << "<upnp:composer>Composer</upnp:composer>\n";
    expectedXml << "<upnp:Conductor>Conductor</upnp:Conductor>\n";
    expectedXml << "<upnp:orchestra>Orchestra</upnp:orchestra>\n";
    expectedXml << "<upnp:date>2001-01-01</upnp:date>\n";
    expectedXml << "<upnp:albumArtURI>http://server/content/media/object_id/10/res_id/0</upnp:albumArtURI>\n";
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

TEST_F(UpnpXmlTest, CreatesSecCaptionInfoElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderCaptionInfo("/media/object_id/1/file.mp4", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "http://server/content/media/object_id/1/file.srt");
    EXPECT_STREQ(result.name(), "sec:CaptionInfoEx");
    EXPECT_STREQ(result.attribute("sec:type").as_string(), "srt");
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
