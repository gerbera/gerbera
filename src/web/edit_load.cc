/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    edit_load.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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

/// \file edit_load.cc

#include "pages.h" // API

#include <cstdio>
#include <fmt/chrono.h>

#include "cds_objects.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "upnp_xml.h"
#include "util/tools.h"

web::edit_load::edit_load(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::edit_load::process()
{
    check_request();

    std::string objID = param("object_id");
    int objectID;
    if (objID.empty())
        throw_std_runtime_error("invalid object id");

    objectID = std::stoi(objID);
    auto obj = database->loadObject(objectID);

    auto root = xmlDoc->document_element();
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("title", "string");

    auto item = root.append_child("item");
    item.append_attribute("object_id") = objectID;

    auto title = item.append_child("title");
    title.append_attribute("value") = obj->getTitle().c_str();
    title.append_attribute("editable") = obj->isVirtual() || objectID == CDS_ID_FS_ROOT;

    auto classEl = item.append_child("class");
    classEl.append_attribute("value") = obj->getClass().c_str();
    classEl.append_attribute("editable") = true;

    if (obj->getMTime() > std::chrono::seconds::zero()) {
        auto lmtEl = item.append_child("last_modified");
        lmtEl.append_attribute("value") = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(obj->getMTime().count())).c_str();
        lmtEl.append_attribute("editable") = false;
    } else {
        auto lmtEl = item.append_child("last_modified");
        lmtEl.append_attribute("value") = "";
        lmtEl.append_attribute("editable") = false;
    }

    item.append_child("obj_type").append_child(pugi::node_pcdata).set_value(CdsObject::mapObjectType(obj->getObjectType()).data());

    auto metaData = item.append_child("metadata");
    xml2JsonHints->setArrayName(metaData, "metadata");
    xml2JsonHints->setFieldType("metavalue", "string");

    for (auto&& [key, val] : obj->getMetadata()) {
        auto metaEntry = metaData.append_child("metadata");
        metaEntry.append_attribute("metaname") = key.c_str();
        metaEntry.append_attribute("metavalue") = val.c_str();
        metaEntry.append_attribute("editable") = false;
    }

    auto auxData = item.append_child("auxdata");
    xml2JsonHints->setArrayName(auxData, "auxdata");
    xml2JsonHints->setFieldType("auxvalue", "string");

    for (auto&& [key, val] : obj->getAuxData()) {
        auto auxEntry = auxData.append_child("auxdata");
        auxEntry.append_attribute("auxname") = key.c_str();
        auxEntry.append_attribute("auxvalue") = val.c_str();
        auxEntry.append_attribute("editable") = false;
    }

    auto resources = item.append_child("resources");
    xml2JsonHints->setArrayName(resources, "resources");
    xml2JsonHints->setFieldType("resvalue", "string");

    int resIndex = 0;
    for (auto&& resItem : obj->getResources()) {
        auto resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "----RESOURCE----";
        resEntry.append_attribute("resvalue") = fmt::format("{}", resIndex).c_str();
        resEntry.append_attribute("editable") = false;

        resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "handlerType";
        resEntry.append_attribute("resvalue") = fmt::format("{}", MetadataHandler::mapContentHandler2String(resItem->getHandlerType())).c_str();
        resEntry.append_attribute("editable") = false;

        for (auto&& [key, val] : resItem->getParameters()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format(".{}", key.c_str()).c_str();
            resEntry.append_attribute("resvalue") = val.c_str();
            resEntry.append_attribute("editable") = false;
        }
        for (auto&& [key, val] : resItem->getAttributes()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format(" {}", key.c_str()).c_str();
            resEntry.append_attribute("resvalue") = val.c_str();
            resEntry.append_attribute("editable") = false;
        }
        for (auto&& [key, val] : resItem->getOptions()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format("-{}", key.c_str()).c_str();
            resEntry.append_attribute("resvalue") = val.c_str();
            resEntry.append_attribute("editable") = false;
        }
        resIndex++;
    }

    if (obj->isItem()) {
        auto objItem = std::static_pointer_cast<CdsItem>(obj);

        auto description = item.append_child("description");
        description.append_attribute("value") = objItem->getMetadata(M_DESCRIPTION).c_str();
        description.append_attribute("editable") = true;

        auto location = item.append_child("location");
        location.append_attribute("value") = objItem->getLocation().c_str();
        location.append_attribute("editable") = !(obj->isPureItem() || !obj->isVirtual());

        auto mimeType = item.append_child("mime-type");
        mimeType.append_attribute("value") = objItem->getMimeType().c_str();
        mimeType.append_attribute("editable") = true;

        std::string url;
        if (UpnpXMLBuilder::renderItemImage(server->getVirtualUrl(), objItem, url)) {
            auto image = item.append_child("image");
            image.append_attribute("value") = url.c_str();
            image.append_attribute("editable") = false;
        }

        if (obj->isExternalItem()) {
            auto protocol = item.append_child("protocol");
            protocol.append_attribute("value") = getProtocol(objItem->getResource(0)->getAttribute(R_PROTOCOLINFO)).c_str();
            protocol.append_attribute("editable") = true;
        }
    }

    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        std::string url;
        if (UpnpXMLBuilder::renderContainerImage(server->getVirtualUrl(), cont, url)) {
            auto image = item.append_child("image");
            image.append_attribute("value") = url.c_str();
            image.append_attribute("editable") = false;
        }
    }
}
