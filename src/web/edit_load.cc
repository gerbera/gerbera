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
#include <utility>

#include "cds_objects.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "util/tools.h"

web::edit_load::edit_load(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
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
    auto obj = storage->loadObject(objectID);

    auto root = xmlDoc->document_element();

    auto item = root.append_child("item");
    item.append_attribute("object_id") = objectID;

    auto title = item.append_child("title");
    title.append_attribute("value") = obj->getTitle().c_str();
    title.append_attribute("editable") = obj->isVirtual() || objectID == CDS_ID_FS_ROOT ? "1" : "0";

    auto classEl = item.append_child("class");
    classEl.append_attribute("value") = obj->getClass().c_str();
    classEl.append_attribute("editable") = true;

    int objectType = obj->getObjectType();
    item.append_child("obj_type").append_child(pugi::node_pcdata).set_value(CdsObject::mapObjectType(objectType).c_str());

    if (IS_CDS_ITEM(objectType)) {
        auto objItem = std::static_pointer_cast<CdsItem>(obj);

        auto description = item.append_child("description");
        description.append_attribute("value") = objItem->getMetadata("dc:description").c_str();
        description.append_attribute("editable") = true;

        auto location = item.append_child("location");
        location.append_attribute("value") = objItem->getLocation().c_str();
        if (IS_CDS_PURE_ITEM(objectType) || !objItem->isVirtual())
            location.append_attribute("editable") = false;
        else
            location.append_attribute("editable") = true;

        auto mimeType = item.append_child("mime-type");
        mimeType.append_attribute("value") = objItem->getMimeType().c_str();
        mimeType.append_attribute("editable") = true;

        auto metaData = item.append_child("metadata");
        xml2JsonHints->setArrayName(metaData, "metadata");
        xml2JsonHints->setFieldType("metavalue", "string");

        for (auto& metaItem : objItem->getMetadata() ) {
            auto metaEntry = metaData.append_child("metadata");
            metaEntry.append_attribute("metaname") = metaItem.first.c_str();
            metaEntry.append_attribute("metavalue") = metaItem.second.c_str();
            metaEntry.append_attribute("editable") = false;
        }

        auto auxData = item.append_child("auxdata");
        xml2JsonHints->setArrayName(auxData, "auxdata");
        xml2JsonHints->setFieldType("auxvalue", "string");

        for (auto& auxItem : objItem->getAuxData() ) {
            auto auxEntry = auxData.append_child("auxdata");
            auxEntry.append_attribute("auxname") = auxItem.first.c_str();
            auxEntry.append_attribute("auxvalue") = auxItem.second.c_str();
            auxEntry.append_attribute("editable") = false;
        }

        auto resources = item.append_child("resources");
        xml2JsonHints->setArrayName(resources, "resources");
        xml2JsonHints->setFieldType("resvalue", "string");

        int resIndex = 0;
        for (auto& resItem : objItem->getResources()) {
            auto resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = "----RESOURCE----";
            resEntry.append_attribute("resvalue") = fmt::format("{}", resIndex).c_str();
            resEntry.append_attribute("editable") = false;

            resEntry = resources.append_child("resources");
            resEntry.append_attribute("resname") = fmt::format("handlerType").c_str();
            resEntry.append_attribute("resvalue") = fmt::format("{}", MetadataHandler::mapContentHandler2String(resItem->getHandlerType()).c_str()).c_str();
            resEntry.append_attribute("editable") = false;

            for (auto& resPar : resItem->getParameters()) {
                auto resEntry = resources.append_child("resources");
                resEntry.append_attribute("resname") = fmt::format(".{}", resPar.first.c_str()).c_str();
                resEntry.append_attribute("resvalue") = resPar.second.c_str();
                resEntry.append_attribute("editable") = false;
            }
            for (auto& resAtt : resItem->getAttributes()) {
                auto resEntry = resources.append_child("resources");
                resEntry.append_attribute("resname") = fmt::format(" {}", resAtt.first.c_str()).c_str();
                resEntry.append_attribute("resvalue") = resAtt.second.c_str();
                resEntry.append_attribute("editable") = false;
            }
            for (auto& resOpt : resItem->getOptions()) {
                auto resEntry = resources.append_child("resources");
                resEntry.append_attribute("resname") = fmt::format("-{}", resOpt.first.c_str()).c_str();
                resEntry.append_attribute("resvalue") = resOpt.second.c_str();
                resEntry.append_attribute("editable") = false;
            }
            resIndex++;
        }

        if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
            auto protocol = item.append_child("protocol");
            protocol.append_attribute("value") = getProtocol(objItem->getResource(0)->getAttribute("protocolInfo")).c_str();
            protocol.append_attribute("editable") = true;
        } else if (IS_CDS_ACTIVE_ITEM(objectType)) {
            auto objActiveItem = std::static_pointer_cast<CdsActiveItem>(objItem);

            auto action = item.append_child("action");
            action.append_attribute("value") = objActiveItem->getAction().c_str();
            action.append_attribute("editable") = true;

            auto state = item.append_child("state");
            state.append_attribute("value") = objActiveItem->getState().c_str();
            state.append_attribute("editable") = true;
        }
    }
}
