/*  sqlite3_storage.h - this file is part of MediaTomb.

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

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include <sqlite3.h>

#include "storage/sql_storage.h"
#include "sync.h"

class Sqlite3Storage;
class Sqlite3Result;

class SLTask : public zmm::Object
{
public:
    SLTask(pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(Sqlite3Storage *sl) = 0;
    bool is_running(); 
protected:
    bool running;
    pthread_cond_t* cond;
    pthread_mutex_t* mutex;
    void sendSignal();
};

class SLSelectTask : public SLTask
{
public:
    SLSelectTask(zmm::String query, pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(Sqlite3Storage *sl);
    zmm::Ref<Sqlite3Result> pres;
protected:
    zmm::String query;
};

class SLExecTask : public SLTask
{
public:
    SLExecTask(zmm::String query, pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(Sqlite3Storage *sl);
protected:
    zmm::String query;
};

class SLGetLastInsertIdTask : public SLTask
{
public:
    SLGetLastInsertIdTask(pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(Sqlite3Storage *sl);
    int lastInsertId;
};

class Sqlite3Row : public SQLRow
{
public:
    Sqlite3Row(char **row);
    virtual zmm::String col(int index);
protected:
    char **row;
    zmm::Ref<Sqlite3Result> res;
    friend class Sqlite3Result;
};

class Sqlite3Result : public SQLResult
{
public:
    Sqlite3Result();
    virtual ~Sqlite3Result();
    virtual zmm::Ref<SQLRow> nextRow();
protected:
    char **table;
    char **row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class Sqlite3Storage;
    friend class SLSelectTask;
};


void unlock_func(void *data);

class Sqlite3Storage : public SQLStorage
{
public:
    Sqlite3Storage();
    virtual ~Sqlite3Storage();
    
    virtual void init();
    virtual zmm::String quote(zmm::String str);
    virtual zmm::Ref<SQLResult> select(zmm::String query);
    virtual void exec(zmm::String query);
    virtual int lastInsertID();
    virtual void shutdown();
    
protected:
    sqlite3 *db;
    
    static void *staticThreadProc(void *arg);
    void threadProc();
    
    int addTask(zmm::Ref<SLTask> task);
    
    pthread_t sqliteThread;
    pthread_cond_t sqliteCond;
    pthread_mutex_t sqliteMutex;
    
    bool shutdownFlag;
    
    zmm::Ref<zmm::Array<SLTask> > taskQueue;
    
    void mutexCondInit(pthread_mutex_t *mutex, pthread_cond_t *cond);
    void waitForTask(zmm::Ref<SLTask> task, pthread_mutex_t *mutex, pthread_cond_t *cond);
    
    void lock();
    void unlock();
    void signal();
    
    void reportError(zmm::String query);

    friend void unlock_func(void *data);
    friend class SLSelectTask;
    friend class SLExecTask;
    friend class SLGetLastInsertIdTask;
};


#endif // __SQLITE3_STORAGE_H__

