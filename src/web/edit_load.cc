/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/edit_load.cc - this file is part of MediaTomb.

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

/// \file web/edit_load.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "database/database.h"
#include "exceptions.h"
#include "upnp/clients.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"

#include <fmt/chrono.h>

const std::string_view Web::EditLoad::PAGE = "edit_load";

/// \brief: process request 'edit_load' to list contents of a folder
bool Web::EditLoad::processPageAction(Json::Value& element, const std::string& action)
{
    std::string objID = param("object_id");
    if (objID.empty())
        throw_std_runtime_error("invalid object id");

    auto objectID = std::stoi(objID);
    auto obj = database->loadObject(getGroup(), objectID);

    Json::Value item;
    writeCoreInfo(obj, item, objectID);
    Json::Value metaData(Json::arrayValue);
    writeMetadata(obj, metaData);
    Json::Value auxData(Json::arrayValue);
    writeAuxData(obj, auxData);
    Json::Value resources(Json::arrayValue);
    writeResourceInfo(obj, resources);

    // write item meta info
    if (obj->isItem()) {
        auto objItem = std::dynamic_pointer_cast<CdsItem>(obj);
        writeItemInfo(objItem, objectID, item, metaData);
    }

    // write container meta info
    if (obj->isContainer()) {
        writeContainerInfo(obj, item);
    }
    item["metadata"] = metaData;
    item["auxdata"] = auxData;
    item["resources"] = resources;
    element["item"] = item;
    return true;
}

void Web::EditLoad::writeCoreInfo(
    const std::shared_ptr<CdsObject>& obj,
    Json::Value& item,
    int objectID)
{
    item["object_id"] = objectID;

    Json::Value title;
    title["value"] = obj->getTitle();
    title["editable"] = obj->isVirtual() || objectID == CDS_ID_FS_ROOT;
    item["title"] = title;

    Json::Value classEl;
    classEl["value"] = obj->getClass();
    classEl["editable"] = true;
    item["class"] = classEl;

    Json::Value flagsEl;
    flagsEl["value"] = CdsObject::mapFlags(obj->getFlags());
    flagsEl["editable"] = false;
    item["flags"] = flagsEl;

    Json::Value lmtEl;
    if (obj->getMTime() > std::chrono::seconds::zero()) {
        lmtEl["value"] = grbLocaltime("{:%Y-%m-%d %H:%M:%S}", obj->getMTime());
    } else {
        lmtEl["value"] = "";
    }
    lmtEl["editable"] = false;
    item["last_modified"] = lmtEl;

    Json::Value lutEl;
    if (obj->getUTime() > std::chrono::seconds::zero()) {
        lutEl["value"] = grbLocaltime("{:%Y-%m-%d %H:%M:%S}", obj->getUTime());
    } else {
        lutEl["value"] = "";
    }
    lutEl["editable"] = false;
    item["last_updated"] = lutEl;

    item["obj_type"] = CdsObject::mapObjectType(obj->getObjectType()).data();
}

void Web::EditLoad::writeMetadata(
    const std::shared_ptr<CdsObject>& obj,
    Json::Value& metadataArray)
{
    for (auto&& [key, val] : obj->getMetaData()) {
        Json::Value metaEntry;
        metaEntry["metaname"] = key;
        metaEntry["metavalue"] = val;
        metaEntry["editable"] = false;
        metadataArray.append(metaEntry);
    }
}

void Web::EditLoad::writeAuxData(
    const std::shared_ptr<CdsObject>& obj,
    Json::Value& auxdataArray)
{
    for (auto&& [key, val] : obj->getAuxData()) {
        Json::Value auxEntry;
        auxEntry["auxname"] = key;
        auxEntry["auxvalue"] = val;
        auxEntry["editable"] = false;
        auxdataArray.append(auxEntry);
    }
}

