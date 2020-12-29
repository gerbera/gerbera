/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sqlite3_database.cc - this file is part of MediaTomb.

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

/// \file sqlite3_database.cc

#include "sqlite_database.h" // API

#include <utility>
#include <zlib.h>

#include "config/config_manager.h"
#include "sqlite3_create_sql.h"

// updates 1->2
#define SQLITE3_UPDATE_1_2_1 "DROP INDEX mt_autoscan_obj_id"
#define SQLITE3_UPDATE_1_2_2 "CREATE UNIQUE INDEX mt_autoscan_obj_id ON mt_autoscan(obj_id)"
#define SQLITE3_UPDATE_1_2_3 "ALTER TABLE \"mt_autoscan\" ADD \"path_ids\" text"
#define SQLITE3_UPDATE_1_2_4 "UPDATE \"mt_internal_setting\" SET \"value\"='2' WHERE \"key\"='db_version' AND \"value\"='1'"

// updates 2->3
#define SQLITE3_UPDATE_2_3_1 "ALTER TABLE \"mt_cds_object\" ADD \"service_id\" varchar(255) default NULL"
#define SQLITE3_UPDATE_2_3_2 "CREATE INDEX mt_cds_object_service_id ON mt_cds_object(service_id)"
#define SQLITE3_UPDATE_2_3_3 "UPDATE \"mt_internal_setting\" SET \"value\"='3' WHERE \"key\"='db_version' AND \"value\"='2'"

// updates 3->4: Move to Metadata table
#define SQLITE3_UPDATE_3_4_1 "CREATE TABLE \"mt_metadata\" ( \
  \"id\" integer primary key, \
  \"item_id\" integer NOT NULL, \
  \"property_name\" varchar(255) NOT NULL, \
  \"property_value\" text NOT NULL, \
  CONSTRAINT \"mt_metadata_idfk1\" FOREIGN KEY (\"item_id\") REFERENCES \"mt_cds_object\" (\"id\") \
  ON DELETE CASCADE ON UPDATE CASCADE )"
#define SQLITE3_UPDATE_3_4_2 "CREATE INDEX mt_metadata_item_id ON mt_metadata(item_id)"
#define SQLITE3_UPDATE_3_4_3 "UPDATE \"mt_internal_setting\" SET \"value\"='4' WHERE \"key\"='db_version' AND \"value\"='3'"

// updates 4->5: Fix incorrect SQLite foreign key
#define SQLITE3_UPDATE_4_5_1 "PRAGMA foreign_keys = OFF;  \
CREATE TABLE mt_cds_object_new \
( \
    id integer primary key, \
    ref_id integer default NULL \
    constraint cds_object_ibfk_1 \
        references mt_cds_object (id) \
            ON update cascade on delete cascade, \
            parent_id integer default '0' not null \
    constraint cds_object_ibfk_2 \
        references mt_cds_object (id) \
            ON update cascade on delete cascade, \
            object_type tinyint unsigned not null, \
    upnp_class varchar(80) default NULL, \
    dc_title varchar(255) default NULL, \
    location text default NULL, \
    location_hash integer unsigned default NULL, \
    metadata text default NULL, \
    auxdata text default NULL, \
    resources text default NULL, \
    update_id integer default '0' not null, \
    mime_type varchar(40) default NULL, \
    flags integer unsigned default '1' not null, \
    track_number integer default NULL, \
    service_id varchar(255) default NULL \
); \
INSERT INTO mt_cds_object_new(id, ref_id, parent_id, object_type, upnp_class, dc_title, location, location_hash, metadata, auxdata, resources, update_id, mime_type, flags, track_number, service_id) SELECT id, ref_id, parent_id, object_type, upnp_class, dc_title, location, location_hash, metadata, auxdata, resources, update_id, mime_type, flags, track_number, service_id FROM mt_cds_object; \
DROP TABLE mt_cds_object; \
ALTER TABLE mt_cds_object_new RENAME TO mt_cds_object; \
CREATE INDEX mt_cds_object_parent_id ON mt_cds_object (parent_id, object_type, dc_title); \
CREATE INDEX mt_cds_object_ref_id ON mt_cds_object (ref_id); \
CREATE INDEX mt_cds_object_service_id ON mt_cds_object (service_id); \
CREATE INDEX mt_location_parent ON mt_cds_object (location_hash, parent_id); \
CREATE INDEX mt_object_type ON mt_cds_object (object_type); \
CREATE INDEX mt_track_number on mt_cds_object (track_number); \
PRAGMA foreign_keys = ON;"
#define SQLITE3_UPDATE_4_5_2 "UPDATE mt_internal_setting SET value='5' WHERE key='db_version' AND value='4'"

