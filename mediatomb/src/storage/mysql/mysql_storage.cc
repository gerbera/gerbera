/*  Mysql_storage.cc - this file is part of MediaTomb.

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

#ifdef HAVE_MYSQL

#include "mysql_storage.h"
#include "config_manager.h"
#include "destroyer.h"

using namespace zmm;
using namespace mxml;

MysqlStorage::MysqlStorage() : SQLStorage()
{
    shutdownFlag = false;
}
MysqlStorage::~MysqlStorage()
{
}

void MysqlStorage::init()
{
    Ref<MSTask> task;
    
    taskQueue = Ref<Array<MSTask> >(new Array<MSTask>());
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    pthread_mutex_init(&taskMutex, NULL);
    pthread_cond_init(&taskCond, NULL);
    
    //threads = new _threads[MYSQL_STORAGE_START_THREADS];
    for (int i=0; i<MYSQL_STORAGE_START_THREADS; ++i)
    {
        int ret;
        struct _threads *thread = &threads[i];
        thread->instance = this;
        thread->error = nil;
        ret = pthread_mutex_init(&thread->mutex, NULL);
        ret = pthread_cond_init(&thread->cond, NULL);
        ret = pthread_create(
            &thread->thread,
            &attr,
            MysqlStorage::staticThreadProc,
            thread
        );
        pthread_mutex_lock(&thread->mutex);
        if (thread->error == nil)
        {
            log_debug("waiting for thread %d ...\n", i);
            pthread_cond_wait(&thread->cond, &thread->mutex);
            log_debug("thread %d signalled.\n", i);
        }
        String error = thread->error;
        pthread_mutex_unlock(&thread->mutex);
        
        pthread_cond_destroy(&thread->cond);
        pthread_mutex_destroy(&thread->mutex);
        if (thread->error != _(""))
        {
            pthread_attr_destroy(&attr);
            throw _StorageException(thread->error);
        }
    }
    log_debug("leaving.\n");
    pthread_attr_destroy(&attr);
    SQLStorage::init();
}

String MysqlStorage::quote(String value)
{
    // \todo replace with "_real_"
    char *q = (char *)MALLOC(value.length() * 2 + 2);
    *q = '\'';
    int size = mysql_escape_string(q + 1, value.c_str(), value.length());
    q[size + 1] = '\'';
    String ret(q, size + 2);
    FREE(q);
    return ret;
}

String MysqlStorage::getError(MYSQL *db)
{
    Ref<StringBuffer> err_buf(new StringBuffer());
    *err_buf << "mysql_error(" << String::from(mysql_errno(db));
    *err_buf << "): \"" << String(mysql_error(db)) << "\"";
    return err_buf->toString();
}

void MysqlStorage::mutexCondInit(pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    pthread_mutex_init(mutex, NULL);
    pthread_cond_init(cond, NULL);
}

void MysqlStorage::waitForTask(Ref<MSTask> task, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    addTask(task);
    // We check before we lock first, because there is no need to lock then.
    // This is certainly not needed, but maybe faster.
    if (task->is_running())
    { 
        pthread_mutex_lock(mutex);
        if (task->is_running()) 
        { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal 
            pthread_cond_wait(cond, mutex); // waiting for the task to complete
        }
        pthread_mutex_unlock(mutex);
    }
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(cond);
    if (task->getError() != nil)
    {
        log_error("%s\n", task->getError().c_str());
        throw Exception(task->getError());
    }
}


Ref<SQLResult> MysqlStorage::select(String query)
{ 
    pthread_mutex_t selectMutex;
    pthread_cond_t selectCond;
    mutexCondInit(&selectMutex, &selectCond);
    Ref<MSSelectTask> ptask (new MSSelectTask(query, &selectMutex, &selectCond));
    waitForTask(RefCast(ptask, MSTask), &selectMutex, &selectCond);
    return RefCast(ptask->pres, SQLResult);
}

int MysqlStorage::exec(String query, bool getLastInsertId)
{
    pthread_mutex_t execMutex;
    pthread_cond_t execCond;
    mutexCondInit(&execMutex, &execCond);
    Ref<MSExecTask> ptask (new MSExecTask(query, getLastInsertId, &execMutex, &execCond));
    waitForTask(RefCast(ptask, MSTask), &execMutex, &execCond);
    if (getLastInsertId) return ptask->lastInsertId;
    else return -1;
}

void MysqlStorage::shutdown()
{
    pthread_mutex_lock(&taskMutex);
    shutdownFlag = true;
    pthread_cond_broadcast(&taskCond);
    pthread_mutex_unlock(&taskMutex);
}

void *MysqlStorage::staticThreadProc(void *arg)
{
    struct _threads *thread = (_threads*) arg;
    thread->instance->threadProc(thread);
    log_debug("exiting thread\n");
    pthread_exit(NULL);
    return NULL;
}

void MysqlStorage::staticThreadCleanup(void *arg)
{
    log_debug("mysql_close\n");
    mysql_close((MYSQL*)arg);
}

void MysqlStorage::threadProc(struct _threads *thread)
{
    Ref<MSTask> task;
    
    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    String dbHost = config->getOption(_("/server/storage/host"));
    String dbName = config->getOption(_("/server/storage/database"));
    String dbUser = config->getOption(_("/server/storage/username"));
    String dbPort = config->getOption(_("/server/storage/port"));
    String dbPass;
    if (config->getElement(_("/server/storage/password")) == nil)
        dbPass = nil;
    else
        dbPass = config->getOption(_("/server/storage/password"), _(""));
    
    
    String dbSock;
    if (config->getElement(_("/server/storage/socket")) == nil)
        dbSock = nil;
    else
        dbSock = config->getOption(_("/server/storage/socket"), _(""));
    
    MYSQL db;
    MYSQL *res_mysql;
    
    res_mysql = mysql_init(&db);
    if(! res_mysql)
    {
        pthread_mutex_lock(&thread->mutex);
        thread->error = getError(&db);
        pthread_cond_signal(&thread->cond);
        pthread_mutex_unlock(&thread->mutex);
        pthread_exit(NULL);
    }
    
    pthread_cleanup_push(MysqlStorage::staticThreadCleanup, res_mysql);
    
    res_mysql = mysql_real_connect(&db,
        dbHost.c_str(),
        dbUser.c_str(),
        (dbPass == nil ? NULL : dbPass.c_str()),
        dbName.c_str(),
        dbPort.toInt(), // port
        (dbSock == nil ? NULL : dbSock.c_str()), // socket
        0 // flags
    );
    if(! res_mysql)
    {
        pthread_mutex_lock(&thread->mutex);
        thread->error = getError(&db);
        pthread_cond_signal(&thread->cond);
        pthread_mutex_unlock(&thread->mutex);
        pthread_exit(NULL);
    }
    
    //signalling calling thread that we are ready and no errors occured
    pthread_mutex_lock(&thread->mutex);
    thread->error = _("");
    pthread_cond_signal(&thread->cond);
    pthread_mutex_unlock(&thread->mutex);
    
    log_debug("mysql thread %u started successfully.\n", pthread_self());
    
    while(! shutdownFlag)
    {
        pthread_mutex_lock(&taskMutex);
        if(taskQueue->size() == 0)
        {
            /* if nothing to do, sleep until awakened */
            pthread_cond_wait(&taskCond, &taskMutex);
            pthread_mutex_unlock(&taskMutex);
            continue;
        }
        else
        {
            task = taskQueue->get(0);
            taskQueue->remove(0, 1);
        }
        pthread_mutex_unlock(&taskMutex);
        
        try
        {
            task->run(&db);
            task->sendSignal();
        }
        catch (Exception e)
        {
            task->sendSignal(e.getMessage()+getError(&db));
        }
    }
    pthread_cleanup_pop(1);
}

