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

#include "sql_storage.h"
#include "config_manager.h"
#include "filesystem.h"
#include "string_converter.h"
#include "tools.h"
#include "update_manager.h"
#include "search_handler.h"
#include <climits>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

using namespace zmm;
using namespace std;

#define MAX_REMOVE_SIZE 1000
#define MAX_REMOVE_RECURSION 500

#define SQL_NULL "NULL"

#define RESOURCE_SEP '|'

enum {
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
#define TQ(data) QTB << data << QTE
/* table quote with dot */
#define TQD(data1, data2) TQ(data1) << '.' << TQ(data2)

#define SEL_F_QUOTED << TQ('f') <<
#define SEL_RF_QUOTED << TQ("rf") <<

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

enum SearchCol
{
    id = 0,
    ref_id,
    parent_id,
    object_type,
    upnp_class,
    dc_title,
    metadata,
    resources,
    mime_type,
    track_number,
    location
};

#define SELECT_DATA_FOR_SEARCH "SELECT distinct c.id, c.ref_id, c.parent_id," \
    << " c.object_type, c.upnp_class, c.dc_title, c.metadata,"       \
    << " c.resources, c.mime_type, c.track_number, c.location"

enum MetadataCol
{
    m_id = 0,
    m_item_id,
    m_property_name,
    m_property_value
};

#define SELECT_METADATA "SELECT id, item_id, property_name, property_value "

#define SQL_QUERY       sql_query
#define SQL_QUERY sql_query

/* enum for createObjectFromRow's mode parameter */


SQLStorage::SQLStorage()
    : Storage()
{
    table_quote_begin = '\0';
    table_quote_end = '\0';
    lastID = INVALID_OBJECT_ID;
    lastMetadataID = INVALID_OBJECT_ID;
}

void SQLStorage::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw _Exception(_("quote vars need to be overridden!"));

    std::ostringstream buf;
    buf << SQL_QUERY_FOR_STRINGBUFFER;
    this->sql_query = buf.str();

    if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_STORAGE_CACHING_ENABLED)) {
        cache = Ref<StorageCache>(new StorageCache());
        insertBufferOn = true;
    } else {
        cache = nullptr;
        insertBufferOn = false;
    }

    insertBufferEmpty = true;
    insertBufferStatementCount = 0;
    insertBufferByteCount = 0;

    sqlEmitter = std::make_shared<DefaultSQLEmitter>();

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
    while((row = res->nextRow()) != nullptr)
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
    loadLastID();
    loadLastMetadataID();
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
    if (!obj->isVirtual())
        throw _Exception(_("checkRefID called for a non-virtual object"));

    int refID = obj->getRefID();
    String location = obj->getLocation();

    if (!string_ok(location))
        throw _Exception(_("tried to check refID without a location set"));

    if (refID > 0) {
        try {
            Ref<CdsObject> refObj = loadObject(refID);
            if (refObj != nullptr && refObj->getLocation() == location)
                return refObj;
        } catch (const Exception& e) {
            throw _Exception(_("illegal refID was set"));
        }
    }

    // This should never happen - but fail softly
    // It means that something doesn't set the refID correctly
    log_warning("Failed to loadObject with refid: %d\n", refID);

    return findObjectByPath(location);
}

Ref<Array<SQLStorage::AddUpdateTable>> SQLStorage::_addUpdateObject(Ref<CdsObject> obj, bool isUpdate, int* changedContainer)
{
    int objectType = obj->getObjectType();
    Ref<CdsObject> refObj = nullptr;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef) {
        if (IS_CDS_PURE_ITEM(objectType))
            throw _Exception(_("tried to add pure item with PLAYLIST_REF flag set"));
        if (obj->getRefID() <= 0)
            throw _Exception(_("PLAYLIST_REF flag set but refId is <=0"));
        refObj = loadObject(obj->getRefID());
        if (refObj == nullptr)
            throw _Exception(_("PLAYLIST_REF flag set but refId doesn't point to an existing object"));
    } else if (obj->isVirtual() && IS_CDS_PURE_ITEM(objectType)) {
        hasReference = true;
        refObj = checkRefID(obj);
        if (refObj == nullptr)
            throw _Exception(_("tried to add or update a virtual object with illegal reference id and an illegal location"));
    } else if (obj->getRefID() > 0) {
        if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            hasReference = true;
            refObj = loadObject(obj->getRefID());
            if (refObj == nullptr)
                throw _Exception(_("OBJECT_FLAG_ONLINE_SERVICE and refID set but refID doesn't point to an existing object"));
        } else if (IS_CDS_CONTAINER(objectType)) {
            // in this case it's a playlist-container. that's ok
            // we don't need to do anything
        } else
            throw _Exception(_("refId set, but it makes no sense"));
    }

    int returnValSize = 2;
    returnValSize += (obj->getMetadata() == nullptr) ? 0 : obj->getMetadata()->size(); 
    Ref<Array<AddUpdateTable>> returnVal(new Array<AddUpdateTable>(returnValSize));
    Ref<Dictionary> cdsObjectSql(new Dictionary());
    returnVal->append(Ref<AddUpdateTable>(
        new AddUpdateTable(_(CDS_OBJECT_TABLE), cdsObjectSql, isUpdate ? "update" : "insert")));

    cdsObjectSql->put(_("object_type"), quote(objectType));

    if (hasReference || playlistRef)
        cdsObjectSql->put(_("ref_id"), quote(refObj->getID()));
    else if (isUpdate)
        cdsObjectSql->put(_("ref_id"), _(SQL_NULL));

    if (!hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql->put(_("upnp_class"), quote(obj->getClass()));
    else if (isUpdate)
        cdsObjectSql->put(_("upnp_class"), _(SQL_NULL));

    //if (!hasReference || refObj->getTitle() != obj->getTitle())
    cdsObjectSql->put(_("dc_title"), quote(obj->getTitle()));
    //else if (isUpdate)
    //    cdsObjectSql->put(_("dc_title"), _(SQL_NULL));


    if (!hasReference || !refObj->getMetadata()->equals(obj->getMetadata())) {
        generateMetadataDBOperations(obj, isUpdate, returnVal);
    }
    
    if (isUpdate)
        cdsObjectSql->put(_("auxdata"), _(SQL_NULL));
    Ref<Dictionary> dict = obj->getAuxData();
    if (dict->size() > 0 && (!hasReference || !refObj->getAuxData()->equals(obj->getAuxData()))) {
        cdsObjectSql->put(_("auxdata"), quote(obj->getAuxData()->encode()));
    }

    if (!hasReference || (!obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF) && !refObj->resourcesEqual(obj))) {
        // encode resources
        std::ostringstream resBuf;
        for (int i = 0; i < obj->getResourceCount(); i++) {
            if (i > 0)
                resBuf << RESOURCE_SEP;
            resBuf << obj->getResource(i)->encode();
        }
        String resStr = resBuf.str();
        if (string_ok(resStr))
            cdsObjectSql->put(_("resources"), quote(resStr));
        else
            cdsObjectSql->put(_("resources"), _(SQL_NULL));
    } else if (isUpdate)
        cdsObjectSql->put(_("resources"), _(SQL_NULL));

    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);

    cdsObjectSql->put(_("flags"), quote(obj->getFlags()));

    if (IS_CDS_CONTAINER(objectType)) {
        if (!(isUpdate && obj->isVirtual()))
            throw _Exception(_("tried to add a container or tried to update a non-virtual container via _addUpdateObject; is this correct?"));
        String dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, obj->getLocation());
        cdsObjectSql->put(_("location"), quote(dbLocation));
        cdsObjectSql->put(_("location_hash"), quote(stringHash(dbLocation)));
    }

    if (IS_CDS_ITEM(objectType)) {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        if (!hasReference) {
            String loc = item->getLocation();
            if (!string_ok(loc))
                throw _Exception(_("tried to create or update a non-referenced item without a location set"));
            if (IS_CDS_PURE_ITEM(objectType)) {
                Ref<Array<StringBase>> pathAr = split_path(loc);
                String path = pathAr->get(0);
                int parentID = ensurePathExistence(path, changedContainer);
                item->setParentID(parentID);
                String dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                cdsObjectSql->put(_("location"), quote(dbLocation));
                cdsObjectSql->put(_("location_hash"), quote(stringHash(dbLocation)));
            } else {
                // URLs and active items
                cdsObjectSql->put(_("location"), quote(loc));
                cdsObjectSql->put(_("location_hash"), _(SQL_NULL));
            }
        } else {
            if (isUpdate) {
                cdsObjectSql->put(_("location"), _(SQL_NULL));
                cdsObjectSql->put(_("location_hash"), _(SQL_NULL));
            }
        }

        if (item->getTrackNumber() > 0) {
            cdsObjectSql->put(_("track_number"), quote(item->getTrackNumber()));
        } else {
            if (isUpdate)
                cdsObjectSql->put(_("track_number"), _(SQL_NULL));
        }

        if (string_ok(item->getServiceID())) {
            if (!hasReference || RefCast(refObj, CdsItem)->getServiceID() != item->getServiceID())
                cdsObjectSql->put(_("service_id"), quote(item->getServiceID()));
            else
                cdsObjectSql->put(_("service_id"), _(SQL_NULL));
        } else {
            if (isUpdate)
                cdsObjectSql->put(_("service_id"), _(SQL_NULL));
        }

        cdsObjectSql->put(_("mime_type"), quote(item->getMimeType()));
    }
    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        Ref<Dictionary> cdsActiveItemSql(new Dictionary());
        returnVal->append(Ref<AddUpdateTable>(new AddUpdateTable(_(CDS_ACTIVE_ITEM_TABLE), cdsActiveItemSql,
                    isUpdate ? "update" : "insert")));
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        cdsActiveItemSql->put(_("id"), String::from(aitem->getID()));
        cdsActiveItemSql->put(_("action"), quote(aitem->getAction()));
        cdsActiveItemSql->put(_("state"), quote(aitem->getState()));
    }

    // check for a duplicate (virtual) object
    if (hasReference && !isUpdate) {
        std::ostringstream q;
        q << "SELECT " << TQ("id")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("parent_id")
            << '=' << quote(obj->getParentID())
            << " AND " << TQ("ref_id")
            << '=' << quote(refObj->getID())
            << " AND " << TQ("dc_title")
            << '=' << quote(obj->getTitle())
            << " LIMIT 1";
        Ref<SQLResult> res = select(q);
        // if duplicate items is found - ignore
        if (res != nullptr && (res->nextRow() != nullptr))
            return nullptr;
    }

    if (obj->getParentID() == INVALID_OBJECT_ID)
        throw _Exception(_("tried to create or update an object with an illegal parent id"));
    cdsObjectSql->put(_("parent_id"), String::from(obj->getParentID()));

    return returnVal;
}

