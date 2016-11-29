/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sql_storage.cc - this file is part of MediaTomb.
    
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

/// \file sql_storage.cc

#include <limits.h>
#include "sql_storage.h"
#include "tools.h"
#include "update_manager.h"
#include "string_converter.h"
#include "config_manager.h"
#include "filesystem.h"

using namespace zmm;

#define MAX_REMOVE_SIZE     10000
#define MAX_REMOVE_RECURSION 500

#define SQL_NULL             "NULL"

#define RESOURCE_SEP '|'

enum
{
    _id = 0,
    _ref_id,
    _parent_id,
    _object_type,
    _upnp_class,
    _dc_title,
    _location,
    _location_hash,
    _metadata,
    _auxdata,
    _resources,
    _update_id,
    _mime_type,
    _flags,
    _track_number,
    _service_id,
    _ref_upnp_class,
    _ref_location,
    _ref_metadata,
    _ref_auxdata,
    _ref_resources,
    _ref_mime_type,
    _ref_service_id,
    _as_persistent
};

/* table quote */
#define TQ(data)        QTB << data << QTE
/* table quote with dot */
#define TQD(data1, data2)        TQ(data1) << '.' << TQ(data2)

#define SEL_F_QUOTED        << TQ('f') <<
#define SEL_RF_QUOTED       << TQ("rf") <<

// end quote, space, f quoted, dot, begin quote
#define SEL_EQ_SP_FQ_DT_BQ  << QTE << ',' << TQ('f') << '.' << QTB <<
#define SEL_EQ_SP_RFQ_DT_BQ  << QTE << ',' << TQ("rf") << '.' << QTB <<

#define SELECT_DATA_FOR_STRINGBUFFER \
  TQ('f') << '.' << QTB << "id" \
    SEL_EQ_SP_FQ_DT_BQ "ref_id" \
    SEL_EQ_SP_FQ_DT_BQ "parent_id" \
    SEL_EQ_SP_FQ_DT_BQ "object_type" \
    SEL_EQ_SP_FQ_DT_BQ "upnp_class" \
    SEL_EQ_SP_FQ_DT_BQ "dc_title" \
    SEL_EQ_SP_FQ_DT_BQ "location" \
    SEL_EQ_SP_FQ_DT_BQ "location_hash" \
    SEL_EQ_SP_FQ_DT_BQ "metadata" \
    SEL_EQ_SP_FQ_DT_BQ "auxdata" \
    SEL_EQ_SP_FQ_DT_BQ "resources" \
    SEL_EQ_SP_FQ_DT_BQ "update_id" \
    SEL_EQ_SP_FQ_DT_BQ "mime_type" \
    SEL_EQ_SP_FQ_DT_BQ "flags" \
    SEL_EQ_SP_FQ_DT_BQ "track_number" \
    SEL_EQ_SP_FQ_DT_BQ "service_id" \
    SEL_EQ_SP_RFQ_DT_BQ "upnp_class" \
    SEL_EQ_SP_RFQ_DT_BQ "location" \
    SEL_EQ_SP_RFQ_DT_BQ "metadata" \
    SEL_EQ_SP_RFQ_DT_BQ "auxdata" \
    SEL_EQ_SP_RFQ_DT_BQ "resources" \
    SEL_EQ_SP_RFQ_DT_BQ "mime_type" \
    SEL_EQ_SP_RFQ_DT_BQ "service_id" << QTE \
    << ',' << TQD("as","persistent")
    
#define SQL_QUERY_FOR_STRINGBUFFER "SELECT " << SELECT_DATA_FOR_STRINGBUFFER << \
    " FROM " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('f') << " LEFT JOIN " \
    << TQ(CDS_OBJECT_TABLE) << ' ' << TQ("rf") << " ON " << TQD('f',"ref_id") \
    << '=' << TQD("rf","id") << " LEFT JOIN " << TQ(AUTOSCAN_TABLE) << ' ' \
    << TQ("as") << " ON " << TQD("as","obj_id") << '=' << TQD('f',"id") << ' '
    
#define SQL_QUERY       sql_query

/* enum for createObjectFromRow's mode parameter */

SQLStorage::SQLStorage() : Storage()
{
    table_quote_begin = '\0';
    table_quote_end = '\0';
    lastID = INVALID_OBJECT_ID;
}

void SQLStorage::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw _Exception(_("quote vars need to be overridden!"));
    
    Ref<StringBuffer> buf(new StringBuffer());
    *buf << SQL_QUERY_FOR_STRINGBUFFER;
    this->sql_query = buf->toString();
   
    if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_STORAGE_CACHING_ENABLED))
    {
        cache = Ref<StorageCache>(new StorageCache());
        insertBufferOn = true;
    }
    else
    {
        cache = nil;
        insertBufferOn = false;
    }
    
    insertBufferEmpty = true;
    insertBufferMutex = Ref<Mutex>(new Mutex());
    insertBufferStatementCount = 0;
    insertBufferByteCount = 0;
    
    //log_debug("using SQL: %s\n", this->sql_query.c_str());
    
    //objectTitleCache = Ref<DSOHash<CdsObject> >(new DSOHash<CdsObject>(OBJECT_CACHE_CAPACITY));
    //objectIDCache = Ref<DBOHash<int, CdsObject> >(new DBOHash<int, CdsObject>(OBJECT_CACHE_CAPACITY, -100));
   
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

void SQLStorage::dbReady()
{
    nextIDMutex = Ref<Mutex>(new Mutex());;
    loadLastID();
}

void SQLStorage::shutdown()
{
    flushInsertBuffer();
    shutdownDriver();
}

/*
SQLStorage::~SQLStorage()
{
}
*/

Ref<CdsObject> SQLStorage::checkRefID(Ref<CdsObject> obj)
{
    if (! obj->isVirtual()) throw _Exception(_("checkRefID called for a non-virtual object"));
    int refID = obj->getRefID();
    String location = obj->getLocation();
    if (! string_ok(location))
        throw _Exception(_("tried to check refID without a location set"));
    if (refID > 0)
    {
        try
        {
            Ref<CdsObject> refObj;
            refObj = loadObject(refID);
            if (refObj != nil && refObj->getLocation() == location)
                return refObj;
        }
        catch (Exception e)
        {
            // this should never happen - but fail softly if compiled without debugging
            assert(0);
            throw _Exception(_("illegal refID was set"));
        }
    }
    
    // this should never happen - but fail softly if compiled without debugging
    // we do this assert to find code while debugging, that doesn't set the
    // refID correctly
    assert(0);
    
    return findObjectByPath(location);
}

