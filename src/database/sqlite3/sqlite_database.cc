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

#include <array>

#include "config/config_manager.h"

#define DB_BACKUP_FORMAT "{}.backup"

// updates 1->2
#define SQLITE3_UPDATE_1_2_1 "DROP INDEX mt_autoscan_obj_id"
#define SQLITE3_UPDATE_1_2_2 "CREATE UNIQUE INDEX mt_autoscan_obj_id ON mt_autoscan(obj_id)"
#define SQLITE3_UPDATE_1_2_3 "ALTER TABLE \"mt_autoscan\" ADD \"path_ids\" text"

// updates 2->3
#define SQLITE3_UPDATE_2_3_1 "ALTER TABLE \"mt_cds_object\" ADD \"service_id\" varchar(255) default NULL"
#define SQLITE3_UPDATE_2_3_2 "CREATE INDEX mt_cds_object_service_id ON mt_cds_object(service_id)"

// updates 3->4: Move to Metadata table
#define SQLITE3_UPDATE_3_4_1 "CREATE TABLE \"mt_metadata\" ( \
  \"id\" integer primary key, \
  \"item_id\" integer NOT NULL, \
  \"property_name\" varchar(255) NOT NULL, \
  \"property_value\" text NOT NULL, \
  CONSTRAINT \"mt_metadata_idfk1\" FOREIGN KEY (\"item_id\") REFERENCES \"mt_cds_object\" (\"id\") \
  ON DELETE CASCADE ON UPDATE CASCADE )"
#define SQLITE3_UPDATE_3_4_2 "CREATE INDEX mt_metadata_item_id ON mt_metadata(item_id)"

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

// updates 5->6: add config value table
#define SQLITE3_UPDATE_5_6_1 "CREATE TABLE \"grb_config_value\" ( \
  \"item\" varchar(255) primary key, \
  \"key\" varchar(255) NOT NULL, \
  \"item_value\" varchar(255) NOT NULL, \
  \"status\" varchar(20) NOT NULL)"
#define SQLITE3_UPDATE_5_6_2 "CREATE INDEX grb_config_value_item ON grb_config_value(item)"

// updates 6->7
#define SQLITE3_UPDATE_6_7_1 "DROP TABLE mt_cds_active_item;"

// updates 7->8: part_number
#define SQLITE3_UPDATE_7_8_1 "ALTER TABLE \"mt_cds_object\" ADD \"part_number\" integer default NULL"
#define SQLITE3_UPDATE_7_8_2 "DROP INDEX mt_track_number"
#define SQLITE3_UPDATE_7_8_3 "CREATE INDEX \"grb_track_number\" ON mt_cds_object (part_number,track_number)"

// updates 8->9: bookmark_pos
#define SQLITE3_UPDATE_8_9_1 "ALTER TABLE \"mt_cds_object\" ADD \"bookmark_pos\" integer unsigned NOT NULL default 0"

// updates 9->10: last_modified
#define SQLITE3_UPDATE_9_10_1 "ALTER TABLE \"mt_cds_object\" ADD \"last_modified\" integer unsigned default NULL"

#define SQLITE3_UPDATE_VERSION "UPDATE \"mt_internal_setting\" SET \"value\"='{}' WHERE \"key\"='db_version' AND \"value\"='{}'"

static const auto dbUpdates = std::array<std::vector<const char*>, 9> { {
    { SQLITE3_UPDATE_1_2_1, SQLITE3_UPDATE_1_2_2, SQLITE3_UPDATE_1_2_3 },
    { SQLITE3_UPDATE_2_3_1, SQLITE3_UPDATE_2_3_2 },
    { SQLITE3_UPDATE_3_4_1, SQLITE3_UPDATE_3_4_2 },
    { SQLITE3_UPDATE_4_5_1 },
    { SQLITE3_UPDATE_5_6_1, SQLITE3_UPDATE_5_6_2 },
    { SQLITE3_UPDATE_6_7_1 },
    { SQLITE3_UPDATE_7_8_1, SQLITE3_UPDATE_7_8_2, SQLITE3_UPDATE_7_8_3 },
    { SQLITE3_UPDATE_8_9_1 },
    { SQLITE3_UPDATE_9_10_1 },
} };

