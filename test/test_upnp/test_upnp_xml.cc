#include <common.h>
#include <cds_objects.h>
#include <upnp_xml.h>
#include "gtest/gtest.h"

using namespace ::testing;

class UpnpXmlTest : public ::testing::Test {

 public:
  UpnpXmlTest() {};
  virtual ~UpnpXmlTest() {};

  virtual void SetUp() {
    std::string virtualDir = "/dir/virtual";
    std::string presentationURl = "http://someurl/";
    subject = new UpnpXMLBuilder(nullptr, nullptr, virtualDir, presentationURl);
  }

  virtual void TearDown() {
    delete subject;
  };

  UpnpXMLBuilder *subject;
};

TEST_F(UpnpXmlTest, CreatesUpnpDateElement) {
  zmm::Ref<mxml::Element> result = subject->renderAlbumDate("2001-01-01");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "2001-01-01");
  EXPECT_STREQ(result->getName().c_str(), "upnp:date");
}

TEST_F(UpnpXmlTest, CreatesUpnpOrchestraElement) {
  zmm::Ref<mxml::Element> result = subject->renderOrchestra("Orchestra");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Orchestra");
  EXPECT_STREQ(result->getName().c_str(), "upnp:orchestra");
}

TEST_F(UpnpXmlTest, CreatesUpnpConductorElement) {
  zmm::Ref<mxml::Element> result = subject->renderConductor("Conductor");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Conductor");
  EXPECT_STREQ(result->getName().c_str(), "upnp:Conductor");
}

TEST_F(UpnpXmlTest, CreatesUpnpAlbumArtUriElement) {
  zmm::Ref<mxml::Element> result = subject->renderAlbumArtURI("/some/uri");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "/some/uri");
  EXPECT_STREQ(result->getName().c_str(), "upnp:albumArtURI");
}

TEST_F(UpnpXmlTest, CreatesDcCreatorElement) {
  zmm::Ref<mxml::Element> result = subject->renderCreator("Creator");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Creator");
  EXPECT_STREQ(result->getName().c_str(), "dc:creator");
}

TEST_F(UpnpXmlTest, CreatesSecCaptionInfoElement) {
  zmm::Ref<mxml::Element> result = subject->renderCaptionInfo("file.srt");

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "file.srt");
  EXPECT_STREQ(result->getName().c_str(), "sec:CaptionInfoEx");
  EXPECT_STREQ(result->getAttribute("sec:type").c_str(), "srt");
}

TEST_F(UpnpXmlTest, CreatesEventPropertySet) {
  zmm::Ref<mxml::Element> result = subject->createEventPropertySet();

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getName().c_str(), "e:propertyset");
  EXPECT_STREQ(result->getAttribute("xmlns:e").c_str(), "urn:schemas-upnp-org:event-1-0");
  EXPECT_NE(result->getChildByName("e:property"), nullptr);
}

TEST_F(UpnpXmlTest, UpdatesObjectActiveItem) {
  auto obj = std::make_shared<CdsActiveItem>(nullptr);
  std::ostringstream inputXml;
  inputXml << "<item>";  // this is not valid UPNP, but just enough to test with
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

TEST_F(UpnpXmlTest, CreateResponse) {
  std::string actionName = "action";
  std::string serviceType = "urn:schemas-upnp-org:service:ContentDirectory:1";

  auto result = subject->createResponse(actionName, serviceType);
  EXPECT_NE(result, nullptr);

  auto root = result->document_element();
  EXPECT_STREQ(root.name(), "u:actionResponse");
  EXPECT_STREQ(root.attribute("xmlns:u").value(), "urn:schemas-upnp-org:service:ContentDirectory:1");
}

TEST_F(UpnpXmlTest, FirstResourceRendersPureWhenExternalUrl) {
  auto obj = std::make_shared<CdsItemExternalURL>(nullptr);
  obj->setLocation("http://localhost/external/url");

  auto item = std::static_pointer_cast<CdsItem>(obj);

  std::string result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, "");
  EXPECT_STREQ(result.c_str(), "http://localhost/external/url");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToExternalUrlWhenOnlineWithProxy) {
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

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToItem) {
  auto obj = std::make_shared<CdsItem>(nullptr);
  obj->setLocation("local/content");
  obj->setID(12345);

  auto item = std::static_pointer_cast<CdsItem>(obj);

  std::string result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, "");
  EXPECT_STREQ(result.c_str(), "content/media/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsContentServeToInternalUrlItem) {
  auto obj = std::make_shared<CdsItemInternalURL>(nullptr);
  obj->setLocation("local/content");
  obj->setID(12345);

  auto item = std::static_pointer_cast<CdsItem>(obj);

  std::string result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, "");
  EXPECT_STREQ(result.c_str(), "/serve/local/content");
}
