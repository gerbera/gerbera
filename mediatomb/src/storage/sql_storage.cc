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
#include "string_converter.h"

using namespace zmm;

/* maximal number of items to store in the hashtable */
//#define MAX_REMOVE_IDS 1000
//#define REMOVE_ID_HASH_CAPACITY 3109

//#define OBJECT_CACHE_CAPACITY 100003

#define LOC_DIR_PREFIX     'D'
#define LOC_FILE_PREFIX    'F'
#define LOC_VIRT_PREFIX    'V'
#define LOC_ILLEGAL_PREFIX 'X'
#define MAX_REMOVE_SIZE 10000

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
    _ref_upnp_class,
    _ref_dc_title,
    _ref_metadata,
    _ref_auxdata,
    _ref_resources,
    _ref_mime_type
};

#define SELECT_DATA "f.id,f.ref_id,f.parent_id,f.object_type,f.upnp_class," \
    "f.dc_title,f.location,f.location_hash,f.metadata," \
    "f.auxdata,f.resources,f.update_id,f.mime_type,f.flags," \
    "rf.upnp_class,rf.dc_title,rf.metadata,rf.auxdata,rf.resources,rf.mime_type"

#define SQL_QUERY "SELECT " SELECT_DATA " FROM " CDS_OBJECT_TABLE \
    " f LEFT JOIN " CDS_OBJECT_TABLE " rf ON f.ref_id = rf.id "

//#define SQL_QUERY_ACTIVE_ITEM "SELECT 

SQLRow::SQLRow(Ref<SQLResult> sqlResult) : Object()
{
    this->sqlResult = sqlResult;
}
SQLRow::~SQLRow()
{}


SQLResult::SQLResult() : Object()
{}
SQLResult::~SQLResult()
{}

/* enum for createObjectFromRow's mode parameter */

SQLStorage::SQLStorage() : Storage()
{
    rmIDs = NULL;
    rmParents = NULL;
    
    mutex = Ref<Mutex>(new Mutex(false));
}

void SQLStorage::init()
{
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

SQLStorage::~SQLStorage()
{
    if (rmIDs)
        FREE(rmIDs);
    if (rmParents)
        FREE(rmParents);
}

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
        }
    }
    
    return findObjectByPath(location);
}