Sqlite3Database::Sqlite3Database(std::shared_ptr<Config> config, std::shared_ptr<Timer> timer)
    : SQLDatabase(std::move(config))
    , timer(std::move(timer))
    , shutdownFlag(false)
    , dirty(false)
    , dbInitDone(false)
    , hasBackupTimer(false)
{
    table_quote_begin = '"';
    table_quote_end = '"';
}

void Sqlite3Database::prepare()
{
    _exec("PRAGMA locking_mode = EXCLUSIVE");
    _exec("PRAGMA foreign_keys = ON");
    _exec("PRAGMA journal_mode = WAL;");
    SQLDatabase::exec(fmt::format("PRAGMA synchronous = {}", config->getIntOption(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS)));
}

void Sqlite3Database::init()
{
    dbInitDone = false;
    SQLDatabase::init();

    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
    log_debug("SQLite path: {}", dbFilePath);

    // check for db-file
    if (access(dbFilePath.c_str(), R_OK | W_OK) != 0 && errno != ENOENT)
        throw DatabaseException("", fmt::format("Error while accessing sqlite database file ({}): {}", dbFilePath.c_str(), std::strerror(errno)));

    taskQueueOpen = true;
    threadRunner = std::make_unique<StdThreadRunner>("SQLiteThread", Sqlite3Database::staticThreadProc, this, config);

    if (!threadRunner->isAlive()) {
        throw DatabaseException("", fmt::format("Could not start sqlite thread: {}", std::strerror(errno)));
    }

    // wait for sqlite3 thread to become ready
    threadRunner->waitForReady();
    if (!startupError.empty())
        throw DatabaseException("", startupError);

    // try to detect already active database client and terminate before doing any harm
    try {
        prepare();
    } catch (const std::runtime_error& e) {
        shutdown();
        throw_std_runtime_error("Sqlite3Database.init: could not open '{}' exclusively", dbFilePath);
    }

    std::string dbVersion;
    std::string dbFilePathbackup = fmt::format(DB_BACKUP_FORMAT, dbFilePath);
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_warning("Sqlite3 database seems to be corrupt or doesn't exist yet.");
        // database seems to be corrupt or nonexistent
        if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_RESTORE) && sqliteStatus == SQLITE_OK) {
            // try to restore database

            // checking for backup file
            if (access(dbFilePathbackup.c_str(), R_OK) == 0) {
                try {
                    // trying to copy backup file
                    auto btask = std::make_shared<SLBackupTask>(config, true);
                    this->addTask(btask);
                    btask->waitForTask();
                    prepare();
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
        log_info("No sqlite3 backup is available or backup is corrupt. Automatically creating new database file...");
        auto itask = std::make_shared<SLInitTask>(config);
        addTask(itask);
        try {
            itask->waitForTask();
            prepare();
            dbVersion = getInternalSetting("db_version");
        } catch (const std::runtime_error& e) {
            shutdown();
            throw_std_runtime_error("Error while creating database: {}", e.what());
        }
        log_info("database created successfully.");
    }

    if (dbVersion.empty()) {
        shutdown();
        throw_std_runtime_error("sqlite3 database seems to be corrupt and restoring from backup failed");
    }

    try {
        log_debug("db_version: {}", dbVersion.c_str());

        /* --- database upgrades --- */
        int version = 1;
        for (auto&& upgrade : dbUpdates) {
            if (dbVersion == fmt::to_string(version)) {
                log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
                for (auto&& upgradeCmd : upgrade) {
                    _exec(upgradeCmd);
                }
                _exec(fmt::format(SQLITE3_UPDATE_VERSION, version + 1, version).c_str());
                dbVersion = fmt::to_string(version + 1);
                log_info("Database upgrade to version {} successful.", dbVersion.c_str());
            }
            version++;
        }

        if (dbVersion != fmt::to_string(version))
            throw_std_runtime_error("The database seems to be from a newer version");

        if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED)) {
            // do a backup now
            auto btask = std::make_shared<SLBackupTask>(config, false);
            this->addTask(btask);
            btask->waitForTask();

            // add timer for backups
            auto backupInterval = std::chrono::seconds(config->getIntOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL));
            timer->addTimerSubscriber(this, backupInterval, nullptr);
            hasBackupTimer = true;
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

std::string Sqlite3Database::getError(const std::string& query, const std::string& error, sqlite3* db, int errorCode)
{
    sqliteStatus = (errorCode == SQLITE_BUSY || errorCode == SQLITE_LOCKED) ? SQLITE_LOCKED : SQLITE_OK;
    return fmt::format("SQLITE3: ({}, : {}) {}\nQuery: {}\nerror: {}", sqlite3_errcode(db), sqlite3_extended_errcode(db), sqlite3_errmsg(db), query.empty() ? "unknown" : query, error.empty() ? "unknown" : error);
}

void Sqlite3Database::beginTransaction(const std::string_view& tName)
{
    if (use_transaction) {
        log_debug("BEGIN TRANSACTION {} {}", tName, inTransaction);
        SqlAutoLock lock(sqlMutex);
        StdThreadRunner::waitFor(
            fmt::format("SqliteDatabase.begin {}", tName), [this] { return !inTransaction; }, 100);
        inTransaction = true;
        _exec("BEGIN TRANSACTION");
    }
}

void Sqlite3Database::rollback(const std::string_view& tName)
{
    if (use_transaction && inTransaction) {
        log_debug("ROLLBACK {} {}", tName, inTransaction);
        _exec("ROLLBACK");
        inTransaction = false;
    }
}

void Sqlite3Database::commit(const std::string_view& tName)
{
    if (use_transaction && inTransaction) {
        log_debug("COMMIT {} {}", tName, inTransaction);
        _exec("COMMIT");
        inTransaction = false;
    }
}

std::shared_ptr<SQLResult> Sqlite3Database::select(const char* query, int length)
{
    try {
        log_debug("Adding select to Queue: {}", query);
        auto stask = std::make_shared<SLSelectTask>(query);
        addTask(stask);
        stask->waitForTask();
        return stask->getResult();
    } catch (const std::runtime_error& e) {
        if (dbInitDone) {
            log_error("prematurely shutting down.");
            rollback("");
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
            rollback("");
            shutdown();
        }
        throw_std_runtime_error(e.what());
    }
}

void* Sqlite3Database::staticThreadProc(void* arg)
{
    log_debug("Sqlite3Database::staticThreadProc - running thread");
    auto inst = static_cast<Sqlite3Database*>(arg);
    try {
        inst->threadProc();
        log_debug("Sqlite3Database::staticThreadProc - exiting thread");
    } catch (const std::runtime_error& e) {
        log_error("Sqlite3Database::staticThreadProc - aborting thread");
    }
    return nullptr;
}

void Sqlite3Database::threadProc()
{
    sqlite3* db;

    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if (res != SQLITE_OK) {
        startupError = fmt::format("Sqlite3Database.init: could not open '{}'", dbFilePath);
        return;
    }

    StdThreadRunner::waitFor("Sqlite3Database", [this] { return threadRunner != nullptr; });
    auto lock = threadRunner->uniqueLockS("threadProc");
    // tell init() that we are ready
    threadRunner->setReady();

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
        threadRunner->wait(lock);
    }
    log_debug("Sqlite3Database::threadProc - exiting");

    taskQueueOpen = false;
    while (!taskQueue.empty()) {
        auto task = taskQueue.front();
        taskQueue.pop();
        task->sendSignal("Sorry, sqlite3 thread is shutting down");
    }

    if (db) {
        log_debug("Sqlite3Database::staticThreadProc - closing database");
        if (sqlite3_close(db) == SQLITE_OK) {
            log_debug("Sqlite3Database::staticThreadProc - closed database");
        } else {
            log_error("Sqlite3Database::staticThreadProc - closing database failed");
        }
        db = nullptr;
    }
}

