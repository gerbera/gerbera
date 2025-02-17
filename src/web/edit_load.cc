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
#include "util/xml_to_json.h"

#include <fmt/chrono.h>

const std::string Web::EditLoad::PAGE = "edit_load";

/// \brief: process request 'edit_load' to list contents of a folder
void Web::EditLoad::processPageAction(pugi::xml_node& element)
{
    std::string objID = param("object_id");
    if (objID.empty())
        throw_std_runtime_error("invalid object id");

    auto objectID = std::stoi(objID);
    auto obj = database->loadObject(getGroup(), objectID);

    // set handling of json properties
    xml2Json->setFieldType("value", FieldType::STRING);
    xml2Json->setFieldType("title", FieldType::STRING);

    auto item = writeCoreInfo(obj, element, objectID);
    auto metaData = writeMetadata(obj, item);
    writeAuxData(obj, item);
    writeResourceInfo(obj, item);

    // write item meta info
    if (obj->isItem()) {
        auto objItem = std::dynamic_pointer_cast<CdsItem>(obj);
        writeItemInfo(objItem, objectID, item, metaData);
    }

    // write container meta info
    if (obj->isContainer()) {
        writeContainerInfo(obj, item);
    }
}

pugi::xml_node Web::EditLoad::writeCoreInfo(const std::shared_ptr<CdsObject>& obj, pugi::xml_node& element, int objectID)
{
    auto item = element.append_child("item");
    item.append_attribute("object_id") = objectID;

    auto title = item.append_child("title");
    title.append_attribute("value") = obj->getTitle().c_str();
    title.append_attribute("editable") = obj->isVirtual() || objectID == CDS_ID_FS_ROOT;

    auto classEl = item.append_child("class");
    classEl.append_attribute("value") = obj->getClass().c_str();
    classEl.append_attribute("editable") = true;

    auto flagsEl = item.append_child("flags");
    flagsEl.append_attribute("value") = CdsObject::mapFlags(obj->getFlags()).c_str();
    flagsEl.append_attribute("editable") = false;

    if (obj->getMTime() > std::chrono::seconds::zero()) {
        auto lmtEl = item.append_child("last_modified");
        lmtEl.append_attribute("value") = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(obj->getMTime().count())).c_str();
        lmtEl.append_attribute("editable") = false;
    } else {
        auto lmtEl = item.append_child("last_modified");
        lmtEl.append_attribute("value") = "";
        lmtEl.append_attribute("editable") = false;
    }

    if (obj->getUTime() > std::chrono::seconds::zero()) {
        auto lmtEl = item.append_child("last_updated");
        lmtEl.append_attribute("value") = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(obj->getUTime().count())).c_str();
        lmtEl.append_attribute("editable") = false;
    } else {
        auto lmtEl = item.append_child("last_updated");
        lmtEl.append_attribute("value") = "";
        lmtEl.append_attribute("editable") = false;
    }

    item.append_child("obj_type").append_child(pugi::node_pcdata).set_value(CdsObject::mapObjectType(obj->getObjectType()).data());
    return item;
}

pugi::xml_node Web::EditLoad::writeMetadata(const std::shared_ptr<CdsObject>& obj, pugi::xml_node& item)
{
    auto metaData = item.append_child("metadata");
    xml2Json->setArrayName(metaData, "metadata");
    xml2Json->setFieldType("metavalue", FieldType::STRING);

    for (auto&& [key, val] : obj->getMetaData()) {
        auto metaEntry = metaData.append_child("metadata");
        metaEntry.append_attribute("metaname") = key.c_str();
        metaEntry.append_attribute("metavalue") = val.c_str();
        metaEntry.append_attribute("editable") = false;
    }
    return metaData;
}

void Web::EditLoad::writeAuxData(const std::shared_ptr<CdsObject>& obj, pugi::xml_node& item)
{
    auto auxData = item.append_child("auxdata");
    xml2Json->setArrayName(auxData, "auxdata");
    xml2Json->setFieldType("auxvalue", FieldType::STRING);

    for (auto&& [key, val] : obj->getAuxData()) {
        auto auxEntry = auxData.append_child("auxdata");
        auxEntry.append_attribute("auxname") = key.c_str();
        auxEntry.append_attribute("auxvalue") = val.c_str();
        auxEntry.append_attribute("editable") = false;
    }
}

