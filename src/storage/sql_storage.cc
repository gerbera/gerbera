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

#include <algorithm>
#include <climits>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "sql_storage.h"
#include "config/config_manager.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "update_manager.h"
#include "search_handler.h"

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
#define TQ(data) QTB << (data) << QTE
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

SQLStorage::SQLStorage(std::shared_ptr<ConfigManager> config)
    : Storage(std::move(config))
{
    table_quote_begin = '\0';
    table_quote_end = '\0';
    lastID = INVALID_OBJECT_ID;
    lastMetadataID = INVALID_OBJECT_ID;
}

void SQLStorage::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw std::runtime_error("quote vars need to be overridden!");

    std::ostringstream buf;
    buf << SQL_QUERY_FOR_STRINGBUFFER;
    this->sql_query = buf.str();

    sqlEmitter = std::make_shared<DefaultSQLEmitter>();
}

void SQLStorage::dbReady()
{
    loadLastID();
    loadLastMetadataID();
}

void SQLStorage::shutdown()
{
    shutdownDriver();
}

std::shared_ptr<CdsObject> SQLStorage::checkRefID(const std::shared_ptr<CdsObject>& obj)
{
    if (!obj->isVirtual())
        throw std::runtime_error("checkRefID called for a non-virtual object");

    int refID = obj->getRefID();
    std::string location = obj->getLocation();

    if (!string_ok(location))
        throw std::runtime_error("tried to check refID without a location set");

    if (refID > 0) {
        try {
            auto refObj = loadObject(refID);
            if (refObj != nullptr && refObj->getLocation() == location)
                return refObj;
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("illegal refID was set");
        }
    }

    // This should never happen - but fail softly
    // It means that something doesn't set the refID correctly
    log_warning("Failed to loadObject with refid: {}", refID);

    return findObjectByPath(location);
}

std::vector<std::shared_ptr<SQLStorage::AddUpdateTable>> SQLStorage::_addUpdateObject(const std::shared_ptr<CdsObject>& obj, bool isUpdate, int* changedContainer)
{
    int objectType = obj->getObjectType();
    std::shared_ptr<CdsObject> refObj = nullptr;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef) {
        if (IS_CDS_PURE_ITEM(objectType))
            throw std::runtime_error("tried to add pure item with PLAYLIST_REF flag set");
        if (obj->getRefID() <= 0)
            throw std::runtime_error("PLAYLIST_REF flag set but refId is <=0");
        refObj = loadObject(obj->getRefID());
        if (refObj == nullptr)
            throw std::runtime_error("PLAYLIST_REF flag set but refId doesn't point to an existing object");
    } else if (obj->isVirtual() && IS_CDS_PURE_ITEM(objectType)) {
        hasReference = true;
        refObj = checkRefID(obj);
        if (refObj == nullptr)
            throw std::runtime_error("tried to add or update a virtual object with illegal reference id and an illegal location");
    } else if (obj->getRefID() > 0) {
        if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            hasReference = true;
            refObj = loadObject(obj->getRefID());
            if (refObj == nullptr)
                throw std::runtime_error("OBJECT_FLAG_ONLINE_SERVICE and refID set but refID doesn't point to an existing object");
        } else if (IS_CDS_CONTAINER(objectType)) {
            // in this case it's a playlist-container. that's ok
            // we don't need to do anything
        } else
            throw std::runtime_error("refId set, but it makes no sense");
    }

    std::map<std::string,std::string> cdsObjectSql;
    cdsObjectSql["object_type"] = quote(objectType);

    if (hasReference || playlistRef)
        cdsObjectSql["ref_id"] = quote(refObj->getID());
    else if (isUpdate)
        cdsObjectSql["ref_id"] = SQL_NULL;

    if (!hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql["upnp_class"] = quote(obj->getClass());
    else if (isUpdate)
        cdsObjectSql["upnp_class"] = SQL_NULL;

    //if (!hasReference || refObj->getTitle() != obj->getTitle())
    cdsObjectSql["dc_title"] = quote(obj->getTitle());
    //else if (isUpdate)
    //    cdsObjectSql["dc_title"] = SQL_NULL;


    auto dict = obj->getMetadata();
    
    if (isUpdate)
        cdsObjectSql["auxdata"] = SQL_NULL;
    dict = obj->getAuxData();
    if (!dict.empty() && (!hasReference || !std::equal(dict.begin(), dict.end(), refObj->getAuxData().begin()))) {
        cdsObjectSql["auxdata"] = quote(dict_encode(obj->getAuxData()));
    }

    if (!hasReference || (!obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF) && !refObj->resourcesEqual(obj))) {
        // encode resources
        std::ostringstream resBuf;
        for (int i = 0; i < obj->getResourceCount(); i++) {
            if (i > 0)
                resBuf << RESOURCE_SEP;
            resBuf << obj->getResource(i)->encode();
        }
        std::string resStr = resBuf.str();
        if (string_ok(resStr))
            cdsObjectSql["resources"] = quote(resStr);
        else
            cdsObjectSql["resources"] = SQL_NULL;
    } else if (isUpdate)
        cdsObjectSql["resources"] = SQL_NULL;

    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);

    cdsObjectSql["flags"] = quote(obj->getFlags());

    if (IS_CDS_CONTAINER(objectType)) {
        if (!(isUpdate && obj->isVirtual()))
            throw std::runtime_error("tried to add a container or tried to update a non-virtual container via _addUpdateObject; is this correct?");
        std::string dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, obj->getLocation());
        cdsObjectSql["location"] = quote(dbLocation);
        cdsObjectSql["location_hash"] = quote(stringHash(dbLocation));
    }

    if (IS_CDS_ITEM(objectType)) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (!hasReference) {
            fs::path loc = item->getLocation();
            if (!string_ok(loc))
                throw std::runtime_error("tried to create or update a non-referenced item without a location set");
            if (IS_CDS_PURE_ITEM(objectType)) {
                int parentID = ensurePathExistence(loc.parent_path(), changedContainer);
                item->setParentID(parentID);
                std::string dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                cdsObjectSql["location"] = quote(dbLocation);
                cdsObjectSql["location_hash"] = quote(stringHash(dbLocation));
            } else {
                // URLs and active items
                cdsObjectSql["location"] = quote(loc);
                cdsObjectSql["location_hash"] = SQL_NULL;
            }
        } else {
            if (isUpdate) {
                cdsObjectSql["location"] = SQL_NULL;
                cdsObjectSql["location_hash"] = SQL_NULL;
            }
        }

        if (item->getTrackNumber() > 0) {
            cdsObjectSql["track_number"] = quote(item->getTrackNumber());
        } else {
            if (isUpdate)
                cdsObjectSql["track_number"] = SQL_NULL;
        }

        if (string_ok(item->getServiceID())) {
            if (!hasReference || std::static_pointer_cast<CdsItem>(refObj)->getServiceID() != item->getServiceID())
                cdsObjectSql["service_id"] = quote(item->getServiceID());
            else
                cdsObjectSql["service_id"] = SQL_NULL;
        } else {
            if (isUpdate)
                cdsObjectSql["service_id"] = SQL_NULL;
        }

        cdsObjectSql["mime_type"] = quote(item->getMimeType());
    }

    std::vector<std::shared_ptr<SQLStorage::AddUpdateTable>> returnVal;

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
        auto res = select(q);
        // if duplicate items is found - ignore
        if (res != nullptr && (res->nextRow() != nullptr))
            return returnVal;
    }

    if (obj->getParentID() == INVALID_OBJECT_ID)
        throw std::runtime_error("tried to create or update an object with an illegal parent id");
    cdsObjectSql["parent_id"] = std::to_string(obj->getParentID());

    returnVal.push_back(
        std::make_shared<AddUpdateTable>(CDS_OBJECT_TABLE, cdsObjectSql, isUpdate ? "update" : "insert")
    );

    if (!hasReference || !std::equal(dict.begin(), dict.end(), refObj->getMetadata().begin())) {
        generateMetadataDBOperations(obj, isUpdate, returnVal);
    }

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        std::map<std::string,std::string> cdsActiveItemSql;
        auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);

        cdsActiveItemSql["id"] = std::to_string(aitem->getID());
        cdsActiveItemSql["action"] = quote(aitem->getAction());
        cdsActiveItemSql["state"] = quote(aitem->getState());

        returnVal.push_back(
            std::make_shared<AddUpdateTable>(CDS_ACTIVE_ITEM_TABLE, cdsActiveItemSql, isUpdate ? "update" : "insert")
        );
    }

    return returnVal;
}

