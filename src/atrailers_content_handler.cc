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
#include "config_manager.h"
#include "metadata_handler.h"
#include "online_service.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

bool ATrailersContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    String temp;

    if (service->getName() != "records")
        throw _Exception(_("Recieved invalid XML for Apple Trailers service"));

    this->service_xml = service;

    trailer_count = service_xml->childCount();

    if (trailer_count == 0)
        return false;

    current_trailer_index = 0;

    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    trailer_mimetype = mappings->get(_("mov"));
    if (!string_ok(trailer_mimetype))
        trailer_mimetype = _("video/quicktime");

    return true;
}

Ref<CdsObject> ATrailersContentHandler::getNextObject()
{
    String temp;
    struct timespec ts;

    while (current_trailer_index < trailer_count) {
        Ref<Node> n = service_xml->getChild(current_trailer_index);

        current_trailer_index++;

        if (n == nullptr)
            return nullptr;

        if (n->getType() != mxml_node_element)
            continue;

        Ref<Element> trailer = RefCast(n, Element);
        if (trailer->getName() != "movieinfo")
            continue;

        // we know what we are adding
        Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
        Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
        item->addResource(resource);

        Ref<Element> info = trailer->getChildByName(_("info"));
        if (info == nullptr)
            continue;

        temp = info->getChildText(_("title"));
        if (!string_ok(temp))
            item->setTitle(_("Unknown"));
        else
            item->setTitle(temp);

        item->setMimeType(trailer_mimetype);
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(trailer_mimetype));

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID), String::from(OS_ATrailers));

        temp = trailer->getAttribute(_("id"));
        if (!string_ok(temp)) {
            log_warning("Failed to retrieve Trailer ID for \"%s\", "
                        "skipping...\n",
                item->getTitle().c_str());
            continue;
        }

        temp = String(OnlineService::getStoragePrefix(OS_ATrailers)) + temp;
        item->setServiceID(temp);

        Ref<Element> preview = trailer->getChildByName(_("preview"));
        if (preview == nullptr) {
            log_warning("Failed to retrieve Trailer location for \"%s\", "
                        "skipping...\n",
                item->getTitle().c_str());
            continue;
        }

        temp = preview->getChildText(_("large"));
        if (string_ok(temp)) {
            item->setURL(temp);
        } else {
            log_error("Could not get location for Trailers item %s, "
                      "skipping.\n",
                item->getTitle().c_str());
            continue;
        }

        item->setClass(_("object.item.videoItem"));

        temp = info->getChildText(_("rating"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_RATING),
                temp);

        temp = info->getChildText(_("studio"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_PRODUCER),
                temp);

        temp = info->getChildText(_("director"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_DIRECTOR),
                temp);

        temp = info->getChildText(_("postdate"));
        if (string_ok(temp))
            item->setAuxData(_(ATRAILERS_AUXDATA_POST_DATE), temp);

        temp = info->getChildText(_("releasedate"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_DATE),
                temp);

        temp = info->getChildText(_("description"));
        if (string_ok(temp)) {
            /// \todo cut out a small part for the usual description
            item->setMetadata(MetadataHandler::getMetaFieldName(M_LONGDESCRIPTION), temp);
        }

        Ref<Element> cast = trailer->getChildByName(_("cast"));
        if (cast != nullptr) {
            String actors;
            for (int i = 0; i < cast->childCount(); i++) {
                Ref<Node> cn = cast->getChild(i);
                if (cn->getType() != mxml_node_element)
                    continue;

                Ref<Element> actor = RefCast(cn, Element);
                if (actor->getName() != "name")
                    continue;

                temp = actor->getText();
                if (string_ok(temp)) {
                    if (string_ok(actors))
                        actors = actors + _(", ");

                    actors = actors + temp;
                }
            }

            if (string_ok(actors))
                item->setMetadata(MetadataHandler::getMetaFieldName(M_GENRE),
                    temp);
        }

        Ref<Element> genre = trailer->getChildByName(_("genre"));
        if (genre != nullptr) {
            String genres;
            for (int i = 0; i < genre->childCount(); i++) {
                Ref<Node> gn = genre->getChild(i);
                if (gn->getType() != mxml_node_element)
                    continue;

                Ref<Element> genre = RefCast(gn, Element);
                if (genre->getName() != "name")
                    continue;

                temp = genre->getText();
                if (string_ok(temp)) {
                    if (string_ok(genres))
                        genres = genres + _(", ");

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

        Ref<Element> poster = trailer->getChildByName(_("poster"));
        if (poster != nullptr)
        {
        }
        */

        getTimespecNow(&ts);
        item->setAuxData(_(ONLINE_SERVICE_LAST_UPDATE),
            String::from(ts.tv_sec));

        item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
        try {
            item->validate();
            return RefCast(item, CdsObject);
        } catch (const Exception& ex) {
            log_warning("Failed to validate newly created Trailer item: %s\n",
                ex.getMessage().c_str());
            continue;
        }
    } // while
    return nullptr;
}

#endif //ATRAILERS
