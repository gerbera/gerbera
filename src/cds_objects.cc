/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_objects.cc - this file is part of MediaTomb.

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

/// \file cds_objects.cc

#include "cds_objects.h" // API

#include "database/database.h"
#include "util/tools.h"
#include "util/upnp_clients.h"

void CdsObject::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    obj->setID(id);
    obj->setRefID(refID);
    obj->setParentID(parentID);
    obj->setTitle(title);
    obj->setClass(upnpClass);
    obj->setLocation(location);
    obj->setMTime(mtime);
    obj->setSizeOnDisk(sizeOnDisk);
    obj->setVirtual(virt);
    obj->setMetaData(metaData);
    obj->setAuxData(auxdata);
    obj->setFlags(objectFlags);
    obj->setSortPriority(sortPriority);
    for (auto&& resource : resources)
        obj->addResource(resource->clone());
}

bool CdsObject::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    if (id != obj->getID()
        || parentID != obj->getParentID()
        || isRestricted() != obj->isRestricted()
        || title != obj->getTitle()
        || upnpClass != obj->getClass()
        || sortPriority != obj->getSortPriority())
        return false;

    if (!resourcesEqual(obj))
        return false;

    if (metaData != obj->getMetaData())
        return false;

    return !exactly
        || (location == obj->getLocation()
            && mtime == obj->getMTime()
            && sizeOnDisk == obj->getSizeOnDisk()
            && virt == obj->isVirtual()
            && std::equal(auxdata.begin(), auxdata.end(), obj->auxdata.begin())
            && objectFlags == obj->getFlags());
}

bool CdsObject::resourcesEqual(const std::shared_ptr<CdsObject>& obj) const
{
    if (resources.size() != obj->resources.size())
        return false;

    // compare all resources
    return std::equal(resources.begin(), resources.end(), obj->resources.begin(),
        [](auto&& r1, auto&& r2) { return r1->equals(r2); });
}

void CdsObject::validate() const
{
    if (this->title.empty())
        throw_std_runtime_error("Object validation failed: missing title");

    if (this->upnpClass.empty())
        throw_std_runtime_error("Object validation failed: missing upnp class");
}

std::shared_ptr<CdsObject> CdsObject::createObject(CdsObject::Type type)
{
    switch (type) {
    case Type::CONTAINER:
        return std::make_shared<CdsContainer>();
    case Type::ITEM:
        return std::make_shared<CdsItem>();
    case Type::EXTERNAL_URL:
        return std::make_shared<CdsItemExternalURL>();
    }
    throw_std_runtime_error("invalid object type: {}", type);
}

/* CdsItem */
CdsItem::CdsItem()
{
    objectType = Type::ITEM;
    upnpClass = "object.item";
}

void CdsItem::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!obj->isItem())
        return;
    auto item = std::static_pointer_cast<CdsItem>(obj);
    //    item->setDescription(description);
    item->setMimeType(mimeType);
    item->setTrackNumber(trackNumber);
    item->setPartNumber(partNumber);
    item->setServiceID(serviceID);
    if (playStatus)
        item->setPlayStatus(playStatus->clone());
}

bool CdsItem::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    auto item = std::static_pointer_cast<CdsItem>(obj);
    if (!CdsObject::equals(obj, exactly))
        return false;
    return (mimeType == item->getMimeType() && partNumber == item->getPartNumber() && trackNumber == item->getTrackNumber() && serviceID == item->getServiceID());
}

void CdsItem::validate() const
{
    CdsObject::validate();
    //    log_info("mime: [{}] loc [{}]", this->mimeType.c_str(), this->location.c_str());
    if (this->mimeType.empty())
        throw_std_runtime_error("Item validation failed: missing mimetype");

    if (this->location.empty())
        throw_std_runtime_error("Item validation failed: missing location");

    if (isExternalItem())
        return;

    std::error_code ec;
    if (!isRegularFile(location, ec))
        throw_std_runtime_error("Item validation failed: file {} not found", location.c_str());
}

//---------

CdsItemExternalURL::CdsItemExternalURL()
{
    objectType = Type::EXTERNAL_URL;

    upnpClass = UPNP_CLASS_ITEM;
}

void CdsItemExternalURL::validate() const
{
    CdsItem::validate();
    if (this->mimeType.empty())
        throw_std_runtime_error("URL Item validation failed: missing mimetype");

    if (this->location.empty())
        throw_std_runtime_error("URL Item validation failed: missing URL");
}
//---------

CdsContainer::CdsContainer()
{
    objectType = Type::CONTAINER;
    upnpClass = UPNP_CLASS_CONTAINER;
}

void CdsContainer::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!obj->isContainer())
        return;
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    cont->setUpdateID(updateID);
}
bool CdsContainer::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    return CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable();
}

/*
void CdsContainer::validate() const
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
      if (!fs::is_directory(this->location, true))
        throw_std_runtime_error("validation failed");
}
*/

std::string_view CdsObject::mapObjectType(Type type)
{
    switch (type) {
    case Type::CONTAINER:
        return "container";
    case Type::ITEM:
        return "item";
    case Type::EXTERNAL_URL:
        return "external_url";
    }
    throw_std_runtime_error("Invalid object type: {}", type);
}
