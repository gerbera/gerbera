#include <common.h>
#include <cds_objects.h>
#include <upnp_xml.h>
#include "gtest/gtest.h"

using namespace ::testing;

TEST(UpnpXmlTest, CreatesUpnpDateElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderAlbumDate(_("2001-01-01"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "2001-01-01");
  EXPECT_STREQ(result->getName().c_str(), "upnp:date");
}

TEST(UpnpXmlTest, CreatesUpnpOrchestraElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderOrchestra(_("Orchestra"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Orchestra");
  EXPECT_STREQ(result->getName().c_str(), "upnp:orchestra");
}

TEST(UpnpXmlTest, CreatesUpnpConductorElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderConductor(_("Conductor"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Conductor");
  EXPECT_STREQ(result->getName().c_str(), "upnp:Conductor");
}

TEST(UpnpXmlTest, CreatesUpnpAlbumArtUriElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderAlbumArtURI(_("/some/uri"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "/some/uri");
  EXPECT_STREQ(result->getName().c_str(), "upnp:albumArtURI");
}

TEST(UpnpXmlTest, CreatesDcCreatorElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderCreator(_("Creator"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "Creator");
  EXPECT_STREQ(result->getName().c_str(), "dc:creator");
}

TEST(UpnpXmlTest, CreatesSecCaptionInfoElement) {
  zmm::Ref<mxml::Element> result = UpnpXML_DIDLRenderCaptionInfo(_("file.srt"));

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getText().c_str(), "file.srt");
  EXPECT_STREQ(result->getName().c_str(), "sec:CaptionInfoEx");
  EXPECT_STREQ(result->getAttribute("sec:type").c_str(), "srt");
}

TEST(UpnpXmlTest, CreatesEventPropertySet) {
  zmm::Ref<mxml::Element> result = UpnpXML_CreateEventPropertySet();

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getName().c_str(), "e:propertyset");
  EXPECT_STREQ(result->getAttribute("xmlns:e").c_str(), "urn:schemas-upnp-org:event-1-0");
  EXPECT_NE(result->getChildByName("e:property"), nullptr);
}

TEST(UpnpXmlTest, UpdatesObjectActiveItem) {
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

  UpnpXML_DIDLUpdateObject(obj, inputXml.str());

  zmm::Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
  EXPECT_NE(aitem, nullptr);
  EXPECT_STREQ(aitem->getTitle().c_str(), "Title");
  EXPECT_STREQ(aitem->getMetadata("dc:description").c_str(), "description");
  EXPECT_STREQ(aitem->getLocation().c_str(), "/location");
  EXPECT_STREQ(aitem->getMimeType().c_str(), "audio/mpeg");
  EXPECT_STREQ(aitem->getAction().c_str(), "action");
  EXPECT_STREQ(aitem->getState().c_str(), "state");
}

TEST(UpnpXmlTest, CreateResponse) {
  zmm::String actionName = "action";
  zmm::String serviceType = "urn:schemas-upnp-org:service:ContentDirectory:1";

  zmm::Ref<mxml::Element> result = UpnpXML_CreateResponse(actionName, serviceType);

  EXPECT_NE(result, nullptr);
  EXPECT_STREQ(result->getName().c_str(), "u:actionResponse");
  EXPECT_STREQ(result->getAttribute("xmlns:u").c_str(), "urn:schemas-upnp-org:service:ContentDirectory:1");
}