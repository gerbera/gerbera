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

#define SQLITE3_SET_VERSION "INSERT INTO \"mt_internal_setting\" VALUES('db_version', '{}')"
#define SQLITE3_UPDATE_VERSION "UPDATE \"mt_internal_setting\" SET \"value\"='{}' WHERE \"key\"='db_version' AND \"value\"='{}'"
#define SQLITE3_ADD_RESOURCE_ATTR "ALTER TABLE \"grb_cds_resource\" ADD COLUMN \"{}\" varchar(255) default NULL"

Sqlite3Database::Sqlite3Database(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime, std::shared_ptr<Timer> timer)
    : SQLDatabase(std::move(config), std::move(mime))
    , timer(std::move(timer))
{
    table_quote_begin = '"';
    table_quote_end = '"';

    // if sqlite3.sql or sqlite3-upgrade.xml is changed hashies have to be updated, index 0 is used for create script
    hashies = { 3147846384, 778996897, 3362507034, 853149842, 4035419264, 3497064885, 974692115, 119767663, 3167732653, 2427825904, 3305506356, 43189396, 2767540493 };
}

void Sqlite3Database::prepare()
{
    _exec("PRAGMA locking_mode = EXCLUSIVE");
    _exec("PRAGMA foreign_keys = ON");
    _exec("PRAGMA journal_mode = WAL");
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
        auto itask = std::make_shared<SLInitTask>(config, hashies[0]);
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
        upgradeDatabase(dbVersion, hashies, CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE, SQLITE3_UPDATE_VERSION, SQLITE3_ADD_RESOURCE_ATTR);
        if (config->getBoolOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED) && timer) {
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

void Sqlite3Database::_exec(const char* query, int length)
{
    if (length == -1) {
        length = strlen(query);
    }
    exec(query, length, false);
}

std::string Sqlite3Database::quote(std::string value) const
{
    // https://www.sqlite.org/printf.html#percentq:
    // The string is printed with all single quote (') characters doubled so that the string can safely appear inside an SQL string literal.
    // The %Q substitution type also puts single-quotes on both ends of the substituted string.
    char* q = sqlite3_mprintf("%Q", value.c_str());
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

std::shared_ptr<SQLResult> Sqlite3Database::select(const char* query, size_t length)
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

int Sqlite3Database::exec(const char* query, size_t length, bool getLastInsertId)
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
        if (hasBackupTimer && timer) {
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
    auto command = fmt::format("INSERT OR REPLACE INTO {0}{2}{1} ({0}key{1}, {0}value{1}) VALUES ({3}, {4})", table_quote_begin, table_quote_begin, INTERNAL_SETTINGS_TABLE, quote(key), quote(value));
    SQLDatabase::exec(command.c_str());
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
    auto&& myHash = stringHash(sql);

    if (myHash == hashie) {
        sql += fmt::format("\n" SQLITE3_SET_VERSION ";", DBVERSION);

        char* err = nullptr;
        int ret = sqlite3_exec(
            *db,
            sql.c_str(),
            nullptr,
            nullptr,
            &err);
        std::string error;
        if (err) {
            error = err;
            sqlite3_free(err);
        }
        if (ret != SQLITE_OK) {
            throw DatabaseException("", sl->getError(sql, error, *db, ret));
        }
        contamination = true;
    } else {
        log_warning("Wrong hash for create script {}: {} != {}", DBVERSION, myHash, hashie);
    }
}

/* SLSelectTask */
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
    if (err) {
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
    if (err) {
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

/* Sqlite3BackupTimerSubscriber */

void Sqlite3Database::timerNotify(std::shared_ptr<Timer::Parameter> param)
{
    auto btask = std::make_shared<SLBackupTask>(config, false);
    this->addTask(btask, true);
}
