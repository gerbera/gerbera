/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/action.cc - this file is part of MediaTomb.

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

/// @file web/action.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "common.h"
#include "config/config.h"
#include "database/database.h"
#include "database/db_param.h"
#include "exceptions.h"
#include "util/grb_fs.h"
#include "util/logger.h"

const std::string_view Web::Action::PAGE = "action";

bool Web::Action::processPageAction(Json::Value& element, const std::string& action)
{
    if (action.empty())
        throw_std_runtime_error("No 'action' given");

    if (action == "export") {
        Json::Value items;
        doBrowse(items);
        element["items"] = items;
    } else if (action == "import") {
        std::string fileName = param("file_name");
        if (fileName.empty())
            throw_std_runtime_error("No 'file_name' given");
        GrbFile file(fileName);
        if (!file.isReadable())
            throw_std_runtime_error("File '{}' cannot be read", fileName);
        auto content = file.readTextFile();
        try {
            auto result = doParse(content);
            log_debug("Extracted {} entries.", result.size());
            doImport(result);
        } catch (const std::exception& ex) {
            log_error(ex.what());
        }
    }

    return true;
}

void Web::Action::doImport(const std::vector<std::pair<std::shared_ptr<CdsObject>, std::string>>& content)
{
    auto getParent = [&](const std::string& parentPath) {
        if (parentPath.empty() || parentPath == "/")
            return database->loadObject(CDS_ID_ROOT);
        return database->findObjectByPath(parentPath, UNUSED_CLIENT_GROUP, DbFileType::Virtual);
    };

    for (auto&& [obj, parentPath] : content) {
        auto parentObj = getParent(parentPath);
        log_debug("parent {} found {}", parentPath, !!parentObj);
        if (parentObj) {
            try {
                // restore into same database
                auto oldObj = database->loadObject(obj->getID());
                if (oldObj && oldObj->getParentID() == parentObj->getID() && obj->getRefID() == oldObj->getRefID()) {
                    obj->setParentID(parentObj->getID());
                    database->updateObject(obj, nullptr);
                    log_info("Restoring {}/{}", parentPath, obj->getTitle());
                    continue;
                }
            } catch (const ObjectNotFoundException& ex) {
                log_debug("Object not found {}/{}", parentPath, obj->getTitle());
            }
        } else {
            log_warning("Cannot restore object {}/{}: parent not found", parentPath, obj->getTitle());
            continue;
        }

        obj->setParentID(parentObj->getID());
        if (obj->getSource() == ObjectSource::ImportModified && obj->isItem()) {
            auto refObj = database->findObjectByPath(obj->getLocation(), UNUSED_CLIENT_GROUP, DbFileType::File);
            if (!refObj) {
                log_warning("Cannot restore object {}/{}: reference not found", parentPath, obj->getTitle());
                continue;
            }
            std::unordered_set<int> children;
            auto cnt = database->getObjects(parentObj->getID(), false, children, false, refObj->getID());
            if (cnt == 1) {
                obj->setRefID(refObj->getID());
                obj->setID(*children.begin());
                database->updateObject(obj, nullptr);
                log_info("Restoring {}/{}", parentPath, obj->getTitle());
                continue;
            } else {
                log_warning("Cannot restore container {}/{}: {} references to update", parentPath, obj->getTitle(), cnt);
                continue;
            }
        }

        if (obj->getSource() == ObjectSource::ImportModified && obj->isContainer()) {
            log_warning("Cannot restore container {}/{}", parentPath, obj->getTitle());
            continue;
        }

        if (obj->getSource() == ObjectSource::User) {
            obj->setID(INVALID_OBJECT_ID);
            database->addObject(obj, nullptr);
            if (obj->isContainer()) {
                database->updateObject(obj, nullptr);
            }
            log_info("Restoring {}/{}", parentPath, obj->getTitle());
            continue;
        }

        log_warning("Cannot restore object {}/{}", parentPath, obj->getTitle());
    }
}

std::vector<std::pair<std::shared_ptr<CdsObject>, std::string>> Web::Action::doParse(const std::string& content)
{
    log_debug("start");
    Json::Value items;
    std::string errs;
    Json::CharReaderBuilder rbuilder;
    std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
    reader->parse(content.c_str(), content.c_str() + content.size(), &items, &errs);
    log_debug("Found {} entries.", items.size());
    if (!items.isArray())
        return {};

    std::vector<std::pair<std::shared_ptr<CdsObject>, std::string>> result;
    for (auto item : items) {
        auto obj = CdsObject::createObject(CdsObject::remapObjectType(item["obj_type"].asString()));
        obj->setID(item["id"].asInt());
        obj->setRefID(item["ref_id"].asInt());
        obj->setTitle(item["title"].asString());
        obj->setSortKey(item["sort_key"].asString());
        obj->setClass(item["upnp_class"].asString());
        obj->setSource(CdsObject::remapSource(item["source"].asString()));
        obj->setLocation(item["location"].asString(), CdsObject::remapEntryType(item["entry_type"].asString()));
        obj->setVirtual(item["virtual"].asBool());
        obj->setFlags(CdsObject::remapFlags(item["flags"].asString()));

        if (obj->isItem()) {
            auto cdsItem = std::static_pointer_cast<CdsItem>(obj);
            cdsItem->setPartNumber(item["part_number"].asInt());
            cdsItem->setTrackNumber(item["track_number"].asInt());
            cdsItem->setMimeType(item["mime_type"].asString());
        }

        for (auto meta : item["metadata"]) {
            obj->addMetaData(meta["name"].asString(), meta["value"].asString());
        }

        for (auto aux : item["auxdata"]) {
            obj->addMetaData(aux["name"].asString(), aux["value"].asString());
        }

        std::vector<std::shared_ptr<CdsResource>> resources;
        resources.reserve(item["resources"].size());
        for (auto resource : item["resources"]) {
            std::map<std::string, std::string> options;
            for (auto option : resource["options"]) {
                options[option["name"].asString()] = option["value"].asString();
            }
            std::map<std::string, std::string> parameters;
            for (auto parameter : resource["parameters"]) {
                parameters[parameter["name"].asString()] = parameter["value"].asString();
            }
            std::map<ResourceAttribute, std::string> attributes;
            for (auto attribute : resource["attributes"]) {
                attributes[EnumMapper::mapAttributeName(attribute["name"].asString())] = attribute["value"].asString();
            }
            auto res = std::make_shared<CdsResource>(
                EnumMapper::remapContentHandler(resource["handler_type"].asString()),
                EnumMapper::mapPurpose(resource["purpose"].asString()),
                attributes, parameters, options);
            res->setResId(resources.size());
            resources.push_back(std::move(res));
        }
        obj->setResources(resources);

        result.push_back(std::pair<std::shared_ptr<CdsObject>, std::string>(obj, item["parent_location"].asString()));
    }

    return result;
}

