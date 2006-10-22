/*  Mysql_storage.h - this file is part of MediaTomb.

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

#ifdef HAVE_MYSQL

#ifndef __MYSQL_STORAGE_H__
#define __MYSQL_STORAGE_H__

#include "common.h"
#include "storage/sql_storage.h"
#include "sync.h"
#include <mysql.h>

class MysqlResult;

/*
/// \brief A virtual class that represents a task to be done by the mysql thread.
class MSTask : public zmm::Object
{
public:
    /// \brief Instantiate a task
    /// \param mutex the pthread_mutex for notifying the creator of the task, that the task is finished
    /// \param cond the pthread_cond for notifying the creator of the task, that the task is finished
    MSTask(pthread_mutex_t* mutex, pthread_cond_t* cond);
    
    /// \brief run the mysql task
    /// \param sl The instance of MysqlStorage to do the queries with.
    virtual void run(MYSQL *db) = 0;
    
    /// \brief returns true if the task is not completed
    /// \return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running();
    
    /// \brief modify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();
    
    void sendSignal(zmm::String error);
    
    zmm::String getError() { return error; }
    
protected:
    /// \brief true as long as the task is not finished
    ///
    /// The value is set by the constructor to true and then to false be sendSignal()
    bool running;
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
    zmm::String error;
};

/// \brief A task for the mysql thread to do a SQL select.
class MSSelectTask : public MSTask
{
public:
    /// \brief Constructor for the mysql select task
    /// \param query The SQL query string
    /// \param mutex see MSTask::MSTask()
    /// \param cond see MSTask::MSTask()
    MSSelectTask(zmm::String query, pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(MYSQL *db);
    /// \brief The MysqlResult
    zmm::Ref<MysqlResult> pres;
protected:
    /// \brief The SQL query string
    zmm::String query;
};

/// \brief A task for the mysql thread to do a SQL exec.
class MSExecTask : public MSTask
{
public:
    /// \brief Constructor for the mysql exec task
    /// \param query The SQL query string
    /// \param mutex see MSTask::MSTask()
    /// \param cond see MSTask::MSTask()
    MSExecTask(zmm::String query, bool getLastInsertId, pthread_mutex_t* mutex, pthread_cond_t* cond);
    virtual void run(MYSQL *db);
    int lastInsertId;
protected:
    /// \brief The SQL query string
    zmm::String query;
    bool getLastInsertId;
};

/// \brief A task for the mysql thread to get the "last_insert_id".
class MSGetLastInsertIdTask : public MSTask
{
public:
    /// \brief Constructor for the mysql "get last insert id" task
    /// \param mutex see MSTask::MSTask()
    /// \param cond see MSTask::MSTask()
    MSGetLastInsertIdTask(pthread_mutex_t* mutex, pthread_cond_t* cond);
    
    virtual void run(MYSQL *db);
    
    /// \brief the result of the task
    int lastInsertId;
};
*/

class MysqlRow : public SQLRow
{
public:
    MysqlRow(MYSQL_ROW mysql_row, zmm::Ref<SQLResult> sqlResult);
    virtual zmm::String col(int index);
protected:
    MYSQL_ROW mysql_row;
};

class MysqlResult : public SQLResult
{
    int nullRead;
public:
    MysqlResult(MYSQL_RES *mysql_res);
    virtual ~MysqlResult();
    virtual zmm::Ref<SQLRow> nextRow();
    virtual unsigned long long getNumRows() { return mysql_num_rows(mysql_res); }
protected:
    MYSQL_RES *mysql_res;
};

class MysqlStorage : public SQLStorage
{
public:
    MysqlStorage();
    virtual ~MysqlStorage();

    virtual void init();
    virtual zmm::String quote(zmm::String str);
    virtual zmm::Ref<SQLResult> select(zmm::String query);
    virtual int exec(zmm::String query, bool getLastInsertId = false);
    virtual void shutdown();
    virtual void storeInternalSetting(zmm::String key, zmm::String value);
protected:
    
    MYSQL db;
    
    bool mysql_connection;
    
    zmm::String getError(MYSQL *db);
    
    zmm::Ref<Mutex> mutex;
    
    /*
    struct _threads
    {
        pthread_t thread;
        pthread_cond_t cond;
        pthread_mutex_t mutex;
        MysqlStorage *instance;
        zmm::String error;
    } threads[MYSQL_STORAGE_START_THREADS];
    
    static void *staticThreadProc(void *arg);
    void threadProc(struct MysqlStorage::_threads *thread);
    static void staticThreadCleanup(void *arg);
    */
    
    //int addTask(zmm::Ref<MSTask> task);
    
    /// \brief is set to true by shutdown() if the mysql connection is already closed
    //bool shutdownFlag;
    
    pthread_key_t mysql_init_key;
    bool mysql_init_key_initialized;
    
    void checkMysqlThreadInit();
    
    /// \brief the tasks to be done by the mysql thread
    //zmm::Ref<zmm::Array<MSTask> > taskQueue;
    //pthread_cond_t taskCond;
    //pthread_mutex_t taskMutex;
    
    //void mutexCondInit(pthread_mutex_t *mutex, pthread_cond_t *cond);
    //void waitForTask(zmm::Ref<MSTask> task, pthread_mutex_t *mutex, pthread_cond_t *cond);
};


#endif // __MYSQL_STORAGE_H__

#endif // HAVE_MYSQL