void SQLStorage::addObject(std::shared_ptr<CdsObject> obj, int* changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw std::runtime_error("tried to add an object with an object ID set");
    //obj->setID(INVALID_OBJECT_ID);
    auto data = _addUpdateObject(obj, false, changedContainer);

    // int lastInsertID = INVALID_OBJECT_ID;
    // int lastMetadataInsertID = INVALID_OBJECT_ID;
    for (const auto& addUpdateTable : data) {
        std::shared_ptr<std::ostringstream> qb = sqlForInsert(obj, addUpdateTable);
        log_debug("insert_query: {}", qb->str().c_str());
        exec(*qb);
    }
}

void SQLStorage::updateObject(std::shared_ptr<CdsObject> obj, int* changedContainer)
{
    std::vector<std::shared_ptr<AddUpdateTable>> data;
    if (obj->getID() == CDS_ID_FS_ROOT) {
        std::map<std::string,std::string> cdsObjectSql;

        cdsObjectSql["dc_title"] = quote(obj->getTitle());
        setFsRootName(obj->getTitle());
        cdsObjectSql["upnp_class"] = quote(obj->getClass());

        data.push_back(std::make_shared<AddUpdateTable>(CDS_OBJECT_TABLE, cdsObjectSql, "update"));
    } else {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw std::runtime_error("tried to update an object with a forbidden ID (" + std::to_string(obj->getID()) + ")!");
        data = _addUpdateObject(obj, true, changedContainer);
    }
    for (const auto& addUpdateTable : data) {
        std::string operation = addUpdateTable->getOperation();
        std::unique_ptr<std::ostringstream> qb;
        if (operation == "update") {
            qb = sqlForUpdate(obj, addUpdateTable);
        } else if (operation == "insert") {
            qb = sqlForInsert(obj, addUpdateTable);
        } else if (operation == "delete") {
            qb = sqlForDelete(obj, addUpdateTable);
        }

        log_debug("upd_query: {}", qb->str().c_str());
        exec(*qb);
    }
}

std::shared_ptr<CdsObject> SQLStorage::loadObject(int objectID)
{
     std::ostringstream qb;
    //log_debug("sql_query = {}",sql_query.c_str());

    qb << SQL_QUERY << " WHERE " << TQD('f', "id") << '=' << objectID;

    auto res = select(qb);
    std::unique_ptr<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return createObjectFromRow(row);
    }
    throw ObjectNotFoundException("Object not found: " + std::to_string(objectID));
}

std::shared_ptr<CdsObject> SQLStorage::loadObjectByServiceID(std::string serviceID)
{
    std::ostringstream qb;
    qb << SQL_QUERY << " WHERE " << TQD('f', "service_id") << '=' << quote(serviceID);
    auto res = select(qb);
    std::unique_ptr<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return createObjectFromRow(row);
    }

    return nullptr;
}

std::unique_ptr<std::vector<int>> SQLStorage::getServiceObjectIDs(char servicePrefix)
{
    auto objectIDs = std::make_unique<std::vector<int>>();

    std::ostringstream qb;
    qb << "SELECT " << TQ("id")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("service_id")
        << " LIKE " << quote(std::string(1, servicePrefix) + '%');

    auto res = select(qb);
    if (res == nullptr)
        throw std::runtime_error("db error");

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        objectIDs->push_back(std::stoi(row->col(0)));
    }

    return objectIDs;
}