void SQLStorage::addObject(Ref<CdsObject> obj, int* changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw _Exception(_("tried to add an object with an object ID set"));
    //obj->setID(INVALID_OBJECT_ID);
    Ref<Array<AddUpdateTable>> data = _addUpdateObject(obj, false, changedContainer);
    if (data == nullptr)
        return;
    // int lastInsertID = INVALID_OBJECT_ID;
    // int lastMetadataInsertID = INVALID_OBJECT_ID;
    for (int i = 0; i < data->size(); i++) {
        Ref<AddUpdateTable> addUpdateTable = data->get(i);
        std::shared_ptr<std::ostringstream> qb = sqlForInsert(obj, addUpdateTable);
        log_debug("insert_query: %s\n", qb->str().c_str());

        /*
        if (lastInsertID == INVALID_OBJECT_ID && tableName == _(CDS_OBJECT_TABLE))
        {
            lastInsertID = exec(qb, true);
            obj->setID(lastInsertID);
        }
        else
            exec(qb);
        */

        if (!doInsertBuffering())
            exec(*qb);
        else
            addToInsertBuffer(qb->str());
    }

    /* add to cache */
    if (cacheOn()) {
        AutoLock lock(cache->getMutex());
        cache->addChild(obj->getParentID());
        if (cache->flushed())
            flushInsertBuffer();
        addObjectToCache(obj, true);
    }
    /* ------------ */
}

void SQLStorage::updateObject(zmm::Ref<CdsObject> obj, int* changedContainer)
{
    flushInsertBuffer();

    Ref<Array<AddUpdateTable>> data;
    if (obj->getID() == CDS_ID_FS_ROOT) {
        data = Ref<Array<AddUpdateTable>>(new Array<AddUpdateTable>(1));
        Ref<Dictionary> cdsObjectSql(new Dictionary());
        data->append(Ref<AddUpdateTable>(new AddUpdateTable(_(CDS_OBJECT_TABLE), cdsObjectSql, "update")));
        cdsObjectSql->put(_("dc_title"), quote(obj->getTitle()));
        setFsRootName(obj->getTitle());
        cdsObjectSql->put(_("upnp_class"), quote(obj->getClass()));
    } else {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw _Exception(_("tried to update an object with a forbidden ID (") + obj->getID() + ")!");
        data = _addUpdateObject(obj, true, changedContainer);
        if (data == nullptr)
            return;
    }
    for (int i = 0; i < data->size(); i++) {
        Ref<AddUpdateTable> addUpdateTable = data->get(i);
        String operation = addUpdateTable->getOperation();
        std::shared_ptr<std::ostringstream> qb;
        if (operation == "update") {
            qb = sqlForUpdate(obj, addUpdateTable);
        } else if (operation == "insert") {
            qb = sqlForInsert(obj, addUpdateTable);
        } else if (operation == "delete") {
            qb = sqlForDelete(obj, addUpdateTable);
        }

        log_debug("upd_query: %s\n", qb->str().c_str());
        exec(*qb);
    }
    /* add to cache */
    addObjectToCache(obj);
    /* ------------ */
}

Ref<CdsObject> SQLStorage::loadObject(int objectID)
{

    /* check cache */
    if (cacheOn()) {
        AutoLock lock(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(objectID);
        if (cObj != nullptr) {
            if (cObj->knowsObject())
                return cObj->getObject();
        }
    }
    /* ----------- */

    /*
    Ref<CdsObject> obj = objectIDCache->get(objectID);
    if (obj != nullptr)
        return obj;
    throw _Exception(_("Object not found: ") + objectID);
*/
    std::ostringstream qb;

    //log_debug("sql_query = %s\n",sql_query.c_str());

    qb << SQL_QUERY << " WHERE " << TQD('f', "id") << '=' << objectID;

    Ref<SQLResult> res = select(qb);
    Ref<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return createObjectFromRow(row);
    }
    throw _ObjectNotFoundException(_("Object not found: ") + objectID);
}

Ref<CdsObject> SQLStorage::loadObjectByServiceID(String serviceID)
{
    flushInsertBuffer();

    std::ostringstream qb;
    qb << SQL_QUERY << " WHERE " << TQD('f', "service_id") << '=' << quote(serviceID);
    Ref<SQLResult> res = select(qb);
    Ref<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return createObjectFromRow(row);
    }

    return nullptr;
}

std::unique_ptr<std::vector<int>> SQLStorage::getServiceObjectIDs(char servicePrefix)
{
    flushInsertBuffer();

    auto objectIDs = std::make_unique<std::vector<int>>();

    std::ostringstream qb;
    qb << "SELECT " << TQ("id")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("service_id")
        << " LIKE " << quote(String(servicePrefix) + '%');

    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        throw _Exception(_("db error"));

    Ref<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        objectIDs->push_back(row->col(0).toInt());
    }

    return objectIDs;
}

Ref<Array<CdsObject>> SQLStorage::browse(Ref<BrowseParam> param)
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
    if (cacheOn()) {
        AutoLock lock(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(objectID);
        if (cObj != nullptr && cObj->knowsObjectType()) {
            objectType = cObj->getObjectType();
            haveObjectType = true;
        }
    }
    /* ----------- */

    if (!haveObjectType) {
        std::ostringstream qb;
        qb << "SELECT " << TQ("object_type")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << '=' << objectID;
        res = select(qb);
        if (res != nullptr && (row = res->nextRow()) != nullptr) {
            objectType = row->col(0).toInt();
            haveObjectType = true;

            /* add to cache */
            if (cacheOn()) {
                AutoLock lock(cache->getMutex());
                cache->getObjectDefinitely(objectID)->setObjectType(objectType);
                if (cache->flushed())
                    flushInsertBuffer();
            }
            /* ------------ */
        } else {
            throw _ObjectNotFoundException(_("Object not found: ") + objectID);
        }

        row = nullptr;
        res = nullptr;
    }

    bool hideFsRoot = param->getFlag(BROWSE_HIDE_FS_ROOT);

    if (param->getFlag(BROWSE_DIRECT_CHILDREN) && IS_CDS_CONTAINER(objectType)) {
        param->setTotalMatches(getChildCount(objectID, getContainers, getItems, hideFsRoot));
    } else {
        param->setTotalMatches(1);
    }

    // order by code..
    auto orderByCode = [&]() {
        std::ostringstream qb;
        if (param->getFlag(BROWSE_TRACK_SORT))
            qb << TQD('f', "track_number") << ',';
        qb << TQD('f', "dc_title");
        return qb.str();
    };

    std::ostringstream qb;
    qb << SQL_QUERY << " WHERE ";

    if (param->getFlag(BROWSE_DIRECT_CHILDREN) && IS_CDS_CONTAINER(objectType)) {
        int count = param->getRequestedCount();
        bool doLimit = true;
        if (!count) {
            if (param->getStartingIndex())
                count = INT_MAX;
            else
                doLimit = false;
        }

        qb << TQD('f', "parent_id") << '=' << objectID;

        if (objectID == CDS_ID_ROOT && hideFsRoot)
            qb << " AND " << TQD('f', "id") << "!="
                << quote(CDS_ID_FS_ROOT);

        if (!getContainers && !getItems) {
            qb << " AND 0=1";
        } else if (getContainers && !getItems) {
            qb << " AND " << TQD('f', "object_type") << '='
                << quote(OBJECT_TYPE_CONTAINER)
                << " ORDER BY " << orderByCode();
        } else if (!getContainers && getItems) {
            qb << " AND (" << TQD('f', "object_type") << " & "
                << quote(OBJECT_TYPE_ITEM) << ") = "
                << quote(OBJECT_TYPE_ITEM)
                << " ORDER BY " << orderByCode();
        } else {
            qb << " ORDER BY ("
                << TQD('f', "object_type") << '=' << quote(OBJECT_TYPE_CONTAINER)
                << ") DESC, " << orderByCode();
        }
        if (doLimit)
            qb << " LIMIT " << count << " OFFSET " << param->getStartingIndex();
    } else // metadata
    {
        qb << TQD('f', "id") << '=' << objectID << " LIMIT 1";
    }
    log_debug("QUERY: %s\n", qb.str().c_str());
    res = select(qb);

    Ref<Array<CdsObject>> arr(new Array<CdsObject>());

    while ((row = res->nextRow()) != nullptr) {
        Ref<CdsObject> obj = createObjectFromRow(row);
        arr->append(obj);
        row = nullptr;
    }

    row = nullptr;
    res = nullptr;

    // update childCount fields
    for (int i = 0; i < arr->size(); i++) {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_CONTAINER(obj->getObjectType())) {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            cont->setChildCount(getChildCount(cont->getID(), getContainers, getItems, hideFsRoot));
        }
    }

    return arr;
}

zmm::Ref<zmm::Array<CdsObject>> SQLStorage::search(zmm::Ref<SearchParam> param, int* numMatches)
{
    std::unique_ptr<SearchParser> searchParser = std::make_unique<SearchParser>(*sqlEmitter, param->searchCriteria());
    std::shared_ptr<ASTNode> rootNode = searchParser->parse();
    std::shared_ptr<std::string> searchSQL(rootNode->emitSQL());
    if (!searchSQL->length())
        throw _Exception(_("failed to generate SQL for search"));

    std::stringstream countSQL;
    countSQL << "select count(*) " << *searchSQL << ';';
    zmm::Ref<SQLResult> sqlResult;
    zmm::Ref<zmm::StringBuffer> buf = Ref<zmm::StringBuffer>(new zmm::StringBuffer(countSQL.str().length()));
    *buf << countSQL.str().c_str();
    sqlResult = select(buf);
    zmm::Ref<SQLRow> countRow = sqlResult->nextRow();
    if (countRow != nullptr) {
        *numMatches = countRow->col(0).toInt();
    }
    
    std::stringstream retrievalSQL;
    retrievalSQL << SELECT_DATA_FOR_SEARCH << " " << *searchSQL;
    int startingIndex = param->getStartingIndex(), requestedCount = param->getRequestedCount();
    if (startingIndex > 0 || requestedCount > 0) {
        retrievalSQL << " order by c.id"
                     << " limit " << (requestedCount == 0 ? 10000000000 : requestedCount)
                     << " offset " << startingIndex;
    }
    retrievalSQL << ';';

    log_debug("Search resolves to SQL [%s]\n", retrievalSQL.str().c_str());
    buf = Ref<zmm::StringBuffer>(new zmm::StringBuffer(retrievalSQL.str().length()));
    *buf << retrievalSQL.str().c_str();
    sqlResult = select(buf);

    zmm::Ref<zmm::Array<CdsObject>> arr(new Array<CdsObject>()); 
    zmm::Ref<SQLRow> sqlRow;
    while ((sqlRow = sqlResult->nextRow()) != nullptr) {
        Ref<CdsObject> obj = createObjectFromSearchRow(sqlRow);
        arr->append(obj);
        sqlRow = nullptr;
    }
    sqlRow = nullptr;
    sqlResult = nullptr;

    return arr;
}