void Sqlite3Database::addTask(const std::shared_ptr<SLTask>& task, bool onlyIfDirty)
{
    if (!taskQueueOpen) {
        throw_std_runtime_error("sqlite3 task queue is already closed");
    }

    auto lock = threadRunner->lockGuard(fmt::format("addTask {}", task->taskType()));

    if (!taskQueueOpen) {
        throw_std_runtime_error("sqlite3 task queue is already closed");
    }
    if (!onlyIfDirty || dirty) {
        taskQueue.push(task);
        threadRunner->notify();
    }
}

void Sqlite3Database::shutdownDriver()
{
    log_debug("start");
    auto lock = threadRunner->uniqueLockS("shutdown");
    if (!shutdownFlag) {
        shutdownFlag = true;
        if (hasBackupTimer) {
            timer->removeTimerSubscriber(this, nullptr);
        }
        log_debug("signalling...");
        threadRunner->notify();
        lock.unlock();
        log_debug("waiting for thread");
        threadRunner->join();
    }
    log_debug("end");
}

void Sqlite3Database::storeInternalSetting(const std::string& key, const std::string& value)
{
    std::ostringstream q;
    q << "INSERT OR REPLACE INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (" << QTB << "key" << QTE << ", " << QTB << "value" << QTE << ") "
                                                                                                                                               "VALUES ("
      << quote(key) << ", " << quote(value) << ") ";
    SQLDatabase::exec(q.str());
}