std::vector<std::shared_ptr<CdsObject>> SQLStorage::browse(const std::unique_ptr<BrowseParam>& param)
{
    int objectID;
    int objectType = 0;

    bool getContainers = param->getFlag(BROWSE_CONTAINERS);
    bool getItems = param->getFlag(BROWSE_ITEMS);

    objectID = param->getObjectID();

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

    bool haveObjectType = false;

    if (!haveObjectType) {
        std::ostringstream qb;
        qb << "SELECT " << TQ("object_type")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << '=' << objectID;
        res = select(qb);
        if (res != nullptr && (row = res->nextRow()) != nullptr) {
            objectType = std::stoi(row->col(0));
            haveObjectType = true;
        } else {
            throw ObjectNotFoundException("Object not found: " + std::to_string(objectID));
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
    log_debug("QUERY: {}", qb.str().c_str());
    res = select(qb);

    std::vector<std::shared_ptr<CdsObject>> arr;

    while ((row = res->nextRow()) != nullptr) {
        auto obj = createObjectFromRow(row);
        arr.push_back(obj);
        row = nullptr;
    }

    row = nullptr;
    res = nullptr;

    // update childCount fields
    for (const auto& obj : arr) {
        if (IS_CDS_CONTAINER(obj->getObjectType())) {
            auto cont = std::static_pointer_cast<CdsContainer>(obj);
            cont->setChildCount(getChildCount(cont->getID(), getContainers, getItems, hideFsRoot));
        }
    }

    return arr;
}

std::vector<std::shared_ptr<CdsObject>> SQLStorage::search(const std::unique_ptr<SearchParam>& param, int* numMatches)
{
    std::unique_ptr<SearchParser> searchParser = std::make_unique<SearchParser>(*sqlEmitter, param->searchCriteria());
    std::shared_ptr<ASTNode> rootNode = searchParser->parse();
    std::string searchSQL(rootNode->emitSQL());
    if (!searchSQL.length())
        throw std::runtime_error("failed to generate SQL for search");

    std::ostringstream countSQL;
    countSQL << "select count(*) " << searchSQL << ';';
    auto sqlResult = select(countSQL);
    std::unique_ptr<SQLRow> countRow = sqlResult->nextRow();
    if (countRow != nullptr) {
        *numMatches = std::stoi(countRow->col(0));
    }
    
    std::ostringstream retrievalSQL;
    retrievalSQL << SELECT_DATA_FOR_SEARCH << " " << searchSQL;
    int startingIndex = param->getStartingIndex(), requestedCount = param->getRequestedCount();
    if (startingIndex > 0 || requestedCount > 0) {
        retrievalSQL << " order by c.id"
                     << " limit " << (requestedCount == 0 ? 10000000000 : requestedCount)
                     << " offset " << startingIndex;
    }
    retrievalSQL << ';';

    log_debug("Search resolves to SQL [{}]", retrievalSQL.str().c_str());
    sqlResult = select(retrievalSQL);

    std::vector<std::shared_ptr<CdsObject>> arr;

    std::unique_ptr<SQLRow> sqlRow;
    while ((sqlRow = sqlResult->nextRow()) != nullptr) {
        auto obj = createObjectFromSearchRow(sqlRow);
        arr.push_back(obj);
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
    auto res = select(qb);

    std::unique_ptr<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        int childCount = std::stoi(row->col(0));
        return childCount;
    }
    return 0;
}

std::vector<std::string> SQLStorage::getMimeTypes()
{
    std::vector<std::string> arr;

    std::ostringstream qb;
    qb << "SELECT DISTINCT " << TQ("mime_type")
        << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("mime_type") << " IS NOT NULL ORDER BY "
        << TQ("mime_type");
    auto res = select(qb);
    if (res == nullptr)
        throw std::runtime_error("db error");

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        arr.push_back(std::string(row->col(0)));
    }

    return arr;
}

std::shared_ptr<CdsObject> SQLStorage::findObjectByPath(fs::path fullpath)
{
    std::string dbLocation;
    if (fs::is_regular_file(fullpath)) {
        dbLocation = addLocationPrefix(LOC_FILE_PREFIX, fullpath);
    } else
        dbLocation = addLocationPrefix(LOC_DIR_PREFIX, fullpath);

    std::ostringstream qb;
    qb << SQL_QUERY
        << " WHERE " << TQD('f', "location_hash") << '=' << quote(stringHash(dbLocation))
        << " AND " << TQD('f', "location") << '=' << quote(dbLocation)
        << " AND " << TQD('f', "ref_id") << " IS NULL "
                                            "LIMIT 1";

    auto res = select(qb);
    if (res == nullptr)
        throw std::runtime_error("error while doing select: " + qb.str());

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;
    return createObjectFromRow(row);
}

int SQLStorage::findObjectIDByPath(fs::path fullpath)
{
    auto obj = findObjectByPath(fullpath);
    if (obj == nullptr)
        return INVALID_OBJECT_ID;
    return obj->getID();
}

int SQLStorage::ensurePathExistence(fs::path path, int* changedContainer)
{
    *changedContainer = INVALID_OBJECT_ID;
    if (path == std::string(1, DIR_SEPARATOR))
        return CDS_ID_FS_ROOT;

    auto obj = findObjectByPath(path);
    if (obj != nullptr)
        return obj->getID();

    int parentID = ensurePathExistence(path.parent_path(), changedContainer);

    auto f2i = StringConverter::f2i(config);
    if (changedContainer != nullptr && *changedContainer == INVALID_OBJECT_ID)
        *changedContainer = parentID;

    return createContainer(parentID, f2i->convert(path.filename()), path, false, "", INVALID_OBJECT_ID, std::map<std::string,std::string>());
}

int SQLStorage::createContainer(int parentID, std::string name, const std::string& virtualPath, bool isVirtual, const std::string& upnpClass, int refID, const std::map<std::string, std::string>& itemMetadata)
{
    // log_debug("Creating Container: parent: {}, name: {}, path {}, isVirt: {}, upnpClass: {}, refId: {}",
    // parentID, name.c_str(), path.c_str(), isVirtual, upnpClass.c_str(), refID);
    if (refID > 0) {
        auto refObj = loadObject(refID);
        if (refObj == nullptr)
            throw std::runtime_error("tried to create container with refID set, but refID doesn't point to an existing object");
    }
    std::string dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), virtualPath);

    /*std::map<std::string,std::string> metadata;
    if (!itemMetadata.empty()) {
        if (upnpClass == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            metadata["artist"] = getValueOrDefault(itemMetadata, "artist");
            metadata["date"] = getValueOrDefault(itemMetadata, "date");
        }
    }*/

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
       << (string_ok(upnpClass) ? quote(upnpClass) : quote(UPNP_DEFAULT_CLASS_CONTAINER)) << ','
       << quote(std::move(name)) << ','
       << quote(dbLocation) << ','
       << quote(stringHash(dbLocation)) << ',';
    if (refID > 0) {
        qb << refID;
    } else {
        qb << SQL_NULL;
    }
        qb << ')';

    exec(qb);

    if (!itemMetadata.empty()) {
        for (const auto& it : itemMetadata) {
            int newMetadataID = getNextMetadataID();
            std::ostringstream ib;
            ib << "INSERT INTO"
               << TQ(METADATA_TABLE)
               << " ("
               << TQ("id") << ','
               << TQ("item_id") << ','
               << TQ("property_name") << ','
               << TQ("property_value") << ") VALUES ("
               << newMetadataID << ','
               << newID << ","
               << quote(it.first) << ","
               << quote(it.second)
               << ")";
            exec(ib);
        }
        log_debug("Wrote metadata for cds_object {}", newID);
    }

    return newID;
}

fs::path SQLStorage::buildContainerPath(int parentID, std::string title)
{
    //title = escape(title, xxx);
    if (parentID == CDS_ID_ROOT)
        return std::string(1, VIRTUAL_CONTAINER_SEPARATOR) + title;

    std::ostringstream qb;
    qb << "SELECT " << TQ("location") << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << '=' << parentID << " LIMIT 1";

    auto res = select(qb);
    if (res == nullptr)
        return "";

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return "";

    char prefix;
    fs::path path = stripLocationPrefix(row->col(0) + VIRTUAL_CONTAINER_SEPARATOR + title, &prefix);
    if (prefix != LOC_VIRT_PREFIX)
        throw std::runtime_error("tried to build a virtual container path with an non-virtual parentID");

    return path;
}

void SQLStorage::addContainerChain(std::string virtualPath, std::string lastClass, int lastRefID, int* containerID, int* updateID, const std::map<std::string,std::string>& lastMetadata)
{
    log_debug("Adding container Chain for path: {}, lastRefId: {}, containerId: {}", virtualPath.c_str(), lastRefID, *containerID);

    virtualPath = reduce_string(virtualPath, VIRTUAL_CONTAINER_SEPARATOR);
    if (virtualPath == std::string(1, VIRTUAL_CONTAINER_SEPARATOR)) {
        *containerID = CDS_ID_ROOT;
        return;
    }
    std::ostringstream qb;
    std::string dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, virtualPath);
    qb << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE)
        << " WHERE " << TQ("location_hash") << '=' << quote(stringHash(dbLocation))
        << " AND " << TQ("location") << '=' << quote(dbLocation)
        << " LIMIT 1";

    auto res = select(qb);
    if (res != nullptr) {
        std::unique_ptr<SQLRow> row = res->nextRow();
        if (row != nullptr) {
            if (containerID != nullptr)
                *containerID = std::stoi(row->col(0));
            return;
        }
    }

    int parentContainerID;
    std::string newpath, container;
    stripAndUnescapeVirtualContainerFromPath(virtualPath, newpath, container);

    addContainerChain(newpath, "", INVALID_OBJECT_ID, &parentContainerID, updateID, std::map<std::string,std::string>());
    if (updateID != nullptr && *updateID == INVALID_OBJECT_ID)
        *updateID = parentContainerID;
    *containerID = createContainer(parentContainerID, container, virtualPath, true, lastClass, lastRefID, lastMetadata);
}

std::string SQLStorage::addLocationPrefix(char prefix, const std::string& path)
{
    return std::string(1, prefix) + path;
}

fs::path SQLStorage::stripLocationPrefix(std::string dbLocation, char* prefix)
{
    if (dbLocation.empty()) {
        if (prefix) *prefix = LOC_ILLEGAL_PREFIX;
        return "";
    }
    if (prefix) *prefix = dbLocation.at(0);
    return dbLocation.substr(1);
}

