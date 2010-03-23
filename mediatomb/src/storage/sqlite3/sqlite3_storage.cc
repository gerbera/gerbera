/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sqlite3_storage.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
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

// updates 1->2
#define SQLITE3_UPDATE_1_2_1 "DROP INDEX mt_autoscan_obj_id"
#define SQLITE3_UPDATE_1_2_2 "CREATE UNIQUE INDEX mt_autoscan_obj_id ON mt_autoscan(obj_id)"
#define SQLITE3_UPDATE_1_2_3 "ALTER TABLE \"mt_autoscan\" ADD \"path_ids\" text"
#define SQLITE3_UPDATE_1_2_4 "UPDATE \"mt_internal_setting\" SET \"value\"='2' WHERE \"key\"='db_version' AND \"value\"='1'"

// updates 2->3
#define SQLITE3_UPDATE_2_3_1 "ALTER TABLE \"mt_cds_object\" ADD \"service_id\" varchar(255) default NULL"
#define SQLITE3_UPDATE_2_3_2 "CREATE INDEX mt_cds_object_service_id ON mt_cds_object(service_id)"
#define SQLITE3_UPDATE_2_3_3 "UPDATE \"mt_internal_setting\" SET \"value\"='3' WHERE \"key\"='db_version' AND \"value\"='2'"

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
    insertBuffer = nil;
    dirty = false;
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
    
    String dbFilePath = ConfigManager::getInstance()->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
    
    // check for db-file
    if (access(dbFilePath.c_str(), R_OK | W_OK) != 0 && errno != ENOENT)
        throw _StorageException(nil, _("Error while accessing sqlite database file (") + dbFilePath +"): " + mt_strerror(errno));
    
    
    
    taskQueue = Ref<ObjectQueue<SLTask> >(new ObjectQueue<SLTask>(SL3_INITITAL_QUEUE_SIZE));
    taskQueueOpen = true;
    
    ret = pthread_create(
        &sqliteThread,
        NULL, //&attr,
        Sqlite3Storage::staticThreadProc,
        this
    );
    
    // wait for sqlite3 thread to become ready
    cond->wait();
    AUTOUNLOCK();
    if (startupError != nil)
        throw _StorageException(nil, startupError);
    
    
    String dbVersion = nil;
    try
    {
        dbVersion = getInternalSetting(_("db_version"));
    }
    catch (Exception)
    {
        log_warning("Sqlite3 database seems to be corrupt or doesn't exist yet.\n");
        // database seems to be corrupt or nonexistent
        if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_STORAGE_SQLITE_RESTORE))
        {
            // try to restore database
            
            // checking for backup file
            String dbFilePathbackup = dbFilePath + ".backup";
            if (access(dbFilePathbackup.c_str(), R_OK) == 0)
            {
                try
                {
                    // trying to copy backup file
                    Ref<SLBackupTask> btask (new SLBackupTask(true));
                    this->addTask(RefCast(btask, SLTask));
                    btask->waitForTask();
                    dbVersion = getInternalSetting(_("db_version"));
                }
                catch (Exception)
                {
                }
            }
            
            if (dbVersion == nil)
            {
#ifdef AUTO_CREATE_DATABASE
                log_info("no sqlite3 backup is available or backup is corrupt. automatically creating database...\n");
                Ref<SLInitTask> ptask (new SLInitTask());
                addTask(RefCast(ptask, SLTask));
                try
                {
                    ptask->waitForTask();
                    dbVersion = getInternalSetting(_("db_version"));
                }
                catch (Exception e)
                {
                    shutdown();
                    throw _Exception(_("error while creating database: ") + e.getMessage());
                }
                log_info("database created successfully.\n");
#else
                shutdown();
                throw _Exception(_("database doesn't seem to exist yet and autocreation wasn't compiled in"));
#endif
            }
        }
        else
        {
            // fail because restore option is false
            shutdown();
            throw _Exception(_("sqlite3 database seems to be corrupt and the 'on-error' option is set to 'fail'"));
        }
    }
    
    if (dbVersion == nil)
    {
        shutdown();
        throw _Exception(_("sqlite3 database seems to be corrupt and restoring from backup failed"));
    }
    
    
    _exec("PRAGMA locking_mode = EXCLUSIVE");
    int synchronousOption = ConfigManager::getInstance()->getIntOption(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);
    Ref<StringBuffer> buf(new StringBuffer());
    *buf << "PRAGMA synchronous = " << synchronousOption;
    SQLStorage::exec(buf);
    
    
    
    log_debug("db_version: %s\n", dbVersion.c_str());
    
    /* --- database upgrades --- */
    
    if (dbVersion == "1")
    {
        log_info("Doing an automatic database upgrade from database version 1 to version 2...\n");
        _exec(SQLITE3_UPDATE_1_2_1);
        _exec(SQLITE3_UPDATE_1_2_2);
        _exec(SQLITE3_UPDATE_1_2_3);
        _exec(SQLITE3_UPDATE_1_2_4);
        log_info("database upgrade successful.\n");
        dbVersion = _("2");
    }
    
    if (dbVersion == "2")
    {
        log_info("Doing an automatic database upgrade from database version 2 to version 3...\n");
        _exec(SQLITE3_UPDATE_2_3_1);
        _exec(SQLITE3_UPDATE_2_3_2);
        _exec(SQLITE3_UPDATE_2_3_3);
        log_info("database upgrade successful.\n");
        dbVersion = _("3");
    }
    
    /* --- --- ---*/
    
    if (! string_ok(dbVersion) || dbVersion != "3")
        throw _Exception(_("The database seems to be from a newer version!"));
    
    
    // add timer for backups
    if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED))
    {
        int backupInterval = ConfigManager::getInstance()->getIntOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
        Ref<TimerSubscriberObject> backupTimerSubscriber(new Sqlite3BackupTimerSubscriber());
        Timer::getInstance()->addTimerSubscriber(backupTimerSubscriber, backupInterval, Ref<Object>(this));
        
        // do a backup now
        Ref<SLBackupTask> btask (new SLBackupTask(false));
        this->addTask(RefCast(btask, SLTask));
        btask->waitForTask();
    }
    
    dbReady();
}

