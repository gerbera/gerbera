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

CdsObject::CdsObject(std::shared_ptr<Database> database)
    : database(std::move(database))
    , mtime(0)
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

std::shared_ptr<CdsObject> CdsObject::createObject(const std::shared_ptr<Database>& database, unsigned int objectType)
{
    std::shared_ptr<CdsObject> obj;

    if (IS_CDS_CONTAINER(objectType)) {
        obj = std::make_shared<CdsContainer>(database);
    } else if (IS_CDS_ITEM_INTERNAL_URL(objectType)) {
        obj = std::make_shared<CdsItemInternalURL>(database);
    } else if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        obj = std::make_shared<CdsItemExternalURL>(database);
    } else if (IS_CDS_ITEM(objectType)) {
        obj = std::make_shared<CdsItem>(database);
    } else {
        throw_std_runtime_error("invalid object type: " + std::to_string(objectType));
    }
    return obj;
}

/* CdsItem */

CdsItem::CdsItem(std::shared_ptr<Database> database)
    : CdsObject(std::move(database))
    , mimeType(MIMETYPE_DEFAULT)
{
    objectType = OBJECT_TYPE_ITEM;
    upnpClass = "object.item";
    trackNumber = 0;
}

void CdsItem::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!IS_CDS_ITEM(obj->getObjectType()))
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

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType) || IS_CDS_ITEM_INTERNAL_URL(objectType))
        return;

    std::error_code ec;
    if (!isRegularFile(location, ec))
        throw_std_runtime_error("Item validation failed: file " + location.string() + " not found");
}

//---------

CdsItemExternalURL::CdsItemExternalURL(std::shared_ptr<Database> database)
    : CdsItem(std::move(database))
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = UPNP_DEFAULT_CLASS_ITEM;
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

CdsItemInternalURL::CdsItemInternalURL(std::shared_ptr<Database> database)
    : CdsItemExternalURL(std::move(database))
{
    objectType |= OBJECT_TYPE_ITEM_INTERNAL_URL;
    mimeType = MIMETYPE_DEFAULT;
    upnpClass = "object.item";
}

void CdsItemInternalURL::validate()
{
    CdsItemExternalURL::validate();

    if (startswith(this->location, "http://"))
        throw_std_runtime_error("Internal URL item validation failed: only relative URLs allowed");
}

CdsContainer::CdsContainer(std::shared_ptr<Database> database)
    : CdsObject(std::move(database))
{
    objectType = OBJECT_TYPE_CONTAINER;
    updateID = 0;
    // searchable = 0; is now in objectFlags; by default all flags (except "restricted") are not set
    childCount = -1;
    upnpClass = UPNP_DEFAULT_CLASS_CONTAINER;
    autoscanType = OBJECT_AUTOSCAN_NONE;
}

void CdsContainer::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!IS_CDS_CONTAINER(obj->getObjectType()))
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

std::string CdsContainer::getVirtualPath() const
{
    std::string location;
    if (getID() == CDS_ID_ROOT) {
        location = std::string(1, VIRTUAL_CONTAINER_SEPARATOR);
    } else if (getID() == CDS_ID_FS_ROOT) {
        location = std::string(1, VIRTUAL_CONTAINER_SEPARATOR) + database->getFsRootName();
    } else if (!getLocation().empty()) {
        location = getLocation();
        if (!isVirtual()) {
            location = std::string(1, VIRTUAL_CONTAINER_SEPARATOR) + database->getFsRootName() + location;
        }
    }

    if (location.empty())
        throw_std_runtime_error("virtual location not available");

    return location;
}

std::string CdsItem::getVirtualPath() const
{
    auto cont = database->loadObject(getParentID());
    std::string location = cont->getVirtualPath();
    location = location + VIRTUAL_CONTAINER_SEPARATOR + getTitle();

    if (location.empty())
        throw_std_runtime_error("virtual location not available");

    return location;
}

std::string CdsObject::mapObjectType(int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (IS_CDS_PURE_ITEM(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    if (IS_CDS_ITEM_INTERNAL_URL(type))
        return STRING_OBJECT_TYPE_INTERNAL_URL;
    throw_std_runtime_error("illegal objectType: " + std::to_string(type));
}

int CdsObject::remapObjectType(const std::string& objectType)
{
    if (objectType == STRING_OBJECT_TYPE_CONTAINER)
        return OBJECT_TYPE_CONTAINER;
    if (objectType == STRING_OBJECT_TYPE_ITEM)
        return OBJECT_TYPE_ITEM;
    if (objectType == STRING_OBJECT_TYPE_ACTIVE_ITEM)
        return OBJECT_TYPE_ITEM | OBJECT_TYPE_ACTIVE_ITEM;
    if (objectType == STRING_OBJECT_TYPE_EXTERNAL_URL)
        return OBJECT_TYPE_ITEM | OBJECT_TYPE_ITEM_EXTERNAL_URL;
    if (objectType == STRING_OBJECT_TYPE_INTERNAL_URL)
        return OBJECT_TYPE_ITEM | OBJECT_TYPE_ITEM_EXTERNAL_URL | OBJECT_TYPE_ITEM_INTERNAL_URL;
    throw_std_runtime_error("illegal objectType: " + objectType);
}