// updates 5->6: add config value table
#define SQLITE3_UPDATE_5_6_1 "CREATE TABLE \"grb_config_value\" ( \
  \"item\" varchar(255) primary key, \
  \"key\" varchar(255) NOT NULL, \
  \"item_value\" varchar(255) NOT NULL, \
  \"status\" varchar(20) NOT NULL)"
#define SQLITE3_UPDATE_5_6_2 "CREATE INDEX grb_config_value_item ON grb_config_value(item)"
#define SQLITE3_UPDATE_5_6_3 "UPDATE \"mt_internal_setting\" SET \"value\"='6' WHERE \"key\"='db_version' AND \"value\"='5'"

#define SQLITE3_UPDATE_6_7_1 "DROP TABLE mt_cds_active_item;"
#define SQLITE3_UPDATE_6_7_2 "UPDATE \"mt_internal_setting\" SET \"value\"='7' WHERE \"key\"='db_version' AND \"value\"='6'"

Sqlite3Database::Sqlite3Database(std::shared_ptr<Config> config, std::shared_ptr<Timer> timer)
    : SQLDatabase(std::move(config))
    , timer(std::move(timer))
{
    shutdownFlag = false;
    table_quote_begin = '"';
    table_quote_end = '"';
    dirty = false;
    dbInitDone = false;
}

void Sqlite3Database::init()
{
    dbInitDone = false;
    SQLDatabase::init();

    AutoLockU lock(sqliteMutex);
    /*
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    */

    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    // check for db-file
    if (access(dbFilePath.c_str(), R_OK | W_OK) != 0 && errno != ENOENT)
        throw DatabaseException("", fmt::format("Error while accessing sqlite database file ({}): {}", dbFilePath.c_str(), strerror(errno)));

    taskQueueOpen = true;

    int ret = pthread_create(
        &sqliteThread,
        nullptr, //&attr,
        Sqlite3Database::staticThreadProc,
        this);

    if (ret != 0) {
        throw DatabaseException("", fmt::format("Could not start sqlite thread: {}", strerror(errno)));
    }

    // wait for sqlite3 thread to become ready
    cond.wait(lock);
    lock.unlock();
    if (!startupError.empty())
        throw DatabaseException("", startupError);

    std::string dbVersion;
    std::string dbFilePathbackup = dbFilePath + ".backup";
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_warning("Sqlite3 database seems to be corrupt or doesn't exist yet.");
        // database seems to be corrupt or nonexistent
        if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_RESTORE)) {
            // try to restore database

            // checking for backup file
            if (access(dbFilePathbackup.c_str(), R_OK) == 0) {
                try {
                    // trying to copy backup file
                    auto btask = std::make_shared<SLBackupTask>(config, true);
                    this->addTask(btask);
                    btask->waitForTask();
                    dbVersion = getInternalSetting("db_version");
                } catch (const std::runtime_error& e) {
                }
            }
        } else {
            // fail because restore option is false
            shutdown();
            throw_std_runtime_error("sqlite3 database seems to be corrupt and the 'on-error' option is set to 'fail'");
        }
    }

    if (dbVersion.empty() && access(dbFilePathbackup.c_str(), R_OK) != 0) {
        log_info("no sqlite3 backup is available or backup is corrupt. automatically creating database...");
        auto itask = std::make_shared<SLInitTask>(config);
        addTask(itask);
        try {
            itask->waitForTask();
            dbVersion = getInternalSetting("db_version");
        } catch (const std::runtime_error& e) {
            shutdown();
            throw_std_runtime_error(std::string { "error while creating database: " } + e.what());
        }
        log_info("database created successfully.");
    }

    if (dbVersion.empty()) {
        shutdown();
        throw_std_runtime_error("sqlite3 database seems to be corrupt and restoring from backup failed");
    }

    try {
        _exec("PRAGMA locking_mode = EXCLUSIVE");
        _exec("PRAGMA foreign_keys = ON");
        int synchronousOption = config->getIntOption(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);
        std::ostringstream buf;
        buf << "PRAGMA synchronous = " << synchronousOption;
        SQLDatabase::exec(buf);

        log_debug("db_version: {}", dbVersion.c_str());

        /* --- database upgrades --- */

        if (dbVersion == "1") {
            log_info("Running an automatic database upgrade from database version 1 to version 2...");
            _exec(SQLITE3_UPDATE_1_2_1);
            _exec(SQLITE3_UPDATE_1_2_2);
            _exec(SQLITE3_UPDATE_1_2_3);
            _exec(SQLITE3_UPDATE_1_2_4);
            log_info("Database upgrade successful.");
            dbVersion = "2";
        }

        if (dbVersion == "2") {
            log_info("Running an automatic database upgrade from database version 2 to version 3...");
            _exec(SQLITE3_UPDATE_2_3_1);
            _exec(SQLITE3_UPDATE_2_3_2);
            _exec(SQLITE3_UPDATE_2_3_3);
            log_info("Database upgrade successful.");
            dbVersion = "3";
        }

        if (dbVersion == "3") {
            log_info("Running an automatic database upgrade from database version 3 to version 4...");
            _exec(SQLITE3_UPDATE_3_4_1);
            _exec(SQLITE3_UPDATE_3_4_2);
            _exec(SQLITE3_UPDATE_3_4_3);
            log_info("Database upgrade successful.");
            dbVersion = "4";
        }

        if (dbVersion == "4") {
            log_info("Running an automatic database upgrade from database version 4 to version 5...");
            _exec(SQLITE3_UPDATE_4_5_1);
            _exec(SQLITE3_UPDATE_4_5_2);
            log_info("Database upgrade successful.");
            dbVersion = "5";
        }

        if (dbVersion == "5") {
            log_info("Running an automatic database upgrade from database version 5 to version 6...");
            _exec(SQLITE3_UPDATE_5_6_1);
            _exec(SQLITE3_UPDATE_5_6_2);
            _exec(SQLITE3_UPDATE_5_6_3);
            log_info("Database upgrade successful.");
            dbVersion = "6";
        }

        if (dbVersion == "6") {
            log_info("Running an automatic database upgrade from database version 6 to version 7...");
            _exec(SQLITE3_UPDATE_6_7_1);
            _exec(SQLITE3_UPDATE_6_7_2);
            log_info("Database upgrade successful.");
            dbVersion = "7";
        }

        if (dbVersion != "7")
            throw_std_runtime_error("The database seems to be from a newer version");

        // add timer for backups
        if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED)) {
            int backupInterval = config->getIntOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
            timer->addTimerSubscriber(this, backupInterval, nullptr);

            // do a backup now
            auto btask = std::make_shared<SLBackupTask>(config, false);
            this->addTask(btask);
            btask->waitForTask();
        }
        dbInitDone = true;
    } catch (const std::runtime_error& e) {
        log_error("prematurely shutting down.");
        shutdown();
        throw_std_runtime_error(e.what());
    }
}

