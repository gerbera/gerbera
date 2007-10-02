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

    channels = service;

    group_count = channels->childCount();
    if (group_count < 1)
        return false;

    current_group_node_index = 0;
    current_channel_index = 0;
    channel_count = 0;
    current_group = nil;

    return true;
}

Ref<CdsObject> SopCastContentHandler::getNextObject()
{
#define DATE_BUF_LEN 12
    String temp;
    time_t epoch;
    struct tm t;
    char datebuf[DATE_BUF_LEN];
    struct timespec ts;


    while (current_group_node_index < group_count)
    {
        if (current_group == nil)
        {
            current_group = channels->getChild(current_group_node_index);
            current_group_node_index++;
            channel_count = current_group->childCount();

            if ((current_group->getName() != "group") ||
                (channel_count < 1))
            {
                current_group = nil;
                continue;
            }

            current_channel_index = 0;
        }

        if (current_channel_index >= channel_count)
        {
            current_group = nil;
            continue;
        }

        while (current_channel_index < channel_count)
        {
            Ref<Element> channel = current_group->getChild(current_channel_index);
            current_channel_index++;
            if (channel->getName() != "channel")
                continue;

            Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
            Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
            resource->addParameter(_(ONLINE_SERVICE_AUX_ID),
                    String::from(OS_SopCast));

            temp = channel->getAttribute(_("id"));
            if (!string_ok(temp))
            {
                log_warning("Failed to retrieve SopCast channel ID\n");
                continue;
            }

            temp = String(OnlineService::getStoragePrefix(OS_SopCast)) + temp;
            item->setServiceID(temp);

            Ref<Element> tmp_el = channel->getChild(_("sop_address"));
            if (tmp_el == nil)
            {
                log_warning("Failed to retrieve SopCast channel URL\n");
                continue;
            }
           
            temp = tmp_el->getChildText(_("item"));
            if (!string_ok(temp))
            {
                log_warning("Failed to retrieve SopCast channel URL\n");
                continue;
            }
            item->setURL(temp);

            tmp_el = channel->getChild(_("name"));
            if (tmp_el == nil)
            {
                log_warning("Failed to retrieve SopCast channel name\n");
                continue;
            }

            temp = tmp_el->getAttribute(_("en"));
            if (string_ok(temp))
                item->setTitle(temp);
            else
                item->setTitle(_("Unknown"));

            tmp_el = channel->getChild(_("region"));
            if (tmp_el != nil)
            {
                temp = tmp_el->getAttribute(_("en"));
                if (string_ok(temp))
                    item->setMetadata(MetadataHandler::getMetaFieldName(M_REGION), temp);
            }

            temp = channel->getChildText(_("description"));
            if (string_ok(temp))
                item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), temp);

            temp = channel->getAttribute(_("language"));
            if (string_ok(temp))
                item->setAuxData(_(SOPCAST_AUXDATA_LANGUAGE), temp);

            item->setClass(_(UPNP_DEFAULT_CLASS_VIDEO_BROADCAST));

            getTimespecNow(&ts);
            item->setAuxData(_(ONLINE_SERVICE_LAST_UPDATE), String::from(ts.tv_sec));

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
        }
    }

    return nil;
}

#endif//SOPCAST