Ref<Array<SQLStorage::AddUpdateTable> > SQLStorage::_addUpdateObject(Ref<CdsObject> obj, bool isUpdate, int *changedContainer)
{
    int objectType = obj->getObjectType();
    Ref<CdsObject> refObj = nil;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef)
    {
        if (IS_CDS_PURE_ITEM(objectType))
            throw _Exception(_("tried to add pure item with PLAYLIST_REF flag set"));
        if (obj->getRefID() <= 0)
            throw _Exception(_("PLAYLIST_REF flag set but refId is <=0"));
        refObj = loadObject(obj->getRefID());
        if (refObj == nil)
            throw _Exception(_("PLAYLIST_REF flag set but refId doesn't point to an existing object"));
    }
    else if (obj->isVirtual() && IS_CDS_PURE_ITEM(objectType))
    {
        hasReference = true;
        refObj = checkRefID(obj);
        if (refObj == nil)
            throw _Exception(_("tried to add or update a virtual object with illegal reference id and an illegal location"));
    }
    else if (obj->getRefID() > 0)
    {
        if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
        {
            hasReference = true;
            refObj = loadObject(obj->getRefID());
            if (refObj == nil)
                throw _Exception(_("OBJECT_FLAG_ONLINE_SERVICE and refID set but refID doesn't point to an existing object"));
        }
        else if (IS_CDS_CONTAINER(objectType))
        {
            // in this case it's a playlist-container. that's ok
            // we don't need to do anything
        }
        else
            throw _Exception(_("refId set, but it makes no sense"));
    }
    
    Ref<Array<AddUpdateTable> > returnVal(new Array<AddUpdateTable>(2));
    Ref<Dictionary> cdsObjectSql(new Dictionary());
    returnVal->append(Ref<AddUpdateTable> (new AddUpdateTable(_(CDS_OBJECT_TABLE), cdsObjectSql)));
    
    cdsObjectSql->put(_("object_type"), quote(objectType));
    
    if (hasReference || playlistRef)
        cdsObjectSql->put(_("ref_id"), quote(refObj->getID()));
    else if (isUpdate)
        cdsObjectSql->put(_("ref_id"), _(SQL_NULL));
    
    if (! hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql->put(_("upnp_class"), quote(obj->getClass()));
    else if (isUpdate)
        cdsObjectSql->put(_("upnp_class"), _(SQL_NULL));
    
    //if (!hasReference || refObj->getTitle() != obj->getTitle())
    cdsObjectSql->put(_("dc_title"), quote(obj->getTitle()));
    //else if (isUpdate)
    //    cdsObjectSql->put(_("dc_title"), _(SQL_NULL));
    
    if (isUpdate)
        cdsObjectSql->put(_("metadata"), _(SQL_NULL));
    Ref<Dictionary> dict = obj->getMetadata();
    if (dict->size() > 0)
    {
        if (! hasReference || ! refObj->getMetadata()->equals(obj->getMetadata()))
        {
            cdsObjectSql->put(_("metadata"), quote(dict->encode()));
        }
    }
    
    if (isUpdate)
        cdsObjectSql->put(_("auxdata"), _(SQL_NULL));
    dict = obj->getAuxData();
    if (dict->size() > 0 && (! hasReference || ! refObj->getAuxData()->equals(obj->getAuxData())))
    {
        cdsObjectSql->put(_("auxdata"), quote(obj->getAuxData()->encode()));
    }
    
    if (! hasReference || (! obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF) && ! refObj->resourcesEqual(obj)))
    {
        // encode resources
        Ref<StringBuffer> resBuf(new StringBuffer());
        for (int i = 0; i < obj->getResourceCount(); i++)
        {
            if (i > 0)
                *resBuf << RESOURCE_SEP;
            *resBuf << obj->getResource(i)->encode();
        }
        String resStr = resBuf->toString();
        if (string_ok(resStr))
            cdsObjectSql->put(_("resources"), quote(resStr));
        else
            cdsObjectSql->put(_("resources"), _(SQL_NULL));
    }
    else if (isUpdate)
        cdsObjectSql->put(_("resources"), _(SQL_NULL));
    
    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    
    cdsObjectSql->put(_("flags"), quote(obj->getFlags()));
    
    if (IS_CDS_CONTAINER(objectType))
    {
        if (! (isUpdate && obj->isVirtual()) )
            throw _Exception(_("tried to add a container or tried to update a non-virtual container via _addUpdateObject; is this correct?"));
        String dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, obj->getLocation());
        cdsObjectSql->put(_("location"), quote(dbLocation));
        cdsObjectSql->put(_("location_hash"), quote(stringHash(dbLocation)));
    }
    
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        
        if (! hasReference)
        {
            String loc = item->getLocation();
            if (!string_ok(loc)) throw _Exception(_("tried to create or update a non-referenced item without a location set"));
            if (IS_CDS_PURE_ITEM(objectType))
            {
                Ref<Array<StringBase> > pathAr = split_path(loc);
                String path = pathAr->get(0);
                int parentID = ensurePathExistence(path, changedContainer);
                item->setParentID(parentID);
                String dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                cdsObjectSql->put(_("location"), quote(dbLocation));
                cdsObjectSql->put(_("location_hash"), quote(stringHash(dbLocation)));
            }
            else 
            {
                // URLs and active items
                cdsObjectSql->put(_("location"), quote(loc));
                cdsObjectSql->put(_("location_hash"), _(SQL_NULL));
            }
        }
        else 
        {
            if (isUpdate)
            {
                cdsObjectSql->put(_("location"), _(SQL_NULL));
                cdsObjectSql->put(_("location_hash"), _(SQL_NULL));
            }
        }
        
        if (item->getTrackNumber() > 0)
        {
            cdsObjectSql->put(_("track_number"), quote(item->getTrackNumber()));
        }
        else
        {
            if (isUpdate)
                cdsObjectSql->put(_("track_number"), _(SQL_NULL));
        }
        
        if (string_ok(item->getServiceID()))
        {
            if (! hasReference || RefCast(refObj,CdsItem)->getServiceID() != item->getServiceID())
                cdsObjectSql->put(_("service_id"), quote(item->getServiceID()));
            else
                cdsObjectSql->put(_("service_id"), _(SQL_NULL));
        }
        else
        {
            if (isUpdate)
                cdsObjectSql->put(_("service_id"), _(SQL_NULL));
        }
        
        cdsObjectSql->put(_("mime_type"), quote(item->getMimeType()));
    }
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<Dictionary> cdsActiveItemSql(new Dictionary());
        returnVal->append(Ref<AddUpdateTable> (new AddUpdateTable(_(CDS_ACTIVE_ITEM_TABLE), cdsActiveItemSql)));
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        
        cdsActiveItemSql->put(_("id"), String::from(aitem->getID()));
        cdsActiveItemSql->put(_("action"), quote(aitem->getAction()));
        cdsActiveItemSql->put(_("state"), quote(aitem->getState()));
    }
    
    
    // check for a duplicate (virtual) object
    if (hasReference && ! isUpdate)
    {
        Ref<StringBuffer> qb(new StringBuffer());
        *qb << "SELECT " << TQ("id") 
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("parent_id") 
            << '=' << quote(obj->getParentID())
            << " AND " << TQ("ref_id") 
            << '=' << quote(refObj->getID())
            << " AND " << TQ("dc_title")
            << '=' << quote(obj->getTitle())
            << " LIMIT 1";
        Ref<SQLResult> res = select(qb);
        // if duplicate items is found - ignore
        if (res != nil && (res->nextRow() != nil))
            return nil;
    }
    
    if (obj->getParentID() == INVALID_OBJECT_ID)
        throw _Exception(_("tried to create or update an object with an illegal parent id"));
    cdsObjectSql->put(_("parent_id"), String::from(obj->getParentID()));
    
    return returnVal;
}

void SQLStorage::addObject(Ref<CdsObject> obj, int *changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw _Exception(_("tried to add an object with an object ID set"));
    //obj->setID(INVALID_OBJECT_ID);
    Ref<Array<AddUpdateTable> > data = _addUpdateObject(obj, false, changedContainer);
    if (data == nil)
        return;
    int lastInsertID = INVALID_OBJECT_ID;
    for (int i = 0; i < data->size(); i++)
    {
        Ref<AddUpdateTable> addUpdateTable = data->get(i);
        String tableName = addUpdateTable->getTable();
        Ref<Array<DictionaryElement> > dataElements = addUpdateTable->getDict()->getElements();
        
        Ref<StringBuffer> fields(new StringBuffer(128));
        Ref<StringBuffer> values(new StringBuffer(128));
        
        for (int j = 0; j < dataElements->size(); j++)
        {
            Ref<DictionaryElement> element = dataElements->get(j);
            if (j != 0)
            {
                *fields << ',';
                *values << ',';
            }
            *fields << TQ(element->getKey());
            if (lastInsertID != INVALID_OBJECT_ID &&
                element->getKey() == "id" &&
                element->getValue().toInt() == INVALID_OBJECT_ID )
                *values << lastInsertID;
            else
            *values << element->getValue();
        }
        
        /* manually generate ID */
        if (lastInsertID == INVALID_OBJECT_ID && tableName == _(CDS_OBJECT_TABLE))
        {
            lastInsertID = getNextID();
            obj->setID(lastInsertID);
            *fields << ',' << TQ("id");
            *values << ',' << quote(lastInsertID);
        }
        /* -------------------- */
        
        Ref<StringBuffer> qb(new StringBuffer(256));
        *qb << "INSERT INTO " << TQ(tableName) << " (" << fields->toString() <<
                ") VALUES (" << values->toString() << ')';
                
        log_debug("insert_query: %s\n", qb->toString().c_str());
        
        /*
        if (lastInsertID == INVALID_OBJECT_ID && tableName == _(CDS_OBJECT_TABLE))
        {
            lastInsertID = exec(qb, true);
            obj->setID(lastInsertID);
        }
        else
            exec(qb);
        */
        
        if (! doInsertBuffering())
            exec(qb);
        else
            addToInsertBuffer(qb);
    }
    
    /* add to cache */
    if (cacheOn())
    {
        AUTOLOCK(cache->getMutex());
        cache->addChild(obj->getParentID());
        if (cache->flushed())
            flushInsertBuffer();
        addObjectToCache(obj, true);
    }
    /* ------------ */
}

void SQLStorage::updateObject(zmm::Ref<CdsObject> obj, int *changedContainer)
{
    flushInsertBuffer();
    
    Ref<Array<AddUpdateTable> > data;
    if (obj->getID() == CDS_ID_FS_ROOT)
    {
        data = Ref<Array<AddUpdateTable> >(new Array<AddUpdateTable>(1));
        Ref<Dictionary> cdsObjectSql(new Dictionary());
        data->append(Ref<AddUpdateTable> (new AddUpdateTable(_(CDS_OBJECT_TABLE), cdsObjectSql)));
        cdsObjectSql->put(_("dc_title"), quote(obj->getTitle()));
        setFsRootName(obj->getTitle());
        cdsObjectSql->put(_("upnp_class"), quote(obj->getClass()));
    }
    else
    {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw _Exception(_("tried to update an object with a forbidden ID (")+obj->getID()+")!");
        data = _addUpdateObject(obj, true, changedContainer);
        if (data == nil)
            return;
    }
    for (int i = 0; i < data->size(); i++)
    {
        Ref<AddUpdateTable> addUpdateTable = data->get(i);
        String tableName = addUpdateTable->getTable();
        Ref<Array<DictionaryElement> > dataElements = addUpdateTable->getDict()->getElements();
        
        Ref<StringBuffer> qb(new StringBuffer(256));
        *qb << "UPDATE " << TQ(tableName) << " SET ";
        
        for (int j = 0; j < dataElements->size(); j++)
        {
            Ref<DictionaryElement> element = dataElements->get(j);
            if (j != 0)
            {
                *qb << ',';
            }
            *qb << TQ(element->getKey()) << '='
                << element->getValue();
        }
        
        *qb << " WHERE id = " << obj->getID();
        
        log_debug("upd_query: %s\n", qb->toString().c_str());
        
        exec(qb);
    }
    /* add to cache */
    addObjectToCache(obj);
    /* ------------ */
}

