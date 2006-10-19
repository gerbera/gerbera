/*  update_manager.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadLOCK.dhs.org>,
                       Sergey Bostandzhyan <jin@deadLOCK.dhs.org>
                                                                                
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

#include "update_manager.h"

#include "upnp_cds.h"
#include "storage.h"
#include "tools.h"

/* following constants in milliseconds */
#define SPEC_INTERVAL 2000
#define MIN_SLEEP 1

#define MAX_OBJECT_IDS 1000
#define OBJECT_ID_HASH_CAPACITY 3109


//#define LOCK()   { printf("!!! try to lock: line %d; thread: %d\n", __LINE__, (int)pthread_self()); lock(); 
//                   printf("!!! locked:      line %d; thread: %d\n", __LINE__, (int)pthread_self()); }
//#define UNLOCK() { printf("!!! unlock:      line %d: thread: %d\n", __LINE__, (int)pthread_self()); unlock(); }

#define LOCK lock
#define UNLOCK unlock

using namespace zmm;

UpdateManager::UpdateManager() : Object()
{
    objectIDHash = Ref<DBRHash<int> >(new DBRHash<int>(OBJECT_ID_HASH_CAPACITY, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    shutdownFlag = false;
    flushPolicy = FLUSH_SPEC;
    lastContainerChanged = INVALID_OBJECT_ID;
}

Mutex UpdateManager::mutex = Mutex();
Ref<UpdateManager> UpdateManager::instance = nil;

Ref<UpdateManager> UpdateManager::getInstance()
{
    if (instance == nil)
    {
        LOCK();
        if (instance == nil) // check again, because there is a very small chance
                             // that 2 threads tried to lock() concurrently
        {
            try
            {
                instance = Ref<UpdateManager>(new UpdateManager());
                instance->init();
            }
            catch (Exception e)
            {
                UNLOCK();
                throw e;
            }
        }
        UNLOCK();
    }
    return instance;
}

UpdateManager::~UpdateManager()
{
    pthread_cond_destroy(&updateCond);
} 

void UpdateManager::init()
{
    int ret;
    
    ret = pthread_cond_init(&updateCond, NULL);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    pthread_t updateThread;
    
    ret = pthread_create(
        &updateThread,
        &attr, // attr
        UpdateManager::staticThreadProc,
        this
    );
    
    pthread_attr_destroy(&attr);
}

void UpdateManager::shutdown()
{
    //log_debug("shutdown, locking\n");
    shutdownFlag = true;
    LOCK(); // locking isn't necessary, because shutdown() is the only place
              // where shutdownFlag get's set (and it's never reset)
              // and we don't need a predictable scheduling here.
    log_debug("signalling...\n");
    pthread_cond_signal(&updateCond);
    //log_debug("signalled, unlocking\n");
    UNLOCK();
    //log_debug("unlocked\n");
}

void UpdateManager::containerChanged(int objectID, int flushPolicy)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    LOCK();
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
            pthread_cond_signal(&updateCond);
        }
    }
    else
    {
        log_debug("last container changed!\n");
    }
    UNLOCK();
}

/* private stuff */

void UpdateManager::threadProc()
{
    struct timeval lastUpdate;
    getTimeval(&lastUpdate);
    
    LOCK();
    while (! shutdownFlag)
    {
        if (haveUpdates())
        {
            long sleepMillis;
            struct timeval now;
            getTimeval(&now);
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
                log_debug("timeDiff: %ld timeout: %ld - %ld\n", timeDiff, timeout.tv_sec, timeout.tv_nsec);
                log_debug("lastUpdate: %ld %ld\n", lastUpdate.tv_sec, lastUpdate.tv_usec);
                log_debug("now: %ld %ld\n", now.tv_sec, now.tv_usec);
                
                int ret = pthread_cond_timedwait(&updateCond, mutex.getMutex(), &timeout);
                
                if (! shutdownFlag)
                {
                    if (ret != 0 && ret != ETIMEDOUT)
                    {
                        log_debug("pthread_cond_timedwait returned errorcode %d\n", ret);
                        throw _Exception(_("pthread_cond_timedwait returned errorcode ") + ret);
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
                
                hash_data_array_t<int> hash_data_array;
                // hash_data_array points to the array inside objectIDHash, so
                // we may only call clear() after we don't need the array anymore
                objectIDHash->getAll(&hash_data_array);
                String updateString = Storage::getInstance()->incrementUpdateIDs(hash_data_array.data,hash_data_array.size);
                objectIDHash->clear(); // hash_data_array will be invalid after clear()
                
                UNLOCK(); // we don't need to hold the lock during the sending of the updates
                ContentDirectoryService::getInstance()->subscription_update(updateString);
                getTimeval(&lastUpdate);
                log_debug("updates sent.\n");
                LOCK();
            }
        }
        else
        {
            //nothing to do
            int ret = pthread_cond_wait(&updateCond, mutex.getMutex());
            if (ret)
                throw _Exception(_("pthread_cond_wait returned errorcode ") + ret);
        }
    }
    UNLOCK();
}

void *UpdateManager::staticThreadProc(void *arg)
{
    log_debug("starting update thread... thread: %d\n", pthread_self());
    UpdateManager *inst = (UpdateManager *)arg;
    inst->threadProc();
    log_debug("update thread shut down. thread: %d\n", pthread_self());
    pthread_exit(NULL);
    return NULL;
}

