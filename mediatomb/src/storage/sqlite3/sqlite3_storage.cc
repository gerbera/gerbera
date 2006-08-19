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

using namespace zmm;
using namespace mxml;

Sqlite3Storage::Sqlite3Storage() : SQLStorage()
{
    db = NULL;
    shutdownFlag = false;
}

Sqlite3Storage::~Sqlite3Storage()
{
}

void Sqlite3Storage::init()
{
    
    int ret;
    
    ret = pthread_mutex_init(&sqliteMutex, NULL);
    ret = pthread_cond_init(&sqliteCond, NULL);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    taskQueue = Ref<Array<SLTask> >(new Array<SLTask>());
    
    ret = pthread_create(
        &sqliteThread,
        &attr,
        Sqlite3Storage::staticThreadProc,
        this
    );
    
    pthread_attr_destroy(&attr);
    
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
    log_error("SQLITE3: (%d) %s\nQuery:%s\n",
        sqlite3_errcode(db),
        sqlite3_errmsg(db),
        (query == nil) ? "unknown" : query.c_str()
    );
}


void Sqlite3Storage::mutexCondInit(pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    pthread_mutex_init(mutex, NULL);
    pthread_cond_init(cond, NULL);
}

void Sqlite3Storage::waitForTask(Ref<SLTask> task, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    addTask(task);
    if (task->is_running()) { // we check before we lock first, because there is no need to lock then
        pthread_mutex_lock(mutex);
        if (task->is_running()) { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal 
            pthread_cond_wait(cond, mutex); // waiting for the task to complete
        }
    }
    pthread_mutex_unlock(mutex);
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(cond);
    if (task->getError() != nil)
    {
        log_error("%s\n", task->getError().c_str());
        throw Exception(task->getError());
    }
}

Ref<SQLResult> Sqlite3Storage::select(String query)
{
    pthread_mutex_t selectMutex;
    pthread_cond_t selectCond;
    mutexCondInit(&selectMutex, &selectCond);
    Ref<SLSelectTask> ptask (new SLSelectTask(query, &selectMutex, &selectCond));
    waitForTask(RefCast(ptask, SLTask), &selectMutex, &selectCond);
    return RefCast(ptask->pres, SQLResult);
}

int Sqlite3Storage::exec(String query, bool getLastInsertId)
{
    pthread_mutex_t execMutex;
    pthread_cond_t execCond;
    mutexCondInit(&execMutex, &execCond);
    Ref<SLExecTask> ptask (new SLExecTask(query, getLastInsertId, &execMutex, &execCond));
    waitForTask(RefCast(ptask, SLTask), &execMutex, &execCond);
    if (getLastInsertId) return ptask->lastInsertId;
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
    
    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    String dbFilePath = config->getOption(_("/server/storage/database-file"));
    
    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if(res != SQLITE_OK)
    {
        throw _StorageException(_("Sqlite3Storage.init: could not open ") +
            dbFilePath);
    }
    while(! shutdownFlag)
    {
        lock();
        if(taskQueue->size() == 0)
        {
            /* if nothing to do, sleep until awakened */
            pthread_cond_wait(&sqliteCond, &sqliteMutex);
            unlock();
            continue;
        }
        else
        {
            task = taskQueue->get(0);
            taskQueue->remove(0, 1);
        }
        unlock();
        
        try
        {
            task->run(this);
            task->sendSignal();
        }
        catch (Exception e)
        {
            task->sendSignal(e.getMessage());
        }
    }
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

int Sqlite3Storage::addTask(zmm::Ref<SLTask> task)
{
    int ret = false;
    lock();
    int size = taskQueue->size();
    if (size >= 1)
        ret = true;
    taskQueue->append(task);
    signal();
    unlock();
    return ret;
}

void Sqlite3Storage::shutdown()
{
    shutdownFlag = true;
    lock();
    signal();
    unlock();
}


/* SLTask */

SLTask::SLTask(pthread_mutex_t *mutex, pthread_cond_t *cond) : Object()
{
    running = true;
    this->cond = cond;
    this->mutex = mutex;
    this->error = nil;
}
bool SLTask::is_running()
{
    return running;
}

void SLTask::sendSignal()
{
    pthread_mutex_lock(mutex);
    running=false;
    pthread_cond_signal(cond);
    pthread_mutex_unlock(mutex);
}

void SLTask::sendSignal(String error)
{
    this->error = error;
    sendSignal();
}


/* SLSelectTask */

SLSelectTask::SLSelectTask(zmm::String query, pthread_mutex_t* mutex, pthread_cond_t* cond) : SLTask(mutex, cond)
{
    this->query = query;
    this->cond = cond;
    this->mutex = mutex;
}

void SLSelectTask::run(Sqlite3Storage *sl)
{
    char *err;
    pres = Ref<Sqlite3Result>(new Sqlite3Result()); 
    
    int ret = sqlite3_get_table(
        sl->db,
        query.c_str(),
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err
    );
    
    if(ret != SQLITE_OK)
    {
        sl->reportError(query);
        throw _StorageException(_("Sqlite3: query error"));
    }

    pres->row = pres->table;
    pres->cur_row = 0;
}


/* SLExecTask */

SLExecTask::SLExecTask(zmm::String query, bool getLastInsertId, pthread_mutex_t* mutex, pthread_cond_t* cond) : SLTask(mutex, cond)
{
    this->query = query;
    this->getLastInsertId = getLastInsertId;
}

void SLExecTask::run(Sqlite3Storage *sl)
{
    char *err;
    int res = sqlite3_exec(
        sl->db,
        query.c_str(),
        NULL,
        NULL,
        &err
    );
    if(res != SQLITE_OK)
    {
        sl->reportError(query);
        throw _StorageException(_("Sqlite3: query error"));
    }
    if (getLastInsertId)
        lastInsertId = sqlite3_last_insert_rowid(sl->db);
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
            Ref<Sqlite3Row> p (new Sqlite3Row(row));
            p->res = Ref<Sqlite3Result>(this);
            return RefCast(p, SQLRow);
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

