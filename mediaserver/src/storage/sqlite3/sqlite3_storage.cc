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
using namespace mxml;

void unlock_func(void *data)
{
    ((Sqlite3Storage *)data)->unlock();
}
#define LOCK_METHOD lock(); \
        Ref<Destroyer> destroyer(new Destroyer(unlock_func, this));
#define UNLOCK_METHOD destroyer->destroy();

Sqlite3Storage::Sqlite3Storage() : SQLStorage()
{
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
    if (db)
        sqlite3_close(db);
    pthread_mutex_destroy(&lock_mutex);
}

void Sqlite3Storage::init()
{
    Ref<ConfigManager> config = ConfigManager::getInstance();

    String dbFile = config->getOption("/server/storage/database-file");
    if (dbFile == nil)
        throw StorageException("Sqlite3Storage: database-file option not found");
    String dbFilePath = config->getOption("/server/home") + DIR_SEPARATOR + dbFile;

    check_path_ex(dbFilePath, false);

    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if(res != SQLITE_OK)
    {
        throw StorageException(String("Sqlite3Storage.init: could not open ")+
            dbFilePath);
    }
}

String Sqlite3Storage::quote(String value)
{
    char *q = sqlite3_mprintf("'%q'",
        (value == nil ? "" : value.c_str()));
    String ret(q);
    sqlite3_free(q);
    return ret;
}

static char *del_query = "DELETE FROM media_files WHERE id IN (";

/*
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
*/

void Sqlite3Storage::reportError(String query)
{
    fprintf(stderr, "SQLITE3: (%d) %s\nQuery:%s\n",
        sqlite3_errcode(db),
        sqlite3_errmsg(db),
        (query == nil) ? "unknown" : query.c_str()
    );
}


Ref<SQLResult> Sqlite3Storage::select(String query)
{
    LOCK_METHOD;

    char *err;
    Sqlite3Result *pres = new Sqlite3Result();
    Ref<SQLResult> res(pres);
    pres->storage = Ref<Sqlite3Storage>(this);

    int ret = sqlite3_get_table(
        db,
        query.c_str(),
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err
    );
    if(ret != SQLITE_OK)
    {
        reportError(query);
        throw StorageException("query error");
    }

    pres->row = pres->table;
    pres->cur_row = 0;

    UNLOCK_METHOD;

    return res;
}

int Sqlite3Storage::exec(String query)
{
    LOCK_METHOD;

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

    UNLOCK_METHOD;
}

int Sqlite3Storage::lastInsertID()
{
    return (int)sqlite3_last_insert_rowid(db);
}

void Sqlite3Storage::lock()
{
    pthread_mutex_lock(&lock_mutex);
}
void Sqlite3Storage::unlock()
{
    pthread_mutex_unlock(&lock_mutex);
}

/* Sqlite3Result */

Sqlite3Result::Sqlite3Result() : SQLResult()
{
    table = NULL;
}
Sqlite3Result::~Sqlite3Result()
{
    if(table)
    {
        sqlite3_free_table(table);
        table = NULL;
    }
}
Ref<SQLRow> Sqlite3Result::nextRow()
{
    if(nrow)
    {
        row += ncolumn;
        cur_row++;
        if (cur_row <= nrow)
        {
            Sqlite3Row *p = new Sqlite3Row(row);
            p->res = Ref<Sqlite3Result>(this);
            return Ref<SQLRow>(p);
        }
        else
            return nil;
    }
    return nil;

}

/* Sqlite3Row */

Sqlite3Row::Sqlite3Row(char **row)
{
    this->row = row;
}
String Sqlite3Row::col(int index)
{
    return String(row[index]);
}

