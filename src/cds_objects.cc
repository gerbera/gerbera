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

#include <filesystem>

#include "database/database.h"
#include "util/tools.h"

static constexpr bool IS_CDS_ITEM(unsigned int type) { return type & OBJECT_TYPE_ITEM; }
static constexpr bool IS_CDS_PURE_ITEM(unsigned int type) { return type == OBJECT_TYPE_ITEM; }

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
    obj->setMetadata(metadata);
    obj->setAuxData(auxdata);
    obj->setFlags(objectFlags);
    obj->setSortPriority(sortPriority);
    for (auto&& resource : resources)
        obj->addResource(resource->clone());
}
bool CdsObject::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    if (!(id == obj->getID()
            && parentID == obj->getParentID()
            && isRestricted() == obj->isRestricted()
            && title == obj->getTitle()
            && upnpClass == obj->getClass()
            && sortPriority == obj->getSortPriority()))
        return false;

    if (!resourcesEqual(obj))
        return false;

    if (metadata != obj->getMetadata())
        return false;

    return !(exactly
        && !(location == obj->getLocation()
            && mtime == obj->getMTime()
            && sizeOnDisk == obj->getSizeOnDisk()
            && virt == obj->isVirtual()
            && std::equal(auxdata.begin(), auxdata.end(), obj->auxdata.begin())
            && objectFlags == obj->getFlags()));
}

bool CdsObject::resourcesEqual(const std::shared_ptr<CdsObject>& obj)
{
    if (resources.size() != obj->resources.size())
        return false;

    // compare all resources
    return std::equal(resources.begin(), resources.end(), obj->resources.begin(),
        [](auto&& r1, auto&& r2) { return r1->equals(r2); });
}

void CdsObject::validate()
{
    if (this->title.empty())
        throw_std_runtime_error("Object validation failed: missing title");

    if (this->upnpClass.empty())
        throw_std_runtime_error("Object validation failed: missing upnp class");
}

std::shared_ptr<CdsObject> CdsObject::createObject(unsigned int objectType)
{
    if (IS_CDS_CONTAINER(objectType)) {
        return std::make_shared<CdsContainer>();
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        return std::make_shared<CdsItemExternalURL>();
    }

    if (IS_CDS_ITEM(objectType)) {
        return std::make_shared<CdsItem>();
    }

    throw_std_runtime_error("invalid object type: {}", objectType);
}

/* CdsItem */

CdsItem::CdsItem()
{
    objectType = OBJECT_TYPE_ITEM;
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
    item->setBookMarkPos(bookMarkPos);
}
bool CdsItem::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    auto item = std::static_pointer_cast<CdsItem>(obj);
    if (!CdsObject::equals(obj, exactly))
        return false;
    return (mimeType == item->getMimeType() && partNumber == item->getPartNumber() && trackNumber == item->getTrackNumber() && serviceID == item->getServiceID() && bookMarkPos == item->getBookMarkPos());
}

void CdsItem::validate()
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
    : CdsItem()
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = UPNP_CLASS_ITEM;
}

void CdsItemExternalURL::validate()
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
    objectType = OBJECT_TYPE_CONTAINER;
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
bool CdsContainer::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    return CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable();
}

void CdsContainer::validate()
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
    /*    if (!fs::is_directory(this->location, true))
        throw_std_runtime_error("validation failed"); */
}

std::string_view CdsObject::mapObjectType(unsigned int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (IS_CDS_PURE_ITEM(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    throw_std_runtime_error("illegal objectType: {}", type);
}
