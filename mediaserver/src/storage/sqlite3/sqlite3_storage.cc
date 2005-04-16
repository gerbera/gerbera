/*  sqlite3_storage.cc - this file is part of MediaTomb.
                                                                                
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

#include "sqlite3_storage.h"

#include "common.h"
#include "config_manager.h"
#include "destroyer.h"

using namespace zmm;

/************************ util stuff **********************/
enum
{
    _object_type,

    _id,
    _parent_id,
    _upnp_class,
    _dc_title,
    _dc_description,
    _restricted,

    _update_id,
    _searchable,

    _location,
    _mime_type,

    _action,
    _state
};

static String select_fields = "\
    object_type, \
    \
    id, \
    parent_id, \
    upnp_class, \
    dc_title, \
    dc_description, \
    restricted, \
    \
    update_id, \
    searchable, \
    \
    location, \
    mime_type, \
    \
    action, \
    state";

/* ************************************* */

void unlock_func(void *data)
{
    ((Sqlite3Storage *)data)->unlock();
}
#define LOCK_METHOD lock(); \
        Ref<Destroyer> destroyer(new Destroyer(unlock_func, this));
#define UNLOCK_METHOD destroyer->destroy();
        
Sqlite3Storage::Sqlite3Storage() : Storage()
{
    table = NULL;
    db = NULL;
    int res;

    pthread_mutexattr_t mutex_attr;
    res = pthread_mutexattr_init(&mutex_attr);
    res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    
    pthread_mutex_init(&lock_mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
}

Sqlite3Storage::~Sqlite3Storage()
{
    finish();
    if (db)
        sqlite3_close(db);
    pthread_mutex_destroy(&lock_mutex);
}

void Sqlite3Storage::init(Ref<Dictionary> params)
{
    String filename = params->get("database-file");
    if (!string_ok(filename))
        throw Exception("Sqlite3Storage.init: \"database-file\" attribute not found");

    check_path_ex(filename, false);

    int res = sqlite3_open(filename.c_str(), &db);
    if(res != SQLITE_OK)
    {
        throw StorageException(String("Sqlite3Storage.init: could not open ")+
            filename);
    }
}


Ref<Array<CdsObject> > Sqlite3Storage::browse(Ref<BrowseParam> param)
{
    LOCK_METHOD;

    String objectID;
    int objectType;

    objectID = param->getObjectID();

    char *endptr;
    (void)strtol(objectID.c_str(), &endptr, 10);
    if (*endptr != 0)
        throw Exception(String("Invalid object ID: ") + objectID);


    select(String("SELECT object_type FROM media_files WHERE id = ") + objectID);
    if(nextRow())
    {
        objectType = atoi(row[0]);
        finish();
    }
    else
    {
        finish();
        throw StorageException(String("Object not found: ") + objectID);
    }

    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        select(String("SELECT COUNT(*) FROM media_files WHERE parent_id = ") + objectID);
        if(nextRow())
        {
            param->setTotalMatches(atoi(row[0]));
            finish();
        }
    }
    else
    {
        param->setTotalMatches(1);
    }

    Ref<StringBuffer> query(new StringBuffer());

    *query << "SELECT " << select_fields << " FROM media_files f WHERE ";

    if(param->getFlag() == BROWSE_DIRECT_CHILDREN && IS_CDS_CONTAINER(objectType))
    {
        int count = param->getRequestedCount();
        if(! count)
            count = 1000000000;

        *query << "f.parent_id = " << objectID;
        *query << " ORDER BY f.object_type, f.dc_title";
        *query << " LIMIT " << param->getStartingIndex() << "," << count;
    }
    else // metadata
    {
        *query << "f.id = " << objectID;
    }
    select(query->toString());

    Ref<Array<CdsObject> > arr(new Array<CdsObject>(nrow));

    while(nextRow())
    {
        objectType = atoi(row[_object_type]);
        Ref<CdsObject> obj = createObjectFromRow();
        arr->append(obj);
    }
    finish();

    // update childCount fields
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_CONTAINER(obj->getObjectType()))
        {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            query->clear();
            *query << "SELECT COUNT(*) FROM media_files WHERE parent_id = " << cont->getID();
            select(query->toString());
            if (nextRow())
            {
                cont->setChildCount(atoi(row[0]));
            }
            else
            {
                cont->setChildCount(0);
            }
            finish();
        }
    }

    UNLOCK_METHOD;
    return arr;
}