Ref<CdsObject> SQLStorage::loadObject(int objectID)
{
    
    /* check cache */
    if (cacheOn())
    {
        AUTOLOCK(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(objectID);
        if (cObj != nil)
        {
            if (cObj->knowsObject())
                return cObj->getObject();
        }
    }
    /* ----------- */
    
/*
    Ref<CdsObject> obj = objectIDCache->get(objectID);
    if (obj != nil)
        return obj;
    throw _Exception(_("Object not found: ") + objectID);
*/
    Ref<StringBuffer> qb(new StringBuffer());
    
    //log_debug("sql_query = %s\n",sql_query.c_str());
    
    *qb << SQL_QUERY << " WHERE " << TQD('f',"id") << '=' << objectID;

    Ref<SQLResult> res = select(qb);
    Ref<SQLRow> row;
    if (res != nil && (row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row);
    }
    throw _ObjectNotFoundException(_("Object not found: ") + objectID);
}

Ref<CdsObject> SQLStorage::loadObjectByServiceID(String serviceID)
{
    flushInsertBuffer();
    
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << SQL_QUERY << " WHERE " << TQD('f',"service_id") << '=' << quote(serviceID);
    Ref<SQLResult> res = select(qb);
    Ref<SQLRow> row;
    if (res != nil && (row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row);
    }
    
    return nil;
}

Ref<IntArray> SQLStorage::getServiceObjectIDs(char servicePrefix)
{
    flushInsertBuffer();
    
    Ref<IntArray> objectIDs(new IntArray());
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT " << TQ("id")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("service_id")
        << " LIKE " << quote(String(servicePrefix)+'%');

    Ref<SQLResult> res = select(qb);
    if (res == nil)
        throw _Exception(_("db error"));
    
    Ref<SQLRow> row;
    while((row = res->nextRow()) != nil)
    {
        objectIDs->append(row->col(0).toInt());
    }
    
    return objectIDs;
}

Ref<Array<CdsObject> > SQLStorage::browse(Ref<BrowseParam> param)
{
    flushInsertBuffer();
    
    int objectID;
    int objectType = 0;
    
    bool getContainers = param->getFlag(BROWSE_CONTAINERS);
    bool getItems = param->getFlag(BROWSE_ITEMS);
    
    objectID = param->getObjectID();
    
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    
    bool haveObjectType = false;
    
    /* check cache */
    if (cacheOn())
    {
        AUTOLOCK(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(objectID);
        if (cObj != nil && cObj->knowsObjectType())
        {
            objectType = cObj->getObjectType();
            haveObjectType = true;
        }
    }
    /* ----------- */
    
    Ref<StringBuffer> qb(new StringBuffer());
    if (! haveObjectType)
    {
        *qb << "SELECT " << TQ("object_type")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << '=' << objectID;
        res = select(qb);
        if(res != nil && (row = res->nextRow()) != nil)
        {
            objectType = row->col(0).toInt();
            haveObjectType = true;
            
            /* add to cache */
            if (cacheOn())
            {
                AUTOLOCK(cache->getMutex());
                cache->getObjectDefinitely(objectID)->setObjectType(objectType);
                if (cache->flushed())
                    flushInsertBuffer();
            }
            /* ------------ */
        }
        else
        {
            throw _ObjectNotFoundException(_("Object not found: ") + objectID);
        }
        
        row = nil;
        res = nil;
    }
    
    
    bool hideFsRoot = param->getFlag(BROWSE_HIDE_FS_ROOT);
    
    if(param->getFlag(BROWSE_DIRECT_CHILDREN) && IS_CDS_CONTAINER(objectType))
    {
        param->setTotalMatches(getChildCount(objectID, getContainers, getItems, hideFsRoot));
    }
    else
    {
        param->setTotalMatches(1);
    }
    
    // order by code..
    qb->clear();
    if (param->getFlag(BROWSE_TRACK_SORT))
        *qb << TQD('f',"track_number") << ',';
    *qb << TQD('f',"dc_title");
    String orderByCode = qb->toString();
    
    qb->clear();
    *qb << SQL_QUERY << " WHERE ";
    
    if(param->getFlag(BROWSE_DIRECT_CHILDREN) && IS_CDS_CONTAINER(objectType))
    {
        int count = param->getRequestedCount();
        bool doLimit = true;
        if (! count)
        {
            if (param->getStartingIndex())
                count = INT_MAX;
            else
                doLimit = false;
        }
        
        *qb << TQD('f',"parent_id") << '=' << objectID;
        
        if (objectID == CDS_ID_ROOT && hideFsRoot)
            *qb << " AND " << TQD('f',"id") << "!="
                << quote (CDS_ID_FS_ROOT);
        
        if (! getContainers && ! getItems)
        {
            *qb << " AND 0=1";
        }
        else if (getContainers && ! getItems)
        {
            *qb << " AND " << TQD('f',"object_type") << '='
                << quote(OBJECT_TYPE_CONTAINER)
                << " ORDER BY " << orderByCode;
        }
        else if (! getContainers && getItems)
        {
            *qb << " AND (" << TQD('f',"object_type") << " & "
                << quote(OBJECT_TYPE_ITEM) << ") = " 
                << quote(OBJECT_TYPE_ITEM)
                << " ORDER BY " << orderByCode;
        }
        else
        {
            *qb << " ORDER BY ("
            << TQD('f',"object_type") << '=' << quote(OBJECT_TYPE_CONTAINER)
            << ") DESC, " << orderByCode;
        }
        if (doLimit)
            *qb << " LIMIT " << count << " OFFSET " << param->getStartingIndex();
    }
    else // metadata
    {
        *qb << TQD('f',"id") << '=' << objectID << " LIMIT 1";
    }
    log_debug("QUERY: %s\n", qb->toString().c_str());
    res = select(qb);
    
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
            cont->setChildCount(getChildCount(cont->getID(), getContainers, getItems, hideFsRoot));
        }
    }
    
    return arr;
}

int SQLStorage::getChildCount(int contId, bool containers, bool items, bool hideFsRoot)
{
    if (! containers && ! items)
        return 0;
    
    /* check cache */
    if (cacheOn() && containers && items && ! (contId == CDS_ID_ROOT && hideFsRoot))
    {
        AUTOLOCK(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(contId);
        if (cObj != nil)
        {
            if (cObj->knowsNumChildren())
                return cObj->getNumChildren();
            //cObj->debug();
        }
    }
    /* ----------- */
    
    flushInsertBuffer();
    
    Ref<SQLRow> row;
    Ref<SQLResult> res;
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("parent_id") << '=' << contId;
    if (containers && ! items)
        *qb << " AND " << TQ("object_type") << '=' << OBJECT_TYPE_CONTAINER;
    else if (items && ! containers)
        *qb << " AND (" << TQ("object_type") << " & " << OBJECT_TYPE_ITEM
            << ") = " << OBJECT_TYPE_ITEM;
    if (contId == CDS_ID_ROOT && hideFsRoot)
    {
        *qb << " AND " << TQ("id") << "!=" << quote (CDS_ID_FS_ROOT);
    }
    res = select(qb);
    if (res != nil && (row = res->nextRow()) != nil)
    {
        int childCount = row->col(0).toInt();
        
        /* add to cache */
        if (cacheOn() && containers && items && ! (contId == CDS_ID_ROOT && hideFsRoot))
        {
            AUTOLOCK(cache->getMutex());
            cache->getObjectDefinitely(contId)->setNumChildren(childCount);
            if (cache->flushed())
                flushInsertBuffer();
        }
        /* ------------ */
        
        return childCount;
    }
    return 0;
}

Ref<Array<StringBase> > SQLStorage::getMimeTypes()
{
    flushInsertBuffer();
    
    Ref<Array<StringBase> > arr(new Array<StringBase>());
    
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT DISTINCT " << TQ("mime_type")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("mime_type") << " IS NOT NULL ORDER BY "
        << TQ("mime_type");
    Ref<SQLResult> res = select(qb);
    if (res == nil)
        throw _Exception(_("db error"));
    
    Ref<SQLRow> row;
    
    while ((row = res->nextRow()) != nil)
    {
        arr->append(String(row->col(0)));
    }
    
    return arr;
}

Ref<CdsObject> SQLStorage::_findObjectByPath(String fullpath)
{
    //log_debug("fullpath: %s\n", fullpath.c_str());
    fullpath = fullpath.reduce(DIR_SEPARATOR);
    //log_debug("fullpath after reduce: %s\n", fullpath.c_str());
    Ref<Array<StringBase> > pathAr = split_path(fullpath);
    String path = pathAr->get(0);
    String filename = pathAr->get(1);
    
    bool file = string_ok(filename);
    
    String dbLocation;
    if (file)
    {
        //flushInsertBuffer(); - not needed if correctly in cache (see below)
        dbLocation = addLocationPrefix(LOC_FILE_PREFIX, fullpath);
    }
    else
        dbLocation = addLocationPrefix(LOC_DIR_PREFIX, path);
    
    
    /* check cache */
    if (cacheOn())
    {
        AUTOLOCK(cache->getMutex());
        Ref<Array<CacheObject> > objects = cache->getObjects(dbLocation);
        if (objects != nil)
        {
            for (int i = 0; i < objects->size(); i++)
            {
                Ref<CacheObject> cObj = objects->get(i);
                if (cObj->knowsObject() && cObj->knowsVirtual() && !cObj->getVirtual())
                    return cObj->getObject();
            }
        }
    }
    /* ----------- */
    
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << SQL_QUERY
            << " WHERE " << TQD('f',"location_hash") << '=' << quote(stringHash(dbLocation))
            << " AND " << TQD('f',"location") << '=' << quote(dbLocation)
            << " AND " << TQD('f',"ref_id") << " IS NULL "
            "LIMIT 1";
    
    Ref<SQLResult> res = select(qb);
    if (res == nil)
        throw _Exception(_("error while doing select: ") + qb->toString());
    
    
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    return createObjectFromRow(row);
}

Ref<CdsObject> SQLStorage::findObjectByPath(String fullpath)
{
    return _findObjectByPath(fullpath);
}

int SQLStorage::findObjectIDByPath(String fullpath)
{
    Ref<CdsObject> obj = _findObjectByPath(fullpath);
    if (obj == nil)
        return INVALID_OBJECT_ID;
    return obj->getID();
}

int SQLStorage::ensurePathExistence(String path, int *changedContainer)
{
    *changedContainer = INVALID_OBJECT_ID;
    String cleanPath = path.reduce(DIR_SEPARATOR);
    if (cleanPath == DIR_SEPARATOR)
        return CDS_ID_FS_ROOT;
    if (cleanPath.charAt(cleanPath.length() - 1) == DIR_SEPARATOR) // cut off trailing slash
        cleanPath = cleanPath.substring(0, cleanPath.length() - 1);
    return _ensurePathExistence(cleanPath, changedContainer);
}

int SQLStorage::_ensurePathExistence(String path, int *changedContainer)
{
    if (path == DIR_SEPARATOR)
        return CDS_ID_FS_ROOT;
    Ref<CdsObject> obj = findObjectByPath(path + DIR_SEPARATOR);
    if (obj != nil)
        return obj->getID();
    Ref<Array<StringBase> > pathAr = split_path(path);
    String parent = pathAr->get(0);
    String folder = pathAr->get(1);
    int parentID;
    parentID = ensurePathExistence(parent, changedContainer);
    
    Ref<StringConverter> f2i = StringConverter::f2i();
    if (changedContainer != NULL && *changedContainer == INVALID_OBJECT_ID)
        *changedContainer = parentID;
    return createContainer(parentID, f2i->convert(folder), path, false, nil, INVALID_OBJECT_ID);
}

int SQLStorage::createContainer(int parentID, String name, String path, bool isVirtual, String upnpClass, int refID)
{
    if (refID > 0)
    {
        Ref<CdsObject> refObj = loadObject(refID);
        if (refObj == nil)
            throw _Exception(_("tried to create container with refID set, but refID doesn't point to an existing object"));
    }
    String dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), path);
    
    int newID = getNextID();
    
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "INSERT INTO " 
        << TQ(CDS_OBJECT_TABLE)
        << " ("
        << TQ("id") << ','
        << TQ("parent_id") << ','
        << TQ("object_type") << ','
        << TQ("upnp_class") << ','
        << TQ("dc_title") << ','
        << TQ("location") << ','
        << TQ("location_hash") << ','
        << TQ("ref_id") << ") VALUES ("
        << newID << ','
        << parentID << ','
        << OBJECT_TYPE_CONTAINER << ','
        << (string_ok(upnpClass) ? quote(upnpClass) : quote(_(UPNP_DEFAULT_CLASS_CONTAINER))) << ',' 
        << quote(name) << ','
        << quote(dbLocation) << ','
        << quote(stringHash(dbLocation)) << ','
        << (refID > 0 ? quote(refID) : _(SQL_NULL))
        << ')';
        
    exec(qb);
    
    /* inform cache */
    if (cacheOn())
    {
        AUTOLOCK(cache->getMutex());
        cache->addChild(parentID);
        if (cache->flushed())
            flushInsertBuffer();
        Ref<CacheObject> cObj = cache->getObjectDefinitely(newID);
        if (cache->flushed())
            flushInsertBuffer();
        cObj->setParentID(parentID);
        cObj->setNumChildren(0);
        cObj->setObjectType(OBJECT_TYPE_CONTAINER);
        cObj->setLocation(path);
    }
    /* ------------ */
    
    return newID;
    
    //return exec(qb, true);
    
}