int SQLStorage::getChildCount(int contId, bool containers, bool items, bool hideFsRoot)
{
    if (!containers && !items)
        return 0;

    /* check cache */
    if (cacheOn() && containers && items && !(contId == CDS_ID_ROOT && hideFsRoot)) {
        AutoLock lock(cache->getMutex());
        Ref<CacheObject> cObj = cache->getObject(contId);
        if (cObj != nullptr) {
            if (cObj->knowsNumChildren())
                return cObj->getNumChildren();
            //cObj->debug();
        }
    }
    /* ----------- */

    flushInsertBuffer();

    Ref<SQLRow> row;
    Ref<SQLResult> res;
    std::ostringstream qb;
    qb << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("parent_id") << '=' << contId;
    if (containers && !items)
        qb << " AND " << TQ("object_type") << '=' << OBJECT_TYPE_CONTAINER;
    else if (items && !containers)
        qb << " AND (" << TQ("object_type") << " & " << OBJECT_TYPE_ITEM
            << ") = " << OBJECT_TYPE_ITEM;
    if (contId == CDS_ID_ROOT && hideFsRoot) {
        qb << " AND " << TQ("id") << "!=" << quote(CDS_ID_FS_ROOT);
    }
    res = select(qb);
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        int childCount = row->col(0).toInt();

        /* add to cache */
        if (cacheOn() && containers && items && !(contId == CDS_ID_ROOT && hideFsRoot)) {
            AutoLock lock(cache->getMutex());
            cache->getObjectDefinitely(contId)->setNumChildren(childCount);
            if (cache->flushed())
                flushInsertBuffer();
        }
        /* ------------ */

        return childCount;
    }
    return 0;
}

Ref<Array<StringBase>> SQLStorage::getMimeTypes()
{
    flushInsertBuffer();

    Ref<Array<StringBase>> arr(new Array<StringBase>());

    std::ostringstream qb;
    qb << "SELECT DISTINCT " << TQ("mime_type")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("mime_type") << " IS NOT NULL ORDER BY "
        << TQ("mime_type");
    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        throw _Exception(_("db error"));

    Ref<SQLRow> row;

    while ((row = res->nextRow()) != nullptr) {
        arr->append(String(row->col(0)));
    }

    return arr;
}

Ref<CdsObject> SQLStorage::_findObjectByPath(String fullpath)
{
    //log_debug("fullpath: %s\n", fullpath.c_str());
    fullpath = fullpath.reduce(DIR_SEPARATOR);
    //log_debug("fullpath after reduce: %s\n", fullpath.c_str());
    Ref<Array<StringBase>> pathAr = split_path(fullpath);
    String path = pathAr->get(0);
    String filename = pathAr->get(1);

    bool file = string_ok(filename);

    String dbLocation;
    if (file) {
        //flushInsertBuffer(); - not needed if correctly in cache (see below)
        dbLocation = addLocationPrefix(LOC_FILE_PREFIX, fullpath);
    } else
        dbLocation = addLocationPrefix(LOC_DIR_PREFIX, path);

    /* check cache */
    if (cacheOn()) {
        AutoLock lock(cache->getMutex());
        Ref<Array<CacheObject>> objects = cache->getObjects(dbLocation);
        if (objects != nullptr) {
            for (int i = 0; i < objects->size(); i++) {
                Ref<CacheObject> cObj = objects->get(i);
                if (cObj->knowsObject() && cObj->knowsVirtual() && !cObj->getVirtual())
                    return cObj->getObject();
            }
        }
    }
    /* ----------- */

    std::ostringstream qb;
    qb << SQL_QUERY
        << " WHERE " << TQD('f', "location_hash") << '=' << quote(stringHash(dbLocation))
        << " AND " << TQD('f', "location") << '=' << quote(dbLocation)
        << " AND " << TQD('f', "ref_id") << " IS NULL "
                                            "LIMIT 1";

    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        throw _Exception(_("error while doing select: ") + qb.str());

    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;
    return createObjectFromRow(row);
}

Ref<CdsObject> SQLStorage::findObjectByPath(String fullpath)
{
    return _findObjectByPath(fullpath);
}

int SQLStorage::findObjectIDByPath(String fullpath)
{
    Ref<CdsObject> obj = _findObjectByPath(fullpath);
    if (obj == nullptr)
        return INVALID_OBJECT_ID;
    return obj->getID();
}

int SQLStorage::ensurePathExistence(String path, int* changedContainer)
{
    *changedContainer = INVALID_OBJECT_ID;
    String cleanPath = path.reduce(DIR_SEPARATOR);
    if (cleanPath == DIR_SEPARATOR)
        return CDS_ID_FS_ROOT;

    if (cleanPath.charAt(cleanPath.length() - 1) == DIR_SEPARATOR) // cut off trailing slash
        cleanPath = cleanPath.substring(0, cleanPath.length() - 1);

    return _ensurePathExistence(cleanPath, changedContainer);
}

int SQLStorage::_ensurePathExistence(String path, int* changedContainer)
{
    if (path == DIR_SEPARATOR)
        return CDS_ID_FS_ROOT;

    Ref<CdsObject> obj = findObjectByPath(path + DIR_SEPARATOR);
    if (obj != nullptr)
        return obj->getID();

    Ref<Array<StringBase>> pathAr = split_path(path);
    String parent = pathAr->get(0);
    String folder = pathAr->get(1);

    int parentID = ensurePathExistence(parent, changedContainer);

    Ref<StringConverter> f2i = StringConverter::f2i();
    if (changedContainer != nullptr && *changedContainer == INVALID_OBJECT_ID)
        *changedContainer = parentID;

    return createContainer(parentID, f2i->convert(folder), path, false, nullptr, INVALID_OBJECT_ID, nullptr);
}

int SQLStorage::createContainer(int parentID, String name, String path, bool isVirtual, String upnpClass, int refID, Ref<Dictionary> itemMetadata)
{
    if (refID > 0) {
        Ref<CdsObject> refObj = loadObject(refID);
        if (refObj == nullptr)
            throw _Exception(_("tried to create container with refID set, but refID doesn't point to an existing object"));
    }
    String dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), path);

    Ref<Dictionary> metadata = NULL;
    if (itemMetadata != nullptr) {
        if (upnpClass == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            Ref<Dictionary> metadata(new Dictionary());
            metadata->put("artist", itemMetadata->get("artist"));
            metadata->put("date", itemMetadata->get("date"));
        }
    }

    int newID = getNextID();

    std::ostringstream qb;
    qb << "INSERT INTO "
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

    if (itemMetadata != nullptr) {
        Ref<Array<DictionaryElement>> metadataElements = itemMetadata->getElements();
        for (int i = 0; i < metadataElements->size(); i++) {
            Ref<DictionaryElement> property = metadataElements->get(i);
            int newMetadataID = getNextMetadataID();
            Ref<StringBuffer> ib(new StringBuffer());
            *ib << "INSERT INTO"
                << TQ(METADATA_TABLE)
                << " ("
                << TQ("id") << ','
                << TQ("item_id") << ','
                << TQ("property_name") << ','
                << TQ("property_value") << ") VALUES ("
                << newMetadataID << ','
                << newID << ","
                << quote(property->getKey()) << ","
                << quote(property->getValue())
                << ")";
            exec(ib);
        }
        log_debug("Wrote metadata for cds_object %s", newID);
    }
    
    /* inform cache */
    if (cacheOn()) {
        AutoLock lock(cache->getMutex());
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

    std::ostringstream qb;
    qb << "SELECT " << TQ("location") << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << '=' << parentID << " LIMIT 1";

    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        return nullptr;

    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;

    char prefix;
    String path = stripLocationPrefix(&prefix, row->col(0)) + VIRTUAL_CONTAINER_SEPARATOR + title;
    if (prefix != LOC_VIRT_PREFIX)
        throw _Exception(_("tried to build a virtual container path with an non-virtual parentID"));

    return path;
}

void SQLStorage::addContainerChain(String path, String lastClass, int lastRefID, int* containerID, int* updateID, Ref<Dictionary> lastMetadata)
{
    path = path.reduce(VIRTUAL_CONTAINER_SEPARATOR);
    if (path == VIRTUAL_CONTAINER_SEPARATOR) {
        *containerID = CDS_ID_ROOT;
        return;
    }
    std::ostringstream qb;
    String dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, path);
    qb << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("location_hash") << '=' << quote(stringHash(dbLocation))
        << " AND " << TQ("location") << '=' << quote(dbLocation)
        << " LIMIT 1";

    Ref<SQLResult> res = select(qb);
    if (res != nullptr) {
        Ref<SQLRow> row = res->nextRow();
        if (row != nullptr) {
            if (containerID != nullptr)
                *containerID = row->col(0).toInt();
            return;
        }
    }

    int parentContainerID;
    String newpath, container;
    stripAndUnescapeVirtualContainerFromPath(path, newpath, container);

    addContainerChain(newpath, nullptr, INVALID_OBJECT_ID, &parentContainerID, updateID, nullptr);
    if (updateID != nullptr && *updateID == INVALID_OBJECT_ID)
        *updateID = parentContainerID;
    *containerID = createContainer(parentContainerID, container, path, true, lastClass, lastRefID, lastMetadata);
}

String SQLStorage::addLocationPrefix(char prefix, String path)
{
    return String(prefix) + path;
}

