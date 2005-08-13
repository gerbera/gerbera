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
#include "tools.h"
#include "update_manager.h"

using namespace zmm;

/* maximal number of items to buffer before removing */
#define MAX_REMOVE_BUFFER_SIZE 1000

enum
{
    _id = 0,
    _ref_id,
    _parent_id,
    _object_type,
    _is_virtual,

    _upnp_class,
    _dc_title,
    _is_restricted,
    _metadata,
    _auxdata,

    _update_id,
    _is_searchable,

    _location,
    _mime_type,

    _action,
    _state,

    _resources,
    _ref_resources
};

static char *_q_base =
    "SELECT"
    "  f.id"
    ", f.ref_id"
    ", f.parent_id"
    ", f.object_type"
    ", f.is_virtual";

static char *_q_ext = 
    ", f.upnp_class"
    ", f.dc_title"
    ", f.is_restricted"
    ", f.metadata"
    ", f.auxdata"
 
    ", f.update_id"
    ", f.is_searchable"

    ", f.location"
    ", f.mime_type"

    ", f.action"
    ", f.state";
    
static char *_q_nores = 
    " FROM cds_objects f";
 
static char *_q_res =
    ", f.resources"

    ", rf.resources"
    " FROM cds_objects f LEFT JOIN cds_objects rf"
    " ON f.ref_id = rf.id";


SQLRow::SQLRow() : Object()
{}
SQLRow::~SQLRow()
{}


SQLResult::SQLResult() : Object()
{}
SQLResult::~SQLResult()
{}

/* enum for createObjectFromRow's mode parameter */

SQLStorage::SQLStorage() : Storage()
{
    selectQueryBasic = String(_q_base) + _q_nores;
    selectQueryExtended = String(_q_base) + _q_ext + _q_nores;
    selectQueryFull = String(_q_base) + _q_ext + _q_res;
}
SQLStorage::~SQLStorage()
{}

String SQLStorage::getSelectQuery(select_mode_t mode)
{
    switch(mode)
    {
        case SELECT_BASIC: return selectQueryBasic;
        case SELECT_EXTENDED: return selectQueryExtended;
        case SELECT_FULL: return selectQueryFull;
        default: throw StorageException(String("SQLStorage: invalid mode: ") + (int)mode);
    }
}

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
            *buf << RESOURCE_SEP;
        *buf << obj->getResource(i)->encode();
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
            *buf << RESOURCE_SEP;
        *buf << obj->getResource(i)->encode();
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

Ref<CdsObject> SQLStorage::loadObject(String objectID, select_mode_t mode)
{
    Ref<StringBuffer> qb(new StringBuffer());

    *qb << getSelectQuery(mode) << " WHERE f.id = " << objectID;

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row, mode);
    }
    throw Exception(String("Object not found: ") + objectID);
}

Ref<Array<CdsObject> > SQLStorage::browse(Ref<BrowseParam> param)
{
    String objectID;
    int objectType;

    objectID = param->getObjectID();

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

    *qb << getSelectQuery(SELECT_FULL) << " WHERE ";

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
    // log_debug(("QUERY: %s\n", qb->toString().c_str()));
    res = select(qb->toString());
    
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());

    while((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row, SELECT_FULL);
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
    *qb << getSelectQuery(SELECT_FULL) << " WHERE ";
    if (parentID != nil)
        *qb << "f.parent_id = " << parentID << " AND ";
    *qb << "f.dc_title = " << quote(title);

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row, SELECT_FULL);
        return obj;
    }
    return nil;
}