String SQLStorage::buildContainerPath(int parentID, String title)
{
    //title = escape(title, xxx);
    if (parentID == CDS_ID_ROOT)
        return String(VIRTUAL_CONTAINER_SEPARATOR) + title;
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT " << TQ("location") << " FROM " << TQ(CDS_OBJECT_TABLE) <<
        " WHERE " << TQ("id") << '=' << parentID << " LIMIT 1";
     Ref<SQLResult> res = select(qb);
    if (res == nil)
        return nil;
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    char prefix;
    String path = stripLocationPrefix(&prefix, row->col(0)) + VIRTUAL_CONTAINER_SEPARATOR + title;
    if (prefix != LOC_VIRT_PREFIX)
        throw _Exception(_("tried to build a virtual container path with an non-virtual parentID"));
    return path;
}

void SQLStorage::addContainerChain(String path, String lastClass, int lastRefID, int *containerID, int *updateID)
{
    path = path.reduce(VIRTUAL_CONTAINER_SEPARATOR);
    if (path == VIRTUAL_CONTAINER_SEPARATOR)
    {
        *containerID = CDS_ID_ROOT;
        return;
    }
    Ref<StringBuffer> qb(new StringBuffer());
    String dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, path);
    *qb << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("location_hash") << '=' << quote(stringHash(dbLocation))
            << " AND " << TQ("location") << '=' << quote(dbLocation)
            << " LIMIT 1";
    Ref<SQLResult> res = select(qb);
    if (res != nil)
    {
        Ref<SQLRow> row = res->nextRow();
        if (row != nil)
        {
            if (containerID != NULL)
                *containerID = row->col(0).toInt();
            return;
        }
    }
    int parentContainerID;
    String newpath, container;
    stripAndUnescapeVirtualContainerFromPath(path, newpath, container);
    addContainerChain(newpath, nil, INVALID_OBJECT_ID, &parentContainerID, updateID);
    if (updateID != NULL && *updateID == INVALID_OBJECT_ID)
        *updateID = parentContainerID;
    *containerID = createContainer(parentContainerID, container, path, true, lastClass, lastRefID);
}

String SQLStorage::addLocationPrefix(char prefix, String path)
{
    return String(prefix) + path;
}

String SQLStorage::stripLocationPrefix(char* prefix, String path)
{
    if (path == nil)
    {
        *prefix = LOC_ILLEGAL_PREFIX;
        return nil;
    }
    *prefix = path.charAt(0);
    return path.substring(1);
}

String SQLStorage::stripLocationPrefix(String path)
{
    if (path == nil)
        return nil;
    return path.substring(1);
}

Ref<CdsObject> SQLStorage::createObjectFromRow(Ref<SQLRow> row)
{
    int objectType = row->col(_object_type).toInt();
    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(row->col(_id).toInt());
    obj->setRefID(row->col(_ref_id).toInt());
    
    obj->setParentID(row->col(_parent_id).toInt());
    obj->setTitle(row->col(_dc_title));
    obj->setClass(fallbackString(row->col(_upnp_class), row->col(_ref_upnp_class)));
    obj->setFlags(row->col(_flags).toUInt());
    
    String metadataStr = fallbackString(row->col(_metadata), row->col(_ref_metadata));
    Ref<Dictionary> meta(new Dictionary());
    meta->decode(metadataStr);
    obj->setMetadata(meta);
    
    String auxdataStr = fallbackString(row->col(_auxdata), row->col(_ref_auxdata));
    Ref<Dictionary> aux(new Dictionary());
    aux->decode(auxdataStr);
    obj->setAuxData(aux);
    
    String resources_str = fallbackString(row->col(_resources), row->col(_ref_resources));
    bool resource_zero_ok = false;
    if (string_ok(resources_str))
    {
        Ref<Array<StringBase> > resources = split_string(resources_str,
                                                    RESOURCE_SEP);
        for (int i = 0; i < resources->size(); i++)
        {
            if (i == 0)
                resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources->get(i)));
        }
    }
    
    if ( (obj->getRefID() && IS_CDS_PURE_ITEM(objectType)) ||
        (IS_CDS_ITEM(objectType) && ! IS_CDS_PURE_ITEM(objectType)) )
        obj->setVirtual(true);
    else
        obj->setVirtual(false); // gets set to true for virtual containers below
    
    int matched_types = 0;
    
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        cont->setUpdateID(row->col(_update_id).toInt());
        char locationPrefix;
        cont->setLocation(stripLocationPrefix(&locationPrefix, row->col(_location)));
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);
        
        String autoscanPersistent = row->col(_as_persistent);
        if (string_ok(autoscanPersistent))
        {
            if (remapBool(autoscanPersistent))
                cont->setAutoscanType(OBJECT_AUTOSCAN_CFG);
            else
                cont->setAutoscanType(OBJECT_AUTOSCAN_UI);
        }
        else
            cont->setAutoscanType(OBJECT_AUTOSCAN_NONE);
        matched_types++;
    }
    
    if (IS_CDS_ITEM(objectType))
    {
        if (! resource_zero_ok)
            throw _Exception(_("tried to create object without at least one resource"));
        
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setMimeType(fallbackString(row->col(_mime_type), row->col(_ref_mime_type)));
        if (IS_CDS_PURE_ITEM(objectType))
        {
            if (! obj->isVirtual())
            item->setLocation(stripLocationPrefix(row->col(_location)));
                else
            item->setLocation(stripLocationPrefix(row->col(_ref_location)));
        }
        else // URLs and active items
        {
            item->setLocation(fallbackString(row->col(_location), row->col(_ref_location)));
        }
        
        item->setTrackNumber(row->col(_track_number).toInt());
        
        if (string_ok(row->col(_ref_service_id)))
            item->setServiceID(row->col(_ref_service_id));
        else
            item->setServiceID(row->col(_service_id));
        
        matched_types++;
    }
    
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        
        Ref<StringBuffer> query(new StringBuffer());
        *query << "SELECT " << TQ("id") << ',' << TQ("action") << ','
            << TQ("state") << " FROM " << TQ(CDS_ACTIVE_ITEM_TABLE)
            << " WHERE " << TQ("id") << '=' << quote(aitem->getID());
        Ref<SQLResult> resAI = select(query);
        Ref<SQLRow> rowAI;
        if (resAI != nil && (rowAI = resAI->nextRow()) != nil)
        {
            aitem->setAction(rowAI->col(1));
            aitem->setState(rowAI->col(2));
        }
        else
            throw _Exception(_("Active Item in cds_objects, but not in cds_active_item"));
        
        matched_types++;
    }

    if(! matched_types)
    {
        throw _StorageException(nil, _("unknown object type: ")+ objectType);
    }
    
    addObjectToCache(obj);
    return obj;
}

