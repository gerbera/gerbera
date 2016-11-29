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

#include "update_manager.h"

#include "upnp_cds.h"
#include "storage.h"
#include "tools.h"
#include <sys/types.h>
#include <signal.h>

/* following constants in milliseconds */
#define SPEC_INTERVAL 2000
#define MIN_SLEEP 1

#define MAX_OBJECT_IDS 1000
#define MAX_OBJECT_IDS_OVERLOAD 30
#define OBJECT_ID_HASH_CAPACITY 3109

using namespace zmm;

SINGLETON_MUTEX(UpdateManager, false);

UpdateManager::UpdateManager() : Singleton<UpdateManager>()
{
    objectIDHash = Ref<DBRHash<int> >(new DBRHash<int>(OBJECT_ID_HASH_CAPACITY, MAX_OBJECT_IDS + 2 * MAX_OBJECT_IDS_OVERLOAD, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    shutdownFlag = false;
    flushPolicy = FLUSH_SPEC;
    lastContainerChanged = INVALID_OBJECT_ID;
    cond = Ref<Cond>(new Cond(mutex));
}

void UpdateManager::init()
{
    /*
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    */
    
    pthread_create(
        &updateThread,
        NULL, // &attr, // attr
        UpdateManager::staticThreadProc,
        this
    );
    
    //cond->wait();
    //pthread_attr_destroy(&attr);
}

void UpdateManager::shutdown()
{
    log_debug("start\n");
    AUTOLOCK(mutex);
    shutdownFlag = true;
    log_debug("signalling...\n");
    cond->signal();
    AUTOUNLOCK();
    log_debug("waiting for thread\n");
    if (updateThread)
        pthread_join(updateThread, NULL);
    updateThread = 0;
    log_debug("end\n");
}

void UpdateManager::containersChanged(Ref<IntArray> objectIDs, int flushPolicy)
{
    if (objectIDs == nil)
        return;
    AUTOLOCK(mutex);
    // signalling thread if it could have been idle, because 
    // there were no unprocessed updates
    bool signal = (! haveUpdates());
    // signalling if the flushPolicy changes, so the thread recalculates
    // the sleep time
    if (flushPolicy > this->flushPolicy)
    {
        this->flushPolicy = flushPolicy;
        signal = true;
    }
    int size = objectIDs->size();
    int hashSize = objectIDHash->size();
    bool split = (hashSize + size >= MAX_OBJECT_IDS + MAX_OBJECT_IDS_OVERLOAD);
    for (int i = 0; i < size; i++)
    {
        int objectID = objectIDs->get(i);
        if (objectID != lastContainerChanged)
        {
            //log_debug("containerChanged. id: %d, signal: %d\n", objectID, signal);
            objectIDHash->put(objectID);
            if (split && objectIDHash->size() > MAX_OBJECT_IDS)
            {
                while(objectIDHash->size() > MAX_OBJECT_IDS)
                {
                    log_debug("in-between signalling...\n");
                    cond->signal();
                    AUTOUNLOCK();
                    AUTORELOCK();
                }
            }
        }
    }
    if (objectIDHash->size() >= MAX_OBJECT_IDS)
        signal = true;
    if (signal)
    {
        log_debug("signalling...\n");
        cond->signal();
    }
}

void UpdateManager::containerChanged(int objectID, int flushPolicy)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    AUTOLOCK(mutex);
    if (objectID != lastContainerChanged || flushPolicy > this->flushPolicy)
    {
        // signalling thread if it could have been idle, because 
        // there were no unprocessed updates
        bool signal = (! haveUpdates());
        log_debug("containerChanged. id: %d, signal: %d\n", objectID, signal);
        objectIDHash->put(objectID);
        
        // signalling if the hash gets too full
        if (objectIDHash->size() >= MAX_OBJECT_IDS)
            signal = true;
        
        // very simple caching, but it get's a lot of hits
        lastContainerChanged = objectID;
        
        // signalling if the flushPolicy changes, so the thread recalculates
        // the sleep time
        if (flushPolicy > this->flushPolicy)
        {
            this->flushPolicy = flushPolicy;
            signal = true;
        }
        if (signal)
        {
            log_debug("signalling...\n");
            cond->signal();
        }
    }
    else
    {
        log_debug("last container changed!\n");
    }
}

/* private stuff */

void UpdateManager::threadProc()
{
    struct timespec lastUpdate;
    getTimespecNow(&lastUpdate);
    
    AUTOLOCK(mutex);
    //cond->signal();
    while (! shutdownFlag)
    {
        if (haveUpdates())
        {
            long sleepMillis = 0;
            struct timespec now;
            getTimespecNow(&now);
            long timeDiff = getDeltaMillis(&lastUpdate, &now);
            switch (flushPolicy)
            {
                case FLUSH_SPEC:
                    sleepMillis = SPEC_INTERVAL - timeDiff;
                    break;
                case FLUSH_ASAP:
                    sleepMillis = 0;
                    break;
            }
            bool sendUpdates = true;
            if (sleepMillis >= MIN_SLEEP && objectIDHash->size() < MAX_OBJECT_IDS)
            {
                struct timespec timeout;
                getTimespecAfterMillis(sleepMillis, &timeout, &now);
                log_debug("threadProc: sleeping for %ld millis\n", sleepMillis);
                
                int ret = cond->timedwait(&timeout);
                
                if (! shutdownFlag)
                {
                    if (ret != 0 && ret != ETIMEDOUT)
                    {
                        log_error("Fatal error: pthread_cond_timedwait returned errorcode %d\n", ret);
                        log_error("Forcing MediaTomb shutdown.\n");
                        print_backtrace();
                        kill(0, SIGINT);
                    }
                    if (ret == ETIMEDOUT)
                        sendUpdates = false;
                }
                else
                    sendUpdates = false;
            }
            
            if (sendUpdates)
            {
                log_debug("sending updates...\n");
                lastContainerChanged = INVALID_OBJECT_ID;
                flushPolicy = FLUSH_SPEC;
                String updateString;

                try
                {
                    hash_data_array_t<int> hash_data_array;
                    // hash_data_array points to the array inside objectIDHash, so
                    // we may only call clear() after we don't need the array anymore
                    objectIDHash->getAll(&hash_data_array);
                    updateString = Storage::getInstance()->incrementUpdateIDs(hash_data_array.data,hash_data_array.size);
                    objectIDHash->clear(); // hash_data_array will be invalid after clear()
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                    log_error("Fatal error when sending updates: %s\n", e.getMessage().c_str());
                    log_error("Forcing MediaTomb shutdown.\n");
                    kill(0, SIGINT);
                }
                AUTOUNLOCK(); // we don't need to hold the lock during the sending of the updates
                if (string_ok(updateString))
                {
                    try
                    {
                    ContentDirectoryService::getInstance()->subscription_update(updateString);
                    log_debug("updates sent.\n");
                    getTimespecNow(&lastUpdate);
                    }
                    catch (Exception e)
                    {
                        log_error("Fatal error when sending updates: %s\n", e.getMessage().c_str());
                        log_error("Forcing MediaTomb shutdown.\n");
                        kill(0, SIGINT);
                    }
                }
                else
                {
                    log_debug("NOT sending updates (string empty or invalid).\n");
                }
                AUTORELOCK();
            }
        }
        else
        {
            //nothing to do
            cond->wait();
        }
    }
}

void *UpdateManager::staticThreadProc(void *arg)
{
    log_debug("starting update thread... thread: %d\n", pthread_self());
    UpdateManager *inst = (UpdateManager *)arg;
    inst->threadProc();
    Storage::getInstance()->threadCleanup();
    
    log_debug("update thread shut down. thread: %d\n", pthread_self());
    pthread_exit(NULL);
    return NULL;
}
