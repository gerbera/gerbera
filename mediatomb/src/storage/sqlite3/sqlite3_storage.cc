/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    sqlite3_storage.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file sqlite3_storage.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_SQLITE3

#include "sqlite3_storage.h"

#include "common.h"
#include "config_manager.h"

#ifdef AUTO_CREATE_DATABASE
    #include "sqlite3_create_sql.h"
    #include <zlib.h>
#endif

#define SL3_INITITAL_QUEUE_SIZE 20

using namespace zmm;
using namespace mxml;

Sqlite3Storage::Sqlite3Storage() : SQLStorage()
{
    shutdownFlag = false;
    table_quote_begin = '"';
    table_quote_end = '"';
    startupError = nil;
    sqliteMutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(sqliteMutex));
}

void Sqlite3Storage::init()
{
    SQLStorage::init();
    
    int ret;
    
    AUTOLOCK(sqliteMutex);
    /*
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    */
    
    taskQueue = Ref<ObjectQueue<SLTask> >(new ObjectQueue<SLTask>(SL3_INITITAL_QUEUE_SIZE));
    taskQueueOpen = true;
    
    ret = pthread_create(
        &sqliteThread,
        NULL, //&attr,
        Sqlite3Storage::staticThreadProc,
        this
    );
    
    cond->wait();
    AUTOUNLOCK();
    if (startupError != nil)
        throw _StorageException(startupError);
    
    String dbVersion = nil;
    try
    {
        dbVersion = getInternalSetting(_("db_version"));
    }
    catch (Exception)
    {
    }
    
    if (dbVersion == nil)
    {
#ifdef AUTO_CREATE_DATABASE
        log_info("database doesn't seem to exist. automatically creating database...\n");
        Ref<SLInitTask> ptask (new SLInitTask());
        addTask(RefCast(ptask, SLTask));
        ptask->waitForTask();
        dbVersion = getInternalSetting(_("db_version"));
        if (dbVersion == nil)
        {
            shutdown();
            throw _Exception(_("error while creating database"));
        }
        log_info("database created successfully.\n");
#else
        shutdown();
        throw _Exception(_("database doesn't seem to exist yet and autocreation wasn't compiled in"));
#endif
    }
    log_debug("db_version: %s\n", dbVersion.c_str());
    if (! string_ok(dbVersion) || dbVersion != "1")
        throw _Exception(_("The database seems to be from a newer version!"));
    
    //pthread_attr_destroy(&attr);
}

String Sqlite3Storage::quote(String value)
{
    char *q = sqlite3_mprintf("'%q'",
        (value == nil ? "" : value.c_str()));
    String ret(q);
    sqlite3_free(q);
    return ret;
}

String Sqlite3Storage::getError(String query, String error, sqlite3 *db)
{
    return _("SQLITE3: (") + sqlite3_errcode(db) + ") " 
        + sqlite3_errmsg(db) +"\nQuery:" + (query == nil ? _("unknown") : query) + "\nerror: " + (error == nil ? _("unknown") : error);
}

Ref<SQLResult> Sqlite3Storage::select(const char *query, int length)
{
    Ref<SLSelectTask> ptask (new SLSelectTask(query));
    addTask(RefCast(ptask, SLTask));
    ptask->waitForTask();
    return ptask->getResult();
}

int Sqlite3Storage::exec(const char *query, int length, bool getLastInsertId)
{
    Ref<SLExecTask> ptask (new SLExecTask(query, getLastInsertId));
    addTask(RefCast(ptask, SLTask));
    ptask->waitForTask();
    if (getLastInsertId) return ptask->getLastInsertId();
    else return -1;
}

void *Sqlite3Storage::staticThreadProc(void *arg)
{
    Sqlite3Storage *inst = (Sqlite3Storage *)arg;
    inst->threadProc();
    log_debug("Sqlite3Storage::staticThreadProc - exiting thread\n");
    pthread_exit(NULL);
    return NULL;
}

void Sqlite3Storage::threadProc()
{
    Ref<SLTask> task;
    
    sqlite3 *db;
    
    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    String dbFilePath = config->getOption(_("/server/storage/database-file"));
    
    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if(res != SQLITE_OK)
    {
        startupError = _("Sqlite3Storage.init: could not open ") +
            dbFilePath;
    }
    AUTOLOCK(sqliteMutex);
    cond->signal();
    
    while(! shutdownFlag)
    {
        if((task = taskQueue->dequeue()) == nil)
        {
            /* if nothing to do, sleep until awakened */
            cond->wait();
            continue;
        }
        AUTOUNLOCK();
        try
        {
            task->run(db, this);
            task->sendSignal();
        }
        catch (Exception e)
        {
            task->sendSignal(e.getMessage());
        }
        AUTORELOCK();
    }
    
    taskQueueOpen = false;
    while((task = taskQueue->dequeue()) != nil)
    {
        task->sendSignal(_("Sorry, sqlite3 thread is shutting down"));
    }
    if (db)
        sqlite3_close(db);
}