int SQLStorage::getTotalFiles()
{
    flushInsertBuffer();
    
    Ref<StringBuffer> query(new StringBuffer());
    *query << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE "
           << TQ("object_type") << " != " << quote(OBJECT_TYPE_CONTAINER);
           //<< " AND is_virtual = 0";
    Ref<SQLResult> res = select(query);
    Ref<SQLRow> row;
    if (res != nil && (row = res->nextRow()) != nil)
    {
        return row->col(0).toInt();
    }
    return 0;
}

String SQLStorage::incrementUpdateIDs(int *ids, int size)
{
    if (size <= 0)
        return nil;
    Ref<StringBuffer> inBuf(new StringBuffer()); // ??? what was that: size * sizeof(int)));
    *inBuf << "IN (" << ids[0];
    for (int i = 1; i < size; i++)
        *inBuf << ',' << ids[i];
    *inBuf << ')';
    
    Ref<StringBuffer> buf(new StringBuffer());
    *buf << "UPDATE " << TQ(CDS_OBJECT_TABLE) << " SET " << TQ("update_id") << '=' << TQ("update_id") << " + 1 WHERE " << TQ("id") << ' ';
    *buf << inBuf;
    exec(buf);
    
    buf->clear();
    *buf << "SELECT " << TQ("id") << ',' << TQ("update_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE " << TQ("id") << ' ';
    *buf << inBuf;
    Ref<SQLResult> res = select(buf);
    if (res == nil)
        throw _Exception(_("Error while fetching update ids"));
    Ref<SQLRow> row;
    buf->clear();
    while((row = res->nextRow()) != nil)
        *buf << ',' << row->col(0) << ',' << row->col(1);
    if (buf->length() <= 0)
        return nil;
    return buf->toString(1);
}

/*
Ref<Array<CdsObject> > SQLStorage::selectObjects(Ref<SelectParam> param)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << SQL_QUERY << " WHERE ";
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
            throw _StorageException(_("selectObjects: invalid operation: ") +
                                   param->flags);
    }
    *q << " ORDER BY f.object_type, f.dc_title";
    Ref<SQLResult> res = select(q->toString());
    Ref<SQLRow> row;
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());

    while((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row);
        arr->append(obj);
    }
    return arr;
}
*/

Ref<DBRHash<int> > SQLStorage::getObjects(int parentID, bool withoutContainer)
{
    flushInsertBuffer();
    
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    if (withoutContainer)
        *q << TQ("object_type") << " != " << OBJECT_TYPE_CONTAINER << " AND ";
    *q << TQ("parent_id") << '=';
    *q << parentID;
    Ref<SQLResult> res = select(q);
    if (res == nil)
        throw _Exception(_("db error"));
    Ref<SQLRow> row;
    
    if (res->getNumRows() <= 0)
        return nil;
    int capacity = res->getNumRows() * 5 + 1;
    if (capacity < 521)
        capacity = 521;
    
    Ref<DBRHash<int> > ret(new DBRHash<int>(capacity, res->getNumRows(), INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    
    while ((row = res->nextRow()) != nil)
    {
        ret->put(row->col(0).toInt());
    }
    return ret;
}

Ref<Storage::ChangedContainers> SQLStorage::removeObjects(zmm::Ref<DBRHash<int> > list, bool all)
{
    flushInsertBuffer();
    
    hash_data_array_t<int> hash_data_array;
    list->getAll(&hash_data_array);
    int count = hash_data_array.size;
    int *array = hash_data_array.data;
    if (count <= 0)
        return nil;
    
    Ref<StringBuffer> idsBuf(new StringBuffer());
    *idsBuf << "SELECT " << TQ("id") << ',' << TQ("object_type")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("id") << " IN (";
    int firstComma = idsBuf->length();
    for (int i = 0; i < count; i++)
    {
        int id = array[i];
        if (IS_FORBIDDEN_CDS_ID(id))
            throw _Exception(_("tried to delete a forbidden ID (") + id + ")!");
        *idsBuf << ',' << id;
    }
    idsBuf->setCharAt(firstComma, ' ');
    *idsBuf << ')';
    Ref<SQLResult> res = select(idsBuf);
    idsBuf = nil;
    if (res == nil)
        throw _Exception(_("sql error"));
    
    Ref<StringBuffer> items(new StringBuffer());
    Ref<StringBuffer> containers(new StringBuffer());
    Ref<SQLRow> row;
    while ((row = res->nextRow()) != nil)
    {
        int objectType = row->col(1).toInt();
        if (IS_CDS_CONTAINER(objectType))
            *containers << ',' << row->col_c_str(0);
        else
            *items << ',' << row->col_c_str(0);
    }
    return _purgeEmptyContainers(_recursiveRemove(items, containers, all));
}

void SQLStorage::_removeObjects(Ref<StringBuffer> objectIDs, int offset)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQD('a',"id") << ',' << TQD('a',"persistent")
        << ',' << TQD('o',"location")
        << " FROM " << TQ(AUTOSCAN_TABLE) << " a"
        " JOIN "  << TQ(CDS_OBJECT_TABLE) << " o"
        " ON " << TQD('o',"id") << '=' << TQD('a',"obj_id")
        << " WHERE " << TQD('o',"id") << " IN (";
    q->concat(objectIDs, offset);
    *q << ')';
    
    log_debug("%s\n", q->c_str());
    
    Ref<SQLResult> res = select(q);
    if (res != nil)
    {
        log_debug("relevant autoscans!\n");
        Ref<StringBuffer> delete_as(new StringBuffer());
        Ref<SQLRow> row;
        while((row = res->nextRow()) != nil)
        {
            bool persistent = remapBool(row->col(1));
            if (persistent)
            {
                String location = stripLocationPrefix(row->col(2));
                *q << "UPDATE " << TQ(AUTOSCAN_TABLE)
                    << " SET " << TQ("obj_id") << "=" SQL_NULL
                    << ',' << TQ("location") << '=' << quote(location)
                    << " WHERE " << TQ("id") << '=' << quote(row->col(0));
            }
            else
                *delete_as << ',' << row->col_c_str(0);
            log_debug("relevant autoscan: %d; persistent: %d\n", row->col_c_str(0), persistent);
        }
        
        if (delete_as->length() > 0)
        {
            q->clear();
            *q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
                << " WHERE " << TQ("id") << " IN (";
            q->concat(delete_as, 1);
            *q << ')';
            exec(q);
            log_debug("deleting autoscans: %s\n", delete_as->c_str());
        }
    }
    
    q->clear();
    *q << "DELETE FROM " << TQ(CDS_ACTIVE_ITEM_TABLE)
        << " WHERE " << TQ("id") << " IN (";
    q->concat(objectIDs, offset);
    *q << ')';
    exec(q);
    
    q->clear();
    *q << "DELETE FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("id") << " IN (";
    q->concat(objectIDs, offset);
    *q << ')';
    exec(q);
}

Ref<Storage::ChangedContainers> SQLStorage::removeObject(int objectID, bool all)
{
    flushInsertBuffer();
    
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ("object_type") << ',' << TQ("ref_id")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("id") << '=' << quote(objectID) << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nil)
        return nil;
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    
    int objectType = row->col(0).toInt();
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && ! isContainer)
    {
        String ref_id_str = row->col(1);
        int ref_id;
        if (string_ok(ref_id_str))
        {
            ref_id = ref_id_str.toInt();
            if (! IS_FORBIDDEN_CDS_ID(ref_id))
                objectID = ref_id;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw _Exception(_("tried to delete a forbidden ID (") + objectID + ")!");
    Ref<StringBuffer> idsBuf(new StringBuffer());
    *idsBuf << ',' << objectID;
    Ref<ChangedContainersStr> changedContainers = nil;
    if (isContainer)
        changedContainers = _recursiveRemove(nil, idsBuf, all);
    else
        changedContainers = _recursiveRemove(idsBuf, nil, all);
    return _purgeEmptyContainers(changedContainers);
}

Ref<SQLStorage::ChangedContainersStr> SQLStorage::_recursiveRemove(Ref<StringBuffer> items, Ref<StringBuffer> containers, bool all)
{
    log_debug("start\n");
    Ref<StringBuffer> recurseItems(new StringBuffer());
    *recurseItems << "SELECT DISTINCT " << TQ("id") << ',' << TQ("parent_id")
        << " FROM " << TQ(CDS_OBJECT_TABLE) <<
            " WHERE " << TQ("ref_id") << " IN (";
    int recurseItemsLen = recurseItems->length();
    
    Ref<StringBuffer> recurseContainers(new StringBuffer());
    *recurseContainers << "SELECT DISTINCT " << TQ("id") 
        << ',' << TQ("object_type");
    if (all)
        *recurseContainers << ',' << TQ("ref_id");
    *recurseContainers << " FROM " << TQ(CDS_OBJECT_TABLE) <<
            " WHERE " << TQ("parent_id") << " IN (";
    int recurseContainersLen = recurseContainers->length();
    
    Ref<StringBuffer> removeAddParents(new StringBuffer());
    *removeAddParents << "SELECT DISTINCT " << TQ("parent_id")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("id") << " IN (";
    int removeAddParentsLen = removeAddParents->length();
    
    Ref<StringBuffer> remove(new StringBuffer());
    Ref<ChangedContainersStr> changedContainers(new ChangedContainersStr());
    
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    
    if (items != nil && items->length() > 1)
    {
        *recurseItems << items;
        *removeAddParents << items;
    }
    
    if (containers != nil && containers->length() > 1)
    {
        *recurseContainers << containers;
        
        *remove << containers;
        *removeAddParents << containers;
        removeAddParents->setCharAt(removeAddParentsLen, ' ');
        *removeAddParents << ')';
        res = select(removeAddParents);
        if (res == nil)
            throw _StorageException(nil, _("sql error"));
        removeAddParents->setLength(removeAddParentsLen);
        while ((row = res->nextRow()) != nil)
            *changedContainers->ui << ',' << row->col_c_str(0);
    }
    
    int count = 0;
    while(recurseItems->length() > recurseItemsLen 
        || removeAddParents->length() > removeAddParentsLen
        || recurseContainers->length() > recurseContainersLen)
    {
        if (removeAddParents->length() > removeAddParentsLen)
        {
            // add ids to remove
            *remove << removeAddParents->c_str(removeAddParentsLen);
            // get rid of first ','
            removeAddParents->setCharAt(removeAddParentsLen, ' ');
            *removeAddParents << ')';
            res = select(removeAddParents);
            if (res == nil)
                throw _StorageException(nil, _("sql error"));
            // reset length
            removeAddParents->setLength(removeAddParentsLen);
            while ((row = res->nextRow()) != nil)
                *changedContainers->upnp << ',' << row->col_c_str(0);
        }
        
        if (recurseItems->length() > recurseItemsLen)
        {
            recurseItems->setCharAt(recurseItemsLen, ' ');
            *recurseItems << ')';
            res = select(recurseItems);
            if (res == nil)
                throw _StorageException(nil, _("sql error"));
            recurseItems->setLength(recurseItemsLen);
            while ((row = res->nextRow()) != nil)
            {
                *remove << ',' << row->col_c_str(0);
                *changedContainers->upnp << ',' << row->col_c_str(1);
                //log_debug("refs-add id: %s; parent_id: %s\n", id.c_str(), parentId.c_str());
            }
        }
        
        if (recurseContainers->length() > recurseContainersLen)
        {
            recurseContainers->setCharAt(recurseContainersLen, ' ');
            *recurseContainers << ')';
            res = select(recurseContainers);
            if (res == nil)
                throw _StorageException(nil, _("sql error"));
            recurseContainers->setLength(recurseContainersLen);
            while ((row = res->nextRow()) != nil)
            {
                //containers->append(row->col(1).toInt());
                
                int objectType = row->col(1).toInt();
                if (IS_CDS_CONTAINER(objectType))
                {
                    *recurseContainers << ',' << row->col_c_str(0);
                    *remove << ',' << row->col_c_str(0);
                }
                else
                {
                    if (all)
                    {
                        String refId = row->col(2);
                        if (string_ok(refId))
                        {
                            *removeAddParents << ',' << refId;
                            *recurseItems << ',' << refId;
                            //*remove << ',' << refId;
                        }
                        else
                        {
                            *remove << ',' << row->col_c_str(0);
                            *recurseItems << ',' << row->col_c_str(0);
                        }
                    }
                    else
                    {
                        *remove << ',' << row->col_c_str(0);
                        *recurseItems << ',' << row->col_c_str(0);
                    }
                }
                //log_debug("id: %s; parent_id: %s\n", id.c_str(), parentId.c_str());
            }
        }
        
        if (remove->length() > MAX_REMOVE_SIZE) // remove->length() > 0) // )
        {
            _removeObjects(remove, 1);
            remove->clear();
        }
        
        if (count++ > MAX_REMOVE_RECURSION)
            throw _Exception(_("there seems to be an infinite loop..."));
    }
    
    if (remove->length() > 0)
        _removeObjects(remove, 1);
    log_debug("end\n");
    return changedContainers;
}

Ref<Storage::ChangedContainers> SQLStorage::_purgeEmptyContainers(Ref<ChangedContainersStr> changedContainersStr)
{
    log_debug("start upnp: %s; ui: %s\n", changedContainersStr->upnp->c_str(), changedContainersStr->ui->c_str());
    Ref<ChangedContainers> changedContainers(new ChangedContainers());
    if (! string_ok(changedContainersStr->upnp) && ! string_ok(changedContainersStr->ui))
        return changedContainers;
    
    Ref<StringBuffer> bufSelUI(new StringBuffer());
    *bufSelUI << "SELECT " << TQD('a',"id")
        << ", COUNT(" << TQD('b',"parent_id") 
        << ")," << TQD('a',"parent_id") << ',' << TQD('a',"flags")
        << " FROM " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('a')
        << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('b')
        << " ON " << TQD('a',"id") << '=' << TQD('b',"parent_id") 
        << " WHERE " << TQD('a',"object_type") << '=' << quote(1)
        << " AND " << TQD('a',"id") << " IN (";  //(a.flags & " << OBJECT_FLAG_PERSISTENT_CONTAINER << ") = 0 AND
    int bufSelLen = bufSelUI->length();
    String strSel2 = _(") GROUP BY a.id"); // HAVING COUNT(b.parent_id)=0");
    
    Ref<StringBuffer> bufSelUpnp(new StringBuffer());
    *bufSelUpnp << bufSelUI;
    
    Ref<StringBuffer> bufDel(new StringBuffer());
    
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    
    *bufSelUI << changedContainersStr->ui;
    *bufSelUpnp << changedContainersStr->upnp;
    
    bool again;
    int count = 0;
    do
    {
        again = false;
        
        if (bufSelUpnp->length() > bufSelLen)
        {
            bufSelUpnp->setCharAt(bufSelLen, ' ');
            *bufSelUpnp << strSel2;
            log_debug("upnp-sql: %s\n", bufSelUpnp->c_str());
            res = select(bufSelUpnp);
            bufSelUpnp->setLength(bufSelLen);
            if (res == nil)
                throw _Exception(_("db error"));
            while ((row = res->nextRow()) != nil)
            {
                int flags = row->col(3).toInt();
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers->upnp->append(row->col(0).toInt());
                else if (row->col(1) == "0")
                {
                    *bufDel << ',' << row->col_c_str(0);
                    *bufSelUI << ',' << row->col_c_str(2);
                }
                else
                {
                    *bufSelUpnp << ',' << row->col_c_str(0);
                }
            }
        }
        
        if (bufSelUI->length() > bufSelLen)
        {
            bufSelUI->setCharAt(bufSelLen, ' ');
            *bufSelUI << strSel2;
            log_debug("ui-sql: %s\n", bufSelUI->c_str());
            res = select(bufSelUI);
            bufSelUI->setLength(bufSelLen);
            if (res == nil)
                throw _Exception(_("db error"));
            while ((row = res->nextRow()) != nil)
            {
                int flags = row->col(3).toInt();
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                {
                    changedContainers->ui->append(row->col(0).toInt());
                    changedContainers->upnp->append(row->col(0).toInt());
                }
                else if (row->col(1) == "0")
                {
                    *bufDel << ',' << row->col_c_str(0);
                    *bufSelUI << ',' << row->col_c_str(2);
                }
                else
                {
                    *bufSelUI << ',' << row->col_c_str(0);
                }
            }
        }
        
        //log_debug("selecting: %s; removing: %s\n", bufSel->c_str(), bufDel->c_str());
        if (bufDel->length() > 0)
        {
            _removeObjects(bufDel, 1);
            bufDel->clear();
            if (bufSelUI->length() > bufSelLen || bufSelUpnp->length() > bufSelLen)
                again = true;
        }
        if (count++ >= MAX_REMOVE_RECURSION)
            throw _Exception(_("there seems to be an infinite loop..."));
    }
    while (again);
    
    if (bufSelUI->length() > bufSelLen)
    {
        changedContainers->ui->addCSV(bufSelUI->toString(bufSelLen + 1));
        changedContainers->upnp->addCSV(bufSelUI->toString(bufSelLen + 1));
    }
    if (bufSelUpnp->length() > bufSelLen)
    {
        changedContainers->upnp->addCSV(bufSelUpnp->toString(bufSelLen + 1));
    }
    log_debug("end; changedContainers (upnp): %s\n", changedContainers->upnp->toCSV().c_str());
    log_debug("end; changedContainers (ui): %s\n", changedContainers->ui->toCSV().c_str());
    
    /* clear cache (for now) */
    if (cacheOn())
        cache->clear();
    /* --------------------- */
    
    return changedContainers;
}

String SQLStorage::getInternalSetting(String key)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ("value") << " FROM " << TQ(INTERNAL_SETTINGS_TABLE) << " WHERE " << TQ("key") << '='
        << quote(key) << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nil)
        return nil;
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    return row->col(0);
}
/*
void SQLStorage::storeInternalSetting(String key, String value)
overwritten due to different SQL syntax for MySQL and SQLite3
*/

void SQLStorage::updateAutoscanPersistentList(scan_mode_t scanmode, Ref<AutoscanList> list)
{
    
    log_debug("setting persistent autoscans untouched - scanmode: %s;\n", AutoscanDirectory::mapScanmode(scanmode).c_str());
    Ref<StringBuffer> q(new StringBuffer());
    *q << "UPDATE " << TQ(AUTOSCAN_TABLE)
        << " SET " << TQ("touched") << '=' << mapBool(false)
        << " WHERE "
        << TQ("persistent") << '=' << mapBool(true)
        << " AND " << TQ("scan_mode") << '='
        << quote(AutoscanDirectory::mapScanmode(scanmode));
    exec(q);
    
    int listSize = list->size();
    log_debug("updating/adding persistent autoscans (count: %d)\n", listSize);
    for (int i = 0; i < listSize; i++)
    {
        log_debug("getting ad %d from list..\n", i);
        Ref<AutoscanDirectory> ad = list->get(i);
        if (ad == nil)
            continue;
        
        // only persistent asD should be given to getAutoscanList
        assert(ad->persistent());
        // the scanmode should match the given parameter
        assert(ad->getScanMode() == scanmode);
        
        String location = ad->getLocation();
        if (! string_ok(location))
            throw _Exception(_("AutoscanDirectoy with illegal location given to SQLStorage::updateAutoscanPersistentList"));
        
        q->clear();
        *q << "SELECT " << TQ("id") << " FROM " << TQ(AUTOSCAN_TABLE)
            << " WHERE ";
        int objectID = findObjectIDByPath(location + '/');
        log_debug("objectID = %d\n", objectID);
        if (objectID == INVALID_OBJECT_ID)
            *q << TQ("location") << '=' << quote(location);
        else
            *q << TQ("obj_id") << '=' << quote(objectID);
        *q << " LIMIT 1";
        Ref<SQLResult> res = select(q);
        if (res == nil)
            throw _StorageException(nil, _("query error while selecting from autoscan list"));
        Ref<SQLRow> row;
        if ((row = res->nextRow()) != nil)
        {
            ad->setStorageID(row->col(0).toInt());
            updateAutoscanDirectory(ad);
        }
        else
            addAutoscanDirectory(ad);
    }
    
    q->clear();
    *q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("touched") << '=' << mapBool(false)
        << " AND " << TQ("scan_mode") << '='
        << quote(AutoscanDirectory::mapScanmode(scanmode));
    exec(q);
}

Ref<AutoscanList> SQLStorage::getAutoscanList(scan_mode_t scanmode)
{
    #define FLD(field) << TQD('a',field) <<
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ','
       FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ','
       FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ','
       FLD("location") ',' << TQD('t',"location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t',"id")
       << " WHERE " FLD("scan_mode") '=' << quote(AutoscanDirectory::mapScanmode(scanmode));
    Ref<SQLResult> res = select(q);
    if (res == nil)
        throw _StorageException(nil, _("query error while fetching autoscan list"));
    Ref<AutoscanList> ret(new AutoscanList());
    Ref<SQLRow> row;
    while((row = res->nextRow()) != nil)
    {
        Ref<AutoscanDirectory> dir = _fillAutoscanDirectory(row);
        if (dir == nil)
            removeAutoscanDirectory(row->col(0).toInt());
        else
            ret->add(dir);
    }
    return ret;
}

Ref<AutoscanDirectory> SQLStorage::getAutoscanDirectory(int objectID)
{
    #define FLD(field) << TQD('a',field) <<
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ','
       FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ','
       FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ','
       FLD("location") ',' << TQD('t',"location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t',"id")
       << " WHERE " << TQD('t',"id") << '=' << quote(objectID);
    Ref<SQLResult> res = select(q);
    if (res == nil)
        throw _StorageException(nil, _("query error while fetching autoscan"));
    Ref<AutoscanList> ret(new AutoscanList());
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    else
        return _fillAutoscanDirectory(row);
}

Ref<AutoscanDirectory> SQLStorage::_fillAutoscanDirectory(Ref<SQLRow> row)
{
    int objectID = INVALID_OBJECT_ID;
    String objectIDstr = row->col(1);
    if (string_ok(objectIDstr))
        objectID = objectIDstr.toInt();
    int storageID = row->col(0).toInt();
    
    String location;
    if (objectID == INVALID_OBJECT_ID)
        location = row->col(9);
    else
    {
        char prefix;
        location = stripLocationPrefix(&prefix, row->col(10));
        if (prefix != LOC_DIR_PREFIX)
            return nil;
    }
    
    scan_level_t level = AutoscanDirectory::remapScanlevel(row->col(2));
    scan_mode_t mode = AutoscanDirectory::remapScanmode(row->col(3));
    bool recursive = remapBool(row->col(4));
    bool hidden = remapBool(row->col(5));
    bool persistent = remapBool(row->col(8));
    int interval = 0;
    if (mode == TimedScanMode)
        interval = row->col(6).toInt();
    time_t last_modified = row->col(7).toLong();
    
    //log_debug("adding autoscan location: %s; recursive: %d\n", location.c_str(), recursive);
    
    Ref<AutoscanDirectory> dir(new AutoscanDirectory(location, mode, level, recursive, persistent, INVALID_SCAN_ID, interval, hidden));
    dir->setObjectID(objectID);
    dir->setStorageID(storageID);
    dir->setCurrentLMT(last_modified);
    dir->updateLMT();
    return dir;
}

void SQLStorage::addAutoscanDirectory(Ref<AutoscanDirectory> adir)
{
    if (adir == nil)
        throw _Exception(_("addAutoscanDirectory called with adir==nil"));
    if (adir->getStorageID() >= 0)
        throw _Exception(_("tried to add autoscan directory with a storage id set"));
    int objectID;
    if (adir->getLocation() == FS_ROOT_DIRECTORY)
        objectID = CDS_ID_FS_ROOT;
    else
        objectID = findObjectIDByPath(adir->getLocation() + DIR_SEPARATOR);
    if (! adir->persistent() && objectID < 0)
        throw _Exception(_("tried to add non-persistent autoscan directory with an illegal objectID or location"));
    
    Ref<IntArray> pathIds = _checkOverlappingAutoscans(adir);
    
    _autoscanChangePersistentFlag(objectID, true);
    
    Ref<StringBuffer> q(new StringBuffer());
    *q << "INSERT INTO " << TQ(AUTOSCAN_TABLE)
        << " (" << TQ("obj_id") << ','
        << TQ("scan_level") << ','
        << TQ("scan_mode") << ','
        << TQ("recursive") << ','
        << TQ("hidden") << ','
        << TQ("interval") << ','
        << TQ("last_modified") << ','
        << TQ("persistent") << ','
        << TQ("location") << ','
        << TQ("path_ids")
        << ") VALUES ("
        << (objectID >= 0 ? quote(objectID) : _(SQL_NULL)) << ','
        << quote(AutoscanDirectory::mapScanlevel(adir->getScanLevel())) << ','
        << quote(AutoscanDirectory::mapScanmode(adir->getScanMode())) << ','
        << mapBool(adir->getRecursive()) << ','
        << mapBool(adir->getHidden()) << ','
        << quote(adir->getInterval()) << ','
        << quote(adir->getPreviousLMT()) << ','
        << mapBool(adir->persistent()) << ','
        << (objectID >= 0 ? _(SQL_NULL) : quote(adir->getLocation())) << ','
        << (pathIds == nil ? _(SQL_NULL) : quote(_(",") + pathIds->toCSV() + ','))
        << ')';
    adir->setStorageID(exec(q, true));
}

void SQLStorage::updateAutoscanDirectory(Ref<AutoscanDirectory> adir)
{
    log_debug("id: %d, obj_id: %d\n", adir->getStorageID(), adir->getObjectID());
    
    if (adir == nil)
        throw _Exception(_("updateAutoscanDirectory called with adir==nil"));
    
    Ref<IntArray> pathIds = _checkOverlappingAutoscans(adir);
    
    int objectID = adir->getObjectID();
    int objectIDold = _getAutoscanObjectID(adir->getStorageID());
    if (objectIDold != objectID)
    {
        _autoscanChangePersistentFlag(objectIDold, false);
        _autoscanChangePersistentFlag(objectID, true);
    }
    Ref<StringBuffer> q(new StringBuffer());
    *q << "UPDATE " << TQ(AUTOSCAN_TABLE)
        << " SET " << TQ("obj_id") << '=' << (objectID >= 0 ? quote(objectID) : _(SQL_NULL))
        << ',' << TQ("scan_level") << '='
        << quote(AutoscanDirectory::mapScanlevel(adir->getScanLevel()))
        << ',' << TQ("scan_mode") << '='
        << quote(AutoscanDirectory::mapScanmode(adir->getScanMode()))
        << ',' << TQ("recursive") << '=' << mapBool(adir->getRecursive())
        << ',' << TQ("hidden") << '=' << mapBool(adir->getHidden())
        << ',' << TQ("interval") << '=' << quote(adir->getInterval());
    if (adir->getPreviousLMT() > 0)
        *q << ',' << TQ("last_modified") << '=' << quote(adir->getPreviousLMT());
    *q << ',' << TQ("persistent") << '=' << mapBool(adir->persistent())
        << ',' << TQ("location") << '=' << (objectID >= 0 ? _(SQL_NULL) : quote(adir->getLocation()))
        << ',' << TQ("path_ids") << '=' << (pathIds == nil ? _(SQL_NULL) : quote(_(",") + pathIds->toCSV() + ','))
        << ',' << TQ("touched") << '=' << mapBool(true)
        << " WHERE " << TQ("id") << '=' << quote(adir->getStorageID());
    exec(q);
}

void SQLStorage::removeAutoscanDirectoryByObjectID(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    Ref<StringBuffer> q(new StringBuffer());
    *q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("obj_id") << '=' << quote(objectID);
    exec(q);
    
    _autoscanChangePersistentFlag(objectID, false);
}

void SQLStorage::removeAutoscanDirectory(int autoscanID)
{
    if (autoscanID == INVALID_OBJECT_ID)
        return;
    int objectID = _getAutoscanObjectID(autoscanID);
    Ref<StringBuffer> q(new StringBuffer());
    *q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("id") << '=' << quote(autoscanID);
    exec(q);
    if (objectID != INVALID_OBJECT_ID)
        _autoscanChangePersistentFlag(objectID, false);
}

int SQLStorage::getAutoscanDirectoryType(int objectID)
{
    return _getAutoscanDirectoryInfo(objectID, _("persistent"));
}

int SQLStorage::isAutoscanDirectoryRecursive(int objectID)
{
    return _getAutoscanDirectoryInfo(objectID, _("recursive"));
}

int SQLStorage::_getAutoscanDirectoryInfo(int objectID, String field)
{
    if (objectID == INVALID_OBJECT_ID)
        return 0;
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ(field) << " FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("obj_id") << '=' << quote(objectID);
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;
    if (res == nil || (row = res->nextRow()) == nil)
        return 0;
    if (! remapBool(row->col(0)))
        return 1;
    else
        return 2;
}

int SQLStorage::_getAutoscanObjectID(int autoscanID)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ("obj_id") << " FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("id") << '=' << quote(autoscanID)
        << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nil)
        throw _StorageException(nil, _("error while doing select on "));
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil && string_ok(row->col(0)))
        return row->col(0).toInt();
    return INVALID_OBJECT_ID;
}