void Web::EditLoad::writeResourceInfo(
    const std::shared_ptr<CdsObject>& obj,
    Json::Value& resourceArray)
{
    auto objItem = std::dynamic_pointer_cast<CdsItem>(obj);

    for (auto&& resItem : obj->getResources()) {
        {
            Json::Value resEntry;
            resEntry["resname"] = "----RESOURCE----";
            resEntry["resvalue"] = fmt::to_string(resItem->getResId());
            resEntry["editable"] = false;
            resourceArray.append(resEntry);
        }

        {
            Json::Value resEntry;
            resEntry["resname"] = "handlerType";
            resEntry["resvalue"] = EnumMapper::mapContentHandler2String(resItem->getHandlerType());
            resEntry["editable"] = false;
            resourceArray.append(resEntry);
        }

        {
            Json::Value resEntry;
            resEntry["resname"] = "purpose";
            resEntry["resvalue"] = EnumMapper::getPurposeDisplay(resItem->getPurpose());
            resEntry["editable"] = false;
            resourceArray.append(resEntry);
        }

        // write resource content
        if (objItem) {
            std::string url = xmlBuilder->renderResourceURL(*objItem, *resItem, {});
            Json::Value resEntry;
            resEntry["resvalue"] = url;
            resEntry["editable"] = false;
            if (resItem->getPurpose() == ResourcePurpose::Thumbnail) {
                resEntry["resname"] = "image";
            } else {
                resEntry["resname"] = "link";
            }
            resourceArray.append(resEntry);
        }

        // write resource parameters
        for (auto&& [key, val] : resItem->getParameters()) {
            Json::Value resEntry;
            resEntry["resname"] = fmt::format(".{}", key);
            resEntry["resvalue"] = val;
            resEntry["editable"] = false;
            resourceArray.append(resEntry);
        }
        // write resource attributes
        for (auto&& attr : ResourceAttributeIterator()) {
            auto val = resItem->getAttribute(attr);
            if (!val.empty()) {
                Json::Value resEntry;
                resEntry["resname"] = EnumMapper::getAttributeDisplay(attr);
                auto aVal = resItem->getAttributeValue(attr);
                if (aVal != val)
                    resEntry["rawvalue"] = val;
                resEntry["resvalue"] = aVal;
                resEntry["editable"] = false;
                resourceArray.append(resEntry);
            }
        }
        // write resource options
        for (auto&& [key, val] : resItem->getOptions()) {
            Json::Value resEntry;
            resEntry["resname"] = fmt::format("-{}", key);
            resEntry["resvalue"] = val;
            resEntry["editable"] = false;
            resourceArray.append(resEntry);
        }
    }
}

void Web::EditLoad::writeItemInfo(
    const std::shared_ptr<CdsItem>& objItem,
    int objectID,
    Json::Value& item,
    Json::Value& metadataArray)
{
    Json::Value description;
    description["value"] = objItem->getMetaData(MetadataFields::M_DESCRIPTION);
    description["editable"] = true;
    item["description"] = description;

    Json::Value location;
    location["value"] = objItem->getLocation().string();
    location["editable"] = !objItem->isPureItem() && objItem->isVirtual();
    item["location"] = location;

    Json::Value mimeType;
    mimeType["value"] = objItem->getMimeType();
    mimeType["editable"] = true;
    item["mime-type"] = mimeType;

    auto url = xmlBuilder->renderItemImageURL(objItem);
    if (url) {
        Json::Value image;
        image["value"] = url.value();
        image["editable"] = false;
        item["image"] = image;
    }

    for (auto&& playStatus : database->getPlayStatusList(objectID)) {
        {
            Json::Value metaEntry;
            metaEntry["metaname"] = fmt::format("upnp:playbackCount@group[{}]", playStatus->getGroup());
            metaEntry["metavalue"] = fmt::format("{}", playStatus->getPlayCount());
            metaEntry["editable"] = false;
            metadataArray.append(metaEntry);
        }

        {
            Json::Value metaEntry;
            metaEntry["metaname"] = fmt::format("upnp:lastPlaybackTime@group[{}]", playStatus->getGroup());
            metaEntry["metavalue"] = grbLocaltime("{:%Y-%m-%d T %H:%M:%S}", playStatus->getLastPlayed());
            metaEntry["editable"] = false;
            metadataArray.append(metaEntry);
        }

        if (playStatus->getLastPlayedPosition() > std::chrono::seconds::zero()) {
            Json::Value metaEntry;
            metaEntry["metaname"] = fmt::format("upnp:lastPlaybackPosition@group[{}]", playStatus->getGroup());
            metaEntry["metavalue"] = fmt::format("{}", millisecondsToHMSF(playStatus->getLastPlayedPosition().count()));
            metaEntry["editable"] = false;
            metadataArray.append(metaEntry);
        }

        if (playStatus->getBookMarkPosition() > std::chrono::seconds::zero()) {
            Json::Value metaEntry;
            metaEntry["metaname"] = fmt::format("samsung:bookmarkpos@group[{}]", playStatus->getGroup());
            metaEntry["metavalue"] = fmt::format("{}", millisecondsToHMSF(playStatus->getBookMarkPosition().count()));
            metaEntry["editable"] = false;
            metadataArray.append(metaEntry);
        }
    }

    if (objItem->isExternalItem()) {
        Json::Value protocol;
        protocol["value"] = getProtocol(objItem->getResource(ContentHandler::DEFAULT)->getAttribute(ResourceAttribute::PROTOCOLINFO)).data();
        protocol["editable"] = true;
        item["protocol"] = protocol;
    }
}

void Web::EditLoad::writeContainerInfo(
    const std::shared_ptr<CdsObject>& obj,
    Json::Value& item)
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    auto url = xmlBuilder->renderContainerImageURL(cont);
    if (url) {
        Json::Value image;
        image["value"] = url.value();
        image["editable"] = false;
        item["image"] = image;
    }
}
