#include "gtest/gtest.h"
#include <cds_objects.h>
#include <common.h>
#include <upnp_xml.h>

using namespace ::testing;

class UpnpXmlTest : public ::testing::Test {

public:
    UpnpXmlTest() {};
    virtual ~UpnpXmlTest() {};

    virtual void SetUp()
    {
        std::string virtualDir = "/dir/virtual";
        std::string presentationURl = "http://someurl/";
        subject = new UpnpXMLBuilder(nullptr, nullptr, virtualDir, presentationURl);
    }

    virtual void TearDown()
    {
        delete subject;
    };

    UpnpXMLBuilder* subject;
};

TEST_F(UpnpXmlTest, CreatesUpnpDateElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderAlbumDate("2001-01-01", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "2001-01-01");
    EXPECT_STREQ(result.name(), "upnp:date");
}

TEST_F(UpnpXmlTest, CreatesUpnpOrchestraElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderOrchestra("Orchestra", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "Orchestra");
    EXPECT_STREQ(result.name(), "upnp:orchestra");
}

TEST_F(UpnpXmlTest, CreatesUpnpConductorElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderConductor("Conductor", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "Conductor");
    EXPECT_STREQ(result.name(), "upnp:Conductor");
}

TEST_F(UpnpXmlTest, CreatesUpnpAlbumArtUriElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderAlbumArtURI("/some/uri", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "/some/uri");
    EXPECT_STREQ(result.name(), "upnp:albumArtURI");
}

TEST_F(UpnpXmlTest, CreatesDcCreatorElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderCreator("Creator", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "Creator");
    EXPECT_STREQ(result.name(), "dc:creator");
}

TEST_F(UpnpXmlTest, CreatesSecCaptionInfoElement)
{
    pugi::xml_document doc;
    auto container = doc.append_child("container");
    subject->renderCaptionInfo("file.srt", &container);
    auto result = container.first_child();

    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(result.text().as_string(), "file.srt");
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

TEST_F(UpnpXmlTest, UpdatesObjectActiveItem)
{
    auto obj = std::make_shared<CdsActiveItem>(nullptr);
    std::ostringstream inputXml;
    inputXml << "<item>"; // this is not valid UPNP, but just enough to test with
    inputXml << "<dc:title>Title</dc:title>";
    inputXml << "<dc:description>description</dc:description>";
    inputXml << "<location>/location</location>";
    inputXml << "<mime-type>audio/mpeg</mime-type>";
    inputXml << "<action>action</action>";
    inputXml << "<state>state</state>";
    inputXml << "</item>";

    subject->updateObject(obj, inputXml.str());

    auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);
    EXPECT_NE(aitem, nullptr);
    EXPECT_STREQ(aitem->getTitle().c_str(), "Title");
    EXPECT_STREQ(aitem->getMetadata("dc:description").c_str(), "description");
    EXPECT_STREQ(aitem->getLocation().c_str(), "/location");
    EXPECT_STREQ(aitem->getMimeType().c_str(), "audio/mpeg");
    EXPECT_STREQ(aitem->getAction().c_str(), "action");
    EXPECT_STREQ(aitem->getState().c_str(), "state");
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
    auto obj = std::make_shared<CdsItemExternalURL>(nullptr);
    obj->setLocation("http://localhost/external/url");

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "http://localhost/external/url");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToExternalUrlWhenOnlineWithProxy)
{
    auto obj = std::make_shared<CdsItemExternalURL>(nullptr);
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
    auto obj = std::make_shared<CdsItem>(nullptr);
    obj->setLocation("local/content");
    obj->setID(12345);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "content/media/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsContentServeToInternalUrlItem)
{
    auto obj = std::make_shared<CdsItemInternalURL>(nullptr);
    obj->setLocation("local/content");
    obj->setID(12345);

    auto item = std::static_pointer_cast<CdsItem>(obj);

    std::string result = subject->getFirstResourcePath(item);

    EXPECT_NE(result, "");
    EXPECT_STREQ(result.c_str(), "/serve/local/content");
}