void SQLStorage::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID && objectID == INVALID_OBJECT_ID_2)
        return;
    Ref<StringBuffer> q(new StringBuffer());
    *q << "UPDATE " << TQ(CDS_OBJECT_TABLE)
        << " SET " << TQ("flags") << " = (" << TQ("flags")
        << (persistent ? _(" | ") : _(" & ~"))
        << OBJECT_FLAG_PERSISTENT_CONTAINER
        << ") WHERE " << TQ("id") << '=' << quote(objectID);
    exec(q);
}

void SQLStorage::autoscanUpdateLM(Ref<AutoscanDirectory> adir)
{
    /*
    int objectID = adir->getObjectID();
    if (IS_FORBIDDEN_CDS_ID(objectID))
    {
        objectID = findObjectIDByPath(adir->getLocation() + '/');
        if (IS_FORBIDDEN_CDS_ID(objectID))
            throw _Exception(_("autoscanUpdateLM called with adir with illegal objectID and location"));
    }
    */
    log_debug("id: %d; last_modified: %d\n", adir->getStorageID(), adir->getPreviousLMT());
    Ref<StringBuffer> q(new StringBuffer());
    *q << "UPDATE " << TQ(AUTOSCAN_TABLE)
        << " SET " << TQ("last_modified") << '=' << quote(adir->getPreviousLMT())
        << " WHERE " << TQ("id") << '=' << quote(adir->getStorageID());
    exec(q);
}