int MysqlStorage::addTask(zmm::Ref<MSTask> task)
{
    int ret = false;
    pthread_mutex_lock(&taskMutex);
    if (taskQueue->size() >= 1)
        ret = true;
    taskQueue->append(task);
    pthread_cond_signal(&taskCond);
    pthread_mutex_unlock(&taskMutex);
    return ret;
}

/* MSTask */

MSTask::MSTask(pthread_mutex_t *mutex, pthread_cond_t *cond) : Object()
{
    running = true;
    this->cond = cond;
    this->mutex = mutex;
    this->error = nil;
}
bool MSTask::is_running()
{
    return running;
}

void MSTask::sendSignal()
{
    pthread_mutex_lock(mutex);
    running=false;
    pthread_cond_signal(cond);
    pthread_mutex_unlock(mutex);
}

void MSTask::sendSignal(String error)
{
    this->error = error;
    sendSignal();
}

/* MSSelectTask */

MSSelectTask::MSSelectTask(zmm::String query, pthread_mutex_t* mutex, pthread_cond_t* cond) : MSTask(mutex, cond)
{
    this->query = query;
    this->cond = cond;
    this->mutex = mutex;
}

void MSSelectTask::run(MYSQL *db)
{
    
//#ifdef HAVE_MYSQL_STMT_INIT
//#ifdef mysql_stmt_init
//    
//#else
//    log_debug("UUUUUUUUhhh\n");
//#endif
//    log_debug("thread_safe? %dn", mysql_thread_safe());
//    mysql_thread_init();
    
//    log_debug("doing mysql_select. thread: %d\n", pthread_self());
    
    int res;
    res = mysql_real_query(db, query.c_str(), query.length());
    if(res)
    {
        //reportError(query, db);
        throw _StorageException(_("Mysql: mysql_real_query() failed: "));
    }

    MYSQL_RES *mysql_res;
    mysql_res = mysql_store_result(db);
    if(! mysql_res)
    {
        //reportError(query, db);
        throw _StorageException(_("Mysql: mysql_store_result() failed: "));
    }
    pres = Ref<MysqlResult> (new MysqlResult(mysql_res));
}


