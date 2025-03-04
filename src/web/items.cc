/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/items.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/items.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "database/database.h"
#include "database/db_param.h"
#include "exceptions.h"
#include "upnp/quirks.h"
#include "upnp/xml_builder.h"
#include "util/xml_to_json.h"

const std::string_view Web::Items::PAGE = "items";

/// \brief orocess request for item list in ui
bool Web::Items::processPageAction(pugi::xml_node& element, const std::string& action)
{
    int parentID = intParam("parent_id");
    int start = intParam("start");
    if (start < 0)
        throw_std_runtime_error("illegal start {} parameter", start);

    int count = intParam("count");
    if (count < 0)
        throw_std_runtime_error("illegal count {} parameter", count);

    // set result options
    auto items = element.append_child("items");
    xml2Json->setArrayName(items, "item");
    xml2Json->setFieldType("title", FieldType::STRING);
    xml2Json->setFieldType("part", FieldType::STRING);
    xml2Json->setFieldType("track", FieldType::STRING);
    xml2Json->setFieldType("index", FieldType::STRING);
    xml2Json->setFieldType("autoscan_mode", FieldType::STRING);
    xml2Json->setFieldType("autoscan_type", FieldType::STRING);
    items.append_attribute("parent_id") = parentID;

    auto container = database->loadObject(getGroup(), parentID);
    std::string trackFmt = "{:02}";
    auto result = action == "browse"
        ? doBrowse(container, start, count, items, trackFmt)
        : doSearch(container, start, count, items, trackFmt);

    // ouput objects of container
    int cnt = start + 1;
    for (auto&& cdsObj : result) {
        auto item = items.append_child("item");
        item.append_attribute("id") = cdsObj->getID();
        item.append_child("title").append_child(pugi::node_pcdata).set_value(cdsObj->getTitle().c_str());
        item.append_child("upnp_class").append_child(pugi::node_pcdata).set_value(cdsObj->getClass().c_str());

        item.append_child("index").append_child(pugi::node_pcdata).set_value(fmt::format(trackFmt, cnt).c_str());
        if (cdsObj->isItem()) {
            auto cdsItem = std::static_pointer_cast<CdsItem>(cdsObj);
            if (cdsItem->getPartNumber() > 0 && container->isSubClass(UPNP_CLASS_MUSIC_ALBUM))
                item.append_child("part").append_child(pugi::node_pcdata).set_value(fmt::format("{:02}", cdsItem->getPartNumber()).c_str());
            if (cdsItem->getTrackNumber() > 0 && !container->isSubClass(UPNP_CLASS_CONTAINER))
                item.append_child("track").append_child(pugi::node_pcdata).set_value(fmt::format(trackFmt, cdsItem->getTrackNumber()).c_str());
            item.append_child("mtype").append_child(pugi::node_pcdata).set_value(cdsItem->getMimeType().c_str());
            auto contRes = cdsObj->getResource(ResourcePurpose::Content);
            if (contRes) {
                item.append_child("size").append_child(pugi::node_pcdata).set_value(contRes->getAttributeValue(ResourceAttribute::SIZE).c_str());
                if (!cdsItem->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                    item.append_child("resolution").append_child(pugi::node_pcdata).set_value(contRes->getAttribute(ResourceAttribute::RESOLUTION).c_str());
                }
                if (!cdsItem->isSubClass(UPNP_CLASS_IMAGE_ITEM)) {
                    item.append_child("duration").append_child(pugi::node_pcdata).set_value(contRes->getAttributeValue(ResourceAttribute::DURATION).c_str());
                }
            }
            std::string resPath = xmlBuilder->getFirstResourcePath(cdsItem);
            if (!resPath.empty())
                item.append_child("res").append_child(pugi::node_pcdata).set_value(resPath.c_str());
            auto url = xmlBuilder->renderItemImageURL(cdsItem);
            if (url) {
                item.append_child("image").append_child(pugi::node_pcdata).set_value(url.value().c_str());
            }
        } else {
            auto cdsCont = std::static_pointer_cast<CdsContainer>(cdsObj);
            auto url = xmlBuilder->renderContainerImageURL(cdsCont);
            if (url) {
                item.append_child("image").append_child(pugi::node_pcdata).set_value(url.value().c_str());
            }
        }
        cnt++;
    }

    return true;
}

std::vector<std::shared_ptr<CdsObject>> Web::Items::doBrowse(
    const std::shared_ptr<CdsObject>& container,
    int start,
    int count,
    pugi::xml_node& items,
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
    items.append_attribute("virtual") = container->isVirtual();
    items.append_attribute("start") = start;
    items.append_attribute("result_size") = result.size();
    items.append_attribute("total_matches") = browseParam.getTotalMatches();

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
    items.append_attribute("autoscan_mode") = autoscanMode.c_str();
    items.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).data();
    items.append_attribute("protect_container") = protectContainer;
    items.append_attribute("protect_items") = protectItems;

    if (browseParam.getTotalMatches() >= 100)
        trackFmt = "{:03}";

    return result;
}

std::vector<std::shared_ptr<CdsObject>> Web::Items::doSearch(
    const std::shared_ptr<CdsObject>& container,
    int start,
    int count,
    pugi::xml_node& items,
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
    items.append_attribute("virtual") = container->isVirtual();
    items.append_attribute("start") = start;
    items.append_attribute("result_size") = result.size();
    items.append_attribute("total_matches") = searchParam.getTotalMatches();

    if (searchParam.getTotalMatches() >= 100)
        trackFmt = "{:03}";

    return result;
}