static String quote(String value)
{
    char *q = sqlite3_mprintf("'%q'",
        (value == nil ? "" : value.c_str()));
    String ret(q);
    sqlite3_free(q);
    return ret;
}

void Sqlite3Storage::addObject(Ref<CdsObject> obj)
{
    LOCK_METHOD;

    int objectType = obj->getObjectType();
    Ref<StringBuffer> fields(new StringBuffer(128));
    Ref<StringBuffer> values(new StringBuffer(128));

    *fields << "parent_id";
    *values << obj->getParentID();

    *fields << ", object_type";
    *values << ", " << objectType;

    *fields << ", upnp_class";
    *values << ", " << quote(obj->getClass());

    *fields << ", dc_title";
    *values << ", " << quote(obj->getTitle());

    *fields << ", restricted";
    *values << ", " << obj->isRestricted();

    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        *fields << ", update_id";
        *values << ", " << cont->getUpdateID();

        *fields << ", searchable";
        *values << ", " << cont->isSearchable();
    }
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        *fields << ", dc_description";
        *values << ", " << quote(item->getDescription());

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

    Ref<StringBuffer> query(new StringBuffer(256));
    *query << "INSERT INTO media_files(" << fields->toString() <<
            ") VALUES (" << values->toString() << ")";

//    printf("insert_query: %s\n", query->toString().c_str());

    this->exec(query->toString());

    obj->setID(String::from((int)sqlite3_last_insert_rowid(db)));

    UNLOCK_METHOD;
}

void Sqlite3Storage::updateObject(zmm::Ref<CdsObject> obj)
{
    LOCK_METHOD;

    int objectType = obj->getObjectType();

    Ref<StringBuffer> query(new StringBuffer(256));
    *query << "UPDATE media_files SET id = id";

    *query << ", upnp_class = " << quote(obj->getClass());
    *query << ", dc_title = " << quote(obj->getTitle());
    *query << ", restricted = " << obj->isRestricted();

    if(IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        *query << ", update_id = " << cont->getUpdateID();
        *query << ", searchable = " << cont->isSearchable();
    }
    if(IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        *query << ", dc_description = " << quote(item->getDescription());
        *query << ", location = " << quote(item->getLocation());
        *query << ", mime_type = " << quote(item->getMimeType());
    }
    if(IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        *query << ", action = " << quote(aitem->getAction());
        *query << ", state = " << quote(aitem->getState());
    }

    *query << " WHERE id = " << obj->getID();

//    printf("upd_query: %s\n", query->toString().c_str());

    this->exec(query->toString());

    UNLOCK_METHOD;
}

void Sqlite3Storage::eraseObject(zmm::Ref<CdsObject> obj)
{
    LOCK_METHOD;

    String query = String("DELETE FROM media_files WHERE id = ") + obj->getID();
    this->exec(query);

    UNLOCK_METHOD;
}

static char *del_query = "DELETE FROM media_files WHERE id IN (";     

void Sqlite3Storage::removeObject(zmm::Ref<CdsObject> obj)
{
    LOCK_METHOD;

    Ref<StringBuffer> query(new StringBuffer());
    *query << del_query;
    if(IS_CDS_CONTAINER(obj->getObjectType()))       
        removeChildren(obj, query);
    *query << obj->getID() << ")";
    exec(query->toString());

    UNLOCK_METHOD;
}
    