std::shared_ptr<CdsObject> SQLStorage::createObjectFromRow(const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(row->col(_object_type));
    auto self = getSelf();
    auto obj = CdsObject::createObject(self, objectType);

    /* set common properties */
    obj->setID(std::stoi(row->col(_id)));
    obj->setRefID(stoi_string(row->col(_ref_id)));

    obj->setParentID(std::stoi(row->col(_parent_id)));
    obj->setTitle(row->col(_dc_title));
    obj->setClass(fallbackString(row->col(_upnp_class), row->col(_ref_upnp_class)));
    obj->setFlags(std::stoi(row->col(_flags)));

    auto meta = retrieveMetadataForObject(obj->getID());
    if (!meta.empty())
        obj->setMetadata(meta);
    else {
        meta = retrieveMetadataForObject(obj->getRefID());
        if (!meta.empty())
            obj->setMetadata(meta);
    }
    if (meta.empty()) {
        // fallback to metadata that might be in mt_cds_object, which
        // will be useful if retrieving for schema upgrade
        std::string metadataStr = row->col(_metadata);
        dict_decode(metadataStr, &meta);
        obj->setMetadata(meta);
    }

    std::string auxdataStr = fallbackString(row->col(_auxdata), row->col(_ref_auxdata));
    std::map<std::string,std::string> aux;
    dict_decode(auxdataStr, &aux);
    obj->setAuxData(aux);

    std::string resources_str = fallbackString(row->col(_resources), row->col(_ref_resources));
    bool resource_zero_ok = false;
    if (string_ok(resources_str)) {
        std::vector<std::string> resources = split_string(resources_str,
            RESOURCE_SEP);
        for (size_t i = 0; i < resources.size(); i++) {
            if (i == 0)
                resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources[i]));
        }
    }

    if ((obj->getRefID() && IS_CDS_PURE_ITEM(objectType)) || (IS_CDS_ITEM(objectType) && !IS_CDS_PURE_ITEM(objectType)))
        obj->setVirtual(true);
    else
        obj->setVirtual(false); // gets set to true for virtual containers below

    int matched_types = 0;

    if (IS_CDS_CONTAINER(objectType)) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        cont->setUpdateID(std::stoi(row->col(_update_id)));
        char locationPrefix;
        cont->setLocation(stripLocationPrefix(row->col(_location), &locationPrefix));
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);

        std::string autoscanPersistent = row->col(_as_persistent);
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
            throw std::runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
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

        item->setTrackNumber(stoi_string(row->col(_track_number)));

        if (string_ok(row->col(_ref_service_id)))
            item->setServiceID(row->col(_ref_service_id));
        else
            item->setServiceID(row->col(_service_id));

        matched_types++;
    }

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);

        std::ostringstream query;
        query << "SELECT " << TQ("id") << ',' << TQ("action") << ','
               << TQ("state") << " FROM " << TQ(CDS_ACTIVE_ITEM_TABLE)
               << " WHERE " << TQ("id") << '=' << quote(aitem->getID());
        auto resAI = select(query);

        std::unique_ptr<SQLRow> rowAI;
        if (resAI != nullptr && (rowAI = resAI->nextRow()) != nullptr) {
            aitem->setAction(rowAI->col(1));
            aitem->setState(rowAI->col(2));
        } else
            throw std::runtime_error("Active Item in cds_objects, but not in cds_active_item");

        matched_types++;
    }

    if (!matched_types) {
        throw StorageException("", "unknown object type: " + std::to_string(objectType));
    }

    return obj;
}

std::shared_ptr<CdsObject> SQLStorage::createObjectFromSearchRow(const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(row->col(_object_type));
    auto self = getSelf();
    auto obj = CdsObject::createObject(self, objectType);

    /* set common properties */
    obj->setID(std::stoi(row->col(SearchCol::id)));
    obj->setRefID(std::stoi(row->col(SearchCol::ref_id)));

    obj->setParentID(std::stoi(row->col(SearchCol::parent_id)));
    obj->setTitle(row->col(SearchCol::dc_title));
    obj->setClass(row->col(SearchCol::upnp_class));

    auto meta = retrieveMetadataForObject(obj->getID());
    if (!meta.empty())
        obj->setMetadata(meta);

    std::string resources_str = row->col(SearchCol::resources);
    bool resource_zero_ok = false;
    if (string_ok(resources_str)) {
        std::vector<std::string> resources = split_string(resources_str, RESOURCE_SEP);
        for (size_t i = 0; i < resources.size(); i++) {
            if (i == 0)
                resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources[i]));
        }
    }

    if (IS_CDS_ITEM(objectType)) {
        if (!resource_zero_ok)
            throw std::runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(row->col(SearchCol::mime_type));
        if (IS_CDS_PURE_ITEM(objectType)) {
            item->setLocation(stripLocationPrefix(row->col(SearchCol::location)));
        } else { // URLs and active items
            item->setLocation(row->col(SearchCol::location));
        }

        item->setTrackNumber(std::stoi(row->col(SearchCol::track_number)));
    } else {
        throw StorageException("", "unknown object type: " + std::to_string(objectType));
    }

    return obj;
}

std::map<std::string,std::string> SQLStorage::retrieveMetadataForObject(int objectId)
{
    std::ostringstream qb;
    qb << SELECT_METADATA
        << " FROM " << TQ(METADATA_TABLE)
        << " WHERE " << TQ("item_id")
        << " = " << objectId;
    auto res = select(qb);

    std::map<std::string,std::string> metadata;
    if (res == nullptr)
        return metadata;

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        metadata[row->col(m_property_name)] = row->col(m_property_value);
    }
    return metadata;
}


int SQLStorage::getTotalFiles()
{
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE "
           << TQ("object_type") << " != " << quote(OBJECT_TYPE_CONTAINER);
    //<< " AND is_virtual = 0";
    auto res = select(query);

    std::unique_ptr<SQLRow> row;
    if (res != nullptr && (row = res->nextRow()) != nullptr) {
        return std::stoi(row->col(0));
    }
    return 0;
}

std::string SQLStorage::incrementUpdateIDs(const unique_ptr<unordered_set<int>>& ids)
{
    if (ids->empty())
        return "";
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
    auto res = select(bufSelect);
    if (res == nullptr)
        throw std::runtime_error("Error while fetching update ids");

    std::unique_ptr<SQLRow> row;
    std::list<std::string> rows;
    while ((row = res->nextRow()) != nullptr) {
        std::ostringstream s;
        s << row->col(0) << ',' << row->col(1);
        rows.emplace_back(s.str());
    }
    if (rows.empty())
        return "";
    return join(rows, ",");
}

// id is the parent_id for cover media to find, and if set, trackArtBase is the case-folded
// name of the track to try as artwork; we rely on LIKE being case-insensitive

// This limit massively improves performance, but is not currently usable on MySql and MariaDB, apparently
// Since the actual driver in use is determined at runtime, not build time, we check for sqlite3 and
// only run when that's what we are using.
#define MAX_ART_CONTAINERS 100

std::string SQLStorage::findFolderImage(int id, std::string trackArtBase)
{
    std::ostringstream q;
    // folder.jpg or cover.jpg [and variants]
    // note - "_" is regexp "." and "%" is regexp ".*" in sql LIKE land
    q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << "( ";
    if (trackArtBase.length() > 0) {
        q << TQ("dc_title") << "LIKE " << quote(trackArtBase + ".jp%") << " OR ";
    }
    q << TQ("dc_title") << "LIKE " << quote("cover.jp%") << " OR ";
    q << TQ("dc_title") << "LIKE " << quote("albumart%.jp%") << " OR ";
    q << TQ("dc_title") << "LIKE " << quote("album.jp%") << " OR ";
    q << TQ("dc_title") << "LIKE " << quote("front.jp%") << " OR ";
    q << TQ("dc_title") << "LIKE " << quote("folder.jp%");
    q << " ) AND ";
    q << TQ("upnp_class") << '=' << quote(UPNP_DEFAULT_CLASS_IMAGE_ITEM) << " AND ";
#ifndef ONLY_REAL_FOLDER_ART
    q << "( ";
    q <<       "( ";
        // virtual listing via Album, Artist etc
    q << TQ("parent_id") << " IN ";
    q <<       "(";
    q << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << TQ("upnp_class") << '=' << quote(UPNP_DEFAULT_CLASS_MUSIC_TRACK) << " AND ";
    q << TQ("id") << " IN ";
    q <<               "(";
    q << "SELECT " << TQ("ref_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    q << TQ("parent_id") << '=' << quote(std::to_string(id)) << " AND ";
    q << TQ("object_type") << '=' << quote(OBJECT_TYPE_ITEM);

    // only use this optimization on sqlite3
    if (config->getOption(CFG_SERVER_STORAGE_DRIVER) == "sqlite3") {
        q <<               " LIMIT " << MAX_ART_CONTAINERS << ")";
    } else {
        q <<               ")";
    }
    q <<       ")";
    q << "     ) OR ";
#endif
        // straightforward folder listing of real filesystem
    q << TQ("parent_id") << '=' << quote(std::to_string(id));
#ifndef ONLY_REAL_FOLDER_ART
    q << ")";
#endif
    q << " LIMIT 1";

    //log_debug("findFolderImage {}, {}", id, q->c_str());
    auto res = select(q);
    if (res == nullptr)
        throw std::runtime_error("db error");

    std::unique_ptr<SQLRow> row;
    if ((row = res->nextRow()) != nullptr) // we only care about the first result
    {
        log_debug("findFolderImage result: {}", row->col(0).c_str());
        return row->col(0);
    }
    return "";
}


unique_ptr<unordered_set<int>> SQLStorage::getObjects(int parentID, bool withoutContainer)
{
    std::ostringstream q;
    q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    if (withoutContainer)
        q << TQ("object_type") << " != " << OBJECT_TYPE_CONTAINER << " AND ";
    q << TQ("parent_id") << '=';
    q << parentID;
    auto res = select(q);
    if (res == nullptr)
        throw std::runtime_error("db error");
    if (res->getNumRows() <= 0)
        return nullptr;

    auto ret = make_unique<unordered_set<int>>();
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        ret->insert(std::stoi(row->col(0)));
    }
    return ret;
}