Ref<Dictionary> SQLStorage::_addUpdateObject(Ref<CdsObject> obj, bool isUpdate)
{
    Ref<CdsObject> refObj = nil;
    bool isVirtual = obj->isVirtual();
    if (isVirtual)
    {
        refObj = checkRefID(obj);
        if (refObj == nil)
            throw _Exception(_("tried to add or update a virtual object with illegal reference id and an illegal location"));
        /// \todo create object on demand?
    }
    
    int objectType = obj->getObjectType();
    Ref<Dictionary> data(new Dictionary());
    data->put(_("object_type"), String::from(objectType));
    
    if (isVirtual)
        data->put(_("ref_id"), String::from(refObj->getID()));
    else if (isUpdate)
        data->put(_("ref_id"), _("NULL"));
    
    if (!isVirtual || refObj->getClass() != obj->getClass())
        data->put(_("upnp_class"), quote(obj->getClass()));
    else if (isUpdate)
        data->put(_("upnp_class"), _("NULL"));
    
    if (!isVirtual || refObj->getTitle() != obj->getTitle())
        data->put(_("dc_title"), quote(obj->getTitle()));
    else if (isUpdate)
        data->put(_("dc_title"), _("NULL"));
    
    data->put(_("flags"), String::from(obj->getFlags()));
    
    if (isUpdate)
        data->put(_("metadata"), _("NULL"));
    Ref<Dictionary> dict = obj->getMetadata();
    if (dict->size() > 0)
    {
        if (!isVirtual || ! refObj->getMetadata()->equals(obj->getMetadata()))
        {
            data->put(_("metadata"), quote(dict->encode()));
        }
    }
    
    if (isUpdate)
        data->put(_("auxdata"), _("NULL"));
    dict = obj->getAuxData();
    if (dict->size() > 0 && (! isVirtual || ! refObj->getAuxData()->equals(obj->getAuxData())))
    {
        data->put(_("auxdata"), quote(obj->getAuxData()->encode()));
    }
    
    if (! isVirtual || (! obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF) && ! refObj->resourcesEqual(obj)))
    {
        // encode resources
        Ref<StringBuffer> resBuf(new StringBuffer());
        for (int i = 0; i < obj->getResourceCount(); i++)
        {
            if (i > 0)
                *resBuf << RESOURCE_SEP;
            *resBuf << obj->getResource(i)->encode();
        }
        data->put(_("resources"), quote(resBuf->toString()));
    }
    else if (isUpdate)
        data->put(_("resources"), _("NULL"));
    
    if (IS_CDS_CONTAINER(objectType))
    {
        throw _Exception(_("tried to add a container via _addUpdateObject; is this correct?"));
        
        /*
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        data->put(_("update_id"), String::from(cont->getUpdateID()));
        
        if (!isVirtual)
        {
            
            /*
            String loc = cont->getLocation();
            if (!string_ok(loc)) throw _Exception(_("tried to create or update a non-virtual container without a location set"));
            String dbLocation = addLocationPrefix(LOC_DIR_PREFIX, loc);
            data->put(_("location"), quote(dbLocation));
            data->put(_("location_hash"), String::from(stringHash(dbLocation)));
            *\/
        }
        else
        {
            throw _Exception(_("tried to add an virtual container via _addUpdateObject; should be done via addContainerChain"));
        }
            
        /*if (isUpdate)
        {
            data->put(_("location"), _("NULL"));
            data->put(_("location_hash"), _("NULL"));
        }*\/
        
        if (isUpdate)
            data->put(_("mime_type"), _("NULL"));
        */
    }
    
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        
        if (!isVirtual)
        {
            String loc = item->getLocation();
            if (!string_ok(loc)) throw _Exception(_("tried to create or update a non-virtual item without a location set"));
            Ref<Array<StringBase> > pathAr = split_path(loc);
            String path = pathAr->get(0);
            int parentID = ensurePathExistence(path);
            item->setParentID(parentID);
            //String filename = pathAr->get(1);
            String dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
            data->put(_("location"), quote(dbLocation));
            data->put(_("location_hash"), String::from(stringHash(dbLocation)));
        }
        else //for URLs 
        {/// \todo fix: urls are virtual!
            //data->put(_("location"), quote(item->getLocation()));
            if (isUpdate)
                data->put(_("location_hash"), _("NULL"));
        }
        
        data->put(_("mime_type"), quote(item->getMimeType()));
    }
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        data->put(_("location"), quote(aitem->getAction()));
        //data->put(_("state"), quote(aitem->getState()));
    }
    //else if (isUpdate)
    //    data->put(_("state"), _("NULL"));
    
    if (obj->getParentID() == INVALID_OBJECT_ID)
        throw _Exception(_("tried to create or update an object with an illegal parent id"));
    data->put(_("parent_id"), String::from(obj->getParentID()));
    
    return data;
}

void SQLStorage::addObject(Ref<CdsObject> obj)
{
    Ref<Dictionary> data = _addUpdateObject(obj, false);
    Ref<Array<DictionaryElement> > dataElements = data->getElements();
    
    Ref<StringBuffer> fields(new StringBuffer(128));
    Ref<StringBuffer> values(new StringBuffer(128));
    for (int i=0; i < dataElements->size(); i++)
    {
        Ref<DictionaryElement> element = dataElements->get(i);
        if (i != 0)
        {
            *fields << ", ";
            *values << ", ";
        }
        *fields << element->getKey();
        *values << element->getValue();
    }
    
    Ref<StringBuffer> qb(new StringBuffer(256));
    *qb << "INSERT INTO " CDS_OBJECT_TABLE "(" << fields->toString() <<
            ") VALUES (" << values->toString() << ")";
            
//    log_debug("insert_query: %s\n", qb->toString().c_str());
    
    obj->setID(exec(qb->toString(), true));
}

