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

#ifdef ATRAILERS
#include "atrailers_content_handler.h" // API

#include "cds_objects.h"
#include "config/config_manager.h"
#include "metadata/metadata_handler.h"
#include "online_service.h"
#include "util/tools.h"

ATrailersContentHandler::ATrailersContentHandler(const std::shared_ptr<Context>& context)
    : CurlContentHandler(context)
{
}

void ATrailersContentHandler::setServiceContent(std::unique_ptr<pugi::xml_document> service)
{
    service_xml = std::move(service);
    auto root = service_xml->document_element();

    if (std::string(root.name()) != "records")
        throw_std_runtime_error("Recieved invalid XML for Apple Trailers service");

    trailer_it = root.begin();

    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    trailer_mimetype = getValueOrDefault(mappings, "mov", "video/quicktime");
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
    auto item = std::make_shared<CdsItemExternalURL>();
    auto resource = std::make_shared<CdsResource>(CH_DEFAULT);
    item->addResource(resource);

    auto info = trailer.child("info");
    if (info == nullptr)
        return nullptr;

    std::string temp = info.child("title").text().as_string();
    if (temp.empty())
        item->setTitle("Unknown");
    else
        item->setTitle(temp);

    item->setMimeType(trailer_mimetype);
    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(trailer_mimetype));

    item->setAuxData(ONLINE_SERVICE_AUX_ID, fmt::to_string(OS_ATrailers));

    temp = trailer.attribute("id").as_string();
    if (temp.empty()) {
        log_warning("Failed to retrieve Trailer ID for \"{}\", "
                    "skipping...\n",
            item->getTitle().c_str());
        return nullptr;
    }

    temp = fmt::format("{}{}", OnlineService::getDatabasePrefix(OS_ATrailers), temp);
    item->setServiceID(temp);

    auto preview = trailer.child("preview");
    if (preview == nullptr) {
        log_warning("Failed to retrieve Trailer location for \"{}\", "
                    "skipping...\n",
            item->getTitle().c_str());
        return nullptr;
    }

    temp = preview.child("large").text().as_string();
    if (!temp.empty()) {
        item->setURL(temp);
    } else {
        log_error("Could not get location for Trailers item {}, "
                  "skipping.\n",
            item->getTitle().c_str());
        return nullptr;
    }

    item->setClass("object.item.videoItem");

    temp = info.child("rating").text().as_string();
    if (!temp.empty())
        item->setMetadata(M_RATING, temp);

    temp = info.child("studio").text().as_string();
    if (!temp.empty())
        item->setMetadata(M_PRODUCER, temp);

    temp = info.child("director").text().as_string();
    if (!temp.empty())
        item->setMetadata(M_DIRECTOR, temp);

    temp = info.child("postdate").text().as_string();
    if (!temp.empty())
        item->setAuxData(ATRAILERS_AUXDATA_POST_DATE, temp);

    temp = info.child("releasedate").text().as_string();
    if (!temp.empty())
        item->setMetadata(M_DATE, temp);

    temp = info.child("description").text().as_string();
    if (!temp.empty()) {
        /// \todo cut out a small part for the usual description
        item->setMetadata(M_LONGDESCRIPTION, temp);
    }

    auto cast = trailer.child("cast");
    if (cast != nullptr) {
        std::string actors;
        for (auto&& actor : cast.children()) {
            if (actor.type() != pugi::node_element)
                return nullptr;
            if (std::string(actor.name()) != "name")
                return nullptr;

            temp = actor.text().as_string();
            if (!temp.empty()) {
                if (!actors.empty())
                    actors.append(", ");

                actors.append(temp);
            }
        }

        if (!actors.empty())
            item->setMetadata(M_GENRE, temp);
    }

    auto genre = trailer.child("genre");
    if (genre != nullptr) {
        std::string genres;
        for (auto&& gn : genre.children()) {
            if (gn.type() != pugi::node_element)
                return nullptr;
            if (std::string(gn.name()) != "name")
                return nullptr;

            temp = gn.text().as_string();
            if (!temp.empty()) {
                if (!genres.empty())
                    genres.append(", ");

                genres.append(temp);
            }
        }

        if (!genres.empty())
            item->setMetadata(M_GENRE, temp);
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

    item->setAuxData(ONLINE_SERVICE_LAST_UPDATE, fmt::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));

    item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
    try {
        item->validate();
        return std::move(item);
    } catch (const std::runtime_error& ex) {
        log_warning("Failed to validate newly created Trailer item: {}",
            ex.what());
        return nullptr;
    }
}

#endif //ATRAILERS
