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

#include <filesystem>

#include "cds_objects.h"
#include "storage/storage.h"
#include "util/tools.h"

CdsObject::CdsObject(std::shared_ptr<Storage> storage)
    : storage(storage)
{
    id = INVALID_OBJECT_ID;
    parentID = INVALID_OBJECT_ID;
    refID = INVALID_OBJECT_ID;
    mtime = 0;
    sizeOnDisk = 0;
    virt = 0;
    sortPriority = 0;
    objectFlags = OBJECT_FLAG_RESTRICTED;
}

void CdsObject::copyTo(std::shared_ptr<CdsObject> obj)
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
    for (size_t i = 0; i < resources.size(); i++)
        obj->addResource(resources[i]->clone());
}
int CdsObject::equals(std::shared_ptr<CdsObject> obj, bool exactly)
{
    if (!(id == obj->getID()
        && parentID == obj->getParentID()
        && isRestricted() == obj->isRestricted()
        && title == obj->getTitle()
        && upnpClass == obj->getClass()
        && sortPriority == obj->getSortPriority())
        )
        return 0;

    if (!resourcesEqual(obj))
        return 0;

    if (!std::equal(metadata.begin(), metadata.end(), obj->getMetadata().begin()))
        return 0;

    if (exactly
        && !(location == obj->getLocation()
            && mtime == obj->getMTime()
            && sizeOnDisk == obj->getSizeOnDisk()
            && virt == obj->isVirtual()
            && std::equal(auxdata.begin(), auxdata.end(), obj->auxdata.begin())
            && objectFlags == obj->getFlags())
        )
        return 0;
    return 1;
}

int CdsObject::resourcesEqual(std::shared_ptr<CdsObject> obj)
{
    if (resources.size() != obj->resources.size())
        return 0;

    // compare all resources
    for (size_t i = 0; i < resources.size(); i++) {
        if (!resources[i]->equals(obj->resources[i]))
            return 0;
    }
    return 1;
}

void CdsObject::validate()
{
    if (!string_ok(this->title))
        throw std::runtime_error("Object validation failed: missing title!\n");

    if (!string_ok(this->upnpClass))
        throw std::runtime_error("Object validation failed: missing upnp class\n");
}

std::shared_ptr<CdsObject> CdsObject::createObject(std::shared_ptr<Storage> storage, unsigned int objectType)
{
    std::shared_ptr<CdsObject> obj;

    if (IS_CDS_CONTAINER(objectType)) {
        obj = std::make_shared<CdsContainer>(storage);
    } else if (IS_CDS_ITEM_INTERNAL_URL(objectType)) {
        obj = std::make_shared<CdsItemInternalURL>(storage);
    } else if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        obj = std::make_shared<CdsItemExternalURL>(storage);
    } else if (IS_CDS_ACTIVE_ITEM(objectType)) {
        obj = std::make_shared<CdsActiveItem>(storage);
    } else if (IS_CDS_ITEM(objectType)) {
        obj = std::make_shared<CdsItem>(storage);
    } else {
        throw std::runtime_error("invalid object type: " + std::to_string(objectType));
    }
    return obj;
}

/* CdsItem */

CdsItem::CdsItem(std::shared_ptr<Storage> storage)
    : CdsObject(storage)
{
    objectType = OBJECT_TYPE_ITEM;
    upnpClass = "object.item";
    mimeType = MIMETYPE_DEFAULT;
    trackNumber = 0;
    serviceID = "";
}

void CdsItem::copyTo(std::shared_ptr<CdsObject> obj)
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
int CdsItem::equals(std::shared_ptr<CdsObject> obj, bool exactly)
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
    if (!string_ok(this->mimeType))
        throw std::runtime_error("Item validation failed: missing mimetype");

    if (!string_ok(this->location))
        throw std::runtime_error("Item validation failed: missing location");

    if (!std::filesystem::is_regular_file(location))
        throw std::runtime_error("Item validation failed: file " + location + " not found");
}

CdsActiveItem::CdsActiveItem(std::shared_ptr<Storage> storage)
    : CdsItem(storage)
{
    objectType |= OBJECT_TYPE_ACTIVE_ITEM;

    upnpClass = UPNP_DEFAULT_CLASS_ACTIVE_ITEM;
    mimeType = MIMETYPE_DEFAULT;
}

