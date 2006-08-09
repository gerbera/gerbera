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
#include "mxml/mxml.h"

using namespace zmm;
using namespace mxml;

CdsObject::CdsObject() : Object()
{
    metadata = Ref<Dictionary>(new Dictionary());
    auxdata = Ref<Dictionary>(new Dictionary());
    resources = Ref<Array<CdsResource> >(new Array<CdsResource>);
    restricted = 1;
    id = INVALID_OBJECT_ID;
    parentID = INVALID_OBJECT_ID;
    refID = 0;
    virt = 0;
}

void CdsObject::copyTo(Ref<CdsObject> obj)
{
    obj->setID(id);
    obj->setParentID(parentID);
    obj->setRestricted(restricted);
    obj->setTitle(title);
    obj->setClass(upnpClass);
    obj->setLocation(location);
    obj->setVirtual(virt);
    obj->setMetadata(metadata->clone());
    obj->setAuxData(auxdata->clone());
    for (int i = 0; i < resources->size(); i++)
        obj->addResource(resources->get(i)->clone());
}
int CdsObject::equals(Ref<CdsObject> obj, bool exactly)
{
    if (!(
        id == obj->getID() &&
        parentID == obj->getParentID() &&
        restricted == obj->isRestricted() &&
        title == obj->getTitle() &&
        upnpClass == obj->getClass() &&
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
    if ((!string_ok(this->title)) ||
        (!string_ok(this->upnpClass)))
    {
        throw Exception(_("CdsObject: validation failed"));
    }

}

Ref<CdsObject> CdsObject::createObject(int objectType)
{
    CdsObject *pobj;
    
    if(IS_CDS_CONTAINER(objectType))
    {
        pobj = new CdsContainer();
    }
    else if(IS_CDS_ITEM_INTERNAL_URL(objectType))
    {
        pobj = new CdsItemInternalURL();
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
        throw Exception(_("invalid object type :") + objectType);
    }
    return Ref<CdsObject>(pobj);
}


/* CdsItem */

CdsItem::CdsItem() : CdsObject()
{
    objectType = OBJECT_TYPE_ITEM;
    upnpClass = _("object.item");
    mimeType = _(MIMETYPE_DEFAULT);
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
    return ( mimeType == item->getMimeType() );
}

void CdsItem::validate()
{
    CdsObject::validate();
//    log_info(("mime: [%s] loc [%s]\n", this->mimeType.c_str(), this->location.c_str()));
    if ((!string_ok(this->mimeType)) || (!check_path(this->location)))
        throw Exception(_("CdsItem: validation failed"));
}

CdsActiveItem::CdsActiveItem() : CdsItem()
{
    objectType |= OBJECT_TYPE_ACTIVE_ITEM;

    upnpClass = _("object.item.activeItem");
    mimeType = _(MIMETYPE_DEFAULT);
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
        throw Exception(_("CdsActiveItem: validation failed"));
}
//---------

CdsItemExternalURL::CdsItemExternalURL() : CdsItem()
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = _("object.item");
    mimeType = _(MIMETYPE_DEFAULT);
}

void CdsItemExternalURL::validate()
{
    CdsObject::validate();
    if ((!string_ok(this->mimeType)) || (!string_ok(this->location)))
        throw Exception(_("CdsItemExternalURL: validation failed"));
}
//---------

CdsItemInternalURL::CdsItemInternalURL() : CdsItemExternalURL()
{
    objectType |= OBJECT_TYPE_ITEM_INTERNAL_URL;

    upnpClass = _("object.item");
    mimeType = _(MIMETYPE_DEFAULT);
}

void CdsItemInternalURL::validate()
{
    CdsObject::validate();
    if ((!string_ok(this->mimeType)) || (!string_ok(this->location)))
        throw Exception(_("CdsItemInternalURL: validation failed"));

    if (this->location.startsWith(_("http://")))
        throw Exception(_("CdsItemInternalURL: validation failed: must be realtive!"));
}

CdsContainer::CdsContainer() : CdsObject()
{
    objectType = OBJECT_TYPE_CONTAINER;
    updateID = 0;
    searchable = 0;
    childCount = -1;
    upnpClass = _("object.container");
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
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
/*    if (!check_path(this->location, true))
        throw Exception(_("CdsContainer: validation failed")); */
}
void CdsObject::optimize()
{
    metadata->optimize();
    auxdata->optimize();
    resources->optimize();
}

int CdsObjectTitleComparator(void *arg1, void *arg2)
{
	/// \todo get rid of getTitle() to avod unnecessary reference counting ops
	return strcmp(((CdsObject *)arg1)->title.c_str(),
			      ((CdsObject *)arg2)->title.c_str());
}



