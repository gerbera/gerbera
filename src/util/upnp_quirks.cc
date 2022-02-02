/*GRB*

    Gerbera - https://gerbera.io/

    upnp_quirks.cc - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

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

/// \file upnp_quirks.cc

#include "upnp_quirks.h" // API

#include "cds_objects.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "server.h"
#include "util/tools.h"
#include "util/upnp_clients.h"
#include "util/upnp_headers.h"

Quirks::Quirks(std::shared_ptr<Context> context, const struct sockaddr_storage* addr, const std::string& userAgent)
    : context(std::move(context))
    , content(this->context->getServer()->getContent())
{
    pClientInfo = this->context->getClients()->getInfo(addr, userAgent);
}

int Quirks::checkFlags(int flags) const
{
    return pClientInfo ? pClientInfo->flags & flags : 0;
}

void Quirks::addCaptionInfo(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Headers>& headers) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG) == 0)
        return;

    if (item->getClass() != UPNP_CLASS_VIDEO_ITEM)
        return;

    auto [url, subAdded] = UpnpXMLBuilder::renderSubtitle(context->getServer()->getVirtualUrl(), item);
    if (subAdded) {
        log_debug("Call for Samsung CaptionInfo.sec: {}", url);
        headers->addHeader("CaptionInfo.sec", url);
        headers->addHeader("getCaptionInfo.sec", url);
    }
}

void Quirks::getSamsungFeatureList(const std::unique_ptr<ActionRequest>& request) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG) == 0)
        return;

    log_debug("Call for Samsung extension: X_GetFeatureList");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    pugi::xml_document respRoot;
    auto features = respRoot.append_child("Features");
    features.append_attribute("xmlns") = "urn:schemas-upnp-org:av:avs";
    features.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    features.append_attribute("xsi:schemaLocation") = "urn:schemas-upnp-org:av:avs http://www.upnp.org/schemas/av/avs.xsd";
    auto feature = features.append_child("Feature");
    feature.append_attribute("name") = "samsung.com_BASICVIEW";
    feature.append_attribute("version") = "1";
    constexpr auto containers = std::array {
        std::pair(UPNP_CLASS_AUDIO_ITEM, "A"),
        std::pair(UPNP_CLASS_VIDEO_ITEM, "V"),
        std::pair(UPNP_CLASS_IMAGE_ITEM, "I"),
        std::pair(UPNP_CLASS_PLAYLIST_ITEM, "P"),
        // std::pair("object.item.textItem", "T"),
    };

    for (auto&& [type, id] : containers) {
        auto container = feature.append_child("container");
        container.append_attribute("id") = id;
        container.append_attribute("type") = type;
    }

    response->document_element().append_child("FeatureList").append_child(pugi::node_pcdata).set_value(UpnpXMLBuilder::printXml(respRoot, "", 0).c_str());
    request->setResponse(std::move(response));
}

std::vector<std::shared_ptr<CdsObject>> Quirks::getSamsungFeatureRoot(const std::string& objId)
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG_FEATURES) == 0)
        return {};
    log_debug("getSamsungFeatureRoot objId [{}]", objId);

    static const auto containers = std::map<std::string, std::string> {
        { "A", UPNP_CLASS_AUDIO_ITEM },
        { "V", UPNP_CLASS_VIDEO_ITEM },
        { "I", UPNP_CLASS_IMAGE_ITEM },
        { "P", UPNP_CLASS_PLAYLIST_ITEM },
        // { "T", "object.item.textItem" },
    };
    if (containers.find(objId) != containers.end()) {
        return this->context->getDatabase()->findObjectByContentClass(containers.at(objId));
    }

    return {};
}

void Quirks::getSamsungObjectIDfromIndex(const std::unique_ptr<ActionRequest>& request) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG_FEATURES) == 0)
        return;

    log_debug("Call for Samsung extension: X_GetObjectIDfromIndex");

    auto reqRoot = request->getRequest()->document_element();

    log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

    [[maybe_unused]] auto categoryType = reqRoot.child("CategoryType").text().as_string();
    [[maybe_unused]] auto index = reqRoot.child("Index").text().as_string();

    log_debug("X_GetObjectIDfromIndex CategoryType [{}] Index[{}]", categoryType, index);

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    response->document_element().append_child("ObjectID").append_child(pugi::node_pcdata).set_value("0");
    request->setResponse(std::move(response));
}

void Quirks::getSamsungIndexfromRID(const std::unique_ptr<ActionRequest>& request) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG_FEATURES) == 0)
        return;

    log_debug("Call for Samsung extension: X_GetIndexfromRID");
    auto reqRoot = request->getRequest()->document_element();

    log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

    [[maybe_unused]] auto categoryType = reqRoot.child("CategoryType").text().as_string();
    [[maybe_unused]] auto rID = reqRoot.child("RID").text().as_string();

    log_debug("X_GetIndexfromRID CategoryType [{}] RID[{}]", categoryType, rID);

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    response->document_element().append_child("Index").append_child(pugi::node_pcdata).set_value("0");
    request->setResponse(std::move(response));
}

void Quirks::restoreSamsungBookMarkedPosition(const std::shared_ptr<CdsItem>& item, pugi::xml_node& result) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC) == 0 && (pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC) == 0)
        return;
    auto positionToRestore = item->getBookMarkPos().count();
    if (positionToRestore > 10)
        positionToRestore -= 10;
    log_debug("restoreSamsungBookMarkedPosition: ObjectID [{}] positionToRestore [{}] sec", item->getID(), positionToRestore);

    if (pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC)
        positionToRestore *= 1000;

    auto dcmInfo = fmt::format("CREATIONDATE=0,FOLDER={},BM={}", item->getTitle(), positionToRestore);
    result.append_child("sec:dcmInfo").append_child(pugi::node_pcdata).set_value(dcmInfo.c_str());
}

void Quirks::saveSamsungBookMarkedPosition(const std::unique_ptr<ActionRequest>& request) const
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC) == 0 && (pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC) == 0) {
        log_debug("saveSamsungBookMarkedPosition called, but it is not enabled for this client");
    } else {
        auto divider = (pClientInfo->flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC) == 0 ? 1 : 1000;
        auto reqRoot = request->getRequest()->document_element();
        auto objectID = reqRoot.child("ObjectID").text().as_string();
        auto bookMarkPos = std::to_string(stoiString(reqRoot.child("PosSecond").text().as_string()) / divider);
        [[maybe_unused]] auto categoryType = reqRoot.child("CategoryType").text().as_string();
        [[maybe_unused]] auto rID = reqRoot.child("RID").text().as_string();

        log_debug("saveSamsungBookMarkedPosition: ObjectID [{}] PosSecond [{}] CategoryType [{}] RID [{}]", objectID, bookMarkPos, categoryType, rID);

        std::map<std::string, std::string> m {
            { "bookmarkpos", bookMarkPos },
        };
        content->updateObject(stoiString(objectID), m);
    }
    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    request->setResponse(std::move(response));
}

bool Quirks::blockXmlDeclaration() const
{
    return (pClientInfo && (pClientInfo->flags & QUIRK_FLAG_IRADIO) == QUIRK_FLAG_IRADIO);
}

int Quirks::getCaptionInfoCount() const
{
    return pClientInfo ? pClientInfo->captionInfoCount : -1;
}