void Sqlite3Storage::removeChildren(Ref<CdsObject> obj, Ref<StringBuffer> query)
{
    Ref<BrowseParam> param(new BrowseParam(obj->getID(),
                           BROWSE_DIRECT_CHILDREN));
    Ref<Array<CdsObject> > arr = browse(param);
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> child = arr->get(i); 
        if (IS_CDS_CONTAINER(child->getObjectType()))
            removeChildren(child, query);
        *query << child->getID();
        if (query->length() > MAX_DELETE_QUERY_LENGTH)
        {
            printf("DEL QUERY: %s...\n", query->toString().substring(0, 65).c_str());
            *query << ")";
            exec(query->toString());            
            query->clear();
            *query << del_query;
        }
        else
            *query << ',';
    }
}


Ref<Array<StringBase> > Sqlite3Storage::getMimeTypes()
{
    LOCK_METHOD;

    Ref<Array<StringBase> > arr(new Array<StringBase>());
    select(String("SELECT DISTINCT mime_type FROM media_files ")
           + "WHERE mime_type IS NOT NULL ORDER BY mime_type");
    while (nextRow())
    {
        arr->append(String(row[0]));
    }

    finish();
    
    UNLOCK_METHOD;
    return arr;
}

Ref<CdsObject> Sqlite3Storage::findObjectByTitle(String title, String parentID)
{
    LOCK_METHOD;

    Ref<StringBuffer> query(new StringBuffer());
    *query << "SELECT " << select_fields << " FROM media_files WHERE ";
    if (parentID != nil)
        *query << "parent_id = " << parentID << " AND ";
    *query << "dc_title = " << quote(title);

    select(query->toString());
    if (nextRow())
    {
        Ref<CdsObject> obj = createObjectFromRow();
        finish();
        return obj;
    }
    finish();

    UNLOCK_METHOD;
    return nil;
}

Ref<CdsObject> Sqlite3Storage::createObjectFromRow()
{
    int objectType = atoi(row[_object_type]);
    Ref<CdsObject> obj = createObject(objectType);

    /* set common properties */
    obj->setID(row[_id]);
    obj->setParentID(row[_parent_id]);
    obj->setRestricted(atoi(row[_restricted]));
    obj->setTitle(row[_dc_title]);
    obj->setClass(row[_upnp_class]);

    int matched_types = 0;

    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        cont->setSearchable(atoi(row[_searchable]));
        cont->setUpdateID(atoi(row[_update_id]));
        matched_types++;
    }

    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        item->setDescription(row[_dc_description]);
        item->setMimeType(row[_mime_type]);
        item->setLocation(row[_location]);
        matched_types++;
    }

    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        aitem->setAction(row[_action]);
        aitem->setState(row[_state]);
        matched_types++;
    }
    if(! matched_types)
    {
        throw StorageException(String("unknown file type: ")+ objectType);
    }
    return obj;
}

void Sqlite3Storage::reportError(String query)
{
    fprintf(stderr, "SQLITE3: (%d) %s\nQuery:%s\n",
        sqlite3_errcode(db),
        sqlite3_errmsg(db),
        (query == nil) ? "unknown" : query.c_str()
    );
}

void Sqlite3Storage::select(String query)
{
    char *err;
    int res = sqlite3_get_table(
        db,
        query.c_str(),
        &table,
        &nrow,
        &ncolumn,
        &err
    );
    row = table;
    cur_row = 0;

    if(res != SQLITE_OK)
    {
        reportError(query);
        throw StorageException("query error");
    }

}
int Sqlite3Storage::nextRow()
{
    if(nrow)
    {
        row += ncolumn;
        cur_row++;
        return (cur_row <= nrow);
    }
    return 0;

}
void Sqlite3Storage::finish()
{
    if(table)
    {
        sqlite3_free_table(table);
        table = NULL;
    }
}


void Sqlite3Storage::exec(String query)
{
    char *err;
    int res = sqlite3_exec(
        db,
        query.c_str(),
        NULL,
        NULL,
        &err
    );
    if(res != SQLITE_OK)
    {
        reportError(query);
        throw StorageException("query error");
    }
}


void Sqlite3Storage::lock()
{
    pthread_mutex_lock(&lock_mutex);
}
void Sqlite3Storage::unlock()
{
    pthread_mutex_unlock(&lock_mutex);
}