void SQLStorage::updateObject(zmm::Ref<CdsObject> obj)
{
    Ref<Dictionary> data = _addUpdateObject(obj, true);
    Ref<Array<DictionaryElement> > dataElements = data->getElements();
    
    Ref<StringBuffer> qb(new StringBuffer(256));
    *qb << "UPDATE " CDS_OBJECT_TABLE " SET ";
    
    for (int i=0; i < dataElements->size(); i++)
    {
        Ref<DictionaryElement> element = dataElements->get(i);
        if (i != 0)
        {
            *qb << ", ";
        }
        *qb << element->getKey() << " = "
            << element->getValue();
    }
    
    *qb << " WHERE id = " << obj->getID();
    
    log_debug("upd_query: %s\n", qb->toString().c_str());
    
    exec(qb->toString());
}

Ref<CdsObject> SQLStorage::loadObject(int objectID)
{
/*
    Ref<CdsObject> obj = objectIDCache->get(objectID);
    if (obj != nil)
        return obj;
    throw _Exception(_("Object not found: ") + objectID);
*/
    Ref<StringBuffer> qb(new StringBuffer());

    *qb << SQL_QUERY " WHERE f.id = " << objectID;

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        return createObjectFromRow(row);
    }
    throw _Exception(_("Object not found: ") + objectID);
}

Ref<Array<CdsObject> > SQLStorage::browse(Ref<BrowseParam> param)
{
    int objectID;
    int objectType;
    
    objectID = param->getObjectID();
    
    String q;
    Ref<SQLResult> res;
    Ref<SQLRow> row;
    
    q = _("SELECT object_type FROM " CDS_OBJECT_TABLE " WHERE id = ") +
                    objectID;
    res = select(q);
    if((row = res->nextRow()) != nil)
    {
        objectType = atoi(row->col(0).c_str());
    }
    else
    {
        throw _StorageException(_("Object not found: ") + objectID);
    }
    
    row = nil;
    res = nil;
    
    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        q = _("SELECT COUNT(*) FROM " CDS_OBJECT_TABLE " WHERE parent_id = ") + objectID;
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

    *qb << SQL_QUERY " WHERE ";

    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        int count = param->getRequestedCount();
        if(! count)
            count = INT_MAX;

        *qb << "f.parent_id = " << objectID;
        *qb << " ORDER BY f.object_type, f.dc_title, rf.object_type, rf.dc_title";
        *qb << " LIMIT " << param->getStartingIndex() << "," << count;
    }
    else // metadata
    {
        *qb << "f.id = " << objectID << " LIMIT 1";
    }
    // log_debug(("QUERY: %s\n", qb->toString().c_str()));
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
            cont->setChildCount(getChildCount(cont->getID(), param->containersOnly));
        }
    }

    return arr;
}

int SQLStorage::getChildCount(int contId, bool containersOnly)
{
    Ref<SQLRow> row;
    Ref<SQLResult> res;
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT COUNT(*) FROM " CDS_OBJECT_TABLE " WHERE parent_id = " << contId;
    if (containersOnly)
        *qb << " AND object_type = " << OBJECT_TYPE_CONTAINER;
    res = select(qb->toString());
    if ((row = res->nextRow()) != nil)
    {
        return row->col(0).toInt();
    }
    return 0;
}

Ref<Array<StringBase> > SQLStorage::getMimeTypes()
{
    Ref<Array<StringBase> > arr(new Array<StringBase>());

    String q = _("SELECT DISTINCT mime_type FROM " CDS_OBJECT_TABLE
                " WHERE mime_type IS NOT NULL ORDER BY mime_type");
    Ref<SQLResult> res = select(q);
    Ref<SQLRow> row;

    while ((row = res->nextRow()) != nil)
    {
        arr->append(String(row->col(0)));
    }

    return arr;
}

