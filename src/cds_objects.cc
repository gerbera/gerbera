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

#include "cds_objects.h"
#include "mxml/mxml.h"
#include "storage/storage.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

CdsObject::CdsObject(std::shared_ptr<Storage> storage)
    : Object()
    , storage(storage)
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

void CdsObject::copyTo(Ref<CdsObject> obj)
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
int CdsObject::equals(Ref<CdsObject> obj, bool exactly)
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

int CdsObject::resourcesEqual(Ref<CdsObject> obj)
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
        throw _Exception("Object validation failed: missing title!\n");

    if (!string_ok(this->upnpClass))
        throw _Exception("Object validation failed: missing upnp class\n");
}

Ref<CdsObject> CdsObject::createObject(std::shared_ptr<Storage> storage, unsigned int objectType)
{
    CdsObject* pobj;

    if (IS_CDS_CONTAINER(objectType)) {
        pobj = new CdsContainer(storage);
    } else if (IS_CDS_ITEM_INTERNAL_URL(objectType)) {
        pobj = new CdsItemInternalURL(storage);
    } else if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        pobj = new CdsItemExternalURL(storage);
    } else if (IS_CDS_ACTIVE_ITEM(objectType)) {
        pobj = new CdsActiveItem(storage);
    } else if (IS_CDS_ITEM(objectType)) {
        pobj = new CdsItem(storage);
    } else {
        throw _Exception("invalid object type: " + std::to_string(objectType));
    }
    return Ref<CdsObject>(pobj);
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

void CdsItem::copyTo(Ref<CdsObject> obj)
{
    CdsObject::copyTo(obj);
    if (!IS_CDS_ITEM(obj->getObjectType()))
        return;
    Ref<CdsItem> item = RefCast(obj, CdsItem);
    //    item->setDescription(description);
    item->setMimeType(mimeType);
    item->setTrackNumber(trackNumber);
    item->setServiceID(serviceID);
}
int CdsItem::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsItem> item = RefCast(obj, CdsItem);
    if (!CdsObject::equals(obj, exactly))
        return 0;
    return (mimeType == item->getMimeType() && trackNumber == item->getTrackNumber() && serviceID == item->getServiceID());
}

void CdsItem::validate()
{
    CdsObject::validate();
    //    log_info("mime: [%s] loc [%s]\n", this->mimeType.c_str(), this->location.c_str());
    if (!string_ok(this->mimeType))
        throw _Exception("Item validation failed: missing mimetype");

    if (!string_ok(this->location))
        throw _Exception("Item validation failed: missing location");

    if (!check_path(this->location))
        throw _Exception("Item validation failed: file " + this->location + " not found");
}

CdsActiveItem::CdsActiveItem(std::shared_ptr<Storage> storage)
    : CdsItem(storage)
{
    objectType |= OBJECT_TYPE_ACTIVE_ITEM;

    upnpClass = UPNP_DEFAULT_CLASS_ACTIVE_ITEM;
    mimeType = MIMETYPE_DEFAULT;
}

void CdsActiveItem::copyTo(Ref<CdsObject> obj)
{
    CdsItem::copyTo(obj);
    if (!IS_CDS_ACTIVE_ITEM(obj->getObjectType()))
        return;
    Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
    item->setAction(action);
    item->setState(state);
}
int CdsActiveItem::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
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
        throw _Exception("Active Item validation failed: missing action\n");

    if (!check_path(this->action))
        throw _Exception("Active Item validation failed: action script " + this->action + " not found\n");
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
        throw _Exception("URL Item validation failed: missing mimetype\n");

    if (!string_ok(this->location))
        throw _Exception("URL Item validation failed: missing URL\n");
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
        throw _Exception("Internal URL item validation failed: only realative URLs allowd\n");
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

void CdsContainer::copyTo(Ref<CdsObject> obj)
{
    CdsObject::copyTo(obj);
    if (!IS_CDS_CONTAINER(obj->getObjectType()))
        return;
    Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    cont->setUpdateID(updateID);
}
int CdsContainer::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    return (
        CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable());
}

void CdsContainer::validate()
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
    /*    if (!check_path(this->location, true))
        throw _Exception("CdsContainer: validation failed"); */
}

int CdsObjectTitleComparator(void* arg1, void* arg2)
{
    /// \todo get rid of getTitle() to avod unnecessary reference counting ops
    return strcmp(((CdsObject*)arg1)->title.c_str(),
        ((CdsObject*)arg2)->title.c_str());
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
        throw _Exception("virtual location not available");

    return location;
}

std::string CdsItem::getVirtualPath()
{
    Ref<CdsObject> cont = storage->loadObject(getParentID());
    std::string location = cont->getVirtualPath();
    location = location + '/' + getTitle();

    if (!string_ok(location))
        throw _Exception("virtual location not available");

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
    throw Exception("illegal objectType: " + std::to_string(type));
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
    throw Exception("illegal objectType: " + objectType);
}