std::shared_ptr<Database> Sqlite3Database::getSelf()
{
    return shared_from_this();
}

void Sqlite3Database::_exec(const char* query)
{
    exec(query, strlen(query), false);
}

std::string Sqlite3Database::quote(std::string value) const
{
    char* q = sqlite3_mprintf("'%q'", value.c_str());
    std::string ret = q;
    sqlite3_free(q);
    return ret;
}

std::string Sqlite3Database::getError(const std::string& query, const std::string& error, sqlite3* db)
{
    return std::string("SQLITE3: (") + std::to_string(sqlite3_errcode(db)) + " : " + std::to_string(sqlite3_extended_errcode(db)) + ") "
        + sqlite3_errmsg(db) + "\nQuery:" + (query.empty() ? "unknown" : query) + "\nerror: " + (error.empty() ? "unknown" : error);
}

std::shared_ptr<SQLResult> Sqlite3Database::select(const char* query, int length)
{
    try {
        auto stask = std::make_shared<SLSelectTask>(query);
        addTask(stask);
        stask->waitForTask();
        return stask->getResult();
    } catch (const std::runtime_error& e) {
        if (dbInitDone) {
            log_error("prematurely shutting down.");
            shutdown();
        }
        throw_std_runtime_error(e.what());
    }
}

int Sqlite3Database::exec(const char* query, int length, bool getLastInsertId)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<SLExecTask>(query, getLastInsertId);
        addTask(etask);
        etask->waitForTask();
        return getLastInsertId ? etask->getLastInsertId() : -1;
    } catch (const std::runtime_error& e) {
        if (dbInitDone) {
            log_error("prematurely shutting down.");
            shutdown();
        }
        throw_std_runtime_error(e.what());
    }
}

