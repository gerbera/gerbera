/*GRB*

    Gerbera - https://gerbera.io/

    upnp_quirks.cc - this file is part of Gerbera.

    Copyright (C) 2020-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::clients

#include "upnp_quirks.h" // API

#include <array>
#include <utility>

#include "action_request.h"
#include "cds/cds_item.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "server.h"
#include "upnp_common.h"
#include "util/grb_net.h"
#include "util/tools.h"
#include "util/upnp_clients.h"
#include "util/upnp_headers.h"

Quirks::Quirks(std::shared_ptr<UpnpXMLBuilder> xmlBuilder, const std::shared_ptr<ClientManager>& clientManager, const std::shared_ptr<GrbNet>& addr, const std::string& userAgent)
    : xmlBuilder(std::move(xmlBuilder))
{
    if (addr || !userAgent.empty())
        pClientInfo = clientManager->getInfo(addr, userAgent);
}

long Quirks::checkFlags(long flags) const
{
    return pClientInfo ? pClientInfo->flags & flags : 0;
}

bool Quirks::hasFlag(long flag) const
{
    return pClientInfo && (pClientInfo->flags & flag) == flag;
}

std::string Quirks::getGroup() const
{
    return pClientInfo ? pClientInfo->group : DEFAULT_CLIENT_GROUP;
}

void Quirks::addCaptionInfo(const std::shared_ptr<CdsItem>& item, Headers& headers)
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG)) {
        log_debug("addCaptionInfo called, but it is not enabled for this client");
        return;
    }

    if (!startswith(item->getClass(), UPNP_CLASS_VIDEO_ITEM)) {
        log_debug("addCaptionInfo only available for videos");
        return;
    }

    auto subAdded = xmlBuilder->renderSubtitleURL(item, pClientInfo->mimeMappings);
    if (subAdded) {
        log_debug("Call for Samsung CaptionInfo.sec: {}", subAdded.value());
        headers.addHeader("CaptionInfo.sec", subAdded.value());
        headers.addHeader("getCaptionInfo.sec", subAdded.value());
    }
}

void Quirks::getSamsungFeatureList(ActionRequest& request) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG)) {
        log_debug("X_GetFeatureList called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for Samsung extension: X_GetFeatureList");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
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
    request.setResponse(std::move(response));
}

std::vector<std::shared_ptr<CdsObject>> Quirks::getSamsungFeatureRoot(const std::shared_ptr<Database>& database, const std::string& objId) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG_FEATURES)) {
        log_debug("getSamsungFeatureRoot called, but it is not enabled for this client");
        return {};
    }
    log_debug("getSamsungFeatureRoot objId [{}]", objId);

    static const auto containers = std::map<std::string, std::string> {
        { "A", UPNP_CLASS_AUDIO_ITEM },
        { "V", UPNP_CLASS_VIDEO_ITEM },
        { "I", UPNP_CLASS_IMAGE_ITEM },
        { "P", UPNP_CLASS_PLAYLIST_ITEM },
        // { "T", "object.item.textItem" },
    };
    if (containers.find(objId) != containers.end()) {
        return database->findObjectByContentClass(containers.at(objId));
    }

    return {};
}

static std::string xmlChild(const pugi::xml_node& root, const char* child)
{
    auto childNode = root.child(child);
    if (childNode)
        return childNode.text().as_string();
    return {};
}

void Quirks::getSamsungObjectIDfromIndex(ActionRequest& request) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG_FEATURES)) {
        log_debug("X_GetObjectIDfromIndex called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for Samsung extension: X_GetObjectIDfromIndex");

    auto reqRoot = request.getRequest()->document_element();
    if (reqRoot) {
        log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

        [[maybe_unused]] auto categoryType = xmlChild(reqRoot, "CategoryType");
        [[maybe_unused]] auto index = xmlChild(reqRoot, "Index");

        log_debug("X_GetObjectIDfromIndex CategoryType [{}] Index[{}]", categoryType, index);
    } else {
        log_warning("X_GetObjectIDfromIndex called without correct content");
    }
    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    response->document_element().append_child("ObjectID").append_child(pugi::node_pcdata).set_value("0");
    request.setResponse(std::move(response));
}

void Quirks::getSamsungIndexfromRID(ActionRequest& request) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG_FEATURES)) {
        log_debug("X_GetIndexfromRID called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for Samsung extension: X_GetIndexfromRID");
    auto reqRoot = request.getRequest()->document_element();
    if (reqRoot) {
        log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

        [[maybe_unused]] auto categoryType = xmlChild(reqRoot, "CategoryType");
        [[maybe_unused]] auto rID = xmlChild(reqRoot, "RID");

        log_debug("X_GetIndexfromRID CategoryType [{}] RID[{}]", categoryType, rID);
    } else {
        log_warning("X_GetIndexfromRID called without correct content");
    }
    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    response->document_element().append_child("Index").append_child(pugi::node_pcdata).set_value("0");
    request.setResponse(std::move(response));
}

void Quirks::restoreSamsungBookMarkedPosition(const std::shared_ptr<CdsItem>& item, pugi::xml_node& result, int offset) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC) && !hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC)) {
        log_debug("restoreSamsungBookMarkedPosition called, but it is not enabled for this client");
        return;
    }

    auto positionToRestore = item->getPlayStatus() ? item->getPlayStatus()->getBookMarkPosition().count() : 0;
    if (positionToRestore > offset)
        positionToRestore -= offset;
    log_debug("restoreSamsungBookMarkedPosition: ObjectID [{}] positionToRestore [{}] sec", item->getID(), positionToRestore);

    if (hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC))
        positionToRestore *= 1000;

    auto dcmInfo = fmt::format("CREATIONDATE=0,FOLDER={},BM={}", item->getTitle(), positionToRestore);
    result.append_child("sec:dcmInfo").append_child(pugi::node_pcdata).set_value(dcmInfo.c_str());
}

void Quirks::saveSamsungBookMarkedPosition(const std::shared_ptr<Database>& database, ActionRequest& request) const
{
    if (!hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC) && !hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC)) {
        log_debug("X_SetBookmark called, but it is not enabled for this client");
    } else {
        auto divider = hasFlag(QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC) ? 1 : 1000;
        auto reqRoot = request.getRequest()->document_element();
        if (reqRoot) {
            auto objectID = stoiString(xmlChild(reqRoot, "ObjectID"));
            auto bookMarkPos = stoiString(xmlChild(reqRoot, "PosSecond")) / divider;
            [[maybe_unused]] auto categoryType = xmlChild(reqRoot, "CategoryType");
            [[maybe_unused]] auto rID = xmlChild(reqRoot, "RID");

            log_debug("X_SetBookmark: ObjectID [{}] PosSecond [{}] CategoryType [{}] RID [{}]", objectID, bookMarkPos, categoryType, rID);
            auto playStatus = database->getPlayStatus(pClientInfo->group, objectID);
            if (!playStatus)
                playStatus = std::make_shared<ClientStatusDetail>(pClientInfo->group, objectID, 1, 0, 0, bookMarkPos);
            else {
                playStatus->setLastPlayed();
                playStatus->setBookMarkPosition(bookMarkPos);
            }
            database->savePlayStatus(playStatus);
        } else {
            log_warning("X_SetBookmark called without correct content");
        }
    }
    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    request.setResponse(std::move(response));
}

bool Quirks::blockXmlDeclaration() const
{
    return hasFlag(QUIRK_FLAG_IRADIO);
}

bool Quirks::needsFileNameUri() const
{
    return !hasFlag(QUIRK_FLAG_PANASONIC);
}

int Quirks::getCaptionInfoCount() const
{
    return pClientInfo ? pClientInfo->captionInfoCount : -1;
}

int Quirks::getStringLimit() const
{
    return pClientInfo ? pClientInfo->stringLimit : -1;
}

bool Quirks::needsStrictXml() const
{
    return hasFlag(QUIRK_FLAG_STRICTXML);
}

bool Quirks::getMultiValue() const
{
    return pClientInfo ? pClientInfo->multiValue : true;
}

std::map<std::string, std::string> Quirks::getMimeMappings() const
{
    return pClientInfo ? pClientInfo->mimeMappings : std::map<std::string, std::string>();
}
