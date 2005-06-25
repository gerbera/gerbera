/*  cds_objects.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "cds_objects.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

CdsObject::CdsObject() : Object()
{
    metadata = Ref<Dictionary>(new Dictionary());
    auxdata = Ref<Dictionary>(new Dictionary());
    resources = Ref<Array<Dictionary> >(new Array<Dictionary>());
    restricted = 1;
    virt = 0;
}
void CdsObject::setID(String id)
{
    this->id = id;
}
String CdsObject::getID()
{
    return id;
}

void CdsObject::setRefID(String ref_id)
{
    this->ref_id = ref_id;
}
String CdsObject::getRefID()
{
    return ref_id;
}

void CdsObject::setParentID(String parentID)
{
    this->parentID = parentID;
}
String CdsObject::getParentID()
{
    return parentID;
}

void CdsObject::setRestricted(int restricted)
{
    this->restricted = restricted ? 1 : 0;
}
int CdsObject::isRestricted()
{
    return restricted;
}

void CdsObject::setTitle(String title)
{
    this->title = title;
}
String CdsObject::getTitle()
{
    return title;
}

void CdsObject::setClass(String upnp_class)
{
    this->upnp_class = upnp_class;
}
String CdsObject::getClass()
{
    return upnp_class;
}

void CdsObject::setLocation(String location)
{
    this->location = location;
}

String CdsObject::getLocation(){
    return location;
}

void CdsObject::setVirtual(int virt)
{
    this->virt = virt ? 1 : 0;
}
int CdsObject::isVirtual()
{
    return virt;
}

int CdsObject::getObjectType()
{
    return objectType;
}


void CdsObject::copyTo(Ref<CdsObject> obj)
{
    obj->setID(id);
    obj->setParentID(parentID);
    obj->setRestricted(restricted);
    obj->setTitle(title);
    obj->setClass(upnp_class);
    obj->setLocation(location);
    obj->setVirtual(virt);
    obj->setMetadata(metadata->clone());
    obj->setAuxData(auxdata->clone());
    for (int i = 0; i < resources->size(); i++)
        obj->addResource(resources->get(i));
}
int CdsObject::equals(Ref<CdsObject> obj, bool exactly)
{
    if (!(
        id == obj->getID() &&
        parentID == obj->getParentID() &&
        restricted == obj->isRestricted() &&
        title == obj->getTitle() &&
        upnp_class == obj->getClass() &&
        resources->size() == obj->resources->size()
       ))
        return 0;
    // compare all resources
    for (int i = 0; i < resources->size(); i++)
        if (! resources->get(i)->equals(obj->resources->get(i)))
            return 0;

    if (! metadata->equals(obj->getMetadata()))
        return 0;
    if (exactly && !
        (location == obj->getLocation() &&
         virt == obj->isVirtual() &&
         auxdata->equals(obj->auxdata)
        ))
        return 0;
    return 1;
}

void CdsObject::validate()
{
    if (!string_ok(this->parentID) ||
            (!string_ok(this->title)) ||
            (!string_ok(this->upnp_class)))
    {
    //    log_info(("this->parentID %s, this->title %s, this->upnp_class %s, this->location %s\n", this->parentID.c_str(), this->title.c_str(), this->upnp_class.c_str(), this->location.c_str()));
        throw Exception(String("CdsObject: validation failed"));
    }

}

Ref<CdsObject> CdsObject::createObject(int objectType)
{
    CdsObject *pobj;

    if(IS_CDS_CONTAINER(objectType))
    {
        pobj = new CdsContainer();
    }
    else if(IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        pobj = new CdsItemExternalURL();
    }
    else if(IS_CDS_ACTIVE_ITEM(objectType))
    {
        pobj = new CdsActiveItem();
    }
    else if(IS_CDS_ITEM(objectType))
    {
        pobj = new CdsItem();
    }
    else
    {
        throw Exception(String("invalid object type :") + objectType);
    }
    return Ref<CdsObject>(pobj);
}


String CdsObject::getMetadata(String key)
{
    return metadata->get(key);
}
Ref<Dictionary> CdsObject::getMetadata()
{
    return metadata;
}
void CdsObject::setMetadata(String key, String value)
{
    metadata->put(key, value);
}
void CdsObject::setMetadata(Ref<Dictionary> metadata)
{
    this->metadata = metadata;
}
void CdsObject::removeMetadata(String key)
{
    metadata->remove(key);
}


String CdsObject::getAuxData(String key)
{
    return auxdata->get(key);
}
Ref<Dictionary> CdsObject::getAuxData()
{
    return auxdata;
}
void CdsObject::setAuxData(String key, String value)
{
    auxdata->put(key, value);
}
void CdsObject::setAuxData(Ref<Dictionary> auxdata)
{
    this->auxdata = auxdata;
}
void CdsObject::removeAuxData(String key)
{
    auxdata->remove(key);
}


int CdsObject::getResourceCount()
{
    return resources->size();
}
Ref<Dictionary> CdsObject::getResource(int index)
{
    return resources->get(index);
}
String CdsObject::getResource(int index, String key)
{
    return resources->get(index)->get(key);
}
void CdsObject::setResource(int index, Ref<Dictionary> resource)
{
    resources->set(resource, index);
}
void CdsObject::setResource(int index, String key, String value)
{
    resources->get(index)->put(key, value);
}
void CdsObject::addResource(Ref<Dictionary> resource)
{
    resources->append(resource);
}
void CdsObject::addResource()
{
    Ref<Dictionary> resource(new Dictionary());
    resources->append(resource);
}


/* CdsItem */