void* Sqlite3Database::staticThreadProc(void* arg)
{
    auto inst = static_cast<Sqlite3Database*>(arg);
    inst->threadProc();
    log_debug("Sqlite3Database::staticThreadProc - exiting thread");
    pthread_exit(nullptr);
}

void Sqlite3Database::threadProc()
{
    sqlite3* db;

    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if (res != SQLITE_OK) {
        startupError = "Sqlite3Database.init: could not open " + dbFilePath;
        return;
    }

    AutoLockU lock(sqliteMutex);
    // tell init() that we are ready
    cond.notify_one();

    while (!shutdownFlag) {
        while (!taskQueue.empty()) {
            auto task = taskQueue.front();
            taskQueue.pop();

            lock.unlock();
            try {
                task->run(&db, this);
                if (task->didContamination())
                    dirty = true;
                else if (task->didDecontamination())
                    dirty = false;
                task->sendSignal();
            } catch (const std::runtime_error& e) {
                task->sendSignal(e.what());
            }
            lock.lock();
        }

        /* if nothing to do, sleep until awakened */
        cond.wait(lock);
    }

    taskQueueOpen = false;
    while (!taskQueue.empty()) {
        auto task = taskQueue.front();
        taskQueue.pop();
        task->sendSignal("Sorry, sqlite3 thread is shutting down");
    }

    if (db)
        sqlite3_close(db);
}

void Sqlite3Database::addTask(const std::shared_ptr<SLTask>& task, bool onlyIfDirty)
{
    if (!taskQueueOpen)
        throw_std_runtime_error("sqlite3 task queue is already closed");
    AutoLock lock(sqliteMutex);
    if (!taskQueueOpen) {
        throw_std_runtime_error("sqlite3 task queue is already closed");
    }
    if (!onlyIfDirty || dirty) {
        taskQueue.push(task);
        cond.notify_one();
    }
}

void Sqlite3Database::shutdownDriver()
{
    log_debug("start");
    AutoLockU lock(sqliteMutex);
    shutdownFlag = true;
    if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED)) {
        timer->removeTimerSubscriber(this, nullptr);
    }
    log_debug("signalling...");
    cond.notify_one();
    lock.unlock();
    log_debug("waiting for thread");
    if (sqliteThread)
        pthread_join(sqliteThread, nullptr);
    sqliteThread = 0;
    log_debug("end");
}

void Sqlite3Database::storeInternalSetting(const std::string& key, const std::string& value)
{
    std::ostringstream q;
    q << "INSERT OR REPLACE INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (" << QTB << "key" << QTE << ", " << QTB << "value" << QTE << ") "
                                                                                                                                               "VALUES ("
      << quote(key) << ", " << quote(value) << ") ";
    SQLDatabase::exec(q);
}

/* SLTask */
SLTask::SLTask()
{
    running = true;
    contamination = false;
    decontamination = false;
}

bool SLTask::is_running() const
{
    return running;
}

void SLTask::sendSignal()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    running = false;
    cond.notify_one();
}

void SLTask::sendSignal(std::string error)
{
    this->error = std::move(error);
    sendSignal();
}

void SLTask::waitForTask()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        std::unique_lock<decltype(mutex)> lock(mutex);
        if (is_running()) { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal
            cond.wait(lock); // waiting for the task to complete
        }
    }

    if (!getError().empty()) {
        log_debug("{}", getError().c_str());
        throw_std_runtime_error(getError());
    }
}

/* SLInitTask */
SLInitTask::SLInitTask(std::shared_ptr<Config> config)
    : SLTask()
    , config(std::move(config))
{
}

void SLInitTask::run(sqlite3** db, Sqlite3Database* sl)
{
    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    sqlite3_close(*db);

    if (unlink(dbFilePath.c_str()) != 0)
        throw DatabaseException("", fmt::format("SQLite: Failed to unlink old database file: {}", strerror(errno)));

    int res = sqlite3_open(dbFilePath.c_str(), db);
    if (res != SQLITE_OK)
        throw DatabaseException("", "SQLite: Failed to create new database");

    unsigned char buf[SL3_CREATE_SQL_INFLATED_SIZE + 1]; // +1 for '\0' at the end of the string
    unsigned long uncompressed_size = SL3_CREATE_SQL_INFLATED_SIZE;
    int ret = uncompress(buf, &uncompressed_size, sqlite3_create_sql, SL3_CREATE_SQL_DEFLATED_SIZE);
    if (ret != Z_OK || uncompressed_size != SL3_CREATE_SQL_INFLATED_SIZE)
        throw DatabaseException("", fmt::format("SQLite: Error decompressing create SQL: {}", ret));
    buf[SL3_CREATE_SQL_INFLATED_SIZE] = '\0';

    char* err = nullptr;
    ret = sqlite3_exec(
        *db,
        reinterpret_cast<const char*>(buf),
        nullptr,
        nullptr,
        &err);
    std::string error;
    if (err != nullptr) {
        error = err;
        sqlite3_free(err);
    }
    if (ret != SQLITE_OK) {
        throw DatabaseException("", sl->getError(reinterpret_cast<const char*>(buf), error, *db));
    }
    contamination = true;
}