/*
Ref<CdsObject> SQLStorage::findObjectByTitle(String title, int parentID)
{
    *
    Ref<CdsObject> obj = objectTitleCache->get(String::from(parentID) +'|'+ title);
    if (obj != nil)
        return obj;
    return nil;
    *

    Ref<StringBuffer> qb(new StringBuffer());
    *qb << getSelectQuery() << " WHERE ";
    if (parentID != INVALID_OBJECT_ID)
        *qb << "f.parent_id = " << parentID << " AND ";
    *qb << "f.dc_title = " << quote(title);

    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row;
    if ((row = res->nextRow()) != nil)
    {
        Ref<CdsObject> obj = createObjectFromRow(row);
        return obj;
    }
    return nil;
}
*/

int SQLStorage::isFolderInDatabase(String fullpath)
{
    fullpath = fullpath.reduce(DIR_SEPARATOR);
    Ref<Array<StringBase> > pathAr = split_path(fullpath);
    String path = pathAr->get(0);
    String filename = pathAr->get(1);
    
    if (! string_ok(path) && ! string_ok(filename))
        return -3;
    
    if (! string_ok(filename))
        fullpath = path;
    else if (! string_ok(path))
        fullpath = _("/") + filename;
    
    Ref<StringBuffer> qb(new StringBuffer());
    String dbLocation = addLocationPrefix(LOC_DIR_PREFIX, path);
    *qb << "SELECT `id` FROM " CDS_OBJECT_TABLE
            " WHERE `location_hash` = " << stringHash(dbLocation)
            << " AND `location` = " << quote(dbLocation)
            << " AND `ref_id` IS NULL "
            "LIMIT 1";
     Ref<SQLResult> res = select(qb->toString());
     if (res == nil)
         return -2;
     Ref<SQLRow> row = res->nextRow();
     if (row == nil)
         return -1;
     return row->col(0).toInt();
}

int SQLStorage::isFileInDatabase(String path)
{
    Ref<SQLRow> row = _findObjectByFilename(path);
    if (row == nil)
        return -1;
    return row->col(_id).toInt();
}

Ref<SQLRow> SQLStorage::_findObjectByFilename(String path)
{
    String dbLocation = addLocationPrefix(LOC_FILE_PREFIX, path);
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << SQL_QUERY
        " WHERE `f`.`location_hash` = " << stringHash(dbLocation)
        << " AND `f`.`location` = " << quote(dbLocation)
        << " AND `f`.`ref_id` IS NULL "
        "LIMIT 1";
        
    Ref<SQLResult> res = select(qb->toString());
    return res->nextRow();
}

Ref<CdsObject> SQLStorage::findObjectByFilename(String path)
{
    Ref<SQLRow> row = _findObjectByFilename(path);
    if (row == nil)
        return nil;
    return createObjectFromRow(row);
}

Ref<SQLRow> SQLStorage::_findObjectByPath(String fullpath)
{
    fullpath = fullpath.reduce(DIR_SEPARATOR);
    Ref<Array<StringBase> > pathAr = split_path(fullpath);
    String path = pathAr->get(0);
    String filename = pathAr->get(1);
    
    Ref<StringBuffer> qb(new StringBuffer());
    bool file = true;
    if (! string_ok(filename) || ! string_ok(path))
    {
        if (! string_ok(filename) && ! string_ok(path))
            throw _Exception(_("tried to add an empty path"));
        
         file = false;
    }
    String dbLocation;
    if (file)
        dbLocation = addLocationPrefix(LOC_FILE_PREFIX, fullpath);
    else
        dbLocation = addLocationPrefix(LOC_DIR_PREFIX, path);
    *qb << SQL_QUERY
            " WHERE `f`.`location_hash` = " << stringHash(dbLocation)
            << " AND `f`.`location` = " << quote(dbLocation)
            << " AND `f`.`ref_id` IS NULL "
            "LIMIT 1";
    
    Ref<SQLResult> res = select(qb->toString());
    return res->nextRow();
}

Ref<CdsObject> SQLStorage::findObjectByPath(String fullpath)
{
    Ref<SQLRow> row = _findObjectByPath(fullpath);
    if (row == nil)
        return nil;
    return createObjectFromRow(row);
}

