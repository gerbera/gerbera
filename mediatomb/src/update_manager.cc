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


//#define LOCK()   { printf("!!! try to lock: line %d; thread: %d\n", __LINE__, (int)pthread_self()); lock(); \
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
        if (instance == nil)
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
    log_debug("shutdown, locking\n");
    shutdownFlag = true;
    LOCK();
    log_debug("signalling...\n");
    pthread_cond_signal(&updateCond);
    log_debug("signalled, unlocking\n");
    UNLOCK();
    log_debug("unlocked\n");
}

/*
void UpdateManager::containersChanged(int *ids, int size)
{
    Storage::getInstance()->incrementUpdateIDs(objectIDs, objectIDcount);
}
*/

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
        if (objectIDHash->size() >= MAX_OBJECT_IDS)
            signal = true;
        lastContainerChanged = objectID;
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
            long timeDiff = getDeltaMillis(&lastUpdate);
            switch (flushPolicy)
            {
                case FLUSH_SPEC:
                    sleepMillis = SPEC_INTERVAL - timeDiff;
                    break;
                case FLUSH_ASAP:
                    sleepMillis = 0;
                    break;
            }
            bool sendUpdates = false;
            if (sleepMillis >= MIN_SLEEP && objectIDHash->size() < MAX_OBJECT_IDS)
            {
                struct timespec timeout;
                getTimespecAfterMillis(timeDiff + sleepMillis, &timeout, &lastUpdate);
                log_debug("threadProc: sleeping for %ld millis\n", sleepMillis);
                
                int ret = pthread_cond_timedwait(&updateCond, mutex.getMutex(), &timeout);
                if (! shutdownFlag)
                {
                    if (ret != 0 && ret != ETIMEDOUT)
                        throw _Exception(_("pthread_cond_timedwait returned errorcode ") + ret);
                    if (ret == 0)
                        sendUpdates = true;
                }
            }
            else
                sendUpdates = true;
            
            if (sendUpdates)
            {
                log_debug("sending updates...\n");
                lastContainerChanged = INVALID_OBJECT_ID;
                flushPolicy = FLUSH_SPEC;
                
                hash_data_array_t<int> hash_data_array;
                objectIDHash->getAll(&hash_data_array);
                String updateString = Storage::getInstance()->incrementUpdateIDs(hash_data_array.data,hash_data_array.size);
                ContentDirectoryService::getInstance()->subscription_update(updateString);
                objectIDHash->clear();
                
                getTimeval(&lastUpdate);
                log_debug("updates sent.\n");
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

