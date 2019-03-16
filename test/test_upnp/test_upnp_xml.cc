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
    zmm::String virtualDir = "/dir/virtual";
    subject = new UpnpXMLBuilder(virtualDir);
  }

  virtual void TearDown() {
    delete subject;
  };

  UpnpXMLBuilder *subject;
};

TEST_F(UpnpXmlTest, CreatesUpnpDateElement) {
  zmm::Ref<mxml::Element> result = subject->renderAlbumDate(_("2001-01-01"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "2001-01-01");
  EXPECT_STREQ(result->getName().c_str(), "upnp:date");
}

TEST_F(UpnpXmlTest, CreatesUpnpOrchestraElement) {
  zmm::Ref<mxml::Element> result = subject->renderOrchestra(_("Orchestra"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Orchestra");
  EXPECT_STREQ(result->getName().c_str(), "upnp:orchestra");
}

TEST_F(UpnpXmlTest, CreatesUpnpConductorElement) {
  zmm::Ref<mxml::Element> result = subject->renderConductor(_("Conductor"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Conductor");
  EXPECT_STREQ(result->getName().c_str(), "upnp:Conductor");
}

TEST_F(UpnpXmlTest, CreatesUpnpAlbumArtUriElement) {
  zmm::Ref<mxml::Element> result = subject->renderAlbumArtURI(_("/some/uri"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "/some/uri");
  EXPECT_STREQ(result->getName().c_str(), "upnp:albumArtURI");
}

TEST_F(UpnpXmlTest, CreatesDcCreatorElement) {
  zmm::Ref<mxml::Element> result = subject->renderCreator(_("Creator"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Creator");
  EXPECT_STREQ(result->getName().c_str(), "dc:creator");
}

TEST_F(UpnpXmlTest, CreatesSecCaptionInfoElement) {
  zmm::Ref<mxml::Element> result = subject->renderCaptionInfo(_("file.srt"));

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
  zmm::Ref<CdsObject> obj(new CdsActiveItem());
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

  zmm::Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
  EXPECT_NE(aitem, nullptr);
  EXPECT_STREQ(aitem->getTitle().c_str(), "Title");
  EXPECT_STREQ(aitem->getMetadata("dc:description").c_str(), "description");
  EXPECT_STREQ(aitem->getLocation().c_str(), "/location");
  EXPECT_STREQ(aitem->getMimeType().c_str(), "audio/mpeg");
  EXPECT_STREQ(aitem->getAction().c_str(), "action");
  EXPECT_STREQ(aitem->getState().c_str(), "state");
}

TEST_F(UpnpXmlTest, CreateResponse) {
  zmm::String actionName = "action";
  zmm::String serviceType = "urn:schemas-upnp-org:service:ContentDirectory:1";

  zmm::Ref<mxml::Element> result = subject->createResponse(actionName, serviceType);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getName().c_str(), "u:actionResponse");
  EXPECT_STREQ(result->getAttribute("xmlns:u").c_str(), "urn:schemas-upnp-org:service:ContentDirectory:1");
}

TEST_F(UpnpXmlTest, FirstResourceRendersPureWhenExternalUrl) {
  zmm::Ref<CdsObject> obj(new CdsItemExternalURL());
  obj->setLocation(_("http://localhost/external/url"));

  zmm::Ref<CdsItem> item = RefCast(obj, CdsItem);

  zmm::String result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result.c_str(), "http://localhost/external/url");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToExternalUrlWhenOnlineWithProxy) {
  zmm::Ref<CdsObject> obj(new CdsItemExternalURL());
  obj->setLocation(_("http://localhost/external/url"));
  obj->setID(12345);
  obj->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
  obj->setFlag(OBJECT_FLAG_PROXY_URL);

  zmm::Ref<CdsItem> item = RefCast(obj, CdsItem);

  zmm::String result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result.c_str(), "content/online/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsLocalResourceIdToItem) {
  zmm::Ref<CdsObject> obj(new CdsItem());
  obj->setLocation(_("local/content"));
  obj->setID(12345);

  zmm::Ref<CdsItem> item = RefCast(obj, CdsItem);

  zmm::String result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result.c_str(), "content/media/object_id/12345/res_id/0");
}

TEST_F(UpnpXmlTest, FirstResourceAddsContentServeToInternalUrlItem) {
  zmm::Ref<CdsObject> obj(new CdsItemInternalURL());
  obj->setLocation(_("local/content"));
  obj->setID(12345);

  zmm::Ref<CdsItem> item = RefCast(obj, CdsItem);

  zmm::String result = subject->getFirstResourcePath(item);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result.c_str(), "/serve/local/content");
}