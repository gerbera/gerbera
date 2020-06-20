/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    update_manager.cc - this file is part of MediaTomb.
    
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

/// \file update_manager.cc

#include "update_manager.h" // API

#include <chrono>
#include <csignal>
#include <sys/types.h>
#include <utility>

#include "server.h"
#include "storage/storage.h"
#include "upnp_cds.h"
#include "util/tools.h"

/* following constants in milliseconds */
#define SPEC_INTERVAL 2000
#define MIN_SLEEP 1

#define MAX_OBJECT_IDS 1000
#define MAX_OBJECT_IDS_OVERLOAD 30
#define OBJECT_ID_HASH_CAPACITY 3109

UpdateManager::UpdateManager(std::shared_ptr<Storage> storage, std::shared_ptr<Server> server)
    : storage(std::move(storage))
    , server(std::move(server))
    , objectIDHash(std::make_unique<std::unordered_set<int>>())
    , shutdownFlag(false)
    , flushPolicy(FLUSH_SPEC)
    , lastContainerChanged(INVALID_OBJECT_ID)
{
}

void UpdateManager::run()
{
    /*
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    */

    pthread_create(
        &updateThread,
        nullptr, // &attr, // attr
        UpdateManager::staticThreadProc,
        this);

    //cond->wait();
    //pthread_attr_destroy(&attr);
}

UpdateManager::~UpdateManager() { log_debug("UpdateManager destroyed"); }

void UpdateManager::shutdown()
{
    log_debug("start");
    AutoLockU lock(mutex);
    shutdownFlag = true;
    log_debug("signalling...");
    cond.notify_one();
    lock.unlock();
    log_debug("waiting for thread");
    if (updateThread)
        pthread_join(updateThread, nullptr);
    updateThread = 0;
    log_debug("end");
}

void UpdateManager::containersChanged(const std::vector<int>& objectIDs, int flushPolicy)
{
    AutoLockU lock(mutex);
    // signalling thread if it could have been idle, because
    // there were no unprocessed updates
    bool signal = (!haveUpdates());
    // signalling if the flushPolicy changes, so the thread recalculates
    // the sleep time
    if (flushPolicy > this->flushPolicy) {
        this->flushPolicy = flushPolicy;
        signal = true;
    }
    size_t size = objectIDs.size();
    int hashSize = objectIDHash->size();

    bool split = (hashSize + size >= MAX_OBJECT_IDS + MAX_OBJECT_IDS_OVERLOAD);
    for (int objectID : objectIDs) {
        if (objectID != lastContainerChanged) {
            //log_debug("containerChanged. id: {}, signal: {}", objectID, signal);
            objectIDHash->insert(objectID);
            if (split && objectIDHash->size() > MAX_OBJECT_IDS) {
                while (objectIDHash->size() > MAX_OBJECT_IDS) {
                    log_debug("in-between signalling...");
                    cond.notify_one();
                    lock.unlock();
                    lock.lock();
                }
            }
        }
    }
    if (objectIDHash->size() >= MAX_OBJECT_IDS)
        signal = true;
    if (signal) {
        log_debug("signalling...");
        cond.notify_one();
    }
}

void UpdateManager::containerChanged(int objectID, int flushPolicy)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    AutoLock lock(mutex);
    if (objectID != lastContainerChanged || flushPolicy > this->flushPolicy) {
        // signalling thread if it could have been idle, because
        // there were no unprocessed updates
        bool signal = (!haveUpdates());
        log_debug("containerChanged. id: {}, signal: {}", objectID, signal);
        objectIDHash->insert(objectID);

        // signalling if the hash gets too full
        if (objectIDHash->size() >= MAX_OBJECT_IDS)
            signal = true;

        // very simple caching, but it get's a lot of hits
        lastContainerChanged = objectID;

        // signalling if the flushPolicy changes, so the thread recalculates
        // the sleep time
        if (flushPolicy > this->flushPolicy) {
            this->flushPolicy = flushPolicy;
            signal = true;
        }
        if (signal) {
            log_debug("signalling...");
            cond.notify_one();
        }
    } else {
        log_debug("last container changed!");
    }
}

/* private stuff */

void UpdateManager::threadProc()
{
    struct timespec lastUpdate;
    getTimespecNow(&lastUpdate);

    AutoLockU lock(mutex);
    //cond.notify_one();
    while (!shutdownFlag) {
        if (haveUpdates()) {
            long sleepMillis = 0;
            struct timespec now;
            getTimespecNow(&now);
            long timeDiff = getDeltaMillis(&lastUpdate, &now);
            switch (flushPolicy) {
            case FLUSH_SPEC:
                sleepMillis = SPEC_INTERVAL - timeDiff;
                break;
            case FLUSH_ASAP:
                sleepMillis = 0;
                break;
            }
            bool sendUpdates = true;
            if (sleepMillis >= MIN_SLEEP && objectIDHash->size() < MAX_OBJECT_IDS) {
                struct timespec timeout;
                getTimespecAfterMillis(sleepMillis, &timeout, &now);
                log_debug("threadProc: sleeping for {} millis", sleepMillis);

                std::cv_status ret = cond.wait_for(lock, std::chrono::milliseconds(sleepMillis));

                if (!shutdownFlag) {
                    if (ret == std::cv_status::timeout)
                        sendUpdates = false;
                } else
                    sendUpdates = false;
            }

            if (sendUpdates) {
                log_debug("sending updates...");
                lastContainerChanged = INVALID_OBJECT_ID;
                flushPolicy = FLUSH_SPEC;
                std::string updateString;

                try {
                    updateString = storage->incrementUpdateIDs(objectIDHash);
                    objectIDHash->clear(); // hash_data_array will be invalid after clear()
                } catch (const std::runtime_error& e) {
                    log_error("Fatal error when sending updates: {}", e.what());
                    log_error("Forcing Gerbera shutdown.");
                    kill(0, SIGINT);
                }
                lock.unlock(); // we don't need to hold the lock during the sending of the updates
                if (!updateString.empty()) {
                    try {
                        log_debug("updates sent: \"{}\"", updateString.c_str());
                        server->sendCDSSubscriptionUpdate(updateString);
                        getTimespecNow(&lastUpdate);
                    } catch (const std::runtime_error& e) {
                        log_error("Fatal error when sending updates: {}", e.what());
                        log_error("Forcing Gerbera shutdown.");
                        kill(0, SIGINT);
                    }
                } else {
                    log_debug("NOT sending updates (string empty or invalid).");
                }
                lock.lock();
            }
        } else {
            //nothing to do
            cond.wait(lock);
        }
    }

    storage->threadCleanup();
}

void* UpdateManager::staticThreadProc(void* arg)
{
    auto inst = static_cast<UpdateManager*>(arg);
    inst->threadProc();

    return nullptr;
}