String SQLStorage::stripLocationPrefix(char* prefix, String path)
{
    if (path == nullptr) {
        *prefix = LOC_ILLEGAL_PREFIX;
        return nullptr;
    }
    *prefix = path.charAt(0);
    return path.substring(1);
}

String SQLStorage::stripLocationPrefix(String path)
{
    if (path == nullptr)
        return nullptr;
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

    Ref<Dictionary> meta = retrieveMetadataForObject(obj->getID());
    if (meta != nullptr && meta->size())
        obj->setMetadata(meta);
    else {
        meta = retrieveMetadataForObject(obj->getRefID());
        if (meta != nullptr && meta->size())
            obj->setMetadata(meta);
    }
    if (meta == nullptr || meta->size() == 0) {
        // fallback to metadata that might be in mt_cds_object, which
        // will be useful if retrieving for schema upgrade
        String metadataStr = row->col(_metadata);
        meta->decode(metadataStr);
        obj->setMetadata(meta);
    }

    String auxdataStr = fallbackString(row->col(_auxdata), row->col(_ref_auxdata));
    Ref<Dictionary> aux(new Dictionary());
    aux->decode(auxdataStr);
    obj->setAuxData(aux);

    String resources_str = fallbackString(row->col(_resources), row->col(_ref_resources));
    bool resource_zero_ok = false;
    if (string_ok(resources_str)) {
        Ref<Array<StringBase>> resources = split_string(resources_str,
            RESOURCE_SEP);
        for (int i = 0; i < resources->size(); i++) {
            if (i == 0)
                resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources->get(i)));
        }
    }

    if ((obj->getRefID() && IS_CDS_PURE_ITEM(objectType)) || (IS_CDS_ITEM(objectType) && !IS_CDS_PURE_ITEM(objectType)))
        obj->setVirtual(true);
    else
        obj->setVirtual(false); // gets set to true for virtual containers below

    int matched_types = 0;

    if (IS_CDS_CONTAINER(objectType)) {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        cont->setUpdateID(row->col(_update_id).toInt());
        char locationPrefix;
        cont->setLocation(stripLocationPrefix(&locationPrefix, row->col(_location)));
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);

        String autoscanPersistent = row->col(_as_persistent);
        if (string_ok(autoscanPersistent)) {
            if (remapBool(autoscanPersistent))
                cont->setAutoscanType(OBJECT_AUTOSCAN_CFG);
            else
                cont->setAutoscanType(OBJECT_AUTOSCAN_UI);
        } else
            cont->setAutoscanType(OBJECT_AUTOSCAN_NONE);
        matched_types++;
    }

    if (IS_CDS_ITEM(objectType)) {
        if (!resource_zero_ok)
            throw _Exception(_("tried to create object without at least one resource"));

        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setMimeType(fallbackString(row->col(_mime_type), row->col(_ref_mime_type)));
        if (IS_CDS_PURE_ITEM(objectType)) {
            if (!obj->isVirtual())
                item->setLocation(stripLocationPrefix(row->col(_location)));
            else
                item->setLocation(stripLocationPrefix(row->col(_ref_location)));
        } else // URLs and active items
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

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        std::ostringstream query;
        query << "SELECT " << TQ("id") << ',' << TQ("action") << ','
               << TQ("state") << " FROM " << TQ(CDS_ACTIVE_ITEM_TABLE)
               << " WHERE " << TQ("id") << '=' << quote(aitem->getID());
        Ref<SQLResult> resAI = select(query);
        Ref<SQLRow> rowAI;
        if (resAI != nullptr && (rowAI = resAI->nextRow()) != nullptr) {
            aitem->setAction(rowAI->col(1));
            aitem->setState(rowAI->col(2));
        } else
            throw _Exception(_("Active Item in cds_objects, but not in cds_active_item"));

        matched_types++;
    }

    if (!matched_types) {
        throw _StorageException(nullptr, _("unknown object type: ") + objectType);
    }

    addObjectToCache(obj);
    return obj;
}

Ref<CdsObject> SQLStorage::createObjectFromSearchRow(Ref<SQLRow> row)
{
    int objectType = row->col(_object_type).toInt();
    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(row->col(SearchCol::id).toInt());
    obj->setRefID(row->col(SearchCol::ref_id).toInt());

    obj->setParentID(row->col(SearchCol::parent_id).toInt());
    obj->setTitle(row->col(SearchCol::dc_title));
    obj->setClass(row->col(SearchCol::upnp_class));

    Ref<Dictionary> meta = retrieveMetadataForObject(obj->getID());
    if (meta != nullptr)
        obj->setMetadata(meta);

    String resources_str = row->col(SearchCol::resources);
    bool resource_zero_ok = false;
    if (string_ok(resources_str)) {
        Ref<Array<StringBase>> resources = split_string(resources_str, RESOURCE_SEP);
        for (int i = 0; i < resources->size(); i++) {
            if (i == 0)
                resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources->get(i)));
        }
    }

    if (IS_CDS_ITEM(objectType)) {
        if (!resource_zero_ok)
            throw _Exception(_("tried to create object without at least one resource"));

        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setMimeType(row->col(SearchCol::mime_type));
        if (IS_CDS_PURE_ITEM(objectType)) {
            item->setLocation(stripLocationPrefix(row->col(SearchCol::location)));
        } else { // URLs and active items
            item->setLocation(row->col(SearchCol::location));
        }

        item->setTrackNumber(row->col(SearchCol::track_number).toInt());
    } else {
        throw _StorageException(nullptr, _("unknown object type: ") + objectType);
    }

    return obj;
}

Ref<Dictionary> SQLStorage::retrieveMetadataForObject(int objectId)
{
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << SELECT_METADATA
        << " FROM " << TQ(METADATA_TABLE)
        << " WHERE " << TQ("item_id")
        << " = " << objectId;
    Ref<SQLResult> res = select(qb);

    if (res == nullptr)
        return nullptr;

    Ref<Dictionary> metadata(new Dictionary);
    Ref<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        metadata->put(row->col(m_property_name), row->col(m_property_value));
    }
    return metadata;
}


int SQLStorage::getTotalFiles()
{
    flushInsertBuffer();

    std::ostringstream query;
    query << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE "
           << TQ("object_type") << " != " << quote(OBJECT_TYPE_CONTAINER);
    //<< " AND is_virtual = 0";
    Ref<SQLResult> res = select(query);
    Ref<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return row->col(0).toInt();
    }
    return 0;
}

String SQLStorage::incrementUpdateIDs(shared_ptr<unordered_set<int>> ids)
{
    if (ids->empty())
        return nullptr;
    std::ostringstream inBuf;

    bool first = true;
    for (const auto& id : *ids) {
        if (first) {
            inBuf << "IN (" << id;
            first = false;
        } else {
            inBuf << ',' << id;
        }
    }
    inBuf << ')';

    std::ostringstream bufUpdate;
    bufUpdate << "UPDATE " << TQ(CDS_OBJECT_TABLE) << " SET " << TQ("update_id")
            << '=' << TQ("update_id") << " + 1 WHERE " << TQ("id") << ' ';
    bufUpdate << inBuf.str();
    exec(bufUpdate);

    std::ostringstream bufSelect;
    bufSelect << "SELECT " << TQ("id") << ',' << TQ("update_id") << " FROM "
            << TQ(CDS_OBJECT_TABLE) << " WHERE " << TQ("id") << ' ';
    bufSelect << inBuf.str();
    Ref<SQLResult> res = select(bufSelect);
    if (res == nullptr)
        throw _Exception(_("Error while fetching update ids"));
    Ref<SQLRow> row;
    std::list<std::string> rows;
    while ((row = res->nextRow()) != nullptr) {
        std::ostringstream s;
        s << row->col(0) << ',' << row->col(1);
        rows.emplace_back(s.str());
    }
    if (rows.empty())
        return nullptr;
    return join(rows, ",");
}

// id is the parent_id for cover media to find, and if set, trackArtBase is the case-folded
// name of the track to try as artwork; we rely on LIKE being case-insensitive

// This limit massively improves performance, but is not currently usable on MySql and MariaDB, apparently
// Since the actual driver in use is determined at runtime, not build time, we check for sqlite3 and
// only run when that's what we are using.
#define MAX_ART_CONTAINERS 100

String SQLStorage::findFolderImage(int id, String trackArtBase)
{
    std::ostringstream q;
    // folder.jpg or cover.jpg [and variants]
    // note - "_" is regexp "." and "%" is regexp ".*" in sql LIKE land
    q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << "( ";
    if (trackArtBase.length() > 0) {
        q << TQ("dc_title") << "LIKE " << quote(trackArtBase + _(".jp%")) << " OR ";
    }
    q << TQ("dc_title") << "LIKE " << quote(_("cover.jp%")) << " OR ";
    q << TQ("dc_title") << "LIKE " << quote(_("albumart%.jp%")) << " OR ";
    q << TQ("dc_title") << "LIKE " << quote(_("album.jp%")) << " OR ";
    q << TQ("dc_title") << "LIKE " << quote(_("front.jp%")) << " OR ";
    q << TQ("dc_title") << "LIKE " << quote(_("folder.jp%"));
    q << " ) AND ";
    q << TQ("upnp_class") << '=' << quote(_(UPNP_DEFAULT_CLASS_IMAGE_ITEM)) << " AND ";
#ifndef ONLY_REAL_FOLDER_ART
    q << "( ";
    q <<       "( ";
        // virtual listing via Album, Artist etc
    q << TQ("parent_id") << " IN ";
    q <<       "(";
    q << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << TQ("upnp_class") << '=' << quote(_(UPNP_DEFAULT_CLASS_MUSIC_TRACK)) << " AND ";
    q << TQ("id") << " IN ";
    q <<               "(";
    q << "SELECT " << TQ("ref_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << TQ("parent_id") << '=' << quote(String::from(id)) << " AND ";
    q << TQ("object_type") << '=' << quote(OBJECT_TYPE_ITEM);

    // only use this optimization on sqlite3
    if (ConfigManager::getInstance()->getOption(CFG_SERVER_STORAGE_DRIVER) == "sqlite3") {
        q <<               " LIMIT " << MAX_ART_CONTAINERS << ")";
    } else {
        q <<               ")";
    }
    q <<       ")";
    q << "     ) OR ";
#endif
        // straightforward folder listing of real filesystem
    q << TQ("parent_id") << '=' << quote(String::from(id));
#ifndef ONLY_REAL_FOLDER_ART
    q << ")";
#endif
    q << " LIMIT 1";

    //log_debug("findFolderImage %d, %s\n", id, q->c_str());
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        throw _Exception(_("db error"));
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nullptr) // we only care about the first result
    {
        log_debug("findFolderImage result: %s\n", row->col(0).c_str());
        return row->col(0);
    }
    return nullptr;
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

    while((row = res->nextRow()) != nullptr)
    {
        Ref<CdsObject> obj = createObjectFromRow(row);
        arr->append(obj);
    }
    return arr;
}
*/