std::unique_ptr<Storage::ChangedContainers> SQLStorage::removeObjects(const unique_ptr<unordered_set<int>>& list, bool all)
{
    int count = list->size();
    if (count <= 0)
        return nullptr;

    for (const auto& id : *list) {
        if (IS_FORBIDDEN_CDS_ID(id))
            throw std::runtime_error("tried to delete a forbidden ID (" + std::to_string(id) + ")!");
    }

    std::ostringstream idsBuf;
    idsBuf << "SELECT " << TQ("id") << ',' << TQ("object_type")
            << " FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id") << " IN (" << join(*list, ",") << ")";

    auto res = select(idsBuf);
    if (res == nullptr)
        throw std::runtime_error("sql error");

    std::vector<int32_t> items;
    std::vector<int32_t> containers;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        int objectType = std::stoi(row->col(1));
        if (IS_CDS_CONTAINER(objectType))
            containers.push_back(std::stol(row->col(0)));
        else
            items.push_back(std::stol(row->col(0)));
    }

    auto rr = _recursiveRemove(items, containers, all);
    return _purgeEmptyContainers(rr);
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

    log_debug("{}", sel.str().c_str());

    auto res = select(sel);
    if (res != nullptr) {
        log_debug("relevant autoscans!");
        std::vector<std::string> delete_as;
        std::unique_ptr<SQLRow> row;
        while ((row = res->nextRow()) != nullptr) {
            bool persistent = remapBool(row->col(1));
            if (persistent) {
                std::string location = stripLocationPrefix(row->col(2));
                std::ostringstream u;
                u << "UPDATE " << TQ(AUTOSCAN_TABLE)
                  << " SET " << TQ("obj_id") << "=" SQL_NULL
                  << ',' << TQ("location") << '=' << quote(location)
                  << " WHERE " << TQ("id") << '=' << quote(row->col(0));
                exec(u);
            } else {
                delete_as.emplace_back(row->col_c_str(0));
            }
            log_debug("relevant autoscan: {}; persistent: {}", row->col_c_str(0), persistent);
        }

        if (!delete_as.empty()) {
            std::ostringstream delAutoscan;
            delAutoscan << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
                        << " WHERE " << TQ("id") << " IN ("
                        << join(delete_as, ',')
                        << ')';
            exec(delAutoscan);
            log_debug("deleting autoscans: {}", delAutoscan.str().c_str());
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

std::unique_ptr<Storage::ChangedContainers> SQLStorage::removeObject(int objectID, bool all)
{
    std::ostringstream q;
    q << "SELECT " << TQ("object_type") << ',' << TQ("ref_id")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("id") << '=' << quote(objectID) << " LIMIT 1";
    auto res = select(q);
    if (res == nullptr)
        return nullptr;

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;

    int objectType = std::stoi(row->col(0));
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && !isContainer) {
        std::string ref_id_str = row->col(1);
        int ref_id;
        if (string_ok(ref_id_str)) {
            ref_id = std::stoi(ref_id_str);
            if (!IS_FORBIDDEN_CDS_ID(ref_id))
                objectID = ref_id;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw std::runtime_error("tried to delete a forbidden ID (" + std::to_string(objectID) + ")!");
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

std::unique_ptr<Storage::ChangedContainers> SQLStorage::_recursiveRemove(
    const std::vector<int32_t> &items, const std::vector<int32_t> &containers,
    bool all)
{
    log_debug("start");
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

    auto changedContainers = std::make_unique<ChangedContainers>();

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

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
            throw StorageException("", "sql error");
        parentIds.clear();
        while ((row = res->nextRow()) != nullptr) {
            changedContainers->ui.push_back(std::stoi(row->col(0)));
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
                throw StorageException("", std::string("sql error: ") + sql.str());
            parentIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                changedContainers->upnp.push_back(std::stoi(row->col(0)));
            }
        }

        if (!itemIds.empty()) {
            std::ostringstream sql;
            sql << itemsSql.str() << join(itemIds, ',') << ')';
            res = select(sql);
            if (res == nullptr)
                throw StorageException("", std::string("sql error: ") + sql.str());
            itemIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                removeIds.push_back(std::stoi(row->col(0)));
                changedContainers->upnp.push_back(std::stoi(row->col(1)));
            }
        }

        if (!containerIds.empty()) {
            std::ostringstream sql;
            sql << containersSql.str() << join(containerIds, ',') << ')';
            res = select(sql);
            if (res == nullptr)
                throw StorageException("", std::string("sql error: ") + sql.str());
            containerIds.clear();
            while ((row = res->nextRow()) != nullptr) {
                int objectType = std::stoi(row->col(1));
                if (IS_CDS_CONTAINER(objectType)) {
                    containerIds.push_back(std::stoi(row->col(0)));
                    removeIds.push_back(std::stoi(row->col(0)));
                } else {
                    if (all) {
                        std::string refId = row->col(2);
                        if (string_ok(refId)) {
                            parentIds.push_back(std::stoi(row->col(2)));
                            itemIds.push_back(std::stoi(row->col(2)));
                            removeIds.push_back(std::stoi(row->col(2)));
                        } else {
                            removeIds.push_back(std::stoi(row->col(0)));
                            itemIds.push_back(std::stoi(row->col(0)));
                        }
                    } else {
                        removeIds.push_back(std::stoi(row->col(0)));
                        itemIds.push_back(std::stoi(row->col(0)));
                    }
                }
            }
        }

        if (removeIds.size() > MAX_REMOVE_SIZE) {
            _removeObjects(removeIds);
            removeIds.clear();
        }

        if (count++ > MAX_REMOVE_RECURSION)
            throw std::runtime_error("there seems to be an infinite loop...");
    }

    if (!removeIds.empty())
        _removeObjects(removeIds);
    log_debug("end");
    return changedContainers;
}

std::string SQLStorage::toCSV(const std::vector<int>& input)
{
    return join(input, ",");
}

std::unique_ptr<Storage::ChangedContainers> SQLStorage::_purgeEmptyContainers(std::unique_ptr<ChangedContainers>& maybeEmpty)
{
    log_debug("start upnp: {}; ui: {}",
            join(maybeEmpty->upnp, ',').c_str(),
            join(maybeEmpty->ui, ',').c_str());
    auto changedContainers = std::make_unique<ChangedContainers>();
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

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

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
            log_debug("upnp-sql: {}", sql.str().c_str());
            res = select(sql.str());
            selUpnp.clear();
            if (res == nullptr)
                throw std::runtime_error("db error");
            while ((row = res->nextRow()) != nullptr) {
                int flags = std::stoi(row->col(3));
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers->upnp.push_back(std::stoi(row->col(0)));
                else if (row->col(1) == "0") {
                    del.push_back(std::stoi(row->col(0)));
                    selUi.push_back(std::stoi(row->col(2)));
                } else {
                    selUpnp.push_back(std::stoi(row->col(0)));
                }
            }
        }

        if (!selUi.empty()) {
            std::ostringstream sql;
            sql << selectSql.str() << join(selUi, ',') << strSel2;
            log_debug("ui-sql: {}", sql.str().c_str());
            res = select(sql.str());
            selUi.clear();
            if (res == nullptr)
                throw std::runtime_error("db error");
            while ((row = res->nextRow()) != nullptr) {
                int flags = std::stoi(row->col(3));
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER) {
                    changedContainers->ui.push_back(std::stoi(row->col(0)));
                    changedContainers->upnp.push_back(std::stoi(row->col(0)));
                } else if (row->col(1) == "0") {
                    del.push_back(std::stoi(row->col(0)));
                    selUi.push_back(std::stoi(row->col(2)));
                } else {
                    selUi.push_back(std::stoi(row->col(0)));
                }
            }
        }

        //log_debug("selecting: {}; removing: {}", bufSel->c_str(), join(del, ',').c_str());
        if (!del.empty()) {
            _removeObjects(del);
            del.clear();
            if (!selUi.empty() || !selUpnp.empty())
                again = true;
        }
        if (count++ >= MAX_REMOVE_RECURSION)
            throw std::runtime_error("there seems to be an infinite loop...");
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
    // log_debug("end; changedContainers (upnp): {}", join(changedUpnp, ',').c_str());
    // log_debug("end; changedContainers (ui): {}", join(changedUi, ',').c_str());
    log_debug("end; changedContainers (upnp): {}", changedUpnp.size());
    log_debug("end; changedContainers (ui): {}", changedUi.size());

    return changedContainers;
}

