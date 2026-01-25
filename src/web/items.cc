/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/items.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// @file web/items.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "database/database.h"
#include "database/db_param.h"
#include "exceptions.h"
#include "upnp/quirks.h"
#include "upnp/xml_builder.h"

const std::string_view Web::Items::PAGE = "items";

/// @brief orocess request for item list in ui
bool Web::Items::processPageAction(Json::Value& element, const std::string& action)
{
    int parentID = intParam("parent_id");
    int start = intParam("start");
    if (start < 0)
        throw_std_runtime_error("illegal start {} parameter", start);

    int count = intParam("count");
    if (count < 0)
        throw_std_runtime_error("illegal count {} parameter", count);

    // set result options
    Json::Value items;
    items["parent_id"] = parentID;

    auto container = database->loadObject(getGroup(), parentID);
    std::string trackFmt = "{:02}";
    auto result = action == "browse"
        ? doBrowse(container, start, count, items, trackFmt)
        : doSearch(container, start, count, items, trackFmt);

    // ouput objects of container
    int cnt = start + 1;
    Json::Value itemArray(Json::arrayValue);
    for (auto&& cdsObj : result) {
        Json::Value item;
        item["id"] = cdsObj->getID();
        item["title"] = cdsObj->getTitle();
        item["upnp_class"] = cdsObj->getClass();
        item["index"] = fmt::format(trackFmt, cnt);
        item["source"] = CdsObject::mapSource(cdsObj->getSource());

        if (cdsObj->isItem()) {
            auto cdsItem = std::static_pointer_cast<CdsItem>(cdsObj);
            if (cdsItem->getPartNumber() > 0 && container->isSubClass(UPNP_CLASS_MUSIC_ALBUM))
                item["part"] = fmt::format("{:02}", cdsItem->getPartNumber());
            if (cdsItem->getTrackNumber() > 0 && !container->isSubClass(UPNP_CLASS_CONTAINER))
                item["track"] = fmt::format(trackFmt, cdsItem->getTrackNumber());
            item["mtype"] = cdsItem->getMimeType();
            auto contRes = cdsObj->getResource(ResourcePurpose::Content);
            if (contRes) {
                item["size"] = contRes->getAttributeValue(ResourceAttribute::SIZE);
                if (!cdsItem->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                    item["resolution"] = contRes->getAttribute(ResourceAttribute::RESOLUTION);
                }
                if (!cdsItem->isSubClass(UPNP_CLASS_IMAGE_ITEM)) {
                    item["duration"] = contRes->getAttributeValue(ResourceAttribute::DURATION);
                }
            }
            std::string resPath = xmlBuilder->getFirstResourcePath(cdsItem);
            if (!resPath.empty())
                item["res"] = resPath;
            auto url = xmlBuilder->renderItemImageURL(cdsItem);
            if (url) {
                item["image"] = url.value();
            }
        } else {
            auto cdsCont = std::static_pointer_cast<CdsContainer>(cdsObj);
            auto url = xmlBuilder->renderContainerImageURL(cdsCont);
            if (url) {
                item["image"] = url.value();
            }
        }
        itemArray.append(item);
        cnt++;
    }

    items["item"] = itemArray;
    element["items"] = items;
    return true;
}