shared_ptr<unordered_set<int>> SQLStorage::getObjects(int parentID, bool withoutContainer)
{
    flushInsertBuffer();

    std::ostringstream q;
    q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    if (withoutContainer)
        q << TQ("object_type") << " != " << OBJECT_TYPE_CONTAINER << " AND ";
    q << TQ("parent_id") << '=';
    q << parentID;
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        throw _Exception(_("db error"));
    Ref<SQLRow> row;

    if (res->getNumRows() <= 0)
        return nullptr;

    unsigned long long capacity = res->getNumRows() * 5 + 1;
    if (capacity < 521)
        capacity = 521;

    shared_ptr<unordered_set<int>> ret = make_shared<unordered_set<int>>();

    while ((row = res->nextRow()) != nullptr) {
        ret->insert(row->col(0).toInt());
    }
    return ret;
}

Ref<Storage::ChangedContainers> SQLStorage::removeObjects(shared_ptr<unordered_set<int>> list, bool all)
{
    flushInsertBuffer();

    int count = list->size();
    if (count <= 0)
        return nullptr;

    for (const auto& id : *list) {
        if (IS_FORBIDDEN_CDS_ID(id))
            throw _Exception(_("tried to delete a forbidden ID (") + id + ")!");
    }

    std::ostringstream idsBuf;
    idsBuf << "SELECT " << TQ("id") << ',' << TQ("object_type")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << " IN (" << join(*list, ",") << ")";

    Ref<SQLResult> res = select(idsBuf);
    if (res == nullptr)
        throw _Exception(_("sql error"));

    std::vector<int32_t> items;
    std::vector<int32_t> containers;
    Ref<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        int objectType = row->col(1).toInt();
        if (IS_CDS_CONTAINER(objectType))
            containers.push_back(row->col(0).toLong());
        else
            items.push_back(row->col(0).toLong());
    }
    return _purgeEmptyContainers(_recursiveRemove(items, containers, all));
}

void SQLStorage::_removeObjects(const std::vector<int32_t> &objectIDs) {
    auto objectIdsStr = join(objectIDs, ',');
    std::ostringstream sel;
    sel << "SELECT " << TQD('a', "id") << ',' << TQD('a', "persistent")
        << ',' << TQD('o', "location")
        << " FROM " << TQ(AUTOSCAN_TABLE) << " a"
                                            " JOIN "
        << TQ(CDS_OBJECT_TABLE) << " o"
                                  " ON "
        << TQD('o', "id") << '=' << TQD('a', "obj_id")
        << " WHERE " << TQD('o', "id")
        << " IN (" << objectIdsStr << ')';

    log_debug("%s\n", sel.str().c_str());

    Ref<SQLResult> res = select(sel);
    if (res != nullptr) {
        log_debug("relevant autoscans!\n");
        std::vector<std::string> delete_as;
        Ref<SQLRow> row;
        while ((row = res->nextRow()) != nullptr) {
            bool persistent = remapBool(row->col(1));
            if (persistent) {
                String location = stripLocationPrefix(row->col(2));
                std::ostringstream u;
                u << "UPDATE " << TQ(AUTOSCAN_TABLE)
                  << " SET " << TQ("obj_id") << "=" SQL_NULL
                  << ',' << TQ("location") << '=' << quote(location)
                  << " WHERE " << TQ("id") << '=' << quote(row->col(0));
                exec(u);
            } else {
                delete_as.emplace_back(row->col_c_str(0));
            }
            log_debug("relevant autoscan: %d; persistent: %d\n", row->col_c_str(0), persistent);
        }

        if (!delete_as.empty()) {
            std::ostringstream delAutoscan;
            delAutoscan << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
                        << " WHERE " << TQ("id") << " IN ("
                        << join(delete_as, ',')
                        << ')';
            exec(delAutoscan);
            log_debug("deleting autoscans: %s\n", delAutoscan.str().c_str());
        }
    }

    std::ostringstream qActiveItem;
    qActiveItem << "DELETE FROM " << TQ(CDS_ACTIVE_ITEM_TABLE)
                << " WHERE " << TQ("id")
                << " IN (" << objectIdsStr << ')';
    exec(qActiveItem);

    std::ostringstream qObject;
    qObject << "DELETE FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id")
            << " IN (" << objectIdsStr << ')';
    exec(qObject);
}

Ref<Storage::ChangedContainers> SQLStorage::removeObject(int objectID, bool all)
{
    flushInsertBuffer();

    std::ostringstream q;
    q << "SELECT " << TQ("object_type") << ',' << TQ("ref_id")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("id") << '=' << quote(objectID) << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        return nullptr;
    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;

    int objectType = row->col(0).toInt();
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && !isContainer) {
        String ref_id_str = row->col(1);
        int ref_id;
        if (string_ok(ref_id_str)) {
            ref_id = ref_id_str.toInt();
            if (!IS_FORBIDDEN_CDS_ID(ref_id))
                objectID = ref_id;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw _Exception(_("tried to delete a forbidden ID (") + objectID + ")!");
    std::vector<int32_t> itemIds;
    std::vector<int32_t> containerIds;
    if (isContainer) {
        containerIds.push_back(objectID);
    } else {
        itemIds.push_back(objectID);
    }
    auto changedContainers = _recursiveRemove(itemIds, containerIds, all);
    return _purgeEmptyContainers(changedContainers);
}

Ref<Storage::ChangedContainers> SQLStorage::_recursiveRemove(
    const std::vector<int32_t> &items, const std::vector<int32_t> &containers,
    bool all)
{
    log_debug("start\n");
    std::ostringstream itemsSql;
    itemsSql << "SELECT DISTINCT " << TQ("id") << ',' << TQ("parent_id")
                 << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE " << TQ("ref_id") << " IN (";

    std::ostringstream containersSql;
    containersSql << "SELECT DISTINCT " << TQ("id")
                      << ',' << TQ("object_type");
    if (all)
        containersSql << ',' << TQ("ref_id");
    containersSql << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE " << TQ("parent_id") << " IN (";

    std::ostringstream parentsSql;
    parentsSql << "SELECT DISTINCT " << TQ("parent_id")
                      << " FROM " << TQ(CDS_OBJECT_TABLE)
                      << " WHERE " << TQ("id") << " IN (";

    Ref<ChangedContainers> changedContainers(new ChangedContainers());

    Ref<SQLResult> res;
    Ref<SQLRow> row;

    std::vector<int32_t> itemIds(items);
    std::vector<int32_t> containerIds(containers);
    std::vector<int32_t> parentIds(items);
    std::vector<int32_t> removeIds(containers);

    if (!containers.empty()) {
        parentIds.insert(parentIds.end(), containers.begin(), containers.end());
        std::ostringstream sql;
        sql << parentsSql.str() << join(parentIds, ',') << ')';
        res = select(sql);
        if (res == nullptr)
            throw _StorageException(nullptr, _("sql error"));
        parentIds.clear();
        while ((row = res->nextRow()) != nullptr) {
            changedContainers->ui.push_back(row->col(0).toInt());
        }
    }

    int count = 0;
    while (!itemIds.empty() || !parentIds.empty() || !containerIds.empty()) {
        if (!parentIds.empty()) {
            // add ids to remove
            removeIds.insert(removeIds.end(), parentIds.begin(), parentIds.end());
            std::ostringstream sql;
            sql << parentsSql.str() << join(parentIds, ',') << ')';
            res = select(sql);
            if (res == nullptr)
                throw _StorageException(nullptr, std::string("sql error: ") + sql.str());
            parentIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                changedContainers->upnp.push_back(row->col(0).toInt());
            }
        }

        if (!itemIds.empty()) {
            std::ostringstream sql;
            sql << itemsSql.str() << join(itemIds, ',') << ')';
            res = select(sql);
            if (res == nullptr)
                throw _StorageException(nullptr, std::string("sql error: ") + sql.str());
            itemIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                removeIds.push_back(row->col(0).toInt());
                changedContainers->upnp.push_back(row->col(1).toInt());
            }
        }

        if (!containerIds.empty()) {
            std::ostringstream sql;
            sql << containersSql.str() << join(containerIds, ',') << ')';
            res = select(sql);
            if (res == nullptr)
                throw _StorageException(nullptr, std::string("sql error: ") + sql.str());
            containerIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                int objectType = row->col(1).toInt();
                if (IS_CDS_CONTAINER(objectType)) {
                    containerIds.push_back(row->col(0).toInt());
                    removeIds.push_back(row->col(0).toInt());
                } else {
                    if (all) {
                        String refId = row->col(2);
                        if (string_ok(refId)) {
                            parentIds.push_back(row->col(2).toInt());
                            itemIds.push_back(row->col(2).toInt());
                            removeIds.push_back(row->col(2).toInt());
                        } else {
                            removeIds.push_back(row->col(0).toInt());
                            itemIds.push_back(row->col(0).toInt());
                        }
                    } else {
                        removeIds.push_back(row->col(0).toInt());
                        itemIds.push_back(row->col(0).toInt());
                    }
                }
            }
        }

        if (removeIds.size() > MAX_REMOVE_SIZE) {
            _removeObjects(removeIds);
            removeIds.clear();
        }

        if (count++ > MAX_REMOVE_RECURSION)
            throw _Exception(_("there seems to be an infinite loop..."));
    }

    if (!removeIds.empty())
        _removeObjects(removeIds);
    log_debug("end\n");
    return changedContainers;
}

zmm::String SQLStorage::toCSV(const std::vector<int>& input)
{
    return join(input, ",");
}

