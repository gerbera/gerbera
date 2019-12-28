/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sopcast_content_handler.cc - this file is part of MediaTomb.
    
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

/// \file sopcast_content_handler.cc
/// \brief Implementation of the SopCastContentHandler class.

#if defined(SOPCAST)

#include "sopcast_content_handler.h"
#include "cds_objects.h"
#include "config_manager.h"
#include "metadata/metadata_handler.h"
#include "online_service.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

bool SopCastContentHandler::setServiceContent(zmm::Ref<mxml::Element> service)
{
    std::string temp;

    if (service->getName() != "channels")
        throw _Exception("Invalid XML for SopCast service received");

    channels = service;

    group_count = channels->childCount();
    if (group_count < 1)
        return false;

    current_group_node_index = 0;
    current_channel_index = 0;
    channel_count = 0;
    current_group = nullptr;

    extension_mimetype_map = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    return true;
}

Ref<CdsObject> SopCastContentHandler::getNextObject()
{
#define DATE_BUF_LEN 12
    std::string temp;
    struct timespec ts;

    while (current_group_node_index < group_count) {
        if (current_group == nullptr) {
            Ref<Node> n = channels->getChild(current_group_node_index);
            current_group_node_index++;

            if (n == nullptr)
                continue;

            if (n->getType() != mxml_node_element)
                continue;

            current_group = RefCast(n, Element);
            channel_count = current_group->childCount();

            if ((current_group->getName() != "group") || (channel_count < 1)) {
                current_group = nullptr;
                current_group_name = nullptr;
                continue;
            } else {
                current_group_name = current_group->getText();
                if (!string_ok(current_group_name)) {
                    current_group_name = current_group->getAttribute("en");
                    if (!string_ok(current_group_name))
                        current_group_name = "Unknown";
                }
            }

            current_channel_index = 0;
        }

        if (current_channel_index >= channel_count) {
            current_group = nullptr;
            continue;
        }

        while (current_channel_index < channel_count) {
            Ref<Node> n = current_group->getChild(current_channel_index);
            current_channel_index++;

            if ((n == nullptr) || (n->getType() != mxml_node_element))
                continue;

            Ref<Element> channel = RefCast(n, Element);

            if (channel->getName() != "channel")
                continue;

            Ref<CdsItemExternalURL> item(new CdsItemExternalURL());
            Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
            item->addResource(resource);

            item->setAuxData(ONLINE_SERVICE_AUX_ID,
                std::to_string(OS_SopCast));

            item->setAuxData(SOPCAST_AUXDATA_GROUP, current_group_name);

            temp = channel->getAttribute("id");
            if (!string_ok(temp)) {
                log_warning("Failed to retrieve SopCast channel ID\n");
                continue;
            }

            temp.insert(temp.begin(), OnlineService::getStoragePrefix(OS_SopCast));
            item->setServiceID(temp);

            temp = channel->getChildText("stream_type");
            if (!string_ok(temp)) {
                log_warning("Failed to retrieve SopCast channel mimetype\n");
                continue;
            }

            // I wish they had a mimetype setting
            //std::string mt = extension_mimetype_map->get(temp);
            std::string mt;
            // map was empty, we have to do construct the mimetype ourselves
            if (!string_ok(mt)) {
                if (temp == "wmv")
                    mt = "video/sopcast-x-ms-wmv";
                else if (temp == "mp3")
                    mt = "audio/sopcast-mpeg";
                else if (temp == "wma")
                    mt = "audio/sopcast-x-ms-wma";
                else {
                    log_warning("Could not determine mimetype for SopCast channel (stream_type: %s)\n", temp.c_str());
                    mt = "application/sopcast-stream";
                }
            }
            resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                renderProtocolInfo(mt, SOPCAST_PROTOCOL));
            item->setMimeType(mt);

            Ref<Element> tmp_el = channel->getChildByName("sop_address");
            if (tmp_el == nullptr) {
                log_warning("Failed to retrieve SopCast channel URL\n");
                continue;
            }

            temp = tmp_el->getChildText("item");
            if (!string_ok(temp)) {
                log_warning("Failed to retrieve SopCast channel URL\n");
                continue;
            }
            item->setURL(temp);

            tmp_el = channel->getChildByName("name");
            if (tmp_el == nullptr) {
                log_warning("Failed to retrieve SopCast channel name\n");
                continue;
            }

            temp = tmp_el->getAttribute("en");
            if (string_ok(temp))
                item->setTitle(temp);
            else
                item->setTitle("Unknown");

            tmp_el = channel->getChildByName("region");
            if (tmp_el != nullptr) {
                temp = tmp_el->getAttribute("en");
                if (string_ok(temp))
                    item->setMetadata(MetadataHandler::getMetaFieldName(M_REGION), temp);
            }

            temp = channel->getChildText("description");
            if (string_ok(temp))
                item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), temp);

            temp = channel->getAttribute("language");
            if (string_ok(temp))
                item->setAuxData(SOPCAST_AUXDATA_LANGUAGE, temp);

            item->setClass(UPNP_DEFAULT_CLASS_VIDEO_BROADCAST);

            getTimespecNow(&ts);
            item->setAuxData(ONLINE_SERVICE_LAST_UPDATE, std::to_string(ts.tv_sec));
            item->setFlag(OBJECT_FLAG_PROXY_URL);
            item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);

            try {
                item->validate();
                return RefCast(item, CdsObject);
            } catch (const Exception& ex) {
                log_warning("Failed to validate newly created SopCast item: %s\n",
                    ex.getMessage().c_str());
                continue;
            }
        }
    }

    return nullptr;
}

#endif //SOPCAST
