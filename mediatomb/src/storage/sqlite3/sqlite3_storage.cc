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

#include "autoconfig.h"

#ifdef HAVE_SQLITE3

#include "sqlite3_storage.h"

#include "common.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

Sqlite3Storage::Sqlite3Storage() : SQLStorage()
{
    db = NULL;

    mutex = Ref<Mutex>(new Mutex());
    /*
    int res;
    pthread_mutexattr_t mutex_attr;
    res = pthread_mutexattr_init(&mutex_attr);
    res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock_mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    
    Recursive mutex is apparently not needed anymore
    */
}

Sqlite3Storage::~Sqlite3Storage()
{
    if (db)
        sqlite3_close(db);
}

void Sqlite3Storage::init()
{
    Ref<ConfigManager> config = ConfigManager::getInstance();

    String dbFilePath = config->getOption(_("/server/storage/database-file"));

    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if(res != SQLITE_OK)
    {
        throw StorageException(_("Sqlite3Storage.init: could not open ") +
            dbFilePath);
    }
    SQLStorage::init();
}

String Sqlite3Storage::quote(String value)
{
    char *q = sqlite3_mprintf("'%q'",
        (value == nil ? "" : value.c_str()));
    String ret(q);
    sqlite3_free(q);
    return ret;
}

void Sqlite3Storage::reportError(String query)
{
    log_warning(("SQLITE3: (%d) %s\nQuery:%s\n",
        sqlite3_errcode(db),
        sqlite3_errmsg(db),
        (query == nil) ? "unknown" : query.c_str()
    ));
}


Ref<SQLResult> Sqlite3Storage::select(String query)
{
    lock();

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
        unlock();
        reportError(query);
        throw StorageException(_("Sqlite3: query error"));
    }

    pres->row = pres->table;
    pres->cur_row = 0;

    unlock();

    return res;
}

void Sqlite3Storage::exec(String query)
{
    lock();

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
        unlock();
        reportError(query);
        throw StorageException(_("Sqlite3: query error"));
    }

    unlock();
}

int Sqlite3Storage::lastInsertID()
{
    return (int)sqlite3_last_insert_rowid(db);
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

#endif // HAVE_SQlITE3

