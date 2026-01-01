/*GRB*

    Gerbera - https://gerbera.io/

    quirks.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

/// @file upnp/quirks.cc
#define GRB_LOG_FAC GrbLogFacility::clients

#include "quirks.h" // API

#include "action_request.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/result/client_config.h"
#include "database/database.h"
#include "upnp/client_manager.h"
#include "upnp/clients.h"
#include "upnp/headers.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/tools.h"

#include <array>
#include <utility>

Quirks::Quirks(
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder,
    const std::shared_ptr<ClientManager>& clientManager,
    const std::shared_ptr<GrbNet>& addr,
    const std::string& userAgent,
    const std::shared_ptr<Headers>& headers)
    : xmlBuilder(std::move(xmlBuilder))
{
    if (addr || !userAgent.empty()) {
        pClient = clientManager->getInfo(addr, userAgent, headers);
        pClientProfile = pClient->pInfo;
    }
}

Quirks::Quirks(const ClientObservation* client)
    : pClientProfile(client->pInfo)
    , pClient(client)
{
}

bool Quirks::hasFlag(QuirkFlags flag, bool matchesWithOut) const
{
    return pClientProfile && ((pClientProfile->flags & flag) == flag || (pClientProfile->flags == QUIRK_FLAG_NONE && matchesWithOut));
}

bool Quirks::hasFlag(Quirk flag) const
{
    return pClientProfile && ClientConfig::hasFlag(pClientProfile->flags, flag);
}

bool Quirks::hasHeader(const std::string& key, const std::string& value) const
{
    return pClient && pClient->headers && pClient->headers->hasHeader(key) && pClient->headers->getHeader(key) == value;
}

std::string Quirks::getGroup() const
{
    return pClientProfile ? pClientProfile->group : DEFAULT_CLIENT_GROUP;
}

void Quirks::addCaptionInfo(const std::shared_ptr<CdsItem>& item, Headers& headers)
{
    if (!hasFlag(Quirk::Samsung)) {
        log_debug("addCaptionInfo called, but it is not enabled for this client");
        return;
    }

    if (!item->isSubClass(UPNP_CLASS_VIDEO_ITEM)) {
        log_debug("addCaptionInfo only available for videos");
        return;
    }

    if (!hasHeader("getCaptionInfo.sec", "1")) {
        log_debug("CaptionInfo.sec not requested by this client");
        return;
    }

    auto subAdded = xmlBuilder->renderSubtitleURL(item, pClientProfile->mimeMappings.getDictionaryOption());
    if (subAdded) {
        log_debug("Call for Samsung CaptionInfo.sec: {}", subAdded.value());
        headers.addHeader("CaptionInfo.sec", subAdded.value());
    }
}

void Quirks::getSamsungFeatureList(ActionRequest& request) const
{
    if (!hasFlag(Quirk::Samsung)) {
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
    constexpr std::array containers {
        std::pair(UPNP_CLASS_AUDIO_ITEM, "A"),
        std::pair(UPNP_CLASS_VIDEO_ITEM, "V"),
        std::pair(UPNP_CLASS_IMAGE_ITEM, "I"),
        std::pair(UPNP_CLASS_PLAYLIST_ITEM, "P"),
#ifdef GRB_DLNA_SAMSUNG_TEXT
        std::pair(UPNP_CLASS_TEXT_ITEM, "T"),
#endif
    };

    for (auto&& [type, id] : containers) {
        auto container = feature.append_child("container");
        container.append_attribute("id") = id;
        container.append_attribute("type") = type;
    }

    response->document_element().append_child("FeatureList").append_child(pugi::node_pcdata).set_value(UpnpXMLBuilder::printXml(respRoot, "", 0).c_str());
    request.setResponse(std::move(response));
}

void Quirks::getShortCutList(const std::shared_ptr<Database>& database,
    pugi::xml_node& features) const
{
    if (hasFlag(Quirk::HideContainerShortcuts)) {
        log_debug("ShortCutList called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for extension: ShortCutList");
    auto feature = features.append_child("Feature");
    feature.append_attribute("name") = "CONTAINER_SHORTCUTS";
    feature.append_attribute("version") = "1";
    auto scl = feature.append_child("shortcutlist");

    /* <Features
     *  xmlns="urn:schemas-upnp-org:av:avs"
     *  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     *  xsi:schemaLocation="urn:schemas-upnp-org:av:avs http://www.upnp.org/schemas/av/avs.xsd">
     *    <Feature name="CONTAINER_SHORTCUTS" version="1">
     *      <shortcutlist>
     *        <shortcut> <name>MUSIC</name> <objectID>Music</objectID> </shortcut>
     *        <shortcut> <name>MUSIC_ALBUMS</name> <objectID>Music/Album</objectID> </shortcut>
     *        <shortcut> <name>MUSIC_ARTISTS</name> <objectID>Music/Artist</objectID> </shortcut>
     *        <shortcut> <name>MUSIC_GENRES</name> <objectID>Music/Genre</objectID> </shortcut>
     *        <shortcut> <name>MUSIC_ALL</name> <objectID>Music/Track</objectID> </shortcut>
     *        <shortcut> <name>VIDEOS</name> <objectID>Videos</objectID> </shortcut>
     *        <shortcut> <name>VIDEOS_GENRES</name> <objectID>Videos/Genre</objectID> </shortcut>
     *        <shortcut> <name>VIDEOS_RECORDINGS</name> <objectID>Recordings</objectID> </shortcut>
     *        <shortcut> <name>VIDEOS_ALL</name> <objectID>Videos/Video</objectID> </shortcut>
     *      </shortcutlist>
     *    </Feature>
     *  </Features>
     */

    /*
     * MUSIC,
     * MUSIC_ALBUMS,
     * MUSIC_ARTISTS,
     * MUSIC_GENRES,
     * MUSIC_PLAYLISTS,
     * MUSIC_RECENTLY_ADDED,
     * MUSIC_LAST_PLAYED,
     * MUSIC_AUDIOBOOKS,
     * MUSIC_STATIONS,
     * MUSIC_ALL,
     * MUSIC_FOLDER_STRUCTURE,

     * IMAGES,
     * IMAGES_YEARS,
     * IMAGES_YEARS_MONTH,
     * IMAGES_ALBUM,
     * IMAGES_SLIDESHOWS,
     * IMAGES_RECENTLY_ADDED,
     * IMAGES_LAST_WATCHED,
     * IMAGES_ALL,
     * IMAGES_FOLDER_STRUCTURE,

     * VIDEOS,
     * VIDEOS_GENRES,
     * VIDEOS_YEARS,
     * VIDEOS_YEARS_MONTH,
     * VIDEOS_ALBUM,
     * VIDEOS_RECENTLY_ADDED,
     * VIDEOS_LAST_PLAYED,
     * VIDEOS_RECORDINGS,
     * VIDEOS_ALL,
     * VIDEOS_FOLDER_STRUCTURE,
     */
    auto shortcuts = database->getShortcuts();

    for (auto&& [shortcut, cont] : shortcuts) {
        if (!shortcut.empty()) {
            log_debug("shortcut {}={}", shortcut, cont->getID());
            auto container = scl.append_child("shortcut");
            container.append_attribute("name") = shortcut.c_str();
            container.append_attribute("objectID") = cont->getID();
        }
    }
}

