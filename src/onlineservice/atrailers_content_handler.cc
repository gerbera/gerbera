/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    atrailers_content_handler.cc - this file is part of MediaTomb.
    
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

/// \file atrailers_content_handler.cc
/// \brief Implementation of the ATrailersContentHandler class.

#if defined(ATRAILERS)

#include "atrailers_content_handler.h"

#include "cds_objects.h"
#include "config/config_manager.h"
#include "metadata/metadata_handler.h"
#include "online_service.h"
#include "util/tools.h"
#include <utility>

ATrailersContentHandler::ATrailersContentHandler(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage)
    : config(std::move(config))
    , storage(std::move(storage))
{
}

void ATrailersContentHandler::setServiceContent(std::unique_ptr<pugi::xml_document>& service)
{
    service_xml = std::move(service);
    auto root = service_xml->document_element();

    if (std::string(root.name()) != "records")
        throw std::runtime_error("Recieved invalid XML for Apple Trailers service");

    trailer_it = root.begin();

    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    trailer_mimetype = getValueOrDefault(mappings, "mov");
    if (!string_ok(trailer_mimetype))
        trailer_mimetype = "video/quicktime";
}

std::shared_ptr<CdsObject> ATrailersContentHandler::getNextObject()
{
    auto root = service_xml->document_element();

    while (trailer_it != root.end()) {
        auto trailer = *trailer_it;
        ++trailer_it;

        if (trailer == nullptr)
            return nullptr;

        if (trailer.type() != pugi::node_element)
            continue;

        if (std::string(trailer.name()) != "movieinfo")
            continue;

        // we know that we have a trailer
        auto item = getObject(trailer);
        if (item != nullptr)
            return item;

    } // while trailer_it

    return nullptr;
}

std::shared_ptr<CdsObject> ATrailersContentHandler::getObject(const pugi::xml_node& trailer) const
{
    auto item = std::make_shared<CdsItemExternalURL>(storage);
    auto resource = std::make_shared<CdsResource>(CH_DEFAULT);
    item->addResource(resource);

    auto info = trailer.child("info");
    if (info == nullptr)
        return nullptr;

    std::string temp = info.child("title").text().as_string();
    if (!string_ok(temp))
        item->setTitle("Unknown");
    else
        item->setTitle(temp);

    item->setMimeType(trailer_mimetype);
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(trailer_mimetype));

    item->setAuxData(ONLINE_SERVICE_AUX_ID, std::to_string(OS_ATrailers));

    temp = trailer.attribute("id").as_string();
    if (!string_ok(temp)) {
        log_warning("Failed to retrieve Trailer ID for \"{}\", "
                    "skipping...\n",
            item->getTitle().c_str());
        return nullptr;
    }

    temp = std::to_string(OnlineService::getStoragePrefix(OS_ATrailers)) + temp;
    item->setServiceID(temp);

    auto preview = trailer.child("preview");
    if (preview == nullptr) {
        log_warning("Failed to retrieve Trailer location for \"{}\", "
                    "skipping...\n",
            item->getTitle().c_str());
        return nullptr;
    }

    temp = preview.child("large").text().as_string();
    if (string_ok(temp)) {
        item->setURL(temp);
    } else {
        log_error("Could not get location for Trailers item {}, "
                    "skipping.\n",
            item->getTitle().c_str());
        return nullptr;
    }

    item->setClass("object.item.videoItem");

    temp = info.child("rating").text().as_string();
    if (string_ok(temp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_RATING),
            temp);

    temp = info.child("studio").text().as_string();
    if (string_ok(temp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_PRODUCER),
            temp);

    temp = info.child("director").text().as_string();
    if (string_ok(temp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DIRECTOR),
            temp);

    temp = info.child("postdate").text().as_string();
    if (string_ok(temp))
        item->setAuxData(ATRAILERS_AUXDATA_POST_DATE, temp);

    temp = info.child("releasedate").text().as_string();
    if (string_ok(temp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DATE),
            temp);

    temp = info.child("description").text().as_string();
    if (string_ok(temp)) {
        /// \todo cut out a small part for the usual description
        item->setMetadata(MetadataHandler::getMetaFieldName(M_LONGDESCRIPTION), temp);
    }

    auto cast = trailer.child("cast");
    if (cast != nullptr) {
        std::string actors;
        for (pugi::xml_node actor: cast.children()) {
            if (actor.type() != pugi::node_element)
                return nullptr;
            if (std::string(actor.name()) != "name")
                return nullptr;

            temp = actor.text().as_string();
            if (string_ok(temp)) {
                if (string_ok(actors))
                    actors = actors + ", ";

                actors = actors + temp;
            }
        }

        if (string_ok(actors))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_GENRE),
                temp);
    }

    auto genre = trailer.child("genre");
    if (genre != nullptr) {
        std::string genres;
        for (pugi::xml_node gn: genre.children()) {
            if (gn.type() != pugi::node_element)
                return nullptr;
            if (std::string(gn.name()) != "name")
                return nullptr;

            temp = gn.text().as_string();
            if (string_ok(temp)) {
                if (string_ok(genres))
                    genres = genres + ", ";

                genres = genres + temp;
            }
        }

        if (string_ok(genres))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_GENRE),
                temp);
    }

    /*
    we do not know the resolution, check if they use a fixed size
    since it's anyway too big for a thumbnail I'll think about it once
    I add the fastscaler

    auto poster = trailer.child("poster");
    if (poster != nullptr)
    {
    }
    */

    struct timespec ts;
    getTimespecNow(&ts);
    item->setAuxData(ONLINE_SERVICE_LAST_UPDATE, std::to_string(ts.tv_sec));

    item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
    try {
        item->validate();
        return item;
    } catch (const std::runtime_error& ex) {
        log_warning("Failed to validate newly created Trailer item: {}",
            ex.what());
        return nullptr;
    }
}

#endif //ATRAILERS
