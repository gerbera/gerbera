/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_content_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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

#define YT_SWF_TYPE "application/x-shockwave-flash"

using namespace zmm;
using namespace mxml;

bool YouTubeContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    String temp;

    if (service->getName() != "rss")
        throw _Exception(_("Invalid XML for YouTube service received"));

    Ref<Element> channel = service->getChildByName(_("channel"));
    if (channel == nil)
        throw _Exception(_("Invalid XML for YouTube service received - channel not found!"));

    this->service_xml = channel;

    feed_name = channel->getChildText(_("title"));

    if (!string_ok(feed_name))
        throw _Exception(_("Invalid XML for YouTube service, received - missing feed title!"));

    channel_child_count = service_xml->childCount();
    
    if (channel_child_count == 0)
        return false;

#warning find out when to abort!
#warning find out when to abort!
#warning find out when to abort!

    current_node_index = 0;

    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    // this is somewhat a dilemma... we know that YT video thumbs are jpeg
    // but we do not check that; we could probably do a HTTP HEAD request,
    // however that would cause quite some network activity since there may
    // be hundreds and thousands of those items, we will have a look at the
    // extension of the URL, this should be enough.
    thumb_mimetype = mappings->get(_(CONTENT_TYPE_JPG));
    if (!string_ok(thumb_mimetype))
        thumb_mimetype = _("image/jpeg");

    printf("---------> начало - channel_child_count = %d\n", 
            channel_child_count);

    return true;
}

Ref<CdsObject> YouTubeContentHandler::getNextObject()
{
#define DATE_BUF_LEN 12
    String temp;
    struct tm t;
    char datebuf[DATE_BUF_LEN];
    struct timespec ts;

    while (current_node_index < channel_child_count)
    {
        Ref<Node> n = service_xml->getChild(current_node_index);

        printf("processing node %d\n", current_node_index);

        current_node_index++;
      
        if (n == nil)
            return nil;

        printf("n не nil\n");

        if (n->getType() != mxml_node_element)
            continue;

        printf("n елемент\n");
       
        Ref<Element> channel_item = RefCast(n, Element);
        if (channel_item->getName() != "item")
            continue;

        // we know what we are adding
        Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
        Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
        item->addResource(resource);
        resource->addParameter(_(ONLINE_SERVICE_AUX_ID), 
                String::from(OS_YouTube));

        item->setAuxData(_(ONLINE_SERVICE_AUX_ID), String::from(OS_YouTube));

        temp = channel_item->getChildText(_("guid"));
        if (!string_ok(temp))
        {
            log_warning("Failed to retrieve YouTube video ID\n");
            continue;
        }

        int slash = temp.rindex('/');
        if (slash > 0)
            temp = temp.substring(slash + 1);

        item->setClass(_("object.item.videoItem"));
        /// \todo create an own class for items that fetch the URL on request
        /// and to not store it permanently
        item->setURL(_(" "));

        temp = String(OnlineService::getStoragePrefix(OS_YouTube)) + temp;
        item->setServiceID(temp);

        temp = channel_item->getChildText(_("pubDate"));
        if (string_ok(temp))
        {
            datebuf[0] = '\0';
            // Tue, 18 Jul 2006 17:43:47 +0000
            if (strptime(temp.c_str(),  "%a, %d %b %Y %T +000", &t) != NULL)
            {
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
        }

        temp = channel_item->getChildText(_("author"));
        if (string_ok(temp))
        {
            item->setAuxData(_(YOUTUBE_AUXDATA_AUTHOR), temp);
        }


        Ref<Element> mediagroup = channel_item->getChildByName(_("media:group"));
        if (mediagroup == nil)
            continue;

        // media:group uses a couple of elements with the same name, so
        // we will cycle through adn fill it that way
        
        bool content_set = false;

        int mediagroup_child_count = mediagroup->elementChildCount();
        for (int mcc = 0; mcc < mediagroup_child_count; mcc++)
        {
            Ref<Element> el = mediagroup->getElementChild(mcc);
            if (el->getName() == "media:title")
            {
                temp = el->getText();
                if (string_ok(temp))
                    item->setTitle(temp);
                else
                    item->setTitle(_("Unknown"));
            }
            else if (el->getName() == "media:description")
            {
                temp = el->getText();
                if (string_ok(temp))
                    item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), temp);
            }
            else if (el->getName() == "media:keywords")
            {
                temp = el->getText();
                if (string_ok(temp))
                    item->setAuxData(_(YOUTUBE_AUXDATA_KEYWORDS), temp);
            }
            else if (el->getName() == "media:category")
            {
                temp = el->getText();
                if (string_ok(temp))
                    item->setAuxData(_(YOUTUBE_AUXDATA_CATEGORY), temp);
            }
            else if (el->getName() == "media:content")
            {
                if (content_set)
                    continue;

                temp = el->getAttribute(_("type"));
                if (temp != YT_SWF_TYPE)
                    continue;

                item->setMimeType(_("video/x-flv"));
                resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(_("video/x-flv")));

                content_set = true;
 
                temp = el->getAttribute(_("duration"));
                if (string_ok(temp))
                {
                    resource->addAttribute(MetadataHandler::getResAttrName(R_DURATION), secondsToHMS(temp.toInt()));
                }
            }
            else if (el->getName() == "media:thumbnail")
            {
                temp = el->getAttribute(_("url"));
                if (!string_ok(temp))
                    continue;

                if (temp.substring(temp.length()-3) != "jpg")
                {
                    log_warning("Found new YouTube thumbnail image type, please report this to contact at mediatomb dot cc! [%s]\n", temp.c_str());
                    continue;
                }

                Ref<CdsResource> thumbnail(new CdsResource(CH_EXTURL));
                thumbnail->addOption(_(RESOURCE_CONTENT_TYPE), _(THUMBNAIL));
                thumbnail->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(thumb_mimetype));
                thumbnail->addOption(_(RESOURCE_OPTION_URL), temp);
            
                String temp2 = el->getAttribute(_("width"));
                temp = el->getAttribute(_("height"));

                if (string_ok(temp) && string_ok(temp2))
                {
                    thumbnail->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), temp2 + "x" + temp);
                }
                else
                    continue;

                thumbnail->addOption(_(RESOURCE_OPTION_PROXY_URL), _(FALSE));
                item->addResource(thumbnail);
            }

        }
      
        Ref<Element> stats = channel_item->getChildByName(_("yt:statistics"));
        temp = stats->getAttribute(_("viewCount"));
        if (string_ok(temp))
            item->setAuxData(_(YOUTUBE_AUXDATA_VIEW_COUNT), temp);

        temp = stats->getAttribute(_("favoriteCount"));
        if (string_ok(temp))
            item->setAuxData(_(YOUTUBE_AUXDATA_FAVORITE_COUNT), temp);

        Ref<Element> rating = channel_item->getChildByName(_("gd:rating"));
        temp = rating->getAttribute(_("average"));
        if (string_ok(temp))
            item->setAuxData(_(YOUTUBE_AUXDATA_AVG_RATING), temp);

        temp = rating->getAttribute(_("numRaters"));
        if (string_ok(temp))
            item->setAuxData(_(YOUTUBE_AUXDATA_RATING_COUNT), temp);
        
        item->setAuxData(_(YOUTUBE_AUXDATA_FEED), feed_name);

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
