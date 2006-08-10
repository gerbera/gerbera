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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "sql_storage.h"
#include "tools.h"
#include "update_manager.h"

using namespace zmm;

/* maximal number of items to store in the hashtable */
#define MAX_REMOVE_IDS 1000
#define REMOVE_ID_HASH_CAPACITY 3109

#define OBJECT_CACHE_CAPACITY 100003

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

    rmIDs = NULL;
    rmParents = NULL;
}

void SQLStorage::init()
{
    objectTitleCache = Ref<DSOHash<CdsObject> >(new DSOHash<CdsObject>(OBJECT_CACHE_CAPACITY));
    objectIDCache = Ref<DBOHash<int, CdsObject> >(new DBOHash<int, CdsObject>(OBJECT_CACHE_CAPACITY, -100));
   
/*    
    Ref<SQLResult> res = select(_("SELECT MAX(id) + 1 FROM cds_objects"));
    Ref<SQLRow> row = res->nextRow();
    nextObjectID = row->col(0).toInt();
    
    log_debug(("PRELOADING OBJECTS...\n"));
    res = select(getSelectQuery(SELECT_FULL));
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());
    while((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row, SELECT_FULL);
        obj->optimize();
        objectTitleCache->put(String::from(obj->getParentID()) + '|' + obj->getTitle(), obj);
        objectIDCache->put(obj->getID(), obj);
    }
    log_debug(("PRELOADING OBJECTS DONE\n"));
*/
}

SQLStorage::~SQLStorage()
{
    if (rmIDs)
        FREE(rmIDs);
    if (rmParents)
        FREE(rmParents);
}

String SQLStorage::getSelectQuery(select_mode_t mode)
{
    switch(mode)
    {
        case SELECT_BASIC: return selectQueryBasic;
        case SELECT_EXTENDED: return selectQueryExtended;
        case SELECT_FULL: return selectQueryFull;
        default: throw StorageException(_("SQLStorage: invalid mode: ") + (int)mode);
    }
}

int SQLStorage::getNextObjectID()
{
    return nextObjectID++;
}
void SQLStorage::addObject(Ref<CdsObject> obj)
{
    /*
    obj->optimize();
    objectTitleCache->put(String::from(obj->getParentID()) +'|'+ obj->getTitle(), obj);
    objectIDCache->put(obj->getID(), obj);
    obj->setID(getNextObjectID());
    */
    
    int objectType = obj->getObjectType();
    Ref<StringBuffer> fields(new StringBuffer(128));
    Ref<StringBuffer> values(new StringBuffer(128));

    *fields << "parent_id";
    *values << obj->getParentID();

    int refID = obj->getRefID();
    if (refID != 0)
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

    obj->setID(lastInsertID());

}

void SQLStorage::updateObject(zmm::Ref<CdsObject> obj)
{
    log_debug(("UPDATE_OBJECT !!!!!!!!!!!!!!!\n"));
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

Ref<CdsObject> SQLStorage::loadObject(int objectID, select_mode_t mode)
{
/*
    Ref<CdsObject> obj = objectIDCache->get(objectID);
    if (obj != nil)
        return obj;
    throw Exception(_("Object not found: ") + objectID);
*/
        
    Ref<StringBuffer> qb(new StringBuffer());

    *qb << getSelectQuery(mode) << " WHERE f.id = " << objectID;

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row, mode);
    }
    throw Exception(_("Object not found: ") + objectID);
}

Ref<Array<CdsObject> > SQLStorage::browse(Ref<BrowseParam> param)
{
    int objectID;
    int objectType;

    objectID = param->getObjectID();

    String q;
    Ref<SQLResult> res;
    Ref<SQLRow> row;

    q = _("SELECT object_type FROM cds_objects WHERE id = ") +
                    objectID;
    res = select(q);
    if((row = res->nextRow()) != nil)
    {
        objectType = atoi(row->col(0).c_str());
    }
    else
    {
        throw StorageException(_("Object not found: ") + objectID);
    }

    row = nil;
    res = nil;
    
    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        q = _("SELECT COUNT(*) FROM cds_objects WHERE parent_id = ") + objectID;
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
            if (param->containersOnly)
                *qb << " AND object_type & " << OBJECT_TYPE_CONTAINER << " <> 0";
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

    String q = _("SELECT DISTINCT mime_type FROM cds_objects "
                "WHERE mime_type IS NOT NULL ORDER BY mime_type");
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;

    while ((row = res->nextRow()) != nil)
    {
        arr->append(String(row->col(0)));
    }

    return arr;
}

Ref<CdsObject> SQLStorage::findObjectByTitle(String title, int parentID)
{
    /*
    Ref<CdsObject> obj = objectTitleCache->get(String::from(parentID) +'|'+ title);
    if (obj != nil)
        return obj;
    return nil;
    */

    Ref<StringBuffer> qb(new StringBuffer());
    *qb << getSelectQuery(SELECT_FULL) << " WHERE ";
    if (parentID != INVALID_OBJECT_ID)
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
    obj->setID(row->col(_id).toInt());
    obj->setRefID(row->col(_ref_id).toInt());
    obj->setParentID(row->col(_parent_id).toInt());
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
        throw StorageException(_("unknown object type: ")+ objectType);
    }
    return obj;
}