int SQLStorage::findObjectIDByPath(String fullpath)
{
    Ref<SQLRow> row = _findObjectByPath(fullpath);
    if (row == nil)
        return -1;
    return row->col(_id).toInt();
}

int SQLStorage::ensurePathExistence(String path)
{
    path = path.reduce(DIR_SEPARATOR);
    Ref<CdsObject> obj = findObjectByPath(path + DIR_SEPARATOR);
    if (obj != nil) return obj->getID();
    Ref<Array<StringBase> > pathAr = split_path(path);
    String parent = pathAr->get(0);
    String folder = pathAr->get(1);
    int parentID;
    if (string_ok(parent))
        parentID = ensurePathExistence(parent);
    else
        parentID = CDS_ID_FS_ROOT;
    Ref<StringConverter> f2i = StringConverter::f2i();
    return createContainer(parentID, f2i->convert(folder), path, false);
}

int SQLStorage::createContainer(int parentID, String name, String path, bool isVirtual)
{
    String dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), path);
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "INSERT INTO " CDS_OBJECT_TABLE
        " (`parent_id`, `object_type`, `upnp_class`, `dc_title`,"
        " `location`, `location_hash`)"
        " VALUES ("
        << parentID
        << ", " << OBJECT_TYPE_CONTAINER
        << ", " << quote(_(UPNP_DEFAULT_CLASS_CONTAINER))
        << ", " << quote(name)
        << ", " << quote(dbLocation)
        << ", " << stringHash(dbLocation)
        << ")";
        
    return exec(qb->toString(), true);
    
}

String SQLStorage::buildContainerPath(int parentID, String title)
{
    //title = escape(title, xxx);
    if (parentID == CDS_ID_ROOT)
        return String(VIRTUAL_CONTAINER_SEPARATOR) + title;
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT `location` FROM " CDS_OBJECT_TABLE
        " WHERE `id` = " << parentID << " LIMIT 1";
     Ref<SQLResult> res = select(qb->toString());
    if (res == nil)
        return nil;
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    return stripLocationPrefix(row->col(0)) + VIRTUAL_CONTAINER_SEPARATOR + title;
}

void SQLStorage::addContainerChain(String path, int *containerID, int *updateID)
{
    path = path.reduce(VIRTUAL_CONTAINER_SEPARATOR);
    *updateID = INVALID_OBJECT_ID;
    if (path == _("/"))
    {
        *containerID = CDS_ID_ROOT;
        return;
    }
    Ref<StringBuffer> qb(new StringBuffer());
    String dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, path);
    *qb << "SELECT `id` FROM " CDS_OBJECT_TABLE
            " WHERE `location_hash` = " << stringHash(dbLocation)
            << " AND `location` = " << quote(dbLocation)
            << " LIMIT 1";
    Ref<SQLResult> res = select(qb->toString());
    if (res != nil)
    {
        Ref<SQLRow> row = res->nextRow();
        if (row != nil)
        {
            *containerID = row->col(0).toInt();
            return;
        }
    }
    int parentContainerID;
    int parentUpdateID;
    String newpath, container;
    stripAndUnescapeVirtualContainerFromPath(path, newpath, container);
    addContainerChain(newpath, &parentContainerID, &parentUpdateID);
    *containerID = createContainer(parentContainerID, container, path, true);
    if (parentUpdateID == INVALID_OBJECT_ID)
        *updateID = parentContainerID;
    else
        *updateID = parentUpdateID;
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

unsigned int SQLStorage::stringHash(String key)
{
    unsigned int hash = 5381;
    unsigned char *data = (unsigned char *)key.c_str();
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
    return hash;
}

/*
String SQLStorage::getRealLocation(int parentID, String location)
{
    log_debug("parentID: %d; location %s\n", parentID, location.c_str());
    Ref<StringBuffer> qb(new StringBuffer());
    *qb << "SELECT location FROM " CDS_OBJECT_TABLE
        " WHERE id = " << parentID << " LIMIT 1";
    Ref<SQLResult> res = select(qb->toString());
    Ref<SQLRow> row = res->nextRow();
    return stripLocationPrefix(row->col(0)) + "/" + location;
}
*/