int SQLStorage::isAutoscanChild(int objectID)
{
    Ref<IntArray> pathIDs = getPathIDs(objectID);
    if (pathIDs == nil)
        return INVALID_OBJECT_ID;
    for (int i = 0; i < pathIDs->size(); i++)
    {
        int recursive = isAutoscanDirectoryRecursive(pathIDs->get(i));
        if (recursive == 2)
            return pathIDs->get(i);
    }
    return INVALID_OBJECT_ID;
}

void SQLStorage::checkOverlappingAutoscans(Ref<AutoscanDirectory> adir)
{
    _checkOverlappingAutoscans(adir);
}

Ref<IntArray> SQLStorage::_checkOverlappingAutoscans(Ref<AutoscanDirectory> adir)
{
    if (adir == nil)
        throw _Exception(_("_checkOverlappingAutoscans called with adir==nil"));
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID)
        return nil;
    int storageID = adir->getStorageID();
    
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    
    Ref<StringBuffer> q(new StringBuffer());
    
    *q << "SELECT " << TQ("id")
        << " FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("obj_id") << " = "
        << quote(checkObjectID);
    if (storageID >= 0)
        *q << " AND " << TQ("id") << " != " << quote(storageID);
    
    res = select(q);
    if (res == nil)
        throw _Exception(_("SQL error"));
    
    if ((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = loadObject(checkObjectID);
        if (obj == nil)
            throw _Exception(_("Referenced object (by Autoscan) not found."));
        log_error("There is already an Autoscan set on %s\n", obj->getLocation().c_str());
        throw _Exception(_("There is already an Autoscan set on ") + obj->getLocation());
    }
    
    if (adir->getRecursive())
    {
        q->clear();
        *q << "SELECT " << TQ("obj_id")
            << " FROM " << TQ(AUTOSCAN_TABLE)
            << " WHERE " << TQ("path_ids") << " LIKE "
            << quote(_("%,") + checkObjectID + ",%");
        if (storageID >= 0)
            *q << " AND " << TQ("id") << " != " << quote(storageID);
        *q << " LIMIT 1";
        
        log_debug("------------ %s\n", q->c_str());
        
        res = select(q);
        if (res == nil)
            throw _Exception(_("SQL error"));
        if ((row = res->nextRow()) != nil)
        {
            int objectID = row->col(0).toInt();
            log_debug("-------------- %d\n", objectID);
            Ref<CdsObject> obj = loadObject(objectID);
            if (obj == nil)
                throw _Exception(_("Referenced object (by Autoscan) not found."));
            log_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on %s\n", obj->getLocation().c_str());
            throw _Exception(_("Overlapping Autoscans are not allowed. There is already an Autoscan set on ") + obj->getLocation());
        }
    }
    
    Ref<IntArray> pathIDs = getPathIDs(checkObjectID);
    if (pathIDs == nil)
        throw _Exception(_("getPathIDs returned nil"));
    q->clear();
    *q << "SELECT " << TQ("obj_id")
        << " FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("obj_id") << " IN ("
        << pathIDs->toCSV()
        << ") AND " << TQ("recursive") << '=' << mapBool(true);
    if (storageID >= 0)
        *q << " AND " << TQ("id") << " != " << quote(storageID);
    *q << " LIMIT 1";
    
    res = select(q);
    if (res == nil)
        throw _Exception(_("SQL error"));
    if ((row = res->nextRow()) == nil)
        return pathIDs;
    else
    {
        int objectID = row->col(0).toInt();
        Ref<CdsObject> obj = loadObject(objectID);
        if (obj == nil)
            throw _Exception(_("Referenced object (by Autoscan) not found."));
        log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on %s\n", obj->getLocation().c_str());
        throw _Exception(_("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on ") + obj->getLocation());
    }
}

Ref<IntArray> SQLStorage::getPathIDs(int objectID)
{
    flushInsertBuffer();
    
    if (objectID == INVALID_OBJECT_ID)
        return nil;
    Ref<IntArray> pathIDs(new IntArray());
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    *q << TQ("id") << '=';
    int selBufLen = q->length();
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    while (objectID != CDS_ID_ROOT)
    {
        pathIDs->append(objectID);
        q->setLength(selBufLen);
        *q << quote(objectID) << " LIMIT 1";
        res = select(q);
        if (res == nil || (row = res->nextRow()) == nil)
            break;
        objectID = row->col(0).toInt();
    }
    return pathIDs;
}

String SQLStorage::getFsRootName()
{
    if (string_ok(fsRootName))
        return fsRootName;
    setFsRootName();
    return fsRootName;
}

void SQLStorage::setFsRootName(String rootName)
{
    if (string_ok(rootName))
    {
        fsRootName = rootName;
    }
    else
    {
        Ref<CdsObject> fsRootObj = loadObject(CDS_ID_FS_ROOT);
        fsRootName = fsRootObj->getTitle();
    }
}

int SQLStorage::getNextID()
{
    if (lastID < CDS_ID_FS_ROOT)
        throw _Exception(_("SQLStorage::getNextID() called, but lastID hasn't been loaded correctly yet"));
    AUTOLOCK(nextIDMutex);
    return ++lastID;
}

void SQLStorage::loadLastID()
{
    // we don't rely on automatic db generated ids, because of our caching
    
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT MAX(" << TQ("id") << ')'
        << " FROM " << TQ(CDS_OBJECT_TABLE);
    Ref<SQLResult> res = select(qb);
    if (res == nil)
        throw _Exception(_("could not load lastID (res==nil)"));
    
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        throw _Exception(_("could not load lastID (row==nil)"));
    
    lastID = row->col(0).toInt();
    if (lastID < CDS_ID_FS_ROOT)
        throw _Exception(_("could not load correct lastID (db not initialized?)"));
}

void SQLStorage::addObjectToCache(Ref<CdsObject> object, bool dontLock)
{
    if (cacheOn() && object != nil)
    {
        AUTOLOCK_DEFINE_ONLY();
        if (! dontLock)
            AUTOLOCK_NO_DEFINE(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObjectDefinitely(object->getID());
        if (cache->flushed())
            flushInsertBuffer();
        cObj->setObject(object);
        cache->checkLocation(cObj);
    }
}

void SQLStorage::addToInsertBuffer(Ref<StringBuffer> query)
{
    assert(doInsertBuffering());
    
    _addToInsertBuffer(query);
    
    AUTOLOCK(mutex);
    insertBufferEmpty = false;
    insertBufferStatementCount++;
    insertBufferByteCount += query->length();
    
    if (insertBufferByteCount > 102400)
        flushInsertBuffer(true);
}

void SQLStorage::flushInsertBuffer(bool dontLock)
{
    if (! doInsertBuffering())
        return;
    //print_backtrace();
    AUTOLOCK_DEFINE_ONLY();
    if (! dontLock)
        AUTOLOCK_NO_DEFINE(mutex);
    if (insertBufferEmpty)
        return;
    _flushInsertBuffer();
    log_debug("flushing insert buffer (%d statements)\n", insertBufferStatementCount);
    insertBufferEmpty = true;
    insertBufferStatementCount = 0;
    insertBufferByteCount = 0;
}

void SQLStorage::clearFlagInDB(int flag)
{
    Ref<StringBuffer> qb(new StringBuffer(256));
    *qb << "UPDATE "
        << TQ(CDS_OBJECT_TABLE)
        << " SET "
        << TQ("flags")
        << " = ("
        << TQ("flags")
        << "&~" << flag
        << ") WHERE "
        << TQ("flags")
        << "&" << flag;
    exec(qb);
}