void Web::EditLoad::writeResourceInfo(const std::shared_ptr<CdsObject>& obj, pugi::xml_node& item)
{
    auto resources = item.append_child("resources");
    xml2Json->setArrayName(resources, "resources");
    xml2Json->setFieldType("resvalue", FieldType::ENCSTRING);
    xml2Json->setFieldType("rawvalue", FieldType::STRING);
    auto objItem = std::dynamic_pointer_cast<CdsItem>(obj);

    for (auto&& resItem : obj->getResources()) {
        auto resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "----RESOURCE----";
        resEntry.append_attribute("resvalue") = fmt::to_string(resItem->getResId()).c_str();
        resEntry.append_attribute("editable") = false;

        resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "handlerType";
        resEntry.append_attribute("resvalue") = EnumMapper::mapContentHandler2String(resItem->getHandlerType()).c_str();
        resEntry.append_attribute("editable") = false;

        resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "purpose";
        resEntry.append_attribute("resvalue") = EnumMapper::getPurposeDisplay(resItem->getPurpose()).c_str();
        resEntry.append_attribute("editable") = false;

        // write resource content
        if (objItem) {
            std::string url = xmlBuilder->renderResourceURL(*objItem, *resItem, {});
            if (resItem->getPurpose() == ResourcePurpose::Thumbnail) {
                auto image = resources.append_child("resources");
                image.append_attribute("resname") = "image";
                image.append_attribute("resvalue") = url.c_str();
                image.append_attribute("editable") = false;
            } else {
                resEntry = resources.append_child("resources");
                resEntry.append_attribute("resname") = "link";
                resEntry.append_attribute("resvalue") = url.c_str();
                resEntry.append_attribute("editable") = false;
            }
        }

        // write resource parameters
        for (auto&& [key, val] : resItem->getParameters()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format(".{}", key).c_str();
            resEntry.append_attribute("resvalue") = val.c_str();
            resEntry.append_attribute("editable") = false;
        }
        // write resource attributes
        for (auto&& attr : ResourceAttributeIterator()) {
            auto val = resItem->getAttribute(attr);
            if (!val.empty()) {
                auto resEntry = resources.append_child("resources");
                resEntry.append_attribute("resname") = EnumMapper::getAttributeDisplay(attr).c_str();
                auto aVal = resItem->getAttributeValue(attr);
                if (aVal != val)
                    resEntry.append_attribute("rawvalue") = val.c_str();
                resEntry.append_attribute("resvalue") = aVal.c_str();
                resEntry.append_attribute("editable") = false;
            }
        }
        // write resource options
        for (auto&& [key, val] : resItem->getOptions()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format("-{}", key).c_str();
            resEntry.append_attribute("resvalue") = val.c_str();
            resEntry.append_attribute("editable") = false;
        }
    }
}

void Web::EditLoad::writeItemInfo(const std::shared_ptr<CdsItem>& objItem, int objectID, pugi::xml_node& item, pugi::xml_node& metaData)
{
    auto description = item.append_child("description");
    description.append_attribute("value") = objItem->getMetaData(MetadataFields::M_DESCRIPTION).c_str();
    description.append_attribute("editable") = true;

    auto location = item.append_child("location");
    location.append_attribute("value") = objItem->getLocation().c_str();
    location.append_attribute("editable") = !objItem->isPureItem() && objItem->isVirtual();

    auto mimeType = item.append_child("mime-type");
    mimeType.append_attribute("value") = objItem->getMimeType().c_str();
    mimeType.append_attribute("editable") = true;

    auto url = xmlBuilder->renderItemImageURL(objItem);
    if (url) {
        auto image = item.append_child("image");
        image.append_attribute("value") = url.value().c_str();
        image.append_attribute("editable") = false;
    }

    for (auto&& playStatus : database->getPlayStatusList(objectID)) {
        auto metaEntry = metaData.append_child("metadata");
        metaEntry.append_attribute("metaname") = fmt::format("upnp:playbackCount@group[{}]", playStatus->getGroup()).c_str();
        metaEntry.append_attribute("metavalue") = fmt::format("{}", playStatus->getPlayCount()).c_str();
        metaEntry.append_attribute("editable") = false;

        metaEntry = metaData.append_child("metadata");
        metaEntry.append_attribute("metaname") = fmt::format("upnp:lastPlaybackTime@group[{}]", playStatus->getGroup()).c_str();
        metaEntry.append_attribute("metavalue") = fmt::format("{:%Y-%m-%d T %H:%M:%S}", fmt::localtime(playStatus->getLastPlayed().count())).c_str();
        metaEntry.append_attribute("editable") = false;

        if (playStatus->getLastPlayedPosition() > std::chrono::seconds::zero()) {
            metaEntry = metaData.append_child("metadata");
            metaEntry.append_attribute("metaname") = fmt::format("upnp:lastPlaybackPosition@group[{}]", playStatus->getGroup()).c_str();
            metaEntry.append_attribute("metavalue") = fmt::format("{}", millisecondsToHMSF(playStatus->getLastPlayedPosition().count())).c_str();
            metaEntry.append_attribute("editable") = false;
        }

        if (playStatus->getBookMarkPosition() > std::chrono::seconds::zero()) {
            metaEntry = metaData.append_child("metadata");
            metaEntry.append_attribute("metaname") = fmt::format("samsung:bookmarkpos@group[{}]", playStatus->getGroup()).c_str();
            metaEntry.append_attribute("metavalue") = fmt::format("{}", millisecondsToHMSF(playStatus->getBookMarkPosition().count())).c_str();
            metaEntry.append_attribute("editable") = false;
        }
    }

    if (objItem->isExternalItem()) {
        auto protocol = item.append_child("protocol");
        protocol.append_attribute("value") = getProtocol(objItem->getResource(ContentHandler::DEFAULT)->getAttribute(ResourceAttribute::PROTOCOLINFO)).data();
        protocol.append_attribute("editable") = true;
    }
}

void Web::EditLoad::writeContainerInfo(const std::shared_ptr<CdsObject>& obj, pugi::xml_node& item)
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    auto url = xmlBuilder->renderContainerImageURL(cont);
    if (url) {
        auto image = item.append_child("image");
        image.append_attribute("value") = url.value().c_str();
        image.append_attribute("editable") = false;
    }
}
