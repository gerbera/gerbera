/*  sql_storage.cc - this file is part of MediaTomb.

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

#include "sql_storage.h"

using namespace zmm;

#define MAX_DELETE_QUERY_LENGTH 4096

enum
{
    _id,
    _ref_id,
    _parent_id,
    _object_type,
    _upnp_class,
    _dc_title,
    _is_restricted,
    _is_virtual,
    _metadata,
    _auxdata,
    _resources,

    _update_id,
    _is_searchable,

    _location,
    _mime_type,

    _action,
    _state,

    _ref_resources
};

static char *select_query_base = "\
    SELECT \
    \
    f.id, \
    f.ref_id, \
    f.parent_id, \
    f.object_type, \
    f.upnp_class, \
    f.dc_title, \
    f.is_restricted, \
    f.is_virtual, \
    f.metadata, \
    f.auxdata, \
    f.resources, \
    \
    f.update_id, \
    f.is_searchable, \
    \
    f.location, \
    f.mime_type, \
    \
    f.action, \
    f.state, \
    \
    rf.resources\
    \
    FROM cds_objects f LEFT JOIN cds_objects rf \
    ON f.ref_id = rf.id";


SQLRow::SQLRow() : Object()
{}
SQLRow::~SQLRow()
{}


SQLResult::SQLResult() : Object()
{}
SQLResult::~SQLResult()
{}


SQLStorage::SQLStorage() : Storage()
{}
SQLStorage::~SQLStorage()
{}


void SQLStorage::addObject(Ref<CdsObject> obj)
{
    int objectType = obj->getObjectType();
    Ref<StringBuffer> fields(new StringBuffer(128));
    Ref<StringBuffer> values(new StringBuffer(128));

    *fields << "parent_id";
    *values << obj->getParentID();

    String refID = obj->getRefID();
    if (refID != nil)
    {
        *fields << ", ref_id";
        *values << ", " << refID;
    }
    
    *fields << ", object_type";
    *values << ", " << objectType;

    *fields << ", upnp_class";
    *values << ", " << quote(obj->getClass());

    *fields << ", dc_title";
    *values << ", " << quote(obj->getTitle());

    *fields << ", is_restricted";
    *values << ", " << obj->isRestricted();

    *fields << ", is_virtual";
    *values << ", " << obj->isVirtual();

    *fields << ", metadata";
    *values << ", " << quote(obj->getMetadata()->encode());

    *fields << ", auxdata";
    *values << ", " << quote(obj->getAuxData()->encode());

    // encode resources
    Ref<StringBuffer> buf(new StringBuffer());
    for (int i = 0; i < obj->getResourceCount(); i++)
    {
        if (i > 0)
            *buf << '&';
        *buf << url_escape(obj->getResource(i)->encode());
    }
    *fields << ", resources";
    *values << ", " << quote(buf->toString());

    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        *fields << ", update_id";
        *values << ", " << cont->getUpdateID();

        *fields << ", is_searchable";
        *values << ", " << cont->isSearchable();
    }
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        *fields << ", location";
        *values << ", " << quote(item->getLocation());

        *fields << ", mime_type";
        *values << ", " << quote(item->getMimeType());
    }
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        *fields << ", action";
        *values << ", " << quote(aitem->getAction());

        *fields << ", state";
        *values << ", " << quote(aitem->getState());
    }

    Ref<StringBuffer> qb(new StringBuffer(256));
    *qb << "INSERT INTO cds_objects(" << fields->toString() <<
            ") VALUES (" << values->toString() << ")";

//    log_info(("insert_query: %s\n", query->toString().c_str()));

    exec(qb->toString());

    obj->setID(String::from(lastInsertID()));
}

void SQLStorage::updateObject(zmm::Ref<CdsObject> obj)
{
    int objectType = obj->getObjectType();

    Ref<StringBuffer> qb(new StringBuffer(256));
    *qb << "UPDATE cds_objects SET id = id";

    *qb << ", upnp_class = " << quote(obj->getClass());
    *qb << ", dc_title = " << quote(obj->getTitle());
    *qb << ", is_restricted = " << obj->isRestricted();
    *qb << ", is_virtual = " << obj->isVirtual();
    *qb << ", metadata = " << quote(obj->getMetadata()->encode());
    *qb << ", auxdata = " << quote(obj->getAuxData()->encode());

    // encode resources
    Ref<StringBuffer> buf(new StringBuffer());
    for (int i = 0; i < obj->getResourceCount(); i++)
    {
        if (i > 0)
            *buf << '&';
        *buf << url_escape(obj->getResource(i)->encode());
    }
    *qb << ", resources = " << quote(buf->toString());

    if(IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        *qb << ", update_id = " << cont->getUpdateID();
        *qb << ", is_searchable = " << cont->isSearchable();
    }
    if(IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        *qb << ", location = " << quote(item->getLocation());
        *qb << ", mime_type = " << quote(item->getMimeType());
    }
    if(IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        *qb << ", action = " << quote(aitem->getAction());
        *qb << ", state = " << quote(aitem->getState());
    }

    *qb << " WHERE id = " << obj->getID();

//    log_info(("upd_query: %s\n", query->toString().c_str()));

    this->exec(qb->toString());
}

Ref<CdsObject> SQLStorage::loadObject(String objectID)
{
    Ref<StringBuffer> qb(new StringBuffer());

    *qb << select_query_base << " WHERE f.id = " << objectID;

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row);
    }
    throw Exception(String("Object not found: ") + objectID);
}

Ref<Array<CdsObject> > SQLStorage::browse(Ref<BrowseParam> param)
{
    String objectID;
    int objectType;

    objectID = param->getObjectID();

    /*
    char *endptr;
    (void)strtol(objectID.c_str(), &endptr, 10);
    if (*endptr != 0)
        throw Exception(String("Invalid object ID: ") + objectID);
    */

    String q;
    Ref<SQLResult> res;
    Ref<SQLRow> row;

    q = String("SELECT object_type FROM cds_objects WHERE id = ") + objectID;
    res = select(q);
    if((row = res->nextRow()) != nil)
    {
        objectType = atoi(row->col(0).c_str());
    }
    else
    {
        throw StorageException(String("Object not found: ") + objectID);
    }

    row = nil;
    res = nil;
    
    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        q = String("SELECT COUNT(*) FROM cds_objects WHERE parent_id = ") + objectID;
        res = select(q);
        if((row = res->nextRow()) != nil)
        {
            param->setTotalMatches(atoi(row->col(0).c_str()));
        }
    }
    else
    {
        param->setTotalMatches(1);
    }

    row = nil;
    res = nil;    
    
    Ref<StringBuffer> qb(new StringBuffer());

    *qb << select_query_base << " WHERE ";

    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        int count = param->getRequestedCount();
        if(! count)
            count = 1000000000;

        *qb << "f.parent_id = " << objectID;
        *qb << " ORDER BY f.object_type, f.dc_title";
        *qb << " LIMIT " << param->getStartingIndex() << "," << count;
    }
    else // metadata
    {
        *qb << "f.id = " << objectID;
    }
    res = select(qb->toString());

    Ref<Array<CdsObject> > arr(new Array<CdsObject>());

    while((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row);
        arr->append(obj);
        row = nil;
    }

    row = nil;
    res = nil;    
    
    // update childCount fields
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_CONTAINER(obj->getObjectType()))
        {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            qb->clear();
            *qb << "SELECT COUNT(*) FROM cds_objects WHERE parent_id = " << cont->getID();
            res = select(qb->toString());
            if ((row = res->nextRow()) != nil)
            {
                cont->setChildCount(atoi(row->col(0).c_str()));
            }
            else
            {
                cont->setChildCount(0);
            }
            row = nil;
            res = nil;
        }
    }

    return arr;
}
Ref<Array<StringBase> > SQLStorage::getMimeTypes()
{
    Ref<Array<StringBase> > arr(new Array<StringBase>());

    String q = String("SELECT DISTINCT mime_type FROM cds_objects ")
            + "WHERE mime_type IS NOT NULL ORDER BY mime_type";
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;

    while ((row = res->nextRow()) != nil)
    {
        arr->append(String(row->col(0)));
    }

    return arr;
}