std::vector<std::shared_ptr<CdsObject>> Web::Items::doBrowse(
    const std::shared_ptr<CdsObject>& container,
    int start,
    int count,
    Json::Value& items,
    std::string& trackFmt)
{
    log_debug("start");
    auto browseParam = BrowseParam(container, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS);
    browseParam.setRange(start, count);
    if (quirks)
        browseParam.setForbiddenDirectories(quirks->getForbiddenDirectories());

    if (container->isSubClass(UPNP_CLASS_MUSIC_ALBUM) || container->isSubClass(UPNP_CLASS_PLAYLIST_CONTAINER))
        browseParam.setFlag(BROWSE_TRACK_SORT);

    // get contents of request
    auto result = database->browse(browseParam);
    items["virtual"] = container->isVirtual();
    items["start"] = start;
    items["result_size"] = Json::Value(static_cast<Json::UInt64>(result.size()));
    items["total_matches"] = browseParam.getTotalMatches();

    bool protectContainer = container->isSubClass(UPNP_CLASS_DYNAMIC_CONTAINER);
    bool protectItems = container->isSubClass(UPNP_CLASS_DYNAMIC_CONTAINER);
    std::string autoscanMode = "none";

    auto parentDir = content->getAutoscanDirectory(container->getLocation());
    auto autoscanType = AutoscanType::None;
    if (parentDir) {
        autoscanType = parentDir->persistent() ? AutoscanType::Config : AutoscanType::Ui;
        autoscanMode = AutoscanDirectory::mapScanmode(parentDir->getScanMode());
        protectItems = true;
        if (!parentDir->persistent())
            protectContainer = true;
    }

    // check path for autoscans
    if (autoscanType == AutoscanType::None && !container->getLocation().empty()) {
        for (fs::path path = container->getLocation(); path != "/" && !path.empty(); path = path.parent_path()) {
            auto startPtDir = content->getAutoscanDirectory(path);
            if (startPtDir && startPtDir->getRecursive()) {
                protectItems = true;
                if (!startPtDir->persistent())
                    protectContainer = true;
                autoscanMode = AutoscanDirectory::mapScanmode(startPtDir->getScanMode());
                break;
            }
        }
    }
    items["autoscan_mode"] = autoscanMode;
    items["autoscan_type"] = mapAutoscanType(autoscanType);
    items["protect_container"] = protectContainer;
    items["protect_items"] = protectItems;

    if (browseParam.getTotalMatches() >= 100)
        trackFmt = "{:03}";

    return result;
}

std::vector<std::shared_ptr<CdsObject>> Web::Items::doSearch(
    const std::shared_ptr<CdsObject>& container,
    int start,
    int count,
    Json::Value& items,
    std::string& trackFmt)
{
    log_debug("start");
    std::vector<std::shared_ptr<CdsObject>> result;

    std::string searchCriteria = param("searchCriteria");
    std::string sortCriteria = param("sortCriteria");
    bool searchableContainers = boolParam("searchableContainers");
    int containerId = container->getID();

    log_debug("Search received parameters: ContainerID [{}] SearchCriteria [{}] SortCriteria [{}] StartingIndex [{}] Filter [{}] RequestedCount [{}]",
        containerId, searchCriteria, sortCriteria, start, '*', count);
    if (searchCriteria.empty()) {
        log_warning("Empty query string");
        throw UpnpException(UPNP_E_INVALID_ARGUMENT, "Empty query string");
    }
    auto searchParam = SearchParam(fmt::format("{}", containerId), searchCriteria, sortCriteria,
        start, count, searchableContainers, getGroup());
    if (quirks)
        searchParam.setForbiddenDirectories(quirks->getForbiddenDirectories());
    // Execute database search

    try {
        result = database->search(searchParam);
        log_debug("Found {}/{} items", result.size(), searchParam.getTotalMatches());
    } catch (const SearchParseException& srcEx) {
        log_warning(srcEx.what());
        throw UpnpException(UPNP_E_INVALID_ARGUMENT, srcEx.getUserMessage());
    } catch (const DatabaseException& dbEx) {
        log_warning(dbEx.what());
        throw UpnpException(UPNP_E_NO_SUCH_ID, dbEx.getUserMessage());
    } catch (const std::runtime_error& e) {
        log_warning(e.what());
        throw UpnpException(UPNP_E_BAD_REQUEST, UpnpXMLBuilder::encodeEscapes(e.what()));
    }
    items["virtual"] = container->isVirtual();
    items["start"] = start;
    items["result_size"] = Json::Value(static_cast<Json::UInt64>(result.size()));
    items["total_matches"] = searchParam.getTotalMatches();

    if (searchParam.getTotalMatches() >= 100)
        trackFmt = "{:03}";

    return result;
}