Ref<Storage::ChangedContainers> SQLStorage::_purgeEmptyContainers(Ref<ChangedContainers> maybeEmpty)
{
    log_debug("start upnp: %s; ui: %s\n",
            join(maybeEmpty->upnp, ',').c_str(),
            join(maybeEmpty->ui, ',').c_str());
    Ref<ChangedContainers> changedContainers(new ChangedContainers());
    if (maybeEmpty->upnp.empty() && maybeEmpty->ui.empty())
        return changedContainers;

    std::ostringstream selectSql;
    selectSql << "SELECT " << TQD('a', "id")
              << ", COUNT(" << TQD('b', "parent_id")
              << ")," << TQD('a', "parent_id") << ',' << TQD('a', "flags")
              << " FROM " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('a')
              << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('b')
              << " ON " << TQD('a', "id") << '=' << TQD('b', "parent_id")
              << " WHERE " << TQD('a', "object_type") << '=' << quote(1)
              << " AND " << TQD('a', "id") << " IN ("; //(a.flags & " << OBJECT_FLAG_PERSISTENT_CONTAINER << ") = 0 AND
    std::string strSel2(") GROUP BY a.id"); // HAVING COUNT(b.parent_id)=0");

    std::ostringstream bufSelUpnp;
    bufSelUpnp << selectSql.str();

    std::vector<int32_t> del;

    Ref<SQLResult> res;
    Ref<SQLRow> row;
    std::vector<int32_t> selUi;
    std::vector<int32_t> selUpnp;

    auto &uiV = maybeEmpty->ui;
    selUi.insert(selUi.end(), uiV.begin(), uiV.end());
    auto &upnpV = maybeEmpty->upnp;
    selUpnp.insert(selUpnp.end(), upnpV.begin(), upnpV.end());

    bool again;
    int count = 0;
    do {
        again = false;

        if (!selUpnp.empty()) {
            std::ostringstream sql;
            sql << selectSql.str() << join(selUpnp, ',') << strSel2;
            log_debug("upnp-sql: %s\n", sql.str().c_str());
            res = select(sql.str());
            selUpnp.clear();
            if (res == nullptr)
                throw _Exception(_("db error"));
            while ((row = res->nextRow()) != nullptr) {
                int flags = row->col(3).toInt();
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers->upnp.push_back(row->col(0).toInt());
                else if (row->col(1) == "0") {
                    del.push_back(row->col(0).toInt());
                    selUi.push_back(row->col(2).toInt());
                } else {
                    selUpnp.push_back(row->col(0).toInt());
                }
            }
        }

        if (!selUi.empty()) {
            std::ostringstream sql;
            sql << selectSql.str() << join(selUi, ',') << strSel2;
            log_debug("ui-sql: %s\n", sql.str().c_str());
            res = select(sql.str());
            selUi.clear();
            if (res == nullptr)
                throw _Exception(_("db error"));
            while ((row = res->nextRow()) != nullptr) {
                int flags = row->col(3).toInt();
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER) {
                    changedContainers->ui.push_back(row->col(0).toInt());
                    changedContainers->upnp.push_back(row->col(0).toInt());
                } else if (row->col(1) == "0") {
                    del.push_back(row->col(0).toInt());
                    selUi.push_back(row->col(2).toInt());
                } else {
                    selUi.push_back(row->col(0).toInt());
                }
            }
        }

        //log_debug("selecting: %s; removing: %s\n", bufSel->c_str(), join(del, ',').c_str());
        if (!del.empty()) {
            _removeObjects(del);
            del.clear();
            if (!selUi.empty() || !selUpnp.empty())
                again = true;
        }
        if (count++ >= MAX_REMOVE_RECURSION)
            throw _Exception(_("there seems to be an infinite loop..."));
    } while (again);

    auto &changedUi = changedContainers->ui;
    auto &changedUpnp = changedContainers->upnp;
    if (!selUi.empty()) {
        changedUi.insert(changedUi.end(), selUi.begin(), selUi.end());
        changedUpnp.insert(changedUpnp.end(), selUi.begin(), selUi.end());
    }
    if (!selUpnp.empty()) {
        changedUpnp.insert(changedUpnp.end(), selUpnp.begin(), selUpnp.end());
    }
    // log_debug("end; changedContainers (upnp): %s\n", join(changedUpnp, ',').c_str());
    // log_debug("end; changedContainers (ui): %s\n", join(changedUi, ',').c_str());
    log_debug("end; changedContainers (upnp): %d\n", changedUpnp.size());
    log_debug("end; changedContainers (ui): %d\n", changedUi.size());

    /* clear cache (for now) */
    if (cacheOn())
        cache->clear();
    /* --------------------- */

    return changedContainers;
}

String SQLStorage::getInternalSetting(String key)
{
    std::ostringstream q;
    q << "SELECT " << TQ("value") << " FROM " << TQ(INTERNAL_SETTINGS_TABLE) << " WHERE " << TQ("key") << '='
       << quote(key) << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        return nullptr;
    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;
    return row->col(0);
}
/*
void SQLStorage::storeInternalSetting(String key, String value)
overwritten due to different SQL syntax for MySQL and SQLite3
*/

void SQLStorage::updateAutoscanPersistentList(ScanMode scanmode, Ref<AutoscanList> list)
{

    log_debug("setting persistent autoscans untouched - scanmode: %s;\n", AutoscanDirectory::mapScanmode(scanmode).c_str());
    std::ostringstream update;
    update << "UPDATE " << TQ(AUTOSCAN_TABLE)
       << " SET " << TQ("touched") << '=' << mapBool(false)
       << " WHERE "
       << TQ("persistent") << '=' << mapBool(true)
       << " AND " << TQ("scan_mode") << '='
       << quote(AutoscanDirectory::mapScanmode(scanmode));
    exec(update);

    int listSize = list->size();
    log_debug("updating/adding persistent autoscans (count: %d)\n", listSize);
    for (int i = 0; i < listSize; i++) {
        log_debug("getting ad %d from list..\n", i);
        Ref<AutoscanDirectory> ad = list->get(i);
        if (ad == nullptr)
            continue;

        // only persistent asD should be given to getAutoscanList
        assert(ad->persistent());
        // the scanmode should match the given parameter
        assert(ad->getScanMode() == scanmode);

        String location = ad->getLocation();
        if (!string_ok(location))
            throw _Exception(_("AutoscanDirectoy with illegal location given to SQLStorage::updateAutoscanPersistentList"));

        std::ostringstream q;
        q << "SELECT " << TQ("id") << " FROM " << TQ(AUTOSCAN_TABLE)
           << " WHERE ";
        int objectID = findObjectIDByPath(location + '/');
        log_debug("objectID = %d\n", objectID);
        if (objectID == INVALID_OBJECT_ID)
            q << TQ("location") << '=' << quote(location);
        else
            q << TQ("obj_id") << '=' << quote(objectID);
        q << " LIMIT 1";
        Ref<SQLResult> res = select(q);
        if (res == nullptr)
            throw _StorageException(nullptr, _("query error while selecting from autoscan list"));
        Ref<SQLRow> row;
        if ((row = res->nextRow()) != nullptr) {
            ad->setStorageID(row->col(0).toInt());
            updateAutoscanDirectory(ad);
        } else
            addAutoscanDirectory(ad);
    }

    std::ostringstream del;
    del << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("touched") << '=' << mapBool(false)
       << " AND " << TQ("scan_mode") << '='
       << quote(AutoscanDirectory::mapScanmode(scanmode));
    exec(del);
}

Ref<AutoscanList> SQLStorage::getAutoscanList(ScanMode scanmode)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t', "id")
       << " WHERE " FLD("scan_mode") '=' << quote(AutoscanDirectory::mapScanmode(scanmode));
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        throw _StorageException(nullptr, _("query error while fetching autoscan list"));
    Ref<AutoscanList> ret(new AutoscanList());
    Ref<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        Ref<AutoscanDirectory> dir = _fillAutoscanDirectory(row);
        if (dir == nullptr)
            removeAutoscanDirectory(row->col(0).toInt());
        else
            ret->add(dir);
    }
    return ret;
}

Ref<AutoscanDirectory> SQLStorage::getAutoscanDirectory(int objectID)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t', "id")
       << " WHERE " << TQD('t', "id") << '=' << quote(objectID);
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        throw _StorageException(nullptr, _("query error while fetching autoscan"));
    Ref<AutoscanList> ret(new AutoscanList());
    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;
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
    if (objectID == INVALID_OBJECT_ID) {
        location = row->col(9);
    } else {
        char prefix;
        location = stripLocationPrefix(&prefix, row->col(10));
        if (prefix != LOC_DIR_PREFIX)
            return nullptr;
    }

    ScanLevel level = AutoscanDirectory::remapScanlevel(row->col(2));
    ScanMode mode = AutoscanDirectory::remapScanmode(row->col(3));
    bool recursive = remapBool(row->col(4));
    bool hidden = remapBool(row->col(5));
    bool persistent = remapBool(row->col(8));
    int interval = 0;
    if (mode == ScanMode::Timed)
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
    if (adir == nullptr)
        throw _Exception(_("addAutoscanDirectory called with adir==nullptr"));
    if (adir->getStorageID() >= 0)
        throw _Exception(_("tried to add autoscan directory with a storage id set"));
    int objectID;
    if (adir->getLocation() == FS_ROOT_DIRECTORY)
        objectID = CDS_ID_FS_ROOT;
    else
        objectID = findObjectIDByPath(adir->getLocation() + DIR_SEPARATOR);
    if (!adir->persistent() && objectID < 0)
        throw _Exception(_("tried to add non-persistent autoscan directory with an illegal objectID or location"));

    auto pathIds = _checkOverlappingAutoscans(adir);

    _autoscanChangePersistentFlag(objectID, true);

    std::ostringstream q;
    q << "INSERT INTO " << TQ(AUTOSCAN_TABLE)
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
       << (pathIds == nullptr ? _(SQL_NULL) : quote(_(",") + toCSV(*pathIds) + ','))
       << ')';
    adir->setStorageID(exec(q, true));
}