Ref<CdsObject> SQLStorage::findObjectByTitle(String title, String parentID)
{
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << select_query_base << " WHERE ";
    if (parentID != nil)
        *qb << "parent_id = " << parentID << " AND ";
    *qb << "dc_title = " << quote(title);

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row);
        return obj;
    }
    return nil;
}

Ref<CdsObject> SQLStorage::createObjectFromRow(Ref<SQLRow> row)
{
    int objectType = atoi(row->col(_object_type).c_str());
    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(row->col(_id));
    obj->setParentID(row->col(_parent_id));
    obj->setRestricted(atoi(row->col(_is_restricted).c_str()));
    obj->setTitle(row->col(_dc_title));
    obj->setClass(row->col(_upnp_class));
    obj->setVirtual(atoi(row->col(_is_virtual).c_str()));
    
    Ref<Dictionary> meta(new Dictionary());
    meta->decode(row->col(_metadata));
    obj->setMetadata(meta);

    Ref<Dictionary> aux(new Dictionary());
    aux->decode(row->col(_auxdata));
    obj->setAuxData(aux);

    /* first try to fetch object's own resources, if it is empty
     * try to get them from the object pointed to by ref_id
     */
    String resources_str = row->col(_resources);
    if (resources_str == nil || resources_str.length() == 0)
    {
        resources_str = row->col(_ref_resources);
    }

    Ref<Array<StringBase> > resources = split_string(resources_str, '&');
    for (int i = 0; i < resources->size(); i++)
    {
        Ref<Dictionary> res(new Dictionary());
        res->decode(url_unescape(String(resources->get(i))));
        obj->addResource(res);
    }

    int matched_types = 0;

    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        cont->setSearchable(atoi(row->col(_is_searchable).c_str()));
        cont->setUpdateID(atoi(row->col(_update_id).c_str()));
        matched_types++;
    }

    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setMimeType(row->col(_mime_type));
        item->setLocation(row->col(_location));
        matched_types++;
    }

    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        aitem->setAction(row->col(_action));
        aitem->setState(row->col(_state));
        matched_types++;
    }

    if(! matched_types)
    {
        throw StorageException(String("unknown object type: ")+ objectType);
    }
    return obj;
}


