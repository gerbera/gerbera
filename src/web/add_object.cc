/*MT*

    MediaTomb - http://www.mediatomb.cc/

    add_object.cc - this file is part of MediaTomb.

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

/// \file add_object.cc
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "content/content_manager.h"
#include "metadata/metadata_handler.h"
#include "util/tools.h"
#include "util/xml_to_json.h"

void Web::AddObject::addContainer(int parentID)
{
    auto cont = content->addContainer(parentID, param("title"), param("class"));

    std::string flags = param("flags");
    if (!flags.empty())
        cont->setFlags(CdsObject::makeFlag(flags));
}

std::shared_ptr<CdsItem> Web::AddObject::addItem(int parentID)
{
    auto item = std::make_shared<CdsItem>();
    item->setParentID(parentID);

    item->setTitle(param("title"));
    item->setLocation(param("location"));
    item->setClass(param("class"));

    std::string tmp = param("description");
    if (!tmp.empty()) {
        item->addMetaData(M_DESCRIPTION, tmp);
    }

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param("mime-type");
    if (tmp.empty())
        tmp = MIMETYPE_DEFAULT;
    item->setMimeType(tmp);

    std::string flags = param("flags");
    if (!flags.empty())
        item->setFlags(CdsObject::makeFlag(flags));

    item->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);

    return item;
}

std::shared_ptr<CdsItemExternalURL> Web::AddObject::addUrl(int parentID, bool addProtocol)
{
    auto item = std::make_shared<CdsItemExternalURL>();
    std::string protocolInfo;

    item->setParentID(parentID);

    item->setTitle(param("title"));
    item->setURL(param("location"));
    item->setClass(param("class"));

    std::string tmp = param("description");
    if (!tmp.empty()) {
        item->addMetaData(M_DESCRIPTION, tmp);
    }

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param("mime-type");
    if (tmp.empty())
        tmp = MIMETYPE_DEFAULT;
    item->setMimeType(tmp);

    std::string flags = param("flags");
    if (!flags.empty())
        item->setFlags(CdsObject::makeFlag(flags));

    if (addProtocol) {
        std::string protocol = param("protocol");
        if (!protocol.empty())
            protocolInfo = renderProtocolInfo(tmp, protocol);
        else
            protocolInfo = renderProtocolInfo(tmp);
    } else
        protocolInfo = renderProtocolInfo(tmp);

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, CdsResource::Purpose::Content);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, protocolInfo);
    item->addResource(resource);

    return item;
}

void Web::AddObject::process()
{
    checkRequest();

    auto objType = std::string(param("obj_type"));
    auto location = fs::path(param("location"));

    if (param("title").empty())
        throw_std_runtime_error("empty title");

    if (param("class").empty())
        throw_std_runtime_error("empty class");

    int parentID = intParam("parent_id", 0);

    std::shared_ptr<CdsObject> obj;

    bool allowFifo = false;
    std::error_code ec;

    if (objType == STRING_OBJECT_TYPE_CONTAINER) {
        this->addContainer(parentID);
    } else if (objType == STRING_OBJECT_TYPE_ITEM) {
        if (location.empty())
            throw_std_runtime_error("no location given");
        if (!isRegularFile(location, ec))
            throw_std_runtime_error("file not found {}", ec.message());
        obj = this->addItem(parentID);
        allowFifo = true;
    } else if (objType == STRING_OBJECT_TYPE_EXTERNAL_URL) {
        if (location.empty())
            throw_std_runtime_error("No URL given");
        obj = this->addUrl(parentID, true);
    } else {
        throw_std_runtime_error("Unknown object type: {}", objType.c_str());
    }

    if (obj) {
        obj->setVirtual(true);
        if (objType == STRING_OBJECT_TYPE_ITEM) {
            content->addVirtualItem(obj, allowFifo);
        } else {
            content->addObject(obj, true);
        }
    }
}