void SQLStorage::updateAutoscanDirectory(Ref<AutoscanDirectory> adir)
{
    log_debug("id: %d, obj_id: %d\n", adir->getStorageID(), adir->getObjectID());

    if (adir == nullptr)
        throw _Exception(_("updateAutoscanDirectory called with adir==nullptr"));

    auto pathIds = _checkOverlappingAutoscans(adir);

    int objectID = adir->getObjectID();
    int objectIDold = _getAutoscanObjectID(adir->getStorageID());
    if (objectIDold != objectID) {
        _autoscanChangePersistentFlag(objectIDold, false);
        _autoscanChangePersistentFlag(objectID, true);
    }
    std::ostringstream q;
    q << "UPDATE " << TQ(AUTOSCAN_TABLE)
       << " SET " << TQ("obj_id") << '=' << (objectID >= 0 ? quote(objectID) : _(SQL_NULL))
       << ',' << TQ("scan_level") << '='
       << quote(AutoscanDirectory::mapScanlevel(adir->getScanLevel()))
       << ',' << TQ("scan_mode") << '='
       << quote(AutoscanDirectory::mapScanmode(adir->getScanMode()))
       << ',' << TQ("recursive") << '=' << mapBool(adir->getRecursive())
       << ',' << TQ("hidden") << '=' << mapBool(adir->getHidden())
       << ',' << TQ("interval") << '=' << quote(adir->getInterval());
    if (adir->getPreviousLMT() > 0)
        q << ',' << TQ("last_modified") << '=' << quote(adir->getPreviousLMT());
    q << ',' << TQ("persistent") << '=' << mapBool(adir->persistent())
       << ',' << TQ("location") << '=' << (objectID >= 0 ? _(SQL_NULL) : quote(adir->getLocation()))
       << ',' << TQ("path_ids") << '=' << (pathIds == nullptr ? _(SQL_NULL) : quote(_(",") + toCSV(*pathIds) + ','))
       << ',' << TQ("touched") << '=' << mapBool(true)
       << " WHERE " << TQ("id") << '=' << quote(adir->getStorageID());
    exec(q);
}

void SQLStorage::removeAutoscanDirectoryByObjectID(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    std::ostringstream q;
    q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << '=' << quote(objectID);
    exec(q);

    _autoscanChangePersistentFlag(objectID, false);
}

void SQLStorage::removeAutoscanDirectory(int autoscanID)
{
    if (autoscanID == INVALID_OBJECT_ID)
        return;
    int objectID = _getAutoscanObjectID(autoscanID);
    std::ostringstream q;
    q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
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
    std::ostringstream q;
    q << "SELECT " << TQ(field) << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << '=' << quote(objectID);
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;
    if (res == nullptr || (row = res->nextRow()) == nullptr)
        return 0;
    if (!remapBool(row->col(0)))
        return 1;
    else
        return 2;
}

int SQLStorage::_getAutoscanObjectID(int autoscanID)
{
    std::ostringstream q;
    q << "SELECT " << TQ("obj_id") << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("id") << '=' << quote(autoscanID)
       << " LIMIT 1";
    Ref<SQLResult> res = select(q);
    if (res == nullptr)
        throw _StorageException(nullptr, _("error while doing select on "));
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nullptr && string_ok(row->col(0)))
        return row->col(0).toInt();
    return INVALID_OBJECT_ID;
}

void SQLStorage::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID || objectID == INVALID_OBJECT_ID_2)
        return;

    std::ostringstream q;
    q << "UPDATE " << TQ(CDS_OBJECT_TABLE)
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
    std::ostringstream q;
    q << "UPDATE " << TQ(AUTOSCAN_TABLE)
       << " SET " << TQ("last_modified") << '=' << quote(adir->getPreviousLMT())
       << " WHERE " << TQ("id") << '=' << quote(adir->getStorageID());
    exec(q);
}

int SQLStorage::isAutoscanChild(int objectID)
{
    auto pathIDs = getPathIDs(objectID);
    if (pathIDs == nullptr)
        return INVALID_OBJECT_ID;

    for (int pathId : *pathIDs) {
        int recursive = isAutoscanDirectoryRecursive(pathId);
        if (recursive == 2)
            return pathId;
    }
    return INVALID_OBJECT_ID;
}

void SQLStorage::checkOverlappingAutoscans(Ref<AutoscanDirectory> adir)
{
    _checkOverlappingAutoscans(adir);
}

std::unique_ptr<std::vector<int>> SQLStorage::_checkOverlappingAutoscans(Ref<AutoscanDirectory> adir)
{
    if (adir == nullptr)
        throw _Exception(_("_checkOverlappingAutoscans called with adir==nullptr"));
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID)
        return nullptr;
    int storageID = adir->getStorageID();

    Ref<SQLResult> res;
    Ref<SQLRow> row;

    std::ostringstream q;
    q << "SELECT " << TQ("id")
       << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << " = "
       << quote(checkObjectID);
    if (storageID >= 0)
        q << " AND " << TQ("id") << " != " << quote(storageID);

    res = select(q);
    if (res == nullptr)
        throw _Exception(_("SQL error"));

    if ((row = res->nextRow()) != nullptr) {
        Ref<CdsObject> obj = loadObject(checkObjectID);
        if (obj == nullptr)
            throw _Exception(_("Referenced object (by Autoscan) not found."));
        log_error("There is already an Autoscan set on %s\n", obj->getLocation().c_str());
        throw _Exception(_("There is already an Autoscan set on ") + obj->getLocation());
    }

    if (adir->getRecursive()) {
        std::ostringstream q;
        q << "SELECT " << TQ("obj_id")
           << " FROM " << TQ(AUTOSCAN_TABLE)
           << " WHERE " << TQ("path_ids") << " LIKE "
           << quote(_("%,") + checkObjectID + ",%");
        if (storageID >= 0)
            q << " AND " << TQ("id") << " != " << quote(storageID);
        q << " LIMIT 1";

        log_debug("------------ %s\n", q.str().c_str());

        res = select(q);
        if (res == nullptr)
            throw _Exception(_("SQL error"));
        if ((row = res->nextRow()) != nullptr) {
            int objectID = row->col(0).toInt();
            log_debug("-------------- %d\n", objectID);
            Ref<CdsObject> obj = loadObject(objectID);
            if (obj == nullptr)
                throw _Exception(_("Referenced object (by Autoscan) not found."));
            log_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on %s\n", obj->getLocation().c_str());
            throw _Exception(_("Overlapping Autoscans are not allowed. There is already an Autoscan set on ") + obj->getLocation());
        }
    }

    auto pathIDs = getPathIDs(checkObjectID);
    if (pathIDs == nullptr)
        throw _Exception(_("getPathIDs returned nullptr"));
    std::ostringstream q2;
    q2 << "SELECT " << TQ("obj_id")
       << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << " IN ("
       << toCSV(*pathIDs)
       << ") AND " << TQ("recursive") << '=' << mapBool(true);
    if (storageID >= 0)
        q2 << " AND " << TQ("id") << " != " << quote(storageID);
    q2 << " LIMIT 1";

    res = select(q2);
    if (res == nullptr)
        throw _Exception(_("SQL error"));
    if ((row = res->nextRow()) == nullptr)
        return pathIDs;
    else {
        int objectID = row->col(0).toInt();
        Ref<CdsObject> obj = loadObject(objectID);
        if (obj == nullptr)
            throw _Exception(_("Referenced object (by Autoscan) not found."));
        log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on %s\n", obj->getLocation().c_str());
        throw _Exception(_("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on ") + obj->getLocation());
    }
}