std::vector<std::shared_ptr<CdsObject>> Quirks::getSamsungFeatureRoot(
    const std::shared_ptr<Database>& database,
    int startIndex,
    int count,
    const std::string& objId) const
{
    if (!hasFlag(Quirk::SamsungFeatures)) {
        log_debug("getSamsungFeatureRoot called, but it is not enabled for this client");
        return {};
    }

    static const auto containers = std::map<std::string, std::string> {
        { "A", UPNP_CLASS_AUDIO_ITEM },
        { "A_D", UPNP_CLASS_AUDIO_ITEM },
        { "A_T", UPNP_CLASS_AUDIO_ITEM }, // seen with "Linux/9.0 UPnP/1.0 PROTOTYPE/1.0"
        { "V", UPNP_CLASS_VIDEO_ITEM },
        { "V_D", UPNP_CLASS_VIDEO_ITEM }, // seen with "Linux/9.0 UPnP/1.0 PROTOTYPE/1.0"
        { "V_T", UPNP_CLASS_VIDEO_ITEM },
        { "I", UPNP_CLASS_IMAGE_ITEM },
        { "I_D", UPNP_CLASS_IMAGE_ITEM },
        { "I_T", UPNP_CLASS_IMAGE_ITEM }, // seen with "Linux/9.0 UPnP/1.0 PROTOTYPE/1.0"
        { "P", UPNP_CLASS_PLAYLIST_ITEM },
        { "P_D", UPNP_CLASS_PLAYLIST_ITEM },
        { "P_T", UPNP_CLASS_PLAYLIST_ITEM },
#ifdef GRB_DLNA_SAMSUNG_TEXT
        { "T", UPNP_CLASS_TEXT_ITEM },
#endif
    };
    if (containers.find(objId) != containers.end()) {
        log_debug("objId [{}] = {}", objId, containers.at(objId));
        return database->findObjectByContentClass(containers.at(objId), startIndex, count, getGroup());
    }

    log_warning("getSamsungFeatureRoot unknown objId [{}]", objId);
    return {};
}

