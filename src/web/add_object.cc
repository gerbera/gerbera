/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/add_object.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/add_object.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_val.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "exceptions.h"
#include "util/tools.h"
#include "util/xml_to_json.h"

const std::string Web::AddObject::PAGE = "add_object";

void Web::AddObject::addContainer(
    int parentID,
    const std::string& title,
    const std::string& upnp_class)
{
    auto cont = content->addContainer(parentID, title, upnp_class);

    std::string flags = param("flags");
    if (!flags.empty()) {
        cont->setFlags(CdsObject::makeFlag(flags));
        content->updateObject(cont, false);
    }
}

std::shared_ptr<CdsItem> Web::AddObject::addItem(
    int parentID,
    const std::string& title,
    const std::string& upnp_class,
    const fs::path& location)
{
    auto item = std::make_shared<CdsItem>();
    item->setParentID(parentID);

    item->setTitle(title);
    item->setLocation(location);
    item->setClass(upnp_class);

    if (isHiddenFile(item)) {
        log_debug("Hidden file '{}' cannot be added", item->getLocation().c_str());
        return {};
    }
    std::string desc = param("description");
    if (!desc.empty()) {
        item->addMetaData(MetadataFields::M_DESCRIPTION, desc);
    }

    std::string mime = param("mime-type");
    if (mime.empty())
        mime = MIMETYPE_DEFAULT;
    item->setMimeType(mime);

    std::string flags = param("flags");
    if (!flags.empty())
        item->setFlags(CdsObject::makeFlag(flags));

    item->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);

    return item;
}

std::shared_ptr<CdsItemExternalURL> Web::AddObject::addUrl(
    int parentID,
    const std::string& title,
    const std::string& upnp_class,
    bool addProtocol,
    const fs::path& location)
{
    auto item = std::make_shared<CdsItemExternalURL>();
    std::string protocolInfo;

    item->setParentID(parentID);

    item->setTitle(title);
    item->setURL(location);
    item->setClass(upnp_class);

    std::string desc = param("description");
    if (!desc.empty()) {
        item->addMetaData(MetadataFields::M_DESCRIPTION, desc);
    }

    std::string flags = param("flags");
    if (!flags.empty())
        item->setFlags(CdsObject::makeFlag(flags));

    {
        std::string mime = param("mime-type");
        if (mime.empty())
            mime = MIMETYPE_DEFAULT;
        item->setMimeType(mime);

        if (addProtocol) {
            std::string protocol = param("protocol");
            if (!protocol.empty())
                protocolInfo = renderProtocolInfo(mime, protocol);
            else
                protocolInfo = renderProtocolInfo(mime);
        } else
            protocolInfo = renderProtocolInfo(mime);

        auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
        item->addResource(resource);
    }

    readResources(item);

    return item;
}

bool Web::AddObject::isHiddenFile(const std::shared_ptr<CdsObject>& cdsObj)
{
    AutoScanSetting asSetting;
    asSetting.recursive = true;
    asSetting.followSymlinks = config->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    asSetting.hidden = config->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);
    asSetting.mergeOptions(config, "/");
    return content->isHiddenFile(fs::directory_entry(cdsObj->getLocation()), false, asSetting);
}

void Web::AddObject::processPageAction(pugi::xml_node& element)
{
    auto title = std::string(param("title"));
    if (title.empty())
        throw_std_runtime_error("Empty 'title'");

    auto upnp_class = std::string(param("class"));
    if (upnp_class.empty())
        throw_std_runtime_error("Empty 'class'");

    int parentID = intParam("parent_id", 0);
    std::shared_ptr<CdsObject> obj;
    bool allowFifo = false;

    auto objType = std::string(param("obj_type"));
    if (objType == STRING_OBJECT_TYPE_CONTAINER) {
        // add additional container in virtual structure
        this->addContainer(parentID, title, upnp_class);
    } else if (objType == STRING_OBJECT_TYPE_ITEM) {
        // reference to some local file on disk
        auto location = fs::path(param("location"));
        if (location.empty())
            throw_std_runtime_error("Empty 'location'");
        std::error_code ec;
        if (!isRegularFile(location, ec))
            throw_std_runtime_error("File '{}' not found {}", location.string(), ec.message());
        obj = this->addItem(parentID, title, upnp_class, location);
        allowFifo = true;
    } else if (objType == STRING_OBJECT_TYPE_EXTERNAL_URL) {
        // URL to serve to upnp devices
        auto location = fs::path(param("location"));
        if (location.empty())
            throw_std_runtime_error("Empty 'location' to be used as URL");
        obj = this->addUrl(parentID, title, upnp_class, true, location);
    } else {
        throw_std_runtime_error("Unknown object type: {}", objType.c_str());
    }

    if (obj) {
        // only for ITEM or EXTERNAL_URL
        obj->setVirtual(true);
        if (objType == STRING_OBJECT_TYPE_ITEM) {
            content->addVirtualItem(obj, allowFifo);
        } else {
            content->addObject(obj, true);
        }
    }
}