std::unique_ptr<std::vector<int>> SQLStorage::getPathIDs(int objectID)
{
    flushInsertBuffer();

    if (objectID == INVALID_OBJECT_ID)
        return nullptr;

    auto pathIDs = make_unique<std::vector<int>>();

    std::ostringstream sel;
    sel << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    sel << TQ("id") << '=';
    
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    while (objectID != CDS_ID_ROOT) {
        pathIDs->push_back(objectID);
        std::ostringstream q;
        q << sel.str() << quote(objectID) << " LIMIT 1";
        res = select(q);
        if (res == nullptr || (row = res->nextRow()) == nullptr)
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
    if (string_ok(rootName)) {
        fsRootName = rootName;
    } else {
        Ref<CdsObject> fsRootObj = loadObject(CDS_ID_FS_ROOT);
        fsRootName = fsRootObj->getTitle();
    }
}

int SQLStorage::getNextID()
{
    if (lastID < CDS_ID_FS_ROOT)
        throw _Exception(_("SQLStorage::getNextID() called, but lastID hasn't been loaded correctly yet"));
    AutoLock lock(nextIDMutex);
    return ++lastID;
}

void SQLStorage::loadLastID()
{
    AutoLock lock(nextIDMutex);

    // we don't rely on automatic db generated ids, because of our caching
    std::ostringstream qb;
    qb << "SELECT MAX(" << TQ("id") << ')'
        << " FROM " << TQ(CDS_OBJECT_TABLE);
    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        throw _Exception(_("could not load lastID (res==nullptr)"));

    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        throw _Exception(_("could not load lastID (row==nullptr)"));

    lastID = row->col(0).toInt();
    if (lastID < CDS_ID_FS_ROOT)
        throw _Exception(_("could not load correct lastID (db not initialized?)"));
}

int SQLStorage::getNextMetadataID()
{
    if (lastMetadataID < CDS_ID_ROOT)
        throw _Exception(_("SQLStorage::getNextMetadataID() called, but lastMetadataID hasn't been loaded correctly yet"));

    AutoLock lock(nextIDMutex);
    return ++lastMetadataID;
}

// if metadata table becomes first-class then should have a parameterized loadLastID()
// to be called for either mt_cds_object or mt_metadata
void SQLStorage::loadLastMetadataID()
{
    AutoLock lock(nextIDMutex);

    // we don't rely on automatic db generated ids, because of our caching
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT MAX(" << TQ("id") << ')'
        << " FROM " << TQ(METADATA_TABLE);
    Ref<SQLResult> res = select(qb);
    if (res == nullptr)
        throw _Exception(_("could not load lastMetadataID (res==nullptr)"));

    Ref<SQLRow> row = res->nextRow();
    if (row == nullptr)
        throw _Exception(_("could not load lastMetadataID (row==nullptr)"));

    lastMetadataID = row->col(0).toInt();
    if (lastMetadataID < CDS_ID_ROOT)
        throw _Exception(_("could not load correct lastMetadataID (db not initialized?)"));
}

void SQLStorage::addObjectToCache(Ref<CdsObject> object, bool dontLock)
{
    if (cacheOn() && object != nullptr) {
        unique_lock<std::mutex> lock(cache->getMutex(), std::defer_lock);
        if (!dontLock)
            lock.lock();
        Ref<CacheObject> cObj = cache->getObjectDefinitely(object->getID());
        if (cache->flushed())
            flushInsertBuffer();
        cObj->setObject(object);
        cache->checkLocation(cObj);
    }
}

void SQLStorage::addToInsertBuffer(const std::string &query)
{
    assert(doInsertBuffering());

    AutoLock lock(mutex);
    _addToInsertBuffer(query);

    insertBufferEmpty = false;
    insertBufferStatementCount++;
    insertBufferByteCount += query.length();

    if (insertBufferByteCount > 102400)
        flushInsertBuffer(true);
}

void SQLStorage::flushInsertBuffer(bool dontLock)
{
    if (!doInsertBuffering())
        return;
    //print_backtrace();
    unique_lock<decltype(mutex)> lock(mutex, std::defer_lock);
    if (!dontLock)
        lock.lock();
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
    std::ostringstream qb;
    qb << "UPDATE "
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

void SQLStorage::generateMetadataDBOperations(Ref<CdsObject> obj, bool isUpdate,
    Ref<Array<AddUpdateTable>> operations)
{
    Ref<Dictionary> dict = obj->getMetadata();
    int objMetadataSize = dict->size();
    if (!isUpdate) {
        Ref<Array<DictionaryElement>> metadataElements = dict->getElements();
        for (int  i = 0; i < objMetadataSize; i++) {
            Ref<Dictionary> metadataSql(new Dictionary());
            operations->append(Ref<AddUpdateTable>(new AddUpdateTable(_(METADATA_TABLE), metadataSql, "insert")));
            Ref<DictionaryElement> property = metadataElements->get(i);
            metadataSql->put(_("property_name"), quote(property->getKey()));
            metadataSql->put(_("property_value"), quote(property->getValue()));
        }
    } else {
        // get current metadata from DB: if only it really was a dictionary...
        Ref<Dictionary> dbMetadata = retrieveMetadataForObject(obj->getID());
        Ref<Array<DictionaryElement>> el = dict->getElements();
        for (int i = 0; i < objMetadataSize; i++) {
            Ref<DictionaryElement> property = el->get(i);
            String operation = dbMetadata->get(property->getKey()).length() == 0 ? "insert" : "update";
            Ref<Dictionary> metadataSql(new Dictionary());
            operations->append(Ref<AddUpdateTable>(new AddUpdateTable(_(METADATA_TABLE), metadataSql, operation)));
            metadataSql->put(_("property_name"), quote(property->getKey()));
            metadataSql->put(_("property_value"), quote(property->getValue()));
        }
        el = dbMetadata->getElements();
        int dbMetadataSize = dbMetadata->size();
        for (int i = 0; i < dbMetadataSize; i++) {
            Ref<DictionaryElement> property = el->get(i);
            if (dict->get(property->getKey()).length() == 0) {
                // key in db metadata but not obj metadata, so needs a delete
                Ref<Dictionary> metadataSql(new Dictionary());
                operations->append(Ref<AddUpdateTable>(new AddUpdateTable(_(METADATA_TABLE), metadataSql, "delete")));
                metadataSql->put(_("property_name"), quote(property->getKey()));
                metadataSql->put(_("property_value"), quote(property->getValue()));
            }
        }
    }
}

std::shared_ptr<std::ostringstream> SQLStorage::sqlForInsert(Ref<CdsObject> obj, Ref<AddUpdateTable> addUpdateTable)
{
    int lastInsertID = INVALID_OBJECT_ID;
    int lastMetadataInsertID = INVALID_OBJECT_ID;

    String tableName = addUpdateTable->getTable();
    Ref<Array<DictionaryElement>> dataElements = addUpdateTable->getDict()->getElements();

    std::ostringstream fields;
    std::ostringstream values;

    for (int j = 0; j < dataElements->size(); j++) {
        Ref<DictionaryElement> element = dataElements->get(j);
        if (j != 0) {
            fields << ',';
            values << ',';
        }
        fields << TQ(element->getKey());
        if (lastInsertID != INVALID_OBJECT_ID && element->getKey() == "id" && element->getValue().toInt() == INVALID_OBJECT_ID) {
            if (tableName == _(METADATA_TABLE))
                values << lastMetadataInsertID;
            else
                values << lastInsertID;
        } else
            values << element->getValue();
    }

    /* manually generate ID */
    if (lastInsertID == INVALID_OBJECT_ID && tableName == _(CDS_OBJECT_TABLE)) {
        lastInsertID = getNextID();
        obj->setID(lastInsertID);
        fields << ',' << TQ("id");
        values << ',' << quote(lastInsertID);
    }
    if (tableName == _(METADATA_TABLE)) {
        lastMetadataInsertID = getNextMetadataID();
        fields << ',' << TQ("id");
        values << ',' << quote(lastMetadataInsertID);
        fields << ',' << TQ("item_id");
        values << ',' << quote(obj->getID());
    }

    std::shared_ptr<std::ostringstream> qb = std::make_shared<std::ostringstream>();
    *qb << "INSERT INTO " << TQ(tableName) << " (" << fields.str() << ") VALUES (" << values.str() << ')';

    return qb;
}

std::shared_ptr<std::ostringstream> SQLStorage::sqlForUpdate(Ref<CdsObject> obj, Ref<AddUpdateTable> addUpdateTable)
{
    if (addUpdateTable == nullptr || addUpdateTable->getDict() == nullptr
        || (addUpdateTable->getTable() == _(METADATA_TABLE) && addUpdateTable->getDict()->size() != 1))
        throw _Exception(_("sqlForDelete called with invalid arguments"));

    String tableName = addUpdateTable->getTable();
    Ref<Array<DictionaryElement>> dataElements = addUpdateTable->getDict()->getElements();

    std::shared_ptr<std::ostringstream> qb = std::make_shared<std::ostringstream>();
    *qb << "UPDATE " << TQ(tableName) << " SET ";
    for (int j = 0; j < dataElements->size(); j++) {
        Ref<DictionaryElement> element = dataElements->get(j);
        if (j != 0)
            *qb << ',';
        *qb << TQ(element->getKey()) << '='
            << element->getValue();
    }
    *qb << " WHERE " << TQ("id") << " = " << obj->getID();

    // relying on only one element when table is mt_metadata
    if (tableName == _(METADATA_TABLE))
        *qb << " AND " << TQ("property_name") << " = " <<  dataElements->get(0)->getKey();

    return qb;
}

std::shared_ptr<std::ostringstream> SQLStorage::sqlForDelete(Ref<CdsObject> obj, Ref<AddUpdateTable> addUpdateTable)
{
    if (addUpdateTable == nullptr || addUpdateTable->getDict() == nullptr
        || (addUpdateTable->getTable() == _(METADATA_TABLE) && addUpdateTable->getDict()->size() != 1))
        throw _Exception(_("sqlForDelete called with invalid arguments"));

    Ref<Array<DictionaryElement>> dataElements = addUpdateTable->getDict()->getElements();
    String tableName = addUpdateTable->getTable();
    std::shared_ptr<std::ostringstream> qb = std::make_shared<std::ostringstream>();
    *qb << "DELETE FROM " << TQ(tableName)
        << " WHERE " << TQ("id") << " = " << obj->getID();
    
    // relying on only one element when table is mt_metadata
    if (tableName == _(METADATA_TABLE))
        *qb << " AND " << TQ("property_name") << " = " <<  dataElements->get(0)->getKey();
    
    return qb;
}

void SQLStorage::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required\n");
    std::ostringstream qbCountNotNull;
    qbCountNotNull << "SELECT COUNT(*)"
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("metadata")
       << " is not null";
    Ref<SQLResult> res = select(qbCountNotNull);
    int expectedConversionCount = res->nextRow()->col(0).toInt();
    log_debug("mt_cds_object rows having metadata: %d\n", expectedConversionCount);

    std::ostringstream qbCountMetadata;
    qbCountMetadata << "SELECT COUNT(*)"
       << " FROM " << TQ(METADATA_TABLE);
    res = select(qbCountMetadata);
    int metadataRowCount = res->nextRow()->col(0).toInt();
    log_debug("mt_metadata rows having metadata: %d\n", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required\n");
        return;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata\n");
    log_info("No data will be removed from mt_cds_object\n");
        
    std::ostringstream qbRetrieveIDs;
    qbRetrieveIDs << "SELECT " << TQ("id")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("metadata")
       << " is not null";
    Ref<SQLResult> resIds = select(qbRetrieveIDs);
    Ref<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow()) != nullptr) {
        Ref<CdsObject> cdsObject = loadObject(row->col(0).toInt());
        migrateMetadata(cdsObject);
        ++objectsUpdated;
    }
    log_info("Migrated metadata - object count: %d\n", objectsUpdated);
}

void SQLStorage::migrateMetadata(Ref<CdsObject> object)
{
    if (object == nullptr)
        return;
 
    Ref<Dictionary> dict = object->getMetadata();
    if (dict != nullptr && dict->size()) {
        log_debug("Migrating metadata for cds object %d\n", object->getID());
        Ref<Dictionary> metadataSQLVals(new Dictionary());
        Ref<Array<DictionaryElement>> metadataElements = dict->getElements();
        for (int i = 0; i < metadataElements->size(); i++) {
            Ref<DictionaryElement> property = metadataElements->get(i);
            metadataSQLVals->put(quote(property->getKey()), quote(property->getValue()));
        }

        Ref<Array<DictionaryElement>> dataElements = metadataSQLVals->getElements();
        for (int j = 0; j < dataElements->size(); j++) {
            Ref<StringBuffer> fields(new StringBuffer(128));
            Ref<StringBuffer> values(new StringBuffer(128));
            Ref<DictionaryElement> element = dataElements->get(j);
            *fields << TQ("id") << ','
                    << TQ("item_id") << ','
                    << TQ("property_name") << ','
                    << TQ("property_value");
            *values << getNextMetadataID() << ','
                    << object->getID() << ','
                    << element->getKey() << ','
                    << element->getValue();
            std::ostringstream qb;
            qb << "INSERT INTO " << TQ(_(METADATA_TABLE))
               << " (" << fields->toString()
               << ") VALUES (" << values->toString() << ')';

            if (!doInsertBuffering())
                exec(qb);
            else
                addToInsertBuffer(qb.str());
        }
    } else {
        log_debug("Skipping migration - no metadata for cds object %d\n", object->getID());
    }
}