std::string SQLStorage::getInternalSetting(std::string key)
{
    std::ostringstream q;
    q << "SELECT " << TQ("value") << " FROM " << TQ(INTERNAL_SETTINGS_TABLE) << " WHERE " << TQ("key") << '='
       << quote(key) << " LIMIT 1";
    auto res = select(q);
    if (res == nullptr)
        return "";

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return "";
    return row->col(0);
}

void SQLStorage::updateAutoscanPersistentList(ScanMode scanmode, std::shared_ptr<AutoscanList> list)
{

    log_debug("setting persistent autoscans untouched - scanmode: {};", AutoscanDirectory::mapScanmode(scanmode).c_str());
    std::ostringstream update;
    update << "UPDATE " << TQ(AUTOSCAN_TABLE)
       << " SET " << TQ("touched") << '=' << mapBool(false)
       << " WHERE "
       << TQ("persistent") << '=' << mapBool(true)
       << " AND " << TQ("scan_mode") << '='
       << quote(AutoscanDirectory::mapScanmode(scanmode));
    exec(update);

    int listSize = list->size();
    log_debug("updating/adding persistent autoscans (count: {})", listSize);
    for (int i = 0; i < listSize; i++) {
        log_debug("getting ad {} from list..", i);
        std::shared_ptr<AutoscanDirectory> ad = list->get(i);
        if (ad == nullptr)
            continue;

        // only persistent asD should be given to getAutoscanList
        assert(ad->persistent());
        // the scanmode should match the given parameter
        assert(ad->getScanMode() == scanmode);

        std::string location = ad->getLocation();
        if (!string_ok(location))
            throw std::runtime_error("AutoscanDirectoy with illegal location given to SQLStorage::updateAutoscanPersistentList");

        std::ostringstream q;
        q << "SELECT " << TQ("id") << " FROM " << TQ(AUTOSCAN_TABLE)
           << " WHERE ";
        int objectID = findObjectIDByPath(location);
        log_debug("objectID = {}", objectID);
        if (objectID == INVALID_OBJECT_ID)
            q << TQ("location") << '=' << quote(location);
        else
            q << TQ("obj_id") << '=' << quote(objectID);
        q << " LIMIT 1";
        auto res = select(q);
        if (res == nullptr)
            throw StorageException("", "query error while selecting from autoscan list");

        std::unique_ptr<SQLRow> row;
        if ((row = res->nextRow()) != nullptr) {
            ad->setStorageID(std::stoi(row->col(0)));
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

std::shared_ptr<AutoscanList> SQLStorage::getAutoscanList(ScanMode scanmode)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t', "id")
       << " WHERE " FLD("scan_mode") '=' << quote(AutoscanDirectory::mapScanmode(scanmode));
    auto res = select(q);
    if (res == nullptr)
        throw StorageException("", "query error while fetching autoscan list");

    auto self = getSelf();
    auto ret = std::make_shared<AutoscanList>(self);
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow()) != nullptr) {
        std::shared_ptr<AutoscanDirectory> dir = _fillAutoscanDirectory(row);
        if (dir == nullptr)
            removeAutoscanDirectory(std::stoi(row->col(0)));
        else
            ret->add(dir);
    }
    return ret;
}

std::shared_ptr<AutoscanDirectory> SQLStorage::getAutoscanDirectory(int objectID)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
       << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
       << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
       << " ON " FLD("obj_id") '=' << TQD('t', "id")
       << " WHERE " << TQD('t', "id") << '=' << quote(objectID);
    auto res = select(q);
    if (res == nullptr)
        throw StorageException("", "query error while fetching autoscan");

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        return nullptr;

    return _fillAutoscanDirectory(row);
}

std::shared_ptr<AutoscanDirectory> SQLStorage::_fillAutoscanDirectory(const std::unique_ptr<SQLRow>& row)
{
    int objectID = INVALID_OBJECT_ID;
    std::string objectIDstr = row->col(1);
    if (string_ok(objectIDstr))
        objectID = std::stoi(objectIDstr);
    int storageID = std::stoi(row->col(0));

    fs::path location;
    if (objectID == INVALID_OBJECT_ID) {
        location = row->col(9);
    } else {
        char prefix;
        location = stripLocationPrefix(row->col(10), &prefix);
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
        interval = std::stoi(row->col(6));
    time_t last_modified = std::stol(row->col(7));

    //log_debug("adding autoscan location: {}; recursive: {}", location.c_str(), recursive);

    auto dir = std::make_shared<AutoscanDirectory>(location, mode, level, recursive, persistent, INVALID_SCAN_ID, interval, hidden);
    dir->setObjectID(objectID);
    dir->setStorageID(storageID);
    dir->setCurrentLMT(last_modified);
    dir->updateLMT();
    return dir;
}

void SQLStorage::addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir)
{
    if (adir == nullptr)
        throw std::runtime_error("addAutoscanDirectory called with adir==nullptr");
    if (adir->getStorageID() >= 0)
        throw std::runtime_error("tried to add autoscan directory with a storage id set");
    int objectID;
    if (adir->getLocation() == FS_ROOT_DIRECTORY)
        objectID = CDS_ID_FS_ROOT;
    else
        objectID = findObjectIDByPath(adir->getLocation());
    if (!adir->persistent() && objectID < 0)
        throw std::runtime_error("tried to add non-persistent autoscan directory with an illegal objectID or location");

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
       << (objectID >= 0 ? quote(objectID) : SQL_NULL) << ','
       << quote(AutoscanDirectory::mapScanlevel(adir->getScanLevel())) << ','
       << quote(AutoscanDirectory::mapScanmode(adir->getScanMode())) << ','
       << mapBool(adir->getRecursive()) << ','
       << mapBool(adir->getHidden()) << ','
       << quote(adir->getInterval()) << ','
       << quote(adir->getPreviousLMT()) << ','
       << mapBool(adir->persistent()) << ','
       << (objectID >= 0 ? SQL_NULL : quote(adir->getLocation())) << ','
       << (pathIds == nullptr ? SQL_NULL : quote("," + toCSV(*pathIds) + ','))
       << ')';
    adir->setStorageID(exec(q, true));
}