bool SLTask::is_running() const
{
    return running;
}

void SLTask::sendSignal()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        std::lock_guard<decltype(mutex)> lock(mutex);
        running = false;
        cond.notify_one();
    }
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
    : config(std::move(config))
{
}

void SLInitTask::run(sqlite3** db, Sqlite3Database* sl)
{
    log_debug("Running: init");
    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    sqlite3_close(*db);

    int res = sqlite3_open(dbFilePath.c_str(), db);
    if (res != SQLITE_OK)
        throw DatabaseException("", "SQLite: Failed to create new database");

    auto sqlFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
    log_debug("Loading initialisation SQL from: {}", sqlFilePath.c_str());
    auto sql = readTextFile(sqlFilePath);

    char* err = nullptr;
    int ret = sqlite3_exec(
        *db,
        sql.c_str(),
        nullptr,
        nullptr,
        &err);
    std::string error;
    if (err != nullptr) {
        error = err;
        sqlite3_free(err);
    }
    if (ret != SQLITE_OK) {
        throw DatabaseException("", sl->getError(sql, error, *db, ret));
    }
    contamination = true;
}

/* SLSelectTask */

SLSelectTask::SLSelectTask(const char* query)
    : query(query)
{
}

void SLSelectTask::run(sqlite3** db, Sqlite3Database* sl)
{
    log_debug("Running: {}", query);
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
        throw DatabaseException("", sl->getError(query, error, *db, ret));
    }

    pres->row = pres->table;
    pres->cur_row = 0;
}

/* SLExecTask */

SLExecTask::SLExecTask(const char* query, bool getLastInsertId)
    : query(query)
    , getLastInsertIdFlag(getLastInsertId)
{
}

void SLExecTask::run(sqlite3** db, Sqlite3Database* sl)
{
    log_debug("Running: {}", query);
    char* err;
    int ret = sqlite3_exec(
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
    if (ret != SQLITE_OK) {
        throw DatabaseException("", sl->getError(query, error, *db, ret));
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
    log_debug("Running: backup");
    std::string dbFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

    if (!restore) {
        try {
            fs::copy(
                dbFilePath,
                fmt::format(DB_BACKUP_FORMAT, dbFilePath),
                fs::copy_options::overwrite_existing);
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
                fmt::format(DB_BACKUP_FORMAT, dbFilePath),
                dbFilePath,
                fs::copy_options::overwrite_existing);
        } catch (const std::runtime_error& e) {
            throw DatabaseException(fmt::format("Error while restoring sqlite3 backup: {}", e.what()), fmt::format("Error while restoring sqlite3 backup: {}", e.what()));
        }
        int res = sqlite3_open(dbFilePath.c_str(), db);
        if (res != SQLITE_OK) {
            throw DatabaseException("", "error while restoring sqlite3 backup: could not reopen sqlite3 database after restore");
        }
        log_info("sqlite3 database successfully restored from backup.");
    }
}

/* Sqlite3Result */

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
            return std::make_unique<Sqlite3Row>(row);
        }
        return nullptr;
    }
    return nullptr;
}

/* Sqlite3Row */

Sqlite3Row::Sqlite3Row(char** row)
    : row(row)
{
}

/* Sqlite3BackupTimerSubscriber */

void Sqlite3Database::timerNotify(std::shared_ptr<Timer::Parameter> param)
{
    auto btask = std::make_shared<SLBackupTask>(config, false);
    this->addTask(btask, true);
}
