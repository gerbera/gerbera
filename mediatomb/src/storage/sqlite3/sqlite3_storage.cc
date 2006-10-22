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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_SQLITE3

#include "sqlite3_storage.h"

#include "common.h"
#include "config_manager.h"

#include "sqlite3_create_sql.h"
#include <zlib.h>

#define SL3_INITITAL_QUEUE_SIZE 20

using namespace zmm;
using namespace mxml;

Sqlite3Storage::Sqlite3Storage() : SQLStorage()
{
    shutdownFlag = false;
    dbRemovesDeps = false;
    table_quote_begin = '"';
    table_quote_end = '"';
    startupError = nil;
}

void Sqlite3Storage::init()
{
    int ret;
    
    pthread_mutex_init(&sqliteMutex, NULL);
    pthread_cond_init(&sqliteCond, NULL);
    
    pthread_mutex_lock(&sqliteMutex);
    
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
    
    pthread_cond_wait(&sqliteCond, &sqliteMutex);
    pthread_mutex_unlock(&sqliteMutex);
    
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
        Ref<SLInitTask> ptask (new SLInitTask());
        addTask(RefCast(ptask, SLTask));
        ptask->waitForTask();
        dbVersion = getInternalSetting(_("db_version"));
        if (dbVersion == nil)
        {
            shutdown();
            throw _Exception(_("error while creating db"));
        }
    }
    log_debug("db_version: %s\n", dbVersion.c_str());
    
    //pthread_attr_destroy(&attr);
    
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

void Sqlite3Storage::reportError(String query, sqlite3 *db)
{
    log_error("SQLITE3: (%d) %s\nQuery:%s\n",
        sqlite3_errcode(db),
        sqlite3_errmsg(db),
        (query == nil) ? "unknown" : query.c_str()
    );
}

Ref<SQLResult> Sqlite3Storage::select(String query)
{
    Ref<SLSelectTask> ptask (new SLSelectTask(query));
    addTask(RefCast(ptask, SLTask));
    ptask->waitForTask();
    return ptask->getResult();
}

int Sqlite3Storage::exec(String query, bool getLastInsertId)
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
    
    pthread_cond_signal(&sqliteCond);
    
    while(! shutdownFlag)
    {
        lock();
        if((task = taskQueue->dequeue()) == nil)
        {
            /* if nothing to do, sleep until awakened */
            pthread_cond_wait(&sqliteCond, &sqliteMutex);
            unlock();
            continue;
        }
        unlock();
        
        try
        {
            task->run(db, this);
            task->sendSignal();
        }
        catch (Exception e)
        {
            task->sendSignal(e.getMessage());
        }
    }
    lock();
    taskQueueOpen = false;
    while((task = taskQueue->dequeue()) != nil)
    {
        task->sendSignal(_("Sorry, sqlite3 thread is shutting down"));
    }
    unlock();
    if (db)
        sqlite3_close(db);
}                                             

void Sqlite3Storage::lock()
{
    pthread_mutex_lock(&sqliteMutex);
}
void Sqlite3Storage::unlock()
{
    pthread_mutex_unlock(&sqliteMutex);
}
void Sqlite3Storage::signal()
{
    pthread_cond_signal(&sqliteCond);
}

void Sqlite3Storage::addTask(zmm::Ref<SLTask> task)
{
    if (! taskQueueOpen)
        throw _Exception(_("sqlite3 task queue is already closed"));
    lock();
    if (! taskQueueOpen)
    {
        unlock();
        throw _Exception(_("sqlite3 task queue is already closed"));
    }
    taskQueue->enqueue(task);
    signal();
    unlock();
}

void Sqlite3Storage::shutdown()
{
    shutdownFlag = true;
    lock();
    signal();
    unlock();
    if (sqliteThread)
        pthread_join(sqliteThread, NULL);
    sqliteThread = 0;
}

void Sqlite3Storage::storeInternalSetting(String key, String value)
{
    Ref<StringBuffer> q(new StringBuffer());
    *q << "INSERT OR REPLACE INTO " INTERNAL_SETTINGS_TABLE " (`key`, `value`) "
    "VALUES (" << quote(key) << ", "<< quote(value) << ") ";
    this->exec(q->toString());
}


/* SLTask */

SLTask::SLTask() : Object()
{
    running = true;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    error = nil;
}
bool SLTask::is_running()
{
    return running;
}

void SLTask::sendSignal()
{
    pthread_mutex_lock(&mutex);
    running=false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void SLTask::sendSignal(String error)
{
    this->error = error;
    sendSignal();
}

void SLTask::waitForTask()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        pthread_mutex_lock(&mutex);
        if (is_running()) { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal 
            pthread_cond_wait(&cond, &mutex); // waiting for the task to complete
        }
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    if (getError() != nil)
    {
        log_error("%s\n", getError().c_str());
        throw _Exception(getError());
    }
}

/* SLInitTask */

void SLInitTask::run(sqlite3 *db, Sqlite3Storage *sl)
{
    unsigned char buf[SL2_CREATE_SQL_INFLATED_SIZE + 1]; // +1 for '\0' at the end of the string
    unsigned long uncompressed_size = SL2_CREATE_SQL_INFLATED_SIZE;
    int ret = uncompress(buf, &uncompressed_size, sqlite3_create_sql, SL2_CREATE_SQL_DEFLATED_SIZE);
    if (ret != Z_OK || uncompressed_size != SL2_CREATE_SQL_INFLATED_SIZE)
        throw _StorageException(_("Error while uncompressing sqlite3 create sql. returned: ") + ret);
    buf[SL2_CREATE_SQL_INFLATED_SIZE] = '\0';
    
    char *err;
    ret = sqlite3_exec(
        db,
        (char *)buf,
        NULL,
        NULL,
        &err
    );
    
    if(ret != SQLITE_OK)
    {
        sl->reportError(_("-"), db);
        throw _StorageException(_("error while creating db"));
    }
}


/* SLSelectTask */

SLSelectTask::SLSelectTask(zmm::String query) : SLTask()
{
    this->query = query;
}

void SLSelectTask::run(sqlite3 *db, Sqlite3Storage *sl)
{
    char *err;
    pres = Ref<Sqlite3Result>(new Sqlite3Result()); 
    
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
        sl->reportError(query, db);
        throw _StorageException(_("Sqlite3: query error"));
    }

    pres->row = pres->table;
    pres->cur_row = 0;
}


/* SLExecTask */

SLExecTask::SLExecTask(zmm::String query, bool getLastInsertId) : SLTask()
{
    this->query = query;
    this->getLastInsertIdFlag = getLastInsertId;
}

void SLExecTask::run(sqlite3 *db, Sqlite3Storage *sl)
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
        sl->reportError(query, db);
        throw _StorageException(_("Sqlite3: query error"));
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
String Sqlite3Row::col(int index)
{
    return String(row[index]);
}


#endif // HAVE_SQlITE3

