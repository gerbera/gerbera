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

#include <sys/time.h>
#include "upnp_cds.h"
#include "storage.h"

/* following constants in milliseconds */
#define SPEC_INTERVAL 2000
#define BUFFERING_INTERVAL 3000
#define MIN_SLEEP 1

#define MAX_OBJECT_IDS 1000
#define MAX_LAZY_OBJECT_IDS 1000
#define OBJECT_ID_HASH_CAPACITY 3109


#define LOCK()   { printf("lock:   line %d\n", __LINE__); this->lock(); \
                   printf("locked: line %d\n", __LINE__); }
#define UNLOCK() { printf("unlock: line %d\n", __LINE__); this->unlock(); }

//#define LOCK lock
//#define UNLOCK unlock

using namespace zmm;

Mutex UpdateManager::mutex = Mutex(false);

static long start_seconds;

void init_start_seconds()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    start_seconds = now.tv_sec;
}

long getMillis()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    long seconds = now.tv_sec - start_seconds;
    long millis = now.tv_usec / 1000L;
    
    return seconds * 1000 + millis;
}
inline void millisToTimespec(long millis, struct timespec *spec)
{
    spec->tv_sec = start_seconds + millis / 1000;
    spec->tv_nsec = (millis % 1000) * 1000000;
}

static Ref<UpdateManager> inst;

UpdateManager::UpdateManager() : Object()
{
    objectIDs = (int *)MALLOC(MAX_OBJECT_IDS * sizeof(int));
    objectIDcount = 0;
    objectIDHash = Ref<DBBHash<int, int> >(new DBBHash<int, int>(OBJECT_ID_HASH_CAPACITY, -100));
} 
UpdateManager::~UpdateManager()
{
    FREE(objectIDs);
    pthread_mutex_destroy(&updateMutex);
    pthread_cond_destroy(&updateCond);
} 

void UpdateManager::init()
{
    shutdownFlag = false;
    flushFlag = 0;
//    lazyMode = true;
    lazyMode = false;

    int ret;

    ret = pthread_mutex_init(&updateMutex, NULL);
    ret = pthread_cond_init(&updateCond, NULL);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
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
// detached
//    pthread_join(updateThread, NULL);
}

void UpdateManager::containersChanged(int *ids, int size)
{
    Storage::getInstance()->incrementUpdateIDs(objectIDs, objectIDcount);
}

void UpdateManager::containerChanged(int objectID)
{
    hash_slot_t slot;
    int updateID;

    LOCK();

    bool wasEmpty = (objectIDcount == 0);
    bool found = objectIDHash->get(objectID, &slot, &updateID);
    if (found)
    {
        if (updateID < 0)
        {
            updateID = -updateID + 1;
            objectIDs[objectIDcount++] = objectID;
            objectIDHash->put(objectID, slot, updateID);
        }
    }
    else
    {
        try
        {
            Ref<Storage> storage = Storage::getInstance();        
            Ref<CdsObject> obj = storage->loadObject(objectID);
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);

            updateID = cont->getUpdateID();
            updateID++;

            objectIDs[objectIDcount++] = objectID;
            objectIDHash->put(objectID, slot, updateID);
            if (! lazyMode)
            {
                cont->setUpdateID(updateID);
                storage->updateObject(obj);
            }
        }
        catch (Exception e)
        {}
    }
    bool flush = false;
    if (wasEmpty && objectIDcount == 1)
        pthread_cond_signal(&updateCond);
    else if (objectIDcount >= MAX_OBJECT_IDS)
        flush = true;
    else if (objectIDHash->size() >= MAX_LAZY_OBJECT_IDS)
        flush = true;
    UNLOCK();
    if (flush)
        flushUpdates(FLUSH_ASAP);
}

void UpdateManager::flushUpdates(int flushFlag)
{
    LOCK();
    if (flushFlag > this->flushFlag)
    {
        this->flushFlag = flushFlag;
        pthread_cond_signal(&updateCond);
    }
    UNLOCK();
}
Ref<UpdateManager> UpdateManager::getInstance()
{
    if (inst == nil)
    {
        mutex.lock();
        if (inst == nil)
        {
            init_start_seconds();
            inst = Ref<UpdateManager>(new UpdateManager());
            ///\todo go through all classes and decide how we want to handle 
            ///class init functions - automatically in getIntsance or by manual calls
//            inst->init();
        }
        mutex.unlock();
    }
    return inst;
}