void Sqlite3Storage::addTask(zmm::Ref<SLTask> task)
{
    if (! taskQueueOpen)
        throw _Exception(_("sqlite3 task queue is already closed"));
    AUTOLOCK(sqliteMutex);
    if (! taskQueueOpen)
    {
        throw _Exception(_("sqlite3 task queue is already closed"));
    }
    taskQueue->enqueue(task);
    signal();
}

void Sqlite3Storage::shutdown()
{
    log_debug("start\n");
    AUTOLOCK(sqliteMutex);
    shutdownFlag = true;
    log_debug("signalling...\n");
    signal();
    AUTOUNLOCK();
    log_debug("waiting for thread\n");
    if (sqliteThread)
        pthread_join(sqliteThread, NULL);
    sqliteThread = 0;
    log_debug("end\n");
}

void Sqlite3Storage::storeInternalSetting(String key, String value)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "INSERT OR REPLACE INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (" << QTB << "key" << QTE << ", " << QTB << "value" << QTE << ") "
    "VALUES (" << quote(key) << ", "<< quote(value) << ") ";
    this->execSB(q);
}

/* SLTask */

SLTask::SLTask() : Object()
{
    running = true;
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    error = nil;
}
bool SLTask::is_running()
{
    return running;
}

void SLTask::sendSignal()
{
    AUTOLOCK(mutex);
    running=false;
    cond->signal();
}

void SLTask::sendSignal(String error)
{
    this->error = error;
    sendSignal();
}

void SLTask::waitForTask()
{
    if (is_running())
    { // we check before we lock first, because there is no need to lock then
        AUTOLOCK(mutex);
        if (is_running())
        { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal 
            cond->wait(); // waiting for the task to complete
        }
    }
    
    if (getError() != nil)
    {
        log_error("%s\n", getError().c_str());
        throw _Exception(getError());
    }
}

#ifdef AUTO_CREATE_DATABASE
/* SLInitTask */

void SLInitTask::run(sqlite3 *db, Sqlite3Storage *sl)
{
    unsigned char buf[SL3_CREATE_SQL_INFLATED_SIZE + 1]; // +1 for '\0' at the end of the string
    unsigned long uncompressed_size = SL3_CREATE_SQL_INFLATED_SIZE;
    int ret = uncompress(buf, &uncompressed_size, sqlite3_create_sql, SL3_CREATE_SQL_DEFLATED_SIZE);
    if (ret != Z_OK || uncompressed_size != SL3_CREATE_SQL_INFLATED_SIZE)
        throw _StorageException(_("Error while uncompressing sqlite3 create sql. returned: ") + ret);
    buf[SL3_CREATE_SQL_INFLATED_SIZE] = '\0';
    
    char *err = NULL;
    ret = sqlite3_exec(
        db,
        (char *)buf,
        NULL,
        NULL,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = String(err);
        sqlite3_free(err);
    }
    if(ret != SQLITE_OK)
    {
        throw _StorageException(sl->getError(String((char *)buf), error, db));
    }
}

#endif

/* SLSelectTask */

SLSelectTask::SLSelectTask(const char *query) : SLTask()
{
    this->query = query;
}

void SLSelectTask::run(sqlite3 *db, Sqlite3Storage *sl)
{
    
    pres = Ref<Sqlite3Result>(new Sqlite3Result()); 
    
    char *err;
    int ret = sqlite3_get_table(
        db,
        query,
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = String(err);
        sqlite3_free(err);
    }
    if(ret != SQLITE_OK)
    {
        throw _StorageException(sl->getError(String(query), error, db));
    }
    
    pres->row = pres->table;
    pres->cur_row = 0;
}


/* SLExecTask */

SLExecTask::SLExecTask(const char *query, bool getLastInsertId) : SLTask()
{
    this->query = query;
    this->getLastInsertIdFlag = getLastInsertId;
}

void SLExecTask::run(sqlite3 *db, Sqlite3Storage *sl)
{
    char *err;
    int res = sqlite3_exec(
        db,
        query,
        NULL,
        NULL,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = String(err);
        sqlite3_free(err);
    }
    if(res != SQLITE_OK)
    {
        throw _StorageException(sl->getError(String(query), error, db));
    }
    if (getLastInsertIdFlag)
        lastInsertId = sqlite3_last_insert_rowid(db);
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
            Ref<Sqlite3Row> p (new Sqlite3Row(row, Ref<SQLResult>(this)));
            p->res = Ref<Sqlite3Result>(this);
            return RefCast(p, SQLRow);
        }
        else
            return nil;
    }
    return nil;

}

/* Sqlite3Row */

Sqlite3Row::Sqlite3Row(char **row, Ref<SQLResult> sqlResult) : SQLRow(sqlResult)
{
    this->row = row;
}

#endif // HAVE_SQlITE3