/* MSExecTask */

MSExecTask::MSExecTask(zmm::String query, bool getLastInsertId, pthread_mutex_t* mutex, pthread_cond_t* cond) : MSTask(mutex, cond)
{
    this->query = query;
    this->getLastInsertId = getLastInsertId;
}

void MSExecTask::run(MYSQL *db)
{
    
    log_debug("doing mysql_exec. thread: %d\n", pthread_self());
    
    int res = mysql_real_query(db, query.c_str(), query.length());
    if(res)
    {
        throw _StorageException(_("mysql: query error: "));
    }
    if (getLastInsertId)
    {
        lastInsertId = mysql_insert_id(db);
    }
}

/* MysqlResult */
MysqlResult::MysqlResult(MYSQL_RES *mysql_res) : SQLResult()
{
    this->mysql_res = mysql_res;
    nullRead = false;
}
MysqlResult::~MysqlResult()
{
    if(mysql_res)
    {
        if (! nullRead)
        {
            MYSQL_ROW mysql_row;
            while((mysql_row = mysql_fetch_row(mysql_res)) != NULL); // read out data
        }
        mysql_free_result(mysql_res);
        mysql_res = NULL;        
    }
}
Ref<SQLRow> MysqlResult::nextRow()
{   
    MYSQL_ROW mysql_row;
    mysql_row = mysql_fetch_row(mysql_res);
    if(mysql_row)
    {
        return Ref<SQLRow>(new MysqlRow(mysql_row));
    }
    nullRead = true;
    mysql_free_result(mysql_res);
    mysql_res = NULL;
    return nil;
}


/* MysqlRow */

MysqlRow::MysqlRow(MYSQL_ROW mysql_row)
{
    this->mysql_row = mysql_row;
}
String MysqlRow::col(int index)
{
    return String(mysql_row[index]);
}

#endif // HAVE_MYSQL