void Web::Action::doBrowse(Json::Value& items)
{
    log_debug("start");
    auto browseParam = BrowseParam(nullptr, 0);
    browseParam.addSource(ObjectSource::ImportModified);
    browseParam.addSource(ObjectSource::User);

    // get contents of request
    auto result = database->browse(browseParam);
    items["result_size"] = Json::Value(static_cast<Json::UInt64>(result.size()));
    items["total_matches"] = browseParam.getTotalMatches();

    Json::Value itemArray(Json::arrayValue);
    for (auto&& cdsObj : result) {
        Json::Value item;
        item["obj_type"] = CdsObject::mapObjectType(cdsObj->getObjectType());
        item["entry_type"] = CdsObject::mapEntryType(cdsObj->getEntryType());
        item["id"] = cdsObj->getID();
        item["ref_id"] = cdsObj->getRefID();
        item["title"] = cdsObj->getTitle();
        item["sort_key"] = cdsObj->getSortKey();
        item["upnp_class"] = cdsObj->getClass();
        item["source"] = CdsObject::mapSource(cdsObj->getSource());
        item["location"] = cdsObj->getLocation().string();
        item["virtual"] = cdsObj->isVirtual();
        item["flags"] = CdsObject::mapFlags(cdsObj->getFlags());
        if (cdsObj->getParentID() > INVALID_OBJECT_ID) {
            auto parent = database->loadObject(cdsObj->getParentID());
            auto parentLocation = parent->getLocation().string();
            if (parentLocation.empty())
                item["parent_location"] = "/";
            else
                item["parent_location"] = parentLocation;
        } else {
            item["parent_location"] = "/";
        }

        Json::Value metaData(Json::arrayValue);
        for (auto&& [key, val] : cdsObj->getMetaData()) {
            Json::Value metaEntry;
            metaEntry["name"] = key;
            metaEntry["value"] = val;
            metaData.append(metaEntry);
        }
        Json::Value auxData(Json::arrayValue);
        for (auto&& [key, val] : cdsObj->getAuxData()) {
            Json::Value auxEntry;
            auxEntry["name"] = key;
            auxEntry["value"] = val;
            auxData.append(auxEntry);
        }
        Json::Value resources(Json::arrayValue);
        for (auto&& resItem : cdsObj->getResources()) {
            Json::Value resource;
            resource["id"] = resItem->getResId();
            resource["handler_type"] = EnumMapper::mapContentHandler2String(resItem->getHandlerType());
            resource["purpose"] = EnumMapper::getPurposeDisplay(resItem->getPurpose());

            // write resource parameters
            Json::Value resourceParameters(Json::arrayValue);
            for (auto&& [key, val] : resItem->getParameters()) {
                Json::Value resEntry;
                resEntry["name"] = key;
                resEntry["value"] = val;
                resourceParameters.append(resEntry);
            }
            resource["parameters"] = resourceParameters;
            // write resource attributes
            Json::Value resourceAttributes(Json::arrayValue);
            for (auto&& attr : ResourceAttributeIterator()) {
                auto val = resItem->getAttribute(attr);
                if (!val.empty()) {
                    Json::Value resEntry;
                    resEntry["name"] = EnumMapper::getAttributeName(attr);
                    resEntry["value"] = val;
                    resourceAttributes.append(resEntry);
                }
            }
            resource["attributes"] = resourceAttributes;
            // write resource options
            Json::Value resourceOptions(Json::arrayValue);
            for (auto&& [key, val] : resItem->getOptions()) {
                Json::Value resEntry;
                resEntry["name"] = key;
                resEntry["value"] = val;
                resourceOptions.append(resEntry);
            }
            resource["options"] = resourceOptions;
            resources.append(resource);
        }

        if (cdsObj->isItem()) {
            auto cdsItem = std::static_pointer_cast<CdsItem>(cdsObj);
            item["part_number"] = cdsItem->getPartNumber();
            item["track_number"] = cdsItem->getTrackNumber();
            item["mime_type"] = cdsItem->getMimeType();
        } else {
            auto cdsCont = std::static_pointer_cast<CdsContainer>(cdsObj);
        }

        item["metadata"] = metaData;
        item["auxdata"] = auxData;
        item["resources"] = resources;
        itemArray.append(item);
    }

    items["item"] = itemArray;
}
