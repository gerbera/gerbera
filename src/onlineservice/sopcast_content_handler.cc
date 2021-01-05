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

#ifdef SOPCAST
#include "sopcast_content_handler.h" // API

#include <utility>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "metadata/metadata_handler.h"
#include "online_service.h"
#include "util/tools.h"

SopCastContentHandler::SopCastContentHandler(std::shared_ptr<Config> config, std::shared_ptr<Database> database)
    : config(std::move(config))
    , database(std::move(database))
{
}

void SopCastContentHandler::setServiceContent(std::unique_ptr<pugi::xml_document>& service)
{
    service_xml = std::move(service);
    auto root = service_xml->document_element();

    if (std::string(root.name()) != "channels")
        throw_std_runtime_error("Invalid XML for SopCast service received");

    group_it = root.begin();
    if (group_it != root.end())
        channel_it = (*group_it).begin();
}

std::shared_ptr<CdsObject> SopCastContentHandler::getNextObject()
{
    auto root = service_xml->document_element();
    bool skipGroup = false;

    while (group_it != root.end()) {
        auto group = *group_it;

        if (skipGroup || channel_it == group.end()) {
            // invalid group or all channels of group are handled, go to next group
            ++group_it;
            if (group_it != root.end())
                channel_it = (*group_it).begin();

            skipGroup = false;
            continue;
        }
        skipGroup = true;

        if (group == nullptr)
            continue;

        if (group.type() != pugi::node_element)
            continue;

        if (std::string(group.name()) != "group")
            continue;

        // we know that we have a group
        skipGroup = false;

        std::string groupName = group.text().as_string();
        if (groupName.empty()) {
            groupName = group.attribute("en").as_string();
            if (groupName.empty())
                groupName = "Unknown";
        }

        while (channel_it != group.end()) {
            auto channel = *channel_it;
            ++channel_it;

            if (channel == nullptr)
                continue;

            if (channel.type() != pugi::node_element)
                continue;

            if (std::string(channel.name()) != "channel")
                continue;

            // we know that we have a channel
            auto item = getObject(groupName, channel);
            if (item != nullptr)
                return item;

        } // for channel
    } // for group

    return nullptr;
}

std::shared_ptr<CdsObject> SopCastContentHandler::getObject(const std::string& groupName, const pugi::xml_node& channel) const
{
    auto item = std::make_shared<CdsItemExternalURL>();
    auto resource = std::make_shared<CdsResource>(CH_DEFAULT);
    item->addResource(resource);

    item->setAuxData(ONLINE_SERVICE_AUX_ID,
        std::to_string(OS_SopCast));

    item->setAuxData(SOPCAST_AUXDATA_GROUP, groupName);

    std::string temp = channel.attribute("id").as_string();
    if (temp.empty()) {
        log_warning("Failed to retrieve SopCast channel ID");
        return nullptr;
    }

    temp.insert(temp.begin(), OnlineService::getDatabasePrefix(OS_SopCast));
    item->setServiceID(temp);

    temp = channel.child("stream_type").text().as_string();
    if (temp.empty()) {
        log_warning("Failed to retrieve SopCast channel mimetype");
        return nullptr;
    }

    // I wish they had a mimetype setting
    //auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    //std::string mt = getValueOrDefault(mappings, temp);
    std::string mt;
    // map was empty, we have to do construct the mimetype ourselves
    if (temp == "wmv")
        mt = "video/sopcast-x-ms-wmv";
    else if (temp == "mp3")
        mt = "audio/sopcast-mpeg";
    else if (temp == "wma")
        mt = "audio/sopcast-x-ms-wma";
    else {
        log_warning("Could not determine mimetype for SopCast channel (stream_type: {})", temp.c_str());
        mt = "application/sopcast-stream";
    }

    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mt, SOPCAST_PROTOCOL));
    item->setMimeType(mt);

    auto tmp_el = channel.child("sop_address");
    if (tmp_el == nullptr) {
        log_warning("Failed to retrieve SopCast channel URL");
        return nullptr;
    }

    temp = tmp_el.child("item").text().as_string();
    if (temp.empty()) {
        log_warning("Failed to retrieve SopCast channel URL");
        return nullptr;
    }
    item->setURL(temp);

    tmp_el = channel.child("name");
    if (tmp_el == nullptr) {
        log_warning("Failed to retrieve SopCast channel name");
        return nullptr;
    }

    temp = tmp_el.attribute("en").as_string();
    if (!temp.empty())
        item->setTitle(temp);
    else
        item->setTitle("Unknown");

    tmp_el = channel.child("region");
    if (tmp_el != nullptr) {
        temp = tmp_el.attribute("en").as_string();
        if (!temp.empty())
            item->setMetadata(M_REGION, temp);
    }

    temp = channel.child("description").text().as_string();
    if (!temp.empty())
        item->setMetadata(M_DESCRIPTION, temp);

    temp = channel.attribute("language").as_string();
    if (!temp.empty())
        item->setAuxData(SOPCAST_AUXDATA_LANGUAGE, temp);

    item->setClass(UPNP_CLASS_VIDEO_BROADCAST);

    struct timespec ts;
    getTimespecNow(&ts);
    item->setAuxData(ONLINE_SERVICE_LAST_UPDATE, std::to_string(ts.tv_sec));
    item->setFlag(OBJECT_FLAG_PROXY_URL);
    item->setFlag(OBJECT_FLAG_ONLINE_SERVICE);

    try {
        item->validate();
        return item;
    } catch (const std::runtime_error& ex) {
        log_warning("Failed to validate newly created SopCast item: {}",
            ex.what());
        return nullptr;
    }
}

#endif //SOPCAST