void SQLStorage::updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir)
{
    log_debug("id: {}, obj_id: {}", adir->getStorageID(), adir->getObjectID());

    if (adir == nullptr)
        throw std::runtime_error("updateAutoscanDirectory called with adir==nullptr");

    auto pathIds = _checkOverlappingAutoscans(adir);

    int objectID = adir->getObjectID();
    int objectIDold = _getAutoscanObjectID(adir->getStorageID());
    if (objectIDold != objectID) {
        _autoscanChangePersistentFlag(objectIDold, false);
        _autoscanChangePersistentFlag(objectID, true);
    }
    std::ostringstream q;
    q << "UPDATE " << TQ(AUTOSCAN_TABLE)
       << " SET " << TQ("obj_id") << '=' << (objectID >= 0 ? quote(objectID) : SQL_NULL)
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
       << ',' << TQ("location") << '=' << (objectID >= 0 ? SQL_NULL : quote(adir->getLocation()))
       << ',' << TQ("path_ids") << '=' << (pathIds == nullptr ? SQL_NULL : quote("," + toCSV(*pathIds) + ','))
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
    return _getAutoscanDirectoryInfo(objectID, "persistent");
}

int SQLStorage::isAutoscanDirectoryRecursive(int objectID)
{
    return _getAutoscanDirectoryInfo(objectID, "recursive");
}

int SQLStorage::_getAutoscanDirectoryInfo(int objectID, const std::string& field)
{
    if (objectID == INVALID_OBJECT_ID)
        return 0;
    std::ostringstream q;
    q << "SELECT " << TQ(field) << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << '=' << quote(objectID);
    auto res = select(q);
    std::unique_ptr<SQLRow> row;
    if (res == nullptr || (row = res->nextRow()) == nullptr)
        return 0;

    if (!remapBool(row->col(0)))
        return 1;

    return 2;
}

int SQLStorage::_getAutoscanObjectID(int autoscanID)
{
    std::ostringstream q;
    q << "SELECT " << TQ("obj_id") << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("id") << '=' << quote(autoscanID)
       << " LIMIT 1";
    auto res = select(q);
    if (res == nullptr)
        throw StorageException("", "error while doing select on ");
    std::unique_ptr<SQLRow> row;
    if ((row = res->nextRow()) != nullptr && string_ok(row->col(0)))
        return std::stoi(row->col(0));
    return INVALID_OBJECT_ID;
}

void SQLStorage::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID || objectID == INVALID_OBJECT_ID_2)
        return;

    std::ostringstream q;
    q << "UPDATE " << TQ(CDS_OBJECT_TABLE)
       << " SET " << TQ("flags") << " = (" << TQ("flags")
       << (persistent ? " | " : " & ~")
       << OBJECT_FLAG_PERSISTENT_CONTAINER
       << ") WHERE " << TQ("id") << '=' << quote(objectID);
    exec(q);
}

void SQLStorage::autoscanUpdateLM(std::shared_ptr<AutoscanDirectory> adir)
{
    log_debug("id: {}; last_modified: {}", adir->getStorageID(), adir->getPreviousLMT());
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

void SQLStorage::checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir)
{
    _checkOverlappingAutoscans(adir);
}

std::unique_ptr<std::vector<int>> SQLStorage::_checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (adir == nullptr)
        throw std::runtime_error("_checkOverlappingAutoscans called with adir==nullptr");
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID)
        return nullptr;
    int storageID = adir->getStorageID();

    std::ostringstream q;
    q << "SELECT " << TQ("id")
       << " FROM " << TQ(AUTOSCAN_TABLE)
       << " WHERE " << TQ("obj_id") << " = "
       << quote(checkObjectID);
    if (storageID >= 0)
        q << " AND " << TQ("id") << " != " << quote(storageID);

    auto res = select(q);
    if (res == nullptr)
        throw std::runtime_error("SQL error");

    std::unique_ptr<SQLRow> row;
    if ((row = res->nextRow()) != nullptr) {
        auto obj = loadObject(checkObjectID);
        if (obj == nullptr)
            throw std::runtime_error("Referenced object (by Autoscan) not found.");
        log_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
        throw std::runtime_error("There is already an Autoscan set on " + obj->getLocation().string());
    }

    if (adir->getRecursive()) {
        std::ostringstream q;
        q << "SELECT " << TQ("obj_id")
           << " FROM " << TQ(AUTOSCAN_TABLE)
           << " WHERE " << TQ("path_ids") << " LIKE "
           << quote("%," + std::to_string(checkObjectID) + ",%");
        if (storageID >= 0)
            q << " AND " << TQ("id") << " != " << quote(storageID);
        q << " LIMIT 1";

        log_debug("------------ {}", q.str().c_str());

        res = select(q);
        if (res == nullptr)
            throw std::runtime_error("SQL error");
        if ((row = res->nextRow()) != nullptr) {
            int objectID = std::stoi(row->col(0));
            log_debug("-------------- {}", objectID);
            auto obj = loadObject(objectID);
            if (obj == nullptr)
                throw std::runtime_error("Referenced object (by Autoscan) not found.");
            log_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw std::runtime_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on " + obj->getLocation().string());
        }
    }

    auto pathIDs = getPathIDs(checkObjectID);
    if (pathIDs == nullptr)
        throw std::runtime_error("getPathIDs returned nullptr");
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
        throw std::runtime_error("SQL error");
    if ((row = res->nextRow()) == nullptr)
        return pathIDs;

    int objectID = std::stoi(row->col(0));
    auto obj = loadObject(objectID);
    if (obj == nullptr)
        throw std::runtime_error("Referenced object (by Autoscan) not found.");
    log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
    throw std::runtime_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on " + obj->getLocation().string());
}

std::unique_ptr<std::vector<int>> SQLStorage::getPathIDs(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return nullptr;

    auto pathIDs = make_unique<std::vector<int>>();

    std::ostringstream sel;
    sel << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    sel << TQ("id") << '=';
    
    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;
    while (objectID != CDS_ID_ROOT) {
        pathIDs->push_back(objectID);
        std::ostringstream q;
        q << sel.str() << quote(objectID) << " LIMIT 1";
        res = select(q);
        if (res == nullptr || (row = res->nextRow()) == nullptr)
            break;
        objectID = std::stoi(row->col(0));
    }
    return pathIDs;
}

std::string SQLStorage::getFsRootName()
{
    if (string_ok(fsRootName))
        return fsRootName;
    setFsRootName();
    return fsRootName;
}

void SQLStorage::setFsRootName(const std::string& rootName)
{
    if (string_ok(rootName)) {
        fsRootName = rootName;
    } else {
        auto fsRootObj = loadObject(CDS_ID_FS_ROOT);
        fsRootName = fsRootObj->getTitle();
    }
}

int SQLStorage::getNextID()
{
    log_debug("NextId: {}", lastID + 1);
    if (lastID < CDS_ID_FS_ROOT)
        throw std::runtime_error("SQLStorage::getNextID() called, but lastID hasn't been loaded correctly yet");
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
    auto res = select(qb);
    if (res == nullptr)
        throw std::runtime_error("could not load lastID (res==nullptr)");

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        throw std::runtime_error("could not load lastID (row==nullptr)");

    lastID = std::stoi(row->col(0));
    if (lastID < CDS_ID_FS_ROOT)
        throw std::runtime_error("could not load correct lastID (db not initialized?)");

    log_debug("LoadedId: {}", lastID);
}

