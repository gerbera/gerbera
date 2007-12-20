/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_content_handler.cc - this file is part of MediaTomb.
    
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

/// \file youtube_content_handler.cc
/// \brief Implementation of the YouTubeContentHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#if defined(YOUTUBE)

#include "youtube_content_handler.h"
#include "online_service.h"
#include "tools.h"
#include "metadata_handler.h"
#include "cds_objects.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

bool YouTubeContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    String temp;

    if (service->getName() != "ut_response")
        throw _Exception(_("Invalid XML for YouTube service received"));

    temp = service->getAttribute(_("status"));

    if (temp != "ok")
        return false;

    Ref<Element> video_list = service->getChild(_("video_list"));
    if (video_list == nil)
        throw _Exception(_("Invalid XML for YouTube service received - video_list not found!"));

    this->service_xml = video_list;

    video_list_child_count = service_xml->childCount();
    if (video_list_child_count == 0)
        return false;

    current_video_node_index = 0;

    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    // this is somewhat a dilemma... we know that YT video thumbs are jpeg
    // but we do not check that; we could probably do a HTTP HEAD request,
    // however that would cause quite some network activity since there may
    // be hundreds and thousands of those items
    thumb_mimetype = mappings->get(_(CONTENT_TYPE_JPG));
    if (!string_ok(thumb_mimetype))
        thumb_mimetype = _("image/jpeg");

    return true;
}

Ref<CdsObject> YouTubeContentHandler::getNextObject()
{
#define DATE_BUF_LEN 12
    String temp;
    time_t epoch;
    struct tm t;
    char datebuf[DATE_BUF_LEN];
    struct timespec ts;

    while (current_video_node_index < video_list_child_count)
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
                               String::from(OS_YouTube));

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID),
                String::from(OS_YouTube));

        temp = video->getChildText(_("id"));
        if (!string_ok(temp))
        {
            log_warning("Failed to retrieve YouTube video ID\n");
            continue;
        }

        item->setClass(_("object.item.videoItem"));
        /// \todo create an own class for items that fetch the URL on request
        /// and to not store it permanently
        item->setURL(_(" "));
        temp = String(OnlineService::getStoragePrefix(OS_YouTube)) + temp;
        item->setServiceID(temp);

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
            item->setAuxData(_(YOUTUBE_AUXDATA_TAGS), temp);
        }

        temp = video->getChildText(_("rating_avg"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_AVG_RATING), temp);
        }

        temp = video->getChildText(_("author"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_AUTHOR), temp);
        }

        temp = video->getChildText(_("view_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_VIEW_COUNT), temp);
        }

        temp = video->getChildText(_("comment_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_COMMENT_COUNT), temp);
        }

        temp = video->getChildText(_("rating_count"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_RATING_COUNT), temp);
        }

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID), String::from(OS_YouTube));

        item->addResource(resource);

        temp = video->getChildText(_("thumbnail_url"));
        if (string_ok(temp))
        {
            Ref<CdsResource> thumbnail(new CdsResource(CH_EXTURL));
            thumbnail->
                addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                        renderProtocolInfo(thumb_mimetype));
            // same as with thumb mimetype.. in most cases this resolution
            // will be correct, we could omit it but then the renderers 
            // would assume that this is a full image and thus not display
            // it as thumbnail
            thumbnail->
                addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), 
                        _("130x97"));
            thumbnail->addOption(_(RESOURCE_OPTION_URL), temp);
            thumbnail->addOption(_(RESOURCE_OPTION_PROXY_URL), _(FALSE));
            item->addResource(thumbnail);
        }

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
            log_warning("Failed to validate newly created YouTube item: %s\n",
                        ex.getMessage().c_str());
            continue;
        }


    } // while
    return nil;
}

#endif//YOUTUBE