static char *del_query = "DELETE FROM cds_objects WHERE id IN (";

void SQLStorage::removeObject(zmm::Ref<CdsObject> obj)
{
    Ref<StringBuffer> query(new StringBuffer());
    *query << del_query;
    if(IS_CDS_CONTAINER(obj->getObjectType()))
        removeChildren(obj->getID(), query);
    *query << obj->getID() << ")";
    exec(query->toString());
}

void SQLStorage::removeChildren(String id, Ref<StringBuffer> query)
{
    String q = String("SELECT id, object_type"
                      " FROM cds_objects WHERE parent_id = ") + id;
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;
    
    while ((row = res->nextRow()) != nil)
    {
        String childID = row->col(0);
        int childObjectType = row->col(1).toInt();
        row = nil;
        if (IS_CDS_CONTAINER(childObjectType))
            removeChildren(childID, query);
        *query << childID;
        
        if (query->length() > MAX_DELETE_QUERY_LENGTH)
        {
            *query << ")";
            exec(query->toString());
            query->clear();
            *query << del_query;
        }
        else
            *query << ',';
    }
}

void SQLStorage::eraseObject(Ref<CdsObject> object)
{
    throw Exception("SQLStorage::eraseObject shuold never be called !!!!\n");
}

int SQLStorage::getTotalFiles()
{
    Ref<StringBuffer> query(new StringBuffer());
    *query << "SELECT COUNT(*) FROM cds_objects WHERE "
           << "object_type <> " << OBJECT_TYPE_CONTAINER
           << " AND is_virtual = 0";
    Ref<SQLResult> res = select(query->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        return row->col(0).toInt();
    }
    return 0;    
}

