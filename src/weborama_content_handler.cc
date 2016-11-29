/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    weborama_content_handler.cc - this file is part of MediaTomb.
    
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

/// \file weborama_content_handler.cc
/// \brief Implementation of the WeboramaContentHandler class.

#if defined(WEBORAMA)

#include "weborama_content_handler.h"
#include "online_service.h"
#include "tools.h"
#include "metadata_handler.h"
#include "cds_objects.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

bool WeboramaContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    String temp;

    if (service->getName() != "playlist")
        throw _Exception(_("Recieved invalid XML for Weborama service"));

    if (service->getAttribute(_("version")) != "1")
        throw _Exception(_("Recieved unsupported playlist version from Weborama"
                           " service"));

    Ref<Element> trackList = service->getChildByName(_("trackList"));
    if (trackList == nil)
        throw _Exception(_("Received invalid XML for Weborama service: "
                           "track list not found!"));

/*    
    String mood = service->getChildText(_("mood"));
    if (string_ok(mood))
        this->mood = mood;
*/

    this->service_xml = trackList;

    track_count = service_xml->childCount();
    
    if (track_count == 0)
        return false;

    current_track_index = 0;

    return true;
}

Ref<CdsObject> WeboramaContentHandler::getNextObject()
{
    String temp;
    struct timespec ts;

    while (current_track_index < track_count)
    {
        Ref<Node> n = service_xml->getChild(current_track_index);

        current_track_index++;
      
        if (n == nil)
            return nil;

        if (n->getType() != mxml_node_element)
            continue;

        Ref<Element> track = RefCast(n, Element);
        if (track->getName() != "track")
            continue;

        // we know what we are adding
        Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
        Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
        item->addResource(resource);

        temp = track->getChildText(_("title"));
        if (!string_ok(temp))
            item->setTitle(_("Unknown"));
        else
            item->setTitle(temp);

        temp = track->getAttribute(_("mimeType"));
        if (string_ok(temp))
        {
            item->setMimeType(temp);
            resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(temp));
        }
        else
        {
            log_warning("Could not retrieve mime type for %s, skipping...\n",
                        item->getTitle().c_str());
            continue;
        }

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID), String::from(OS_Weborama));

        temp = track->getChildText(_("identifier"));
        if (!string_ok(temp))
        {
            log_warning("Failed to retrieve Weborama track ID for \"%s\", skipping...\n",
                        item->getTitle().c_str());
            continue;
        }

        temp = String(OnlineService::getStoragePrefix(OS_Weborama)) + temp;
        item->setServiceID(temp);

        temp = track->getChildText(_("location")); 
        if (string_ok(temp))
            item->setURL(temp);
        else
        {
            log_error("Could not get location for Weborama item %s, "
                      "skipping.\n", item->getTitle().c_str());
            continue;
        }

        /// \todo what about upnp class mappings from the configuration?
        item->setClass(_("object.item.audioItem.musicTrack"));

        temp = track->getChildText(_("creator"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_ARTIST), temp);
      //      item->setAuxData(_(WEBORAMA_AUXDATA_CREATOR), temp);

        temp = track->getChildText(_("album"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_ALBUM), temp);

        temp = track->getChildText(_("duration")); // milliseconds
        if (string_ok(temp))
        {
            int d = temp.toInt();
            if (d > 0)
                resource->addAttribute(MetadataHandler::getResAttrName(
                                                                R_DURATION),
                                       secondsToHMS(d/1000));
        }

        temp = track->getChildText(_("bitrate"));
        if (string_ok(temp))
        {
            int br = temp.toInt();
            br = br / 8;
            if (br > 0)
                resource->addAttribute(
                                    MetadataHandler::getResAttrName(R_BITRATE),
                                                           String::from(br));
        }

        temp = track->getChildText(_("frequency"));
        if (string_ok(temp))
            resource->addAttribute(
                            MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY),
                            temp);

        /// \todo check about trackNum - what is it really?
        
        Ref<Element> image = track->getChildByName(_("image"));
        if (image != nil)
        {
            temp = image->getAttribute(_("mimeType"));
            if (string_ok(temp))
            {
                Ref<CdsResource> albumart(new CdsResource(CH_EXTURL));
                albumart->addOption(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
                albumart->addAttribute(MetadataHandler::getResAttrName(
                            R_PROTOCOLINFO), renderProtocolInfo(temp));

                temp = image->getAttribute(_("size"));
                if (string_ok(temp))
                    albumart->addAttribute(MetadataHandler::getResAttrName(
                                R_RESOLUTION), temp + "x" + temp);

                temp = image->getText();
                if (string_ok(temp))
                {
                    albumart->addOption(_(RESOURCE_OPTION_URL), temp);
                    albumart->addOption(_(RESOURCE_OPTION_PROXY_URL), _(FALSE));
                    item->addResource(albumart);
                }
            }
        }

//        item->setAuxData(_(WEBORAMA_AUXDATA_MOOD), mood);

        getTimespecNow(&ts);
        item->setAuxData(_(ONLINE_SERVICE_LAST_UPDATE), 
                         String::from(ts.tv_sec));

        item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
        try
        {
            item->validate();
            return RefCast(item, CdsObject);
        }
        catch (Exception ex)
        {
            log_warning("Failed to validate newly created Weborama item: %s\n",
                        ex.getMessage().c_str());
            continue;
        }
    } // while
    return nil;
}

#endif//WEBORAMA