void SQLStorage::eraseObject(Ref<CdsObject> object)
{
    throw Exception(_("SQLStorage::eraseObject shuold never be called !!!!\n"));
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

void SQLStorage::incrementUpdateIDs(int *ids, int size)
{
    Ref<StringBuffer> buf(new StringBuffer(size * 4));
    *buf << "UPDATE cds_objects SET update_id = update_id + 1 WHERE ID IN(-333";
    for (int i = 0; i < size; i++)
        *buf << ',' << ids[i];
    *buf << ')';
    exec(buf->toString());
}

Ref<Array<CdsObject> > SQLStorage::selectObjects(Ref<SelectParam> param,
                                                 select_mode_t mode)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << getSelectQuery(mode) << " WHERE ";
    switch (param->flags)
    {
        case FILTER_PARENT_ID:
            *q << "f.parent_id = " << param->iArg1;
            break;
        case FILTER_REF_ID:
            *q << "f.ref_id = " << param->iArg1;
            break;
        case FILTER_PARENT_ID_ITEMS:
            *q << "f.parent_id = " << param->iArg1 << " AND "
               << "f.object_type & " << OBJECT_TYPE_ITEM << " <> 0";
            break;
        case FILTER_PARENT_ID_CONTAINERS:
            *q << "f.parent_id = " << param->iArg1 << " AND "
               << "f.object_type & " << OBJECT_TYPE_CONTAINER << " <> 0";
            break;
        default:
            throw StorageException(_("selectObjects: invalid operation: ") +
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
        log_debug(("CLEANUP (EXCEPTION)\n"));
        rmCleanup();
        throw e;
    }
    log_debug(("CLEANUP (NORMAL)\n"));
    rmCleanup();
}


/* helpers for removeObject method */
void SQLStorage::rmInit()
{
    rmIDs = (int *)MALLOC(MAX_REMOVE_IDS * sizeof(int));        
    rmIDHash = Ref<DBHash<int> >(new DBHash<int>(REMOVE_ID_HASH_CAPACITY, 0));

    rmParents = (int *)MALLOC(MAX_REMOVE_IDS * sizeof(int));
    rmParentHash = Ref<DBBHash<int, int> >(new DBBHash<int, int>(REMOVE_ID_HASH_CAPACITY, -10));
}
void SQLStorage::rmCleanup()
{
    FREE(rmIDs);
    rmIDs = NULL;
    FREE(rmParents);
    rmParents = NULL;
    
    rmIDHash = nil;
    rmParentHash = nil;
}

void SQLStorage::rmDeleteIDs()
{
    int size = rmIDHash->size();
    Ref<StringBuffer> buf(new StringBuffer(size * 5));
    *buf << "DELETE FROM cds_objects WHERE id IN (-333";
    for (int i = 0; i < size; i++)
        *buf << ',' << rmIDs[i];
    *buf << ')';
    // log_debug(("Removing %s\n", buf->toString().c_str()));
   
    exec(buf->toString());
    rmIDHash->clear();
}
void SQLStorage::rmUpdateParents()
{
    Ref<UpdateManager> um = UpdateManager::getInstance();
    um->containersChanged(rmParents, rmParentHash->size());
    rmParentHash->clear();
}

/* flush scheduled items */
void SQLStorage::rmFlush()
{
    rmUpdateParents();
    rmDeleteIDs();
}

void SQLStorage::rmDecChildCount(Ref<CdsObject> obj)
{
    int id = obj->getID();
    int value;

    hash_slot_t hash_slot;
    bool exists = rmParentHash->get(id, &hash_slot, &value);
    if (! exists) // child count not yet known
    {
        String q = _("SELECT COUNT(*) FROM cds_objects WHERE parent_id=") + id;
        Ref<SQLResult> res = select(q);
        Ref<SQLRow> row = res->nextRow();
        value = row->col(0).toInt();
    }
    if (value > 0)
    {
        value--;
        rmParents[rmParentHash->size()] = id;
        if (exists)
            rmParentHash->put(id, hash_slot, value);
        else
            rmParentHash->put(id, value);
        if (value <= 0)
        {
            /// \todo unhardcode the id's below
            if (id != -1 && id != 0 && id != 1)
                rmObject(obj);
        }
    }
}


/* schedule an object for removal, will be called
 * for EVERY deleted object */
void SQLStorage::rmObject(Ref<CdsObject> obj)
{
//    log_debug(("rmObject %d, size; %d\n", obj->getID(), rmIDHash->size()));
    int id = obj->getID();
    hash_slot_t hash_slot;
    if (rmIDHash->exists(id, &hash_slot)) // do nothing if already scheduled
        return;

    // schedule for removal
    rmIDs[rmIDHash->size()] = id;
    rmIDHash->put(id, hash_slot);

    rmDecChildCount(loadObject(obj->getParentID()));

    if (rmIDHash->size() > MAX_REMOVE_IDS ||
        rmParentHash->size() > MAX_REMOVE_IDS)
        rmFlush();
}

/* item removal helper */
void SQLStorage::rmItem(Ref<CdsObject> obj)
{
    int origID; // id of the original object in PC dir
    Ref<CdsObject> orig;

    int refID = obj->getRefID();
    if (refID != 0) // this object is a referer (virtual)
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
    int objID = obj->getID();
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