Ref<CdsObject> SQLStorage::createObjectFromRow(Ref<SQLRow> row, select_mode_t mode)
{
    int objectType = atoi(row->col(_object_type).c_str());
    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(row->col(_id));
    obj->setRefID(row->col(_ref_id));
    obj->setParentID(row->col(_parent_id));
    obj->setVirtual(atoi(row->col(_is_virtual).c_str()));

    if (mode > SELECT_BASIC)
    {
        obj->setRestricted(atoi(row->col(_is_restricted).c_str()));
    
        obj->setTitle(row->col(_dc_title));
        obj->setClass(row->col(_upnp_class));
    
        Ref<Dictionary> meta(new Dictionary());
        meta->decode(row->col(_metadata));
        obj->setMetadata(meta);

        Ref<Dictionary> aux(new Dictionary());
        aux->decode(row->col(_auxdata));
        obj->setAuxData(aux);
    }

    if (mode > SELECT_EXTENDED)
    {
        /* first try to fetch object's own resources, if it is empty
         * try to get them from the object pointed to by ref_id
         */
        String resources_str = row->col(_resources);
        if (resources_str == nil || resources_str.length() == 0)
        {
            resources_str = row->col(_ref_resources);
        }

        Ref<Array<StringBase> > resources = split_string(resources_str,
                                                     RESOURCE_SEP);
        for (int i = 0; i < resources->size(); i++)
        {
            obj->addResource(CdsResource::decode(resources->get(i)));
        }
    }

    int matched_types = 0;

    if (IS_CDS_CONTAINER(objectType))
    {
        if (mode > SELECT_BASIC)
        {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            cont->setSearchable(atoi(row->col(_is_searchable).c_str()));
            cont->setUpdateID(atoi(row->col(_update_id).c_str()));
        }
        matched_types++;
    }

    if (IS_CDS_ITEM(objectType))
    {
        if (mode > SELECT_BASIC)
        {
            Ref<CdsItem> item = RefCast(obj, CdsItem);
            item->setMimeType(row->col(_mime_type));
            item->setLocation(row->col(_location));
        }
        matched_types++;
    }

    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        if (mode > SELECT_BASIC)
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
            aitem->setAction(row->col(_action));
            aitem->setState(row->col(_state));
        }
        matched_types++;
    }

    if(! matched_types)
    {
        throw StorageException(String("unknown object type: ")+ objectType);
    }
    return obj;
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

Ref<Array<CdsObject> > SQLStorage::selectObjects(Ref<SelectParam> param,
                                                 select_mode_t mode)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << getSelectQuery(mode) << " WHERE ";
    switch (param->flags)
    {
        case FILTER_PARENT_ID:
            *q << "f.parent_id = " << param->arg1;
            break;
        case FILTER_REF_ID:
            *q << "f.ref_id = " << param->arg1;
            break;
        default:
            throw StorageException(String("selectObjects: invalid operation: ") +
                                   param->flags);
    }
    Ref<SQLResult> res = select(q->toString());
    Ref<SQLRow> row;
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());

    while((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row, mode);
        arr->append(obj);
    }
    return arr;
}

void SQLStorage::removeObject(Ref<CdsObject> obj)
{
    rmInit();
    try
    {
        if(IS_CDS_CONTAINER(obj->getObjectType()))
        {
            rmChildren(obj);
//            rmObject(obj);
        }
        else
            rmItem(obj);

        rmFlush();
    }
    catch (Exception e)
    {
        rmCleanup();
        throw e;
    }
    rmCleanup();
}


/* helpers for removeObject method */
void SQLStorage::rmInit()
{
    rmIDs = Ref<Dictionary>(new Dictionary());        
    rmParents = Ref<Dictionary>(new Dictionary());
    if (rmDummy == nil)
        rmDummy = String("");
}
void SQLStorage::rmCleanup()
{
    rmIDs = nil;
    rmParents = nil;
}

void SQLStorage::rmDeleteIDs()
{
    Ref<StringBuffer> buf(new StringBuffer(256));
    *buf << "DELETE FROM cds_objects WHERE id IN (-333";
    Ref<Array<DictionaryElement> > elements = rmIDs->getElements();
    int size = elements->size();
    for (int i = 0; i < size; i++)
        *buf << "," << elements->get(i)->getKey();
    *buf << ")";
    // log_debug(("Removing %s\n", buf->toString().c_str()));
    
    String q = buf->toString();
    exec(buf->toString());
    rmIDs->clear();
}
void SQLStorage::rmUpdateParents()
{
    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<Array<DictionaryElement> > elements = rmParents->getElements();
    for (int i = 0; i < elements->size(); i++)
        um->containerChanged(elements->get(i)->getKey());
    rmParents->clear();
}