void Sqlite3Storage::_exec(const char *query)
{
    exec(query, strlen(query), false);
}

String Sqlite3Storage::quote(String value)
{
    char *q = sqlite3_mprintf("'%q'",
        (value == nil ? "" : value.c_str()));
    String ret = q;
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
    //fprintf(stdout, "%s\n",query);
    //fflush(stdout);
    Ref<SLSelectTask> ptask (new SLSelectTask(query));
    addTask(RefCast(ptask, SLTask));
    ptask->waitForTask();
    return ptask->getResult();
}

int Sqlite3Storage::exec(const char *query, int length, bool getLastInsertId)
{
    //fprintf(stdout, "%s\n",query);
    //fflush(stdout);
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
    
    String dbFilePath = ConfigManager::getInstance()->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
    
    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if(res != SQLITE_OK)
    {
        startupError = _("Sqlite3Storage.init: could not open ") +
            dbFilePath;
        return;
    }
    AUTOLOCK(sqliteMutex);
    // tell init() that we are ready
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
            task->run(&db, this);
            if (task->didContamination())
                dirty = true;
            else if (task->didDecontamination())
                dirty = false;
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

void Sqlite3Storage::addTask(zmm::Ref<SLTask> task, bool onlyIfDirty)
{
    if (! taskQueueOpen)
        throw _Exception(_("sqlite3 task queue is already closed"));
    AUTOLOCK(sqliteMutex);
    if (! taskQueueOpen)
    {
        throw _Exception(_("sqlite3 task queue is already closed"));
    }
    if (! onlyIfDirty || dirty)
    {
        taskQueue->enqueue(task);
        signal();
    }
}

void Sqlite3Storage::shutdownDriver()
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
    SQLStorage::exec(q);
}

void Sqlite3Storage::_addToInsertBuffer(Ref<StringBuffer> query)
{
    if (insertBuffer == nil)
    {
        insertBuffer = Ref<StringBuffer>(new StringBuffer());
        *insertBuffer << "BEGIN TRANSACTION;";
    }
    
    *insertBuffer << query << ';';
}

void Sqlite3Storage::_flushInsertBuffer()
{
    if (insertBuffer == nil)
        return;
    *insertBuffer << "COMMIT;";
    SQLStorage::exec(insertBuffer);
    insertBuffer->clear();
    *insertBuffer << "BEGIN TRANSACTION;";
}

/* SLTask */

SLTask::SLTask() : Object()
{
    running = true;
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    error = nil;
    contamination = false;
    decontamination = false;
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
        log_debug("%s\n", getError().c_str());
        throw _Exception(getError());
    }
}

#ifdef AUTO_CREATE_DATABASE
/* SLInitTask */