int SQLStorage::getNextMetadataID()
{
    if (lastMetadataID < CDS_ID_ROOT)
        throw std::runtime_error("SQLStorage::getNextMetadataID() called, but lastMetadataID hasn't been loaded correctly yet");

    AutoLock lock(nextIDMutex);
    return ++lastMetadataID;
}

// if metadata table becomes first-class then should have a parameterized loadLastID()
// to be called for either mt_cds_object or mt_metadata
void SQLStorage::loadLastMetadataID()
{
    AutoLock lock(nextIDMutex);

    // we don't rely on automatic db generated ids, because of our caching
    std::ostringstream qb;
    qb << "SELECT MAX(" << TQ("id") << ')'
        << " FROM " << TQ(METADATA_TABLE);
    auto res = select(qb);
    if (res == nullptr)
        throw std::runtime_error("could not load lastMetadataID (res==nullptr)");

    std::unique_ptr<SQLRow> row = res->nextRow();
    if (row == nullptr)
        throw std::runtime_error("could not load lastMetadataID (row==nullptr)");

    lastMetadataID = stoi_string(row->col(0));
    if (lastMetadataID < CDS_ID_ROOT)
        throw std::runtime_error("could not load correct lastMetadataID (db not initialized?)");
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

void SQLStorage::generateMetadataDBOperations(const std::shared_ptr<CdsObject>& obj, bool isUpdate,
    std::vector<std::shared_ptr<AddUpdateTable>>& operations)
{
    auto dict = obj->getMetadata();
    if (!isUpdate) {
        for (const auto& it : dict) {
            std::map<std::string,std::string> metadataSql;
            metadataSql["property_name"] = quote(it.first);
            metadataSql["property_value"] = quote(it.second);
            operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, "insert"));
        }
    } else {
        // get current metadata from DB: if only it really was a dictionary...
        auto dbMetadata = retrieveMetadataForObject(obj->getID());
        for (const auto& it : dict) {
            std::string operation = dbMetadata.find(it.first) == dbMetadata.end() ? "insert" : "update";
            std::map<std::string,std::string> metadataSql;
            metadataSql["property_name"] = quote(it.first);
            metadataSql["property_value"] = quote(it.second);
            operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, operation));
        }
        for (const auto& it : dbMetadata) {
            if (dict.find(it.first) == dict.end()) {
                // key in db metadata but not obj metadata, so needs a delete
                std::map<std::string,std::string> metadataSql;
                metadataSql["property_name"] = quote(it.first);
                metadataSql["property_value"] = quote(it.second);
                operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, "delete"));
            }
        }
    }
}

std::unique_ptr<std::ostringstream> SQLStorage::sqlForInsert(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable)
{
    int lastInsertID = INVALID_OBJECT_ID;
    int lastMetadataInsertID = INVALID_OBJECT_ID;

    std::string tableName = addUpdateTable->getTable();
    auto dict = addUpdateTable->getDict();

    std::ostringstream fields;
    std::ostringstream values;

    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin()) {
            fields << ',';
            values << ',';
        }
        fields << TQ(it->first);
        if (lastInsertID != INVALID_OBJECT_ID && it->first == "id" && std::stoi(it->second) == INVALID_OBJECT_ID) {
            if (tableName == METADATA_TABLE)
                values << lastMetadataInsertID;
            else
                values << lastInsertID;
        } else
            values << it->second;
    }

    /* manually generate ID */
    if (lastInsertID == INVALID_OBJECT_ID && tableName == CDS_OBJECT_TABLE) {
        lastInsertID = getNextID();
        obj->setID(lastInsertID);
        fields << ',' << TQ("id");
        values << ',' << quote(lastInsertID);
    }
    if (tableName == METADATA_TABLE) {
        lastMetadataInsertID = getNextMetadataID();
        fields << ',' << TQ("id");
        values << ',' << quote(lastMetadataInsertID);
        fields << ',' << TQ("item_id");
        values << ',' << quote(obj->getID());
    }

    auto qb = std::make_unique<std::ostringstream>();
    *qb << "INSERT INTO " << TQ(tableName) << " (" << fields.str() << ") VALUES (" << values.str() << ')';

    return qb;
}

std::unique_ptr<std::ostringstream> SQLStorage::sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable)
{
    if (addUpdateTable == nullptr
        || (addUpdateTable->getTable() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw std::runtime_error("sqlForUpdate called with invalid arguments");

    std::string tableName = addUpdateTable->getTable();
    auto dict = addUpdateTable->getDict();

    auto qb = std::make_unique<std::ostringstream>();
    *qb << "UPDATE " << TQ(tableName) << " SET ";
    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin())
            *qb << ',';
        *qb << TQ(it->first) << '='
            << it->second;
    }
    *qb << " WHERE " << TQ("id") << " = " << obj->getID();

    // relying on only one element when table is mt_metadata
    if (tableName == METADATA_TABLE)
        *qb << " AND " << TQ("property_name") << " = " <<  dict.begin()->first;

    return qb;
}

std::unique_ptr<std::ostringstream> SQLStorage::sqlForDelete(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable)
{
    if (addUpdateTable == nullptr
        || (addUpdateTable->getTable() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw std::runtime_error("sqlForDelete called with invalid arguments");

    std::string tableName = addUpdateTable->getTable();
    auto dict = addUpdateTable->getDict();

    auto qb = std::make_unique<std::ostringstream>();
    *qb << "DELETE FROM " << TQ(tableName)
        << " WHERE " << TQ("id") << " = " << obj->getID();
    
    // relying on only one element when table is mt_metadata
    if (tableName == METADATA_TABLE)
        *qb << " AND " << TQ("property_name") << " = " <<  dict.begin()->first;
    
    return qb;
}

void SQLStorage::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required");
    std::ostringstream qbCountNotNull;
    qbCountNotNull << "SELECT COUNT(*)"
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("metadata")
       << " is not null";
    auto res = select(qbCountNotNull);
    int expectedConversionCount = std::stoi(res->nextRow()->col(0));
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    std::ostringstream qbCountMetadata;
    qbCountMetadata << "SELECT COUNT(*)"
       << " FROM " << TQ(METADATA_TABLE);
    res = select(qbCountMetadata);
    int metadataRowCount = std::stoi(res->nextRow()->col(0));
    log_debug("mt_metadata rows having metadata: {}", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required");
        return;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata");
    log_info("No data will be removed from mt_cds_object");
        
    std::ostringstream qbRetrieveIDs;
    qbRetrieveIDs << "SELECT " << TQ("id")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("metadata")
       << " is not null";
    auto resIds = select(qbRetrieveIDs);
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow()) != nullptr) {
        auto cdsObject = loadObject(std::stoi(row->col(0)));
        migrateMetadata(cdsObject);
        ++objectsUpdated;
    }
    log_info("Migrated metadata - object count: {}", objectsUpdated);
}

void SQLStorage::migrateMetadata(const std::shared_ptr<CdsObject>& object)
{
    if (object == nullptr)
        return;
 
    auto dict = object->getMetadata();
    if (!dict.empty()) {
        log_debug("Migrating metadata for cds object {}", object->getID());
        std::map<std::string,std::string> metadataSQLVals;
        for (auto& it : dict) {
            metadataSQLVals[quote(it.first)] = quote(it.second);
        }

        for (auto& metadataSQLVal : metadataSQLVals) {
            std::ostringstream fields, values;
            fields << TQ("id") << ','
                    << TQ("item_id") << ','
                    << TQ("property_name") << ','
                    << TQ("property_value");
            values << getNextMetadataID() << ','
                   << object->getID() << ','
                   << metadataSQLVal.first << ','
                   << metadataSQLVal.second;
            std::ostringstream qb;
            qb << "INSERT INTO " << TQ(METADATA_TABLE)
               << " (" << fields.str()
               << ") VALUES (" << values.str() << ')';


            exec(qb);
        }
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", object->getID());
    }
}