/* private stuff */
void UpdateManager::lock()
{
    pthread_mutex_lock(&updateMutex);
}
void UpdateManager::unlock()
{
    pthread_mutex_unlock(&updateMutex);
}

bool UpdateManager::haveUpdates()
{
    return (objectIDcount > 0);
}

void UpdateManager::sendUpdates()
{
    log_debug("SEND UPDATES\n");
    Ref<StringBuffer> buf(new StringBuffer(objectIDcount * 4));
    int updateID;
    if (lazyMode)
    {
        for (int i = 0; i < objectIDcount; i++)
        {
            hash_slot_t slot;
            objectIDHash->get(objectIDs[i], &slot, &updateID);
            *buf << objectIDs[i] << ',' << updateID << ',';
            objectIDHash->put(objectIDs[i], slot, -updateID);
        }
        Storage::getInstance()->incrementUpdateIDs(objectIDs, objectIDcount);
    }
    else
    {
        for (int i = 0; i < objectIDcount; i++)
        {
            objectIDHash->get(objectIDs[i], &updateID);
            *buf << objectIDs[i] << ',' << updateID << ',';
        }
        objectIDHash->clear();
    }
    objectIDcount = 0;
    
    // skip last comma
    String updateString(buf->c_str(), buf->length() - 1);
    // send update string...
    ContentDirectoryService::getInstance()->subscription_update(updateString);
    log_debug("SEND UPDATES RETURNING\n");
}


void UpdateManager::threadProc()
{
    struct timespec timeout;
    int ret;

    long nowMillis;
    long sleepMillis;
    long lastIdleMillis = 0; // for buffered update interval calculation
    long lastUpdateMillis = 0; // for spec-based update interval calculation
        
    LOCK();

    while (!shutdownFlag)
    {
        log_debug("threadProc: awakened... have updates: %d\n", haveUpdates());

        /* if nothing to do, sleep until awakened */
        if (! haveUpdates())
        {

            log_debug("threadProc: idle sleep ...\n");
            ret = pthread_cond_wait(&updateCond, &updateMutex);
            lastIdleMillis = nowMillis;
            continue;
        }
        else
        {
            nowMillis = getMillis();
        }

        /* decide how long to sleep and put to sleepMillis var */
        if (flushFlag)
        {
            switch (flushFlag)
            {
                case FLUSH_ASAP: // send ASAP!
                    sleepMillis = 0;
                    break;
                default: // send as soon as UPnP minimal event interval allows
                    sleepMillis = SPEC_INTERVAL - (nowMillis - lastUpdateMillis);
                    break;
            }
        }
        else
        {
            sleepMillis = BUFFERING_INTERVAL - (nowMillis - lastIdleMillis);
        }

        if (sleepMillis >= MIN_SLEEP) // sleep for sleepMillis milliseconds
        {
            millisToTimespec(nowMillis + sleepMillis, &timeout);
            log_debug("threadProc: sleeping for %d millis\n", (int)sleepMillis);
            bool wait_done = false;
            while (!wait_done)
            {
                ret = pthread_cond_timedwait(&updateCond, &updateMutex, &timeout);
                switch (ret)
                {
                    case 0:
                        wait_done = true;
                        break;
                    default:
                        if (ret == ETIMEDOUT)
                        {
                            wait_done = true;
                        }
                        break;
                }

            }
            if ((ret) && (ret != ETIMEDOUT))
            {
                log_debug("pthread_cont_timedwait(): %d, %s\n", ret, strerror(errno));
            }
        }
        else // send updates 
        {
            flushFlag = 0;
            sendUpdates();
            lastUpdateMillis = getMillis();
        }
    }
    log_debug("shutting down update thread!\n");
    UNLOCK();
    log_debug("threadProc: update thread shut down, %d\n", pthread_self());
}

void *UpdateManager::staticThreadProc(void *arg)
{
    UpdateManager *inst = (UpdateManager *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}


