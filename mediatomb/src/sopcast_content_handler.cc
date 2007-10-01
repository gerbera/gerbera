/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sopcast_content_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file sopcast_content_handler.cc
/// \brief Implementation of the SopCastContentHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#if defined(SOPCAST)

#include "sopcast_content_handler.h"
#include "online_service.h"
#include "tools.h"
#include "metadata_handler.h"
#include "cds_objects.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

bool SopCastContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    String temp;

    if (service->getName() != "channels")
        throw _Exception(_("Invalid XML for SopCast service received"));

    this->service_xml = service;

    group_count = service_xml->childCount();
    if (group_count < 1)
        return false;

    current_group_node_index = 0;

    return true;
}

Ref<CdsObject> SopCastContentHandler::getNextObject()
{
#define DATE_BUF_LEN 12
    String temp;
    time_t epoch;
    struct tm t;
    char datebuf[DATE_BUF_LEN];

    while (current_group_node_index < group_count)
    {
        if (current_group == nil)
        {
            current_group = service_xml->getChild(current_group_node_index);
            channel_count = current_group->getChildCount();
            if (channel_count < 1)
            {
                current_group_node_index++;
                current_group = nil;
            }

        continue;
 
        }

       
        while (current_channel_index < channel_count)
        {
        Ref<Element> video = service_xml->getChild(current_video_node_index);
        current_video_node_index++;
       
        if (video == nil)
            continue;

        if (video->getName() != "video")
            continue;

        // we know what we are adding
        Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
        Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
        resource->addParameter(_(ONLINE_SERVICE_AUX_ID), 
                               String::from(OS_SopCast));

        temp = video->getChildText(_("id"));
        if (!string_ok(temp))
        {
            log_warning("Failed to retrieve SopCast video ID\n");
            continue;
        }
        resource->addParameter(_(SOPCAST_VIDEO_ID), temp);
        /// \todo remove this
        item->setURL(temp);

        temp = video->getChildText(_("title"));
        if (string_ok(temp))
            item->setTitle(temp);
        else
            item->setTitle(_("Unknown"));

        item->setMimeType(_("video/x-flv"));
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                renderProtocolInfo(_("video/x-flv")));
        
        temp = video->getChildText(_("length_in_seconds"));
        if (string_ok(temp))
        {
            resource->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                                   secondsToHMS(temp.toInt()));
        }
       
        temp = video->getChildText(_("description"));
        if (string_ok(temp))
            item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
                              temp);

        temp = video->getChildText(_("upload_time"));
        if (string_ok(temp))
        {
            epoch = (time_t)temp.toLong();
            t = *gmtime(&epoch);
            datebuf[0] = '\0';
            if (strftime(datebuf, sizeof(datebuf), "%F", &t) != 0)
            {
                datebuf[DATE_BUF_LEN-1] = '\0';
                if (strlen(datebuf) > 0)
                {
                    item->setMetadata(MetadataHandler::getMetaFieldName(M_DATE),
                            String(datebuf));
                }
            }
        }

        temp = video->getChildText(_("tags"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_TAGS), temp);
        }

        temp = video->getChildText(_("rating_avg"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_AVG_RATING), temp);
        }

        temp = video->getChildText(_("author"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_AUTHOR), temp);
        }

        temp = video->getChildText(_("view_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_VIEW_COUNT), temp);
        }

        temp = video->getChildText(_("comment_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_COMMENT_COUNT), temp);
        }

        temp = video->getChildText(_("rating_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(SOPCAST_AUXDATA_RATING_COUNT), temp);
        }

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID), String::from(OS_SopCast));

        item->addResource(resource);
        
/*
        temp = video->getChildText(_("thumbnail_url"));
        if (string_ok(temp))
        {
            item->setURL(temp);
            Ref<CdsResource> thumbnail(new CdsResource(CH_DEFAULT));
            thumbnail->
                addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                             renderProtocolInfo(thumb_mimetype));
            thumbnail->
                addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), 
                                _("130x97"));
           item->addResource(thumbnail);
 
        }
*/

        item->setFlag(OBJECT_FLAG_PROXY_URL);
        item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);
        try
        {
            item->validate();
            return RefCast(item, CdsObject);
        }
        catch (Exception ex)
        {
            log_warning("Failed to validate newly created SopCast item: %s\n",
                        ex.getMessage().c_str());
            continue;
        }


    } // while
    }
    return nil;
}

#endif//SOPCAST
