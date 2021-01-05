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
#include <utility>

#include "database/database.h"
#include "util/tools.h"

static constexpr bool IS_CDS_ITEM(unsigned int type) { return type & OBJECT_TYPE_ITEM; };
static constexpr bool IS_CDS_PURE_ITEM(unsigned int type) { return type == OBJECT_TYPE_ITEM; };

CdsObject::CdsObject()
    : mtime(0)
    , sizeOnDisk(0)
{
    id = INVALID_OBJECT_ID;
    parentID = INVALID_OBJECT_ID;
    refID = INVALID_OBJECT_ID;
    virt = false;
    sortPriority = 0;
    objectFlags = OBJECT_FLAG_RESTRICTED;
}

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
    for (auto& resource : resources)
        obj->addResource(resource->clone());
}
int CdsObject::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    if (!(id == obj->getID()
            && parentID == obj->getParentID()
            && isRestricted() == obj->isRestricted()
            && title == obj->getTitle()
            && upnpClass == obj->getClass()
            && sortPriority == obj->getSortPriority()))
        return 0;

    if (!resourcesEqual(obj))
        return 0;

    if (metadata != obj->getMetadata())
        return 0;

    if (exactly
        && !(location == obj->getLocation()
            && mtime == obj->getMTime()
            && sizeOnDisk == obj->getSizeOnDisk()
            && virt == obj->isVirtual()
            && std::equal(auxdata.begin(), auxdata.end(), obj->auxdata.begin())
            && objectFlags == obj->getFlags()))
        return 0;
    return 1;
}

int CdsObject::resourcesEqual(const std::shared_ptr<CdsObject>& obj)
{
    if (resources.size() != obj->resources.size())
        return 0;

    // compare all resources
    return std::equal(resources.begin(), resources.end(), obj->resources.begin(),
        [](const auto& r1, const auto& r2) { return r1->equals(r2); });
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
    std::shared_ptr<CdsObject> obj;

    if (IS_CDS_CONTAINER(objectType)) {
        obj = std::make_shared<CdsContainer>();
    } else if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        obj = std::make_shared<CdsItemExternalURL>();
    } else if (IS_CDS_ITEM(objectType)) {
        obj = std::make_shared<CdsItem>();
    } else {
        throw_std_runtime_error("invalid object type: " + std::to_string(objectType));
    }
    return obj;
}

/* CdsItem */

CdsItem::CdsItem()
    : CdsObject()
    , mimeType(MIMETYPE_DEFAULT)
{
    objectType = OBJECT_TYPE_ITEM;
    upnpClass = "object.item";
    trackNumber = 0;
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
    item->setServiceID(serviceID);
}
int CdsItem::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    auto item = std::static_pointer_cast<CdsItem>(obj);
    if (!CdsObject::equals(obj, exactly))
        return 0;
    return (mimeType == item->getMimeType() && trackNumber == item->getTrackNumber() && serviceID == item->getServiceID());
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
        throw_std_runtime_error("Item validation failed: file " + location.string() + " not found");
}

//---------

CdsItemExternalURL::CdsItemExternalURL()
    : CdsItem()
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = UPNP_CLASS_ITEM;
    mimeType = MIMETYPE_DEFAULT;
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
    : CdsObject()
{
    objectType = OBJECT_TYPE_CONTAINER;
    updateID = 0;
    // searchable = 0; is now in objectFlags; by default all flags (except "restricted") are not set
    childCount = -1;
    upnpClass = UPNP_CLASS_CONTAINER;
    autoscanType = OBJECT_AUTOSCAN_NONE;
}

void CdsContainer::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!obj->isContainer())
        return;
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    cont->setUpdateID(updateID);
}
int CdsContainer::equals(const std::shared_ptr<CdsObject>& obj, bool exactly)
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    return (
        CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable());
}

void CdsContainer::validate()
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
    /*    if (!fs::is_directory(this->location, true))
        throw_std_runtime_error("validation failed"); */
}

std::string CdsObject::mapObjectType(unsigned int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (IS_CDS_PURE_ITEM(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    throw_std_runtime_error("illegal objectType: " + std::to_string(type));
}