CdsItem::CdsItem() : CdsObject()
{
    objectType = OBJECT_TYPE_ITEM;
    upnp_class = "object.item";
    mimeType = "application/octet-stream";
}

/*void CdsItem::setDescription(String description)
{
    this->description = description;
}
String CdsItem::getDescription()
{
    return this->description;
}
*/
void CdsItem::setMimeType(String mimeType)
{
    this->mimeType = mimeType;
}
String CdsItem::getMimeType()
{
    return mimeType;
}

void CdsItem::copyTo(Ref<CdsObject> obj)
{
    CdsObject::copyTo(obj);
    if (! IS_CDS_ITEM(obj->getObjectType()))
        return;
    Ref<CdsItem> item = RefCast(obj, CdsItem);
//    item->setDescription(description);
    item->setMimeType(mimeType);
}
int CdsItem::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsItem> item = RefCast(obj, CdsItem);
    if (! CdsObject::equals(obj, exactly))
        return 0;
    return (
//        description == item->getDescription() &&
        mimeType == item->getMimeType()
    );
}

void CdsItem::validate()
{
    CdsObject::validate();
//    log_info(("mime: [%s] loc [%s]\n", this->mimeType.c_str(), this->location.c_str()));
    if ((!string_ok(this->mimeType)) || (!check_path(this->location)))
        throw Exception(String("CdsItem: validation failed"));
}

CdsActiveItem::CdsActiveItem() : CdsItem()
{
    objectType |= OBJECT_TYPE_ACTIVE_ITEM;

    upnp_class = "object.item.activeItem";
    mimeType = "application/octet-stream";
}

void CdsActiveItem::setAction(zmm::String action)
{
    this->action = action;
}
zmm::String CdsActiveItem::getAction()
{
    return action;
}
void CdsActiveItem::setState(zmm::String state)
{
    this->state = state;
}
zmm::String CdsActiveItem::getState()
{
    return state;
}
void CdsActiveItem::copyTo(Ref<CdsObject> obj)
{
    CdsItem::copyTo(obj);
    if (! IS_CDS_ACTIVE_ITEM(obj->getObjectType()))
        return;
    Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
    item->setAction(action);
    item->setState(state);
}
int CdsActiveItem::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
    if (! CdsItem::equals(obj, exactly))
        return 0;
    if (exactly &&
       (action != item->getAction() ||
        state != item->getState())
    )
        return 0;
    return 1;
}

void CdsActiveItem::validate()
{
    CdsItem::validate();
    if ((!string_ok(this->action)) || (!check_path(this->action)))
        throw Exception(String("CdsActiveItem: validation failed"));
}
//---------

CdsItemExternalURL::CdsItemExternalURL() : CdsItem()
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnp_class = "object.item";
    mimeType = "application/octet-stream";
}

void CdsItemExternalURL::setURL(String URL)
{
    setLocation(URL);
}

String CdsItemExternalURL::getURL()
{
    return getLocation();
}

void CdsItemExternalURL::validate()
{
    CdsObject::validate();
    if ((!string_ok(this->mimeType)) || (!string_ok(this->location)))
        throw Exception(String("CdsItem: validation failed"));
}
//---------


CdsContainer::CdsContainer() : CdsObject()
{
    objectType = OBJECT_TYPE_CONTAINER;
    updateID = 0;
    searchable = 0;
    childCount = -1;
    upnp_class = "object.container";
}

void CdsContainer::setSearchable(int searchable)
{
    this->searchable = searchable ? 1 : 0;
}
int CdsContainer::isSearchable()
{
    return searchable;
}

void CdsContainer::setUpdateID(int updateID)
{
    this->updateID = updateID;
}
int CdsContainer::getUpdateID()
{
    return updateID;
}
void CdsContainer::setChildCount(int childCount)
{
    this->childCount = childCount;
}
int CdsContainer::getChildCount()
{
    return childCount;
}
void CdsContainer::copyTo(Ref<CdsObject> obj)
{
    CdsObject::copyTo(obj);
    if (! IS_CDS_CONTAINER(obj->getObjectType()))
        return;
    Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    cont->setUpdateID(updateID);
    cont->setSearchable(searchable);
}
int CdsContainer::equals(Ref<CdsObject> obj, bool exactly)
{
    Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    return (
        CdsObject::equals(obj, exactly) &&
        searchable == cont->isSearchable()
    );
}

void CdsContainer::validate()
{
    CdsObject::validate();
    /// \TODO well.. we have to know if a container is a real directory or just a virtual container in the database
/*    if (!check_path(this->location, true))
        throw Exception(String("CdsContainer: validation failed")); */
}

int CdsObjectTitleComparator(void *arg1, void *arg2)
{
	// \TODO get rid of getTitle() to avod unnecessary reference counting ops
	return strcmp(((CdsObject *)arg1)->title.c_str(),
			      ((CdsObject *)arg2)->title.c_str());
}