/* flush scheduled items */
void SQLStorage::rmFlush()
{
    rmUpdateParents();
    rmDeleteIDs();
}

void SQLStorage::rmDecChildCount(Ref<CdsObject> obj)
{
    String id = obj->getID();
    Ref<StringBase> val = rmParents->get(id);
    if (val == nil) // child count not yet known
    {
        String q = String("SELECT COUNT(*) FROM cds_objects WHERE parent_id=") + id;
        Ref<SQLResult> res = select(q);
        Ref<SQLRow> row = res->nextRow();
        val = row->col(0);
    }
    int value = String(val).toInt();
    if (value > 0)
    {
        value--;
        val = String::from(value);
        rmParents->put(id, val);
        if (value <= 0)
        {
            int iid = id.toInt();
            /// \TODO unhardcode the id's below
            if (iid != -1 && iid != 0 && iid != 1)
                rmObject(obj);
        }
    }
}


/* schedule an object for removal, will be called
 * for EVERY deleted object */
void SQLStorage::rmObject(Ref<CdsObject> obj)
{
    String id = obj->getID();
    if (rmIDs->get(id) != nil) // do nothing if already scheduled
        return;

    rmIDs->put(id, rmDummy); // schedule for removal

    rmDecChildCount(loadObject(obj->getParentID()));

    if (rmIDs->size() > MAX_REMOVE_BUFFER_SIZE ||
        rmParents->size() > MAX_REMOVE_BUFFER_SIZE)
        rmFlush();
}

/* item removal helper */
void SQLStorage::rmItem(Ref<CdsObject> obj)
{
    String origID; // id of the original object in PC dir
    Ref<CdsObject> orig;

    String refID = obj->getRefID();
    if (string_ok(refID)) // this object is a referer (virtual)
    {
        origID = refID;
        orig = loadObject(origID, SELECT_BASIC);
    }
    else // this object is the original object itself
    {
        origID = obj->getID();
        orig = obj;
    }

    // schedule all referers for removal
    Ref<SelectParam> param(new SelectParam(FILTER_REF_ID, origID));
    Ref<Array<CdsObject> > referers = selectObjects(param, SELECT_BASIC);
    for (int i = 0; i < referers->size(); i++)
        rmObject(referers->get(i));

    // schedule original object for removal
    rmObject(orig);
}

// remvoe children of the given containers
void SQLStorage::rmChildren(Ref<CdsObject> obj)
{
    String objID = obj->getID();
    Ref<SelectParam> param(new SelectParam(FILTER_PARENT_ID, objID));
    Ref<Array<CdsObject> > children = selectObjects(param, SELECT_BASIC);
    for (int i = 0; i < children->size(); i++)
    {
        Ref<CdsObject> child = children->get(i);
        if(IS_CDS_CONTAINER(child->getObjectType()))
        {
            rmChildren(child);
//            rmObject(child);
        }
        else
            rmItem(child);
    }
}

/* ******** */

/* experimental stuff 
void removeEmptyContainers(String parentID)
{
    Ref<CdsObject> obj = loadObject(parentID);
    if (! IS_CDS_CONTAINER(obj))
       return;
    bool nonEmpty = removeEmptyChildren(obj);
    if (! nonEmpty)
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        if (obj->is_virtual())
        {
            /// \TODO: updateID
            query(String("DELETE FROM cds_objects WHERE id = ") + parentID);
        }
    }
}
bool removeEmptyChildren(Ref<CdsContainer> parent);
{
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN));
    arr = browse(param);
    bool notEmpty = false;
    int count;
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_CONTAINER(obj))
        {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            if (cont->is_virtual())
            {
                notEmpty = removeEmptyContainers(cont);
                if (notEmpty)
                    count++;
                else
                {
                    /// \TODO: updateID
                    query(String("DELETE FROM cds_objects WHERE id = ") + parent->getID());
                }
            }
        }
        else
            count++;
    }
    return (bool)count;
}
*/