void Quirks::getSamsungObjectIDfromIndex(ActionRequest& request) const
{
    if (!hasFlag(Quirk::SamsungFeatures)) {
        log_debug("X_GetObjectIDfromIndex called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for Samsung extension: X_GetObjectIDfromIndex");

    auto doc = request.getRequest();
    auto reqRoot = doc->document_element();
    if (reqRoot) {
        log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

        [[maybe_unused]] auto categoryType = reqRoot.child_value("CategoryType");
        [[maybe_unused]] auto index = reqRoot.child_value("Index");

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
    if (!hasFlag(Quirk::SamsungFeatures)) {
        log_debug("X_GetIndexfromRID called, but it is not enabled for this client");
        return;
    }

    log_debug("Call for Samsung extension: X_GetIndexfromRID");
    auto doc = request.getRequest();
    auto reqRoot = doc->document_element();
    if (reqRoot) {
        log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

        [[maybe_unused]] auto categoryType = reqRoot.child_value("CategoryType");
        [[maybe_unused]] auto rID = reqRoot.child_value("RID");

        log_debug("X_GetIndexfromRID CategoryType [{}] RID[{}]", categoryType, rID);
    } else {
        log_warning("X_GetIndexfromRID called without correct content");
    }
    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CDS_SERVICE_TYPE);
    response->document_element().append_child("Index").append_child(pugi::node_pcdata).set_value("0");
    request.setResponse(std::move(response));
}

void Quirks::restoreSamsungBookMarkedPosition(const std::shared_ptr<CdsItem>& item, pugi::xml_node& result, int offsetSecond) const
{
    if (!hasFlag(Quirk::SamsungBookmarkSeconds) && !hasFlag(Quirk::SamsungBookmarkMilliSeconds)) {
        log_debug("restoreSamsungBookMarkedPosition called, but it is not enabled for this client");
        return;
    }

    auto playStatus = item->getPlayStatus();
    auto bookMarkPos = playStatus ? playStatus->getBookMarkPosition().count() : 0;

    auto offsetMilliseconds = offsetSecond * 1000;
    if (bookMarkPos > offsetMilliseconds) {
        bookMarkPos -= offsetMilliseconds;
    } else {
        bookMarkPos = 0;
    }
    if (bookMarkPos <= 0)
        return;

    log_debug("restoreSamsungBookMarkedPosition: ObjectID [{}] BookMarkPos [{}] msec", item->getID(), bookMarkPos);

    if (hasFlag(Quirk::SamsungBookmarkSeconds))
        bookMarkPos /= 1000;

    auto dcmInfo = fmt::format("CREATIONDATE=0,FOLDER={},BM={}", item->getTitle(), bookMarkPos);
    result.append_child("sec:dcmInfo").append_child(pugi::node_pcdata).set_value(dcmInfo.c_str());
}

void Quirks::saveSamsungBookMarkedPosition(const std::shared_ptr<Database>& database, ActionRequest& request) const
{
    if (!hasFlag(Quirk::SamsungBookmarkSeconds) && !hasFlag(Quirk::SamsungBookmarkMilliSeconds)) {
        log_debug("X_SetBookmark called, but it is not enabled for this client");
    } else {
        auto doc = request.getRequest();
        auto reqRoot = doc->document_element();
        if (reqRoot) {
            log_debug("request {}", UpnpXMLBuilder::printXml(reqRoot, " "));

            auto objectID = stoiString(reqRoot.child_value("ObjectID"));
            auto bookMarkPos = stoiString(reqRoot.child_value("PosSecond"));
            [[maybe_unused]] auto categoryType = reqRoot.child_value("CategoryType");
            [[maybe_unused]] auto rID = reqRoot.child_value("RID");

            if (hasFlag(Quirk::SamsungBookmarkSeconds))
                bookMarkPos *= 1000;

            log_debug("X_SetBookmark: ObjectID [{}] CategoryType [{}] RID [{}] BookMarkPos [{}] msec", objectID, categoryType, rID, bookMarkPos);

            auto playStatus = database->getPlayStatus(pClientProfile->group, objectID);
            if (!playStatus)
                playStatus = std::make_shared<ClientStatusDetail>(pClientProfile->group, objectID, 1, 0, 0, bookMarkPos);
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

bool Quirks::supportsResource(ResourcePurpose purpose) const
{
    return pClientProfile ? std::find(pClientProfile->supportedResources.begin(), pClientProfile->supportedResources.end(), purpose) != pClientProfile->supportedResources.end() : true;
}

int Quirks::getCaptionInfoCount() const
{
    return pClientProfile ? pClientProfile->captionInfoCount : -1;
}

int Quirks::getStringLimit() const
{
    return pClientProfile ? pClientProfile->stringLimit : -1;
}

bool Quirks::getMultiValue() const
{
    return pClientProfile ? pClientProfile->multiValue : true;
}

bool Quirks::getFullFilter() const
{
    return pClientProfile ? pClientProfile->fullFilter : false;
}

std::map<std::string, std::string> Quirks::getMimeMappings() const
{
    return pClientProfile ? pClientProfile->mimeMappings.getDictionaryOption() : std::map<std::string, std::string>();
}

std::vector<std::vector<std::pair<std::string, std::string>>> Quirks::getDlnaMappings() const
{
    return pClientProfile ? pClientProfile->dlnaMappings.getVectorOption() : std::vector<std::vector<std::pair<std::string, std::string>>>();
}

bool Quirks::isAllowed() const
{
    return !pClientProfile || pClientProfile->isAllowed;
};

std::vector<std::string> Quirks::getForbiddenDirectories() const
{
    static auto empty = std::vector<std::string>();
    return (pClientProfile && pClientProfile->groupConfig) ? pClientProfile->groupConfig->getForbiddenDirectories() : empty;
}

void Quirks::updateHeaders(Headers& headers) const
{
    if (pClientProfile && !pClientProfile->headers.getDictionaryOption().empty())
        for (auto&& [key, value] : pClientProfile->headers.getDictionaryOption())
            headers.updateHeader(key, value);
}