/* SLSelectTask */

SLSelectTask::SLSelectTask(const char* query)
    : SLTask()
{
    this->query = query;
}

void SLSelectTask::run(sqlite3** db, Sqlite3Database* sl)
{
    pres = std::make_shared<Sqlite3Result>();

    char* err = nullptr;
    int ret = sqlite3_get_table(
        *db,
        query,
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err);
    std::string error;
    if (err != nullptr) {
        log_debug(err);
        error = err;
        sqlite3_free(err);
    }
    if (ret != SQLITE_OK) {
        throw DatabaseException("", sl->getError(query, error, *db));
    }

    pres->row = pres->table;
    pres->cur_row = 0;
}

/* SLExecTask */

SLExecTask::SLExecTask(const char* query, bool getLastInsertId)
    : SLTask()
{
    this->query = query;
    this->getLastInsertIdFlag = getLastInsertId;
}

void SLExecTask::run(sqlite3** db, Sqlite3Database* sl)
{
    log_debug("{}", query);
    char* err;
    int res = sqlite3_exec(
        *db,
        query,
        nullptr,
        nullptr,
        &err);
    std::string error;
    if (err != nullptr) {
        error = err;
        sqlite3_free(err);
    }
    if (res != SQLITE_OK) {
        throw DatabaseException("", sl->getError(query, error, *db));
    }
    if (getLastInsertIdFlag)
        lastInsertId = sqlite3_last_insert_rowid(*db);
    contamination = true;
}

/* SLBackupTask */
SLBackupTask::SLBackupTask(std::shared_ptr<Config> config, bool restore)
    : config(std::move(config))
    , restore(restore)
{
}

void SLBackupTask::run(sqlite3** db, Sqlite3Database* sl)
{
    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    if (!restore) {
        try {
            fs::copy(
                dbFilePath,
                dbFilePath + ".backup");
            log_debug("sqlite3 backup successful");
            decontamination = true;
        } catch (const std::runtime_error& e) {
            log_error("error while making sqlite3 backup: {}", e.what());
        }
    } else {
        log_info("trying to restore sqlite3 database from backup...");
        sqlite3_close(*db);
        try {
            fs::copy(
                dbFilePath + ".backup",
                dbFilePath);

        } catch (const std::runtime_error& e) {
            throw DatabaseException(std::string { "Error while restoring sqlite3 backup: " } + e.what(), std::string { "Error while restoring sqlite3 backup: " } + e.what());
        }
        int res = sqlite3_open(dbFilePath.c_str(), db);
        if (res != SQLITE_OK) {
            throw DatabaseException("", "error while restoring sqlite3 backup: could not reopen sqlite3 database after restore");
        }
        log_info("sqlite3 database successfully restored from backup.");
    }
}

/* Sqlite3Result */

Sqlite3Result::Sqlite3Result()
{
    table = nullptr;
}
Sqlite3Result::~Sqlite3Result()
{
    if (table) {
        sqlite3_free_table(table);
        table = nullptr;
    }
}
std::unique_ptr<SQLRow> Sqlite3Result::nextRow()
{
    if (nrow) {
        row += ncolumn;
        cur_row++;
        if (cur_row <= nrow) {
            auto p = std::make_unique<Sqlite3Row>(row);
            return p;
        }
        return nullptr;
    }
    return nullptr;
}

/* Sqlite3Row */

Sqlite3Row::Sqlite3Row(char** row)
{
    this->row = row;
}

/* Sqlite3BackupTimerSubscriber */

void Sqlite3Database::timerNotify(std::shared_ptr<Timer::Parameter> param)
{
    auto btask = std::make_shared<SLBackupTask>(config, false);
    this->addTask(btask, true);
}