void SLInitTask::run(sqlite3 **db, Sqlite3Storage *sl)
{
    String dbFilePath = ConfigManager::getInstance()->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
    
    sqlite3_close(*db);
    
    if (unlink(dbFilePath.c_str()) != 0)
        throw _StorageException(nil, _("error while autocreating sqlite3 database: could not unlink old database file: ") + mt_strerror(errno));
    
    int res = sqlite3_open(dbFilePath.c_str(), db);
    if (res != SQLITE_OK)
        throw _StorageException(nil, _("error while autocreating sqlite3 database: could not create new database"));
    
    unsigned char buf[SL3_CREATE_SQL_INFLATED_SIZE + 1]; // +1 for '\0' at the end of the string
    unsigned long uncompressed_size = SL3_CREATE_SQL_INFLATED_SIZE;
    int ret = uncompress(buf, &uncompressed_size, sqlite3_create_sql, SL3_CREATE_SQL_DEFLATED_SIZE);
    if (ret != Z_OK || uncompressed_size != SL3_CREATE_SQL_INFLATED_SIZE)
        throw _StorageException(nil, _("Error while uncompressing sqlite3 create sql. returned: ") + ret);
    buf[SL3_CREATE_SQL_INFLATED_SIZE] = '\0';
    
    char *err = NULL;
    ret = sqlite3_exec(
        *db,
        (const char *)buf,
        NULL,
        NULL,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = err;
        sqlite3_free(err);
    }
    if(ret != SQLITE_OK)
    {
        throw _StorageException(nil, sl->getError((const char*)buf, error, *db));
    }
    contamination = true;
}

#endif

/* SLSelectTask */

SLSelectTask::SLSelectTask(const char *query) : SLTask()
{
    this->query = query;
}

void SLSelectTask::run(sqlite3 **db, Sqlite3Storage *sl)
{
    
    pres = Ref<Sqlite3Result>(new Sqlite3Result());
    
    char *err;
    int ret = sqlite3_get_table(
        *db,
        query,
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = err;
        sqlite3_free(err);
    }
    if(ret != SQLITE_OK)
    {
        throw _StorageException(nil, sl->getError(query, error, *db));
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

void SLExecTask::run(sqlite3 **db, Sqlite3Storage *sl)
{
    //log_debug("%s\n", query);
    char *err;
    int res = sqlite3_exec(
        *db,
        query,
        NULL,
        NULL,
        &err
    );
    String error = nil;
    if (err != NULL)
    {
        error = err;
        sqlite3_free(err);
    }
    if(res != SQLITE_OK)
    {
        throw _StorageException(nil, sl->getError(query, error, *db));
    }
    if (getLastInsertIdFlag)
        lastInsertId = sqlite3_last_insert_rowid(*db);
    contamination = true;
}

/* SLBackupTask */

void SLBackupTask::run(sqlite3 **db, Sqlite3Storage *sl)
{
    
    String dbFilePath = ConfigManager::getInstance()->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
    
    if (! restore)
    {
        try
        {
            copy_file(
                dbFilePath,
                dbFilePath + ".backup"
            );
            log_debug("sqlite3 backup successful\n");
            decontamination = true;
        }
        catch (Exception e)
        {
            log_error("error while making sqlite3 backup: %s\n", e.getMessage().c_str());
        }
    }
    else
    {
        log_info("trying to restore sqlite3 database from backup...\n");
        sqlite3_close(*db);
        try
        {
            copy_file(
                dbFilePath + ".backup",
                dbFilePath
            );
            
        }
        catch (Exception e)
        {
            throw _StorageException(nil, _("error while restoring sqlite3 backup: ") + e.getMessage());
        }
        int res = sqlite3_open(dbFilePath.c_str(), db);
        if (res != SQLITE_OK)
        {
            throw _StorageException(nil, _("error while restoring sqlite3 backup: could not reopen sqlite3 database after restore"));
        }
        log_info("sqlite3 database successfully restored from backup.\n");   
    }
    
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

/* Sqlite3BackupTimerSubscriber */

void Sqlite3BackupTimerSubscriber::timerNotify(Ref<Object> sqlite3storage)
{
    Sqlite3Storage *storage = (Sqlite3Storage*)(sqlite3storage.getPtr());
    
    Ref<SLBackupTask> btask (new SLBackupTask(false));
    storage->addTask(RefCast(btask, SLTask), true);
}

#endif // HAVE_SQLITE3