Ref<CdsObject> SQLStorage::createObjectFromRow(Ref<SQLRow> row)
{
    int objectType = atoi(row->col(_object_type).c_str());
    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(row->col(_id).toInt());
    obj->setRefID(row->col(_ref_id).toInt());
    if (obj->getRefID())
        obj->setVirtual(true);
    else
        obj->setVirtual(false);
    
    obj->setParentID(row->col(_parent_id).toInt());
    obj->setTitle(fallbackString(row->col(_dc_title), row->col(_ref_dc_title)));
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
            if (i == 0) resource_zero_ok = true;
            obj->addResource(CdsResource::decode(resources->get(i)));
        }
    }
    
    int matched_types = 0;
    
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        cont->setUpdateID(atoi(row->col(_update_id).c_str()));
        char locationPrefix;
        cont->setLocation(stripLocationPrefix(&locationPrefix, row->col(_location)));
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);
        else
            cont->setVirtual(false);
        matched_types++;
    }
    
    if (IS_CDS_ITEM(objectType))
    {
        if (! resource_zero_ok)
            throw _Exception(_("trying to create object without at least one resource"));
        
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setMimeType(fallbackString(row->col(_mime_type), row->col(_ref_mime_type)));
        if (IS_CDS_PURE_ITEM(objectType) && ! obj->isVirtual())
            item->setLocation(stripLocationPrefix(row->col(_location)));
        else
            item->setLocation(row->col(_location));
        matched_types++;
    }
    
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        //aitem->setAction(row->col(_location)); // TODO: use different field
        //aitem->setState(row->col(_state));
        matched_types++;
    }

    if(! matched_types)
    {
        throw _StorageException(_("unknown object type: ")+ objectType);
    }
    return obj;
}

int SQLStorage::getTotalFiles()
{
    Ref<StringBuffer> query(new StringBuffer());
    *query << "SELECT COUNT(*) FROM " CDS_OBJECT_TABLE " WHERE "
           << "object_type != " << OBJECT_TYPE_CONTAINER;
           //<< " AND is_virtual = 0";
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
    if (size<1) return;
    Ref<StringBuffer> buf(new StringBuffer(size * sizeof(int)));
    *buf << "UPDATE " CDS_OBJECT_TABLE " SET update_id = update_id + 1 WHERE ID IN(";
    *buf << ids[0];
    for (int i = 1; i < size; i++)
        *buf << ',' << ids[i];
    *buf << ')';
    exec(buf->toString());
}

void SQLStorage::incrementUIUpdateID(int id)
{
    //Ref<StringBuffer> buf(new StringBuffer(size * sizeof(int)));
    log_debug("id: %d\n", id);
    /// \todo implement this
    //exec(buf->toString());
}

Ref<Array<CdsObject> > SQLStorage::selectObjects(Ref<SelectParam> param)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << SQL_QUERY " WHERE ";
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
    *q << " ORDER BY f.object_type, f.dc_title, rf.object_type, rf.dc_title";
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

Ref<DBRHash<int> > SQLStorage::getObjects(int parentID)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT id FROM " CDS_OBJECT_TABLE " WHERE parent_id = ";
    *q << parentID;
    Ref<SQLResult> res = select(q->toString());
    Ref<SQLRow> row;
    
    int capacity = res->getNumRows() * 5 + 1;
    if (capacity < 521)
        capacity = 521;
    
    Ref<DBRHash<int> > ret(new DBRHash<int>(capacity, -2, -3));
    
    while ((row = res->nextRow()) != nil)
    {
        ret->put(row->col(0).toInt());
    }
    return ret;
}



void SQLStorage::removeObjects(zmm::Ref<DBRHash<int> > list)
{
    hash_data_array_t<int> hash_data_array;
    list->getAll(&hash_data_array);
    int count = hash_data_array.size;
    int *array = hash_data_array.data;
    if (count <= 0) return;
    
    Ref<StringBuffer> ids(new StringBuffer());
    for (int i = 0; i < count; i++)
    {
        * ids << "," << array[i];
    }
    
    if (dbRemovesDeps)
    {
        _removeObjects(ids->toString(1));
    }
    else
    {
        _recursiveRemove(ids->toString(1));
    }                                     
}

