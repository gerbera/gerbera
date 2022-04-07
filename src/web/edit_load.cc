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

#include <fmt/chrono.h>

#include "cds_objects.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "upnp_xml.h"
#include "util/tools.h"
#include "util/upnp_clients.h"

Web::EditLoad::EditLoad(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder)
    : WebRequestHandler(content)
    , xmlBuilder(std::move(xmlBuilder))
{
}

static constexpr auto upnpFlags = std::array {
    std::pair("Restricted", OBJECT_FLAG_RESTRICTED),
    std::pair("Searchable", OBJECT_FLAG_SEARCHABLE),
    std::pair("UseResourceRef", OBJECT_FLAG_USE_RESOURCE_REF),
    std::pair("PersistentContainer", OBJECT_FLAG_PERSISTENT_CONTAINER),
    std::pair("PlaylistRef", OBJECT_FLAG_PLAYLIST_REF),
    std::pair("ProxyUrl", OBJECT_FLAG_PROXY_URL),
    std::pair("OnlineService", OBJECT_FLAG_ONLINE_SERVICE),
    std::pair("OggTheora", OBJECT_FLAG_OGG_THEORA),
};

static std::string mapFlags(int flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [uLabel, uFlag] : upnpFlags) {
        if (flags & uFlag) {
            myFlags.emplace_back(uLabel);
            flags &= ~uFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

/// \brief: process request 'edit_load' to list contents of a folder
void Web::EditLoad::process()
{
    checkRequest();

    std::string objID = param("object_id");
    if (objID.empty())
        throw_std_runtime_error("invalid object id");

    auto objectID = std::stoi(objID);
    auto obj = database->loadObject(DEFAULT_CLIENT_GROUP, objectID);

    // set handling of json properties
    auto root = xmlDoc->document_element();
    xml2JsonHints->setFieldType("value", "string");
    xml2JsonHints->setFieldType("title", "string");

    // write object core info
    auto item = root.append_child("item");
    item.append_attribute("object_id") = objectID;

    auto title = item.append_child("title");
    title.append_attribute("value") = obj->getTitle().c_str();
    title.append_attribute("editable") = obj->isVirtual() || objectID == CDS_ID_FS_ROOT;

    auto classEl = item.append_child("class");
    classEl.append_attribute("value") = obj->getClass().c_str();
    classEl.append_attribute("editable") = true;

    auto flagsEl = item.append_child("flags");
    flagsEl.append_attribute("value") = mapFlags(obj->getFlags()).c_str();
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

    // write metadata
    auto metaData = item.append_child("metadata");
    xml2JsonHints->setArrayName(metaData, "metadata");
    xml2JsonHints->setFieldType("metavalue", "string");

    for (auto&& [key, val] : obj->getMetaData()) {
        auto metaEntry = metaData.append_child("metadata");
        metaEntry.append_attribute("metaname") = key.c_str();
        metaEntry.append_attribute("metavalue") = val.c_str();
        metaEntry.append_attribute("editable") = false;
    }

    // write auxdata
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
    xml2JsonHints->setFieldType("rawvalue", "string");

    auto objItem = std::dynamic_pointer_cast<CdsItem>(obj);

    // write resource info
    for (auto&& resItem : obj->getResources()) {
        auto resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "----RESOURCE----";
        resEntry.append_attribute("resvalue") = fmt::to_string(resItem->getResId()).c_str();
        resEntry.append_attribute("editable") = false;

        resEntry = resources.append_child("resources");
        resEntry.append_attribute("resname") = "handlerType";
        resEntry.append_attribute("resvalue") = MetadataHandler::mapContentHandler2String(resItem->getHandlerType()).c_str();
        resEntry.append_attribute("editable") = false;

        // write resource content
        if (objItem) {
            std::string url = UpnpXMLBuilder::renderOneResource(server->getVirtualUrl(), objItem, resItem);
            if (resItem->isMetaResource(ID3_ALBUM_ART) //
                || (resItem->getHandlerType() == ContentHandler::LIBEXIF && resItem->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) //
                || (resItem->getHandlerType() == ContentHandler::FFTH && resItem->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL)) {
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
                resEntry.append_attribute("resname") = CdsResource::getAttributeDisplay(attr).c_str();
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

    // write item meta info
    if (obj->isItem()) {
        auto description = item.append_child("description");
        description.append_attribute("value") = objItem->getMetaData(M_DESCRIPTION).c_str();
        description.append_attribute("editable") = true;

        auto location = item.append_child("location");
        location.append_attribute("value") = objItem->getLocation().c_str();
        location.append_attribute("editable") = !obj->isPureItem() && obj->isVirtual();

        auto mimeType = item.append_child("mime-type");
        mimeType.append_attribute("value") = objItem->getMimeType().c_str();
        mimeType.append_attribute("editable") = true;

        auto [url, artAdded] = xmlBuilder->renderItemImage(server->getVirtualUrl(), objItem);
        if (artAdded) {
            auto image = item.append_child("image");
            image.append_attribute("value") = url.c_str();
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

        if (obj->isExternalItem()) {
            auto protocol = item.append_child("protocol");
            protocol.append_attribute("value") = getProtocol(objItem->getResource(ContentHandler::DEFAULT)->getAttribute(CdsResource::Attribute::PROTOCOLINFO)).data();
            protocol.append_attribute("editable") = true;
        }
    }

    // write container meta info
    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto [url, artAdded] = xmlBuilder->renderContainerImage(server->getVirtualUrl(), cont);
        if (artAdded) {
            auto image = item.append_child("image");
            image.append_attribute("value") = url.c_str();
            image.append_attribute("editable") = false;
        }
    }
}