void CdsActiveItem::copyTo(std::shared_ptr<CdsObject> obj)
{
    CdsItem::copyTo(obj);
    if (!IS_CDS_ACTIVE_ITEM(obj->getObjectType()))
        return;
    auto item = std::static_pointer_cast<CdsActiveItem>(obj);
    item->setAction(action);
    item->setState(state);
}
int CdsActiveItem::equals(std::shared_ptr<CdsObject> obj, bool exactly)
{
    auto item = std::static_pointer_cast<CdsActiveItem>(obj);
    if (!CdsItem::equals(obj, exactly))
        return 0;
    if (exactly && (action != item->getAction() || state != item->getState()))
        return 0;
    return 1;
}

void CdsActiveItem::validate()
{
    CdsItem::validate();
    if (!string_ok(this->action))
        throw std::runtime_error("Active Item validation failed: missing action\n");

    if (!std::filesystem::is_regular_file(this->action))
        throw std::runtime_error("Active Item validation failed: action script " + action + " not found\n");
}
//---------

CdsItemExternalURL::CdsItemExternalURL(std::shared_ptr<Storage> storage)
    : CdsItem(storage)
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = UPNP_DEFAULT_CLASS_ITEM;
    mimeType = MIMETYPE_DEFAULT;
}

void CdsItemExternalURL::validate()
{
    CdsObject::validate();
    if (!string_ok(this->mimeType))
        throw std::runtime_error("URL Item validation failed: missing mimetype\n");

    if (!string_ok(this->location))
        throw std::runtime_error("URL Item validation failed: missing URL\n");
}
//---------

CdsItemInternalURL::CdsItemInternalURL(std::shared_ptr<Storage> storage)
    : CdsItemExternalURL(storage)
{
    objectType |= OBJECT_TYPE_ITEM_INTERNAL_URL;

    upnpClass = "object.item";
    mimeType = MIMETYPE_DEFAULT;
}

void CdsItemInternalURL::validate()
{
    CdsItemExternalURL::validate();

    if (startswith(this->location, "http://"))
        throw std::runtime_error("Internal URL item validation failed: only realative URLs allowd\n");
}

CdsContainer::CdsContainer(std::shared_ptr<Storage> storage)
    : CdsObject(storage)
{
    objectType = OBJECT_TYPE_CONTAINER;
    updateID = 0;
    // searchable = 0; is now in objectFlags; by default all flags (except "restricted") are not set
    childCount = -1;
    upnpClass = UPNP_DEFAULT_CLASS_CONTAINER;
    autoscanType = OBJECT_AUTOSCAN_NONE;
}

void CdsContainer::copyTo(std::shared_ptr<CdsObject> obj)
{
    CdsObject::copyTo(obj);
    if (!IS_CDS_CONTAINER(obj->getObjectType()))
        return;
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    cont->setUpdateID(updateID);
}
int CdsContainer::equals(std::shared_ptr<CdsObject> obj, bool exactly)
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    return (
        CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable());
}

void CdsContainer::validate()
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
    /*    if (!std::filesystem::is_directory(this->location, true))
        throw std::runtime_error("CdsContainer: validation failed"); */
}

std::string CdsContainer::getVirtualPath()
{
    std::string location;
    if (getID() == CDS_ID_ROOT) {
        location = "/";
    } else if (getID() == CDS_ID_FS_ROOT) {
        location = "/" + storage->getFsRootName();
    } else if (string_ok(getLocation())) {
        location = getLocation();
        if (!isVirtual()) {
            location = "/" + storage->getFsRootName() + location;
        }
    }

    if (!string_ok(location))
        throw std::runtime_error("virtual location not available");

    return location;
}

std::string CdsItem::getVirtualPath()
{
    auto cont = storage->loadObject(getParentID());
    std::string location = cont->getVirtualPath();
    location = location + '/' + getTitle();

    if (!string_ok(location))
        throw std::runtime_error("virtual location not available");

    return location;
}

std::string CdsObject::mapObjectType(int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (IS_CDS_PURE_ITEM(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ACTIVE_ITEM(type))
        return STRING_OBJECT_TYPE_ACTIVE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    if (IS_CDS_ITEM_INTERNAL_URL(type))
        return STRING_OBJECT_TYPE_INTERNAL_URL;
    throw std::runtime_error("illegal objectType: " + std::to_string(type));
}

int CdsObject::remapObjectType(std::string objectType)
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
    throw std::runtime_error("illegal objectType: " + objectType);
}