void SQLStorage::_removeObjects(String objectIDs)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "DELETE FROM " CDS_OBJECT_TABLE " WHERE id IN (" << objectIDs
        << ")";
    exec(q->toString());
}

void SQLStorage::removeObject(int objectID, bool all)
{
    Ref<StringBuffer> q = nil;
    if (all)
    {
        q = Ref<StringBuffer>(new StringBuffer());
        *q << "SELECT ref_id FROM " CDS_OBJECT_TABLE " WHERE id = "
            << objectID << " LIMIT 1";
        Ref<SQLResult> res = select(q->toString());
        if (res == nil)
            return;
        Ref<SQLRow> row = res->nextRow();
        if (row == nil)
            return;
        objectID = row->col(0).toInt();
    }
    if (dbRemovesDeps)
    {
        if (q == nil)
            q = Ref<StringBuffer>(new StringBuffer());
        else
            q->clear();
        *q << "DELETE FROM " CDS_OBJECT_TABLE " WHERE id = " << objectID;
        exec(q->toString());
    }
    else
    {
        _recursiveRemove(String::from(objectID));
    }
}

void SQLStorage::_recursiveRemove(String objectIDs)
{
    Ref<StringBuffer> q(new StringBuffer());
    Ref<StringBuffer> remove(new StringBuffer());
    Ref<StringBuffer> recurse(new StringBuffer());
    *recurse << "," << objectIDs;
    *remove << "," << objectIDs;
    while(recurse->length() != 0)
    {
        q->clear();
        String recurseStr = recurse->toString(1);
        *q << "SELECT DISTINCT id FROM " CDS_OBJECT_TABLE
            " WHERE parent_id IN (" << recurseStr << ") OR"
            " ref_id IN (" << recurseStr << ")";
        Ref<SQLResult> res = select(q->toString());
        Ref<SQLRow> row;
        recurse->clear();
        while ((row = res->nextRow()) != nil)
        {
            *recurse << "," << row->col(0);
            *remove << "," << row->col(0);
        }
        if (remove->length() > MAX_REMOVE_SIZE)
        {
            _removeObjects(remove->toString(1));
            remove->clear();
        }
    }
    
    if (remove->length() > 0)
        _removeObjects(remove->toString(1));
}


String SQLStorage::getInternalSetting(String key)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "SELECT `value` FROM " INTERNAL_SETTINGS_TABLE " WHERE `key` =  "
        << quote(key) << " LIMIT 1";
    Ref<SQLResult> res = select(q->toString());
    if (res == nil)
        return nil;
    Ref<SQLRow> row = res->nextRow();
    if (row == nil)
        return nil;
    return row->col(0);
}
/*
void SQLStorage::storeInternalSetting(String key, String value)
overwritten due to different SQL syntax
*/

/* helpers for removeObject method */
/*
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
*/
/* flush scheduled items */
/*
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
*/

/* schedule an object for removal, will be called
 * for EVERY deleted object */
 /*
void SQLStorage::rmObject(Ref<CdsObject> obj)
{
//    log_debug(("rmObject %d, size; %d\n", obj->getID(), rmIDHash->size()));
    int id = obj->getID();
    hash_slot_t hash_slot;
    if (rmIDHash->exists(id, &hash_slot)) // do nothing if already scheduled
        return;


    //log_debug("rmIDHash->size()  %d (max: %d)\n", rmIDHash->size(), MAX_REMOVE_IDS);
    // schedule for removal
    rmIDs[rmIDHash->size()] = id;
    rmIDHash->put(id, hash_slot);

   
    if (rmIDHash->size() >= MAX_REMOVE_IDS ||
        rmParentHash->size() >= MAX_REMOVE_IDS)
        rmFlush();

    rmDecChildCount(loadObject(obj->getParentID()));
   
}
*/
/* item removal helper */
/*
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

*/
