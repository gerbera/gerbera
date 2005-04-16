/*  update_manager.cc - this file is part of MediaTomb.
                                                                                
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

#include "update_manager.h"

#include <sys/time.h>
#include "upnp_cds.h"
#include "storage.h"

/* following constants in milliseconds */
#define SPEC_INTERVAL 2000
#define BUFFERING_INTERVAL 3000
#define MIN_SLEEP 1

using namespace zmm;

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
    spec->tv_nsec = millis % 1000;
}

static Ref<UpdateManager> inst;

UpdateInfo::UpdateInfo(String objectID, int updateID)
{
    this->objectID = objectID;
    this->updateID = updateID;
}

UpdateManager::UpdateManager() : Object()
{
    resetUpdates();
} 
void UpdateManager::resetUpdates()
{
    updates = Ref<Array<UpdateInfo> >(new Array<UpdateInfo>(10));
}
void UpdateManager::init()
{
    shutdownFlag = false;
    flushFlag = 0;

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
    shutdownFlag = true;
    lock();
    pthread_cond_signal(&updateCond);
    unlock();
    pthread_mutex_destroy(&updateMutex);
// detached
//    pthread_join(updateThread, NULL);
}

void UpdateManager::containerChanged(String objectID)
{
    if(! haveUpdate(objectID))
    {
        Ref<Storage> storage = Storage::getInstance();
        Ref<CdsObject> obj = storage->loadObject(objectID);
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);

        int updateID = cont->getUpdateID();
        updateID++;

        Ref<UpdateInfo> info(new UpdateInfo(objectID, updateID));

        cont->setUpdateID(updateID);
        storage->updateObject(obj);

        addUpdate(info);
    }
}
void UpdateManager::flushUpdates(int flushFlag)
{
    lock();
    if(flushFlag > this->flushFlag)
    {
        this->flushFlag = flushFlag;
        pthread_cond_signal(&updateCond);
    }
    unlock();
}
Ref<UpdateManager> UpdateManager::getInstance()
{
    if(inst == nil)
    {
        init_start_seconds();
        inst = Ref<UpdateManager>(new UpdateManager());
        inst->init();
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

bool UpdateManager::haveUpdate(String objectID)
{
    lock();
    int size = updates->size();
    for(int i = 0; i < size; i++)
    {
        if(updates->get(i)->objectID == objectID)
        {
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

bool UpdateManager::haveUpdates()
{
    return (updates->size() > 0);
}

void UpdateManager::addUpdate(Ref<UpdateInfo> info)
{
    lock();
    updates->append(info);
    if(updates->size() == 1)
        pthread_cond_signal(&updateCond);
    unlock();
}

void UpdateManager::sendUpdates()
{
    Ref<StringBuffer> buf(new StringBuffer(64));
    int size = updates->size();
    for(int i = 0; i < size; i++)
    {
        Ref<UpdateInfo> info = updates->get(i);
        *buf << info->objectID << ',' << info->updateID << ',';
    }
    
    // skip last comma
    String updateString(buf->c_str(), buf->length() - 1);
    fprintf(stderr, "threadProc: sending updates: %s\n", updateString.c_str());

    // send update string...
    ContentDirectoryService::getInstance()->subscription_update(updateString);

    // reset update list
    resetUpdates();
}


void UpdateManager::threadProc()
{
    struct timespec timeout;
    int ret;

    long nowMillis;
    long sleepMillis;
    long lastIdleMillis = 0; // for buffered update interval calculation
    long lastUpdateMillis = 0; // for spec-based update interval calculation
        
    lock();

    while(! shutdownFlag)
    {
        fprintf(stderr, "threadProc: awakened...\n");

        /* if nothing to do, sleep until awakened */
        if(updates->size() == 0)
        {

            fprintf(stderr, "threadProc: idle sleep ...\n");
            ret = pthread_cond_wait(&updateCond, &updateMutex);
            nowMillis = getMillis();
            lastIdleMillis = nowMillis;
            continue;
        }
        else
        {
            nowMillis = getMillis();
        }

        /* decide how long to sleep and put to sleepMillis var */
        if(flushFlag)
        {
            switch(flushFlag)
            {
                case FLUSH_ASAP: // send ASAP!
                    sleepMillis = 0;
                    break;
                default: // send as soon as UPnP minimal event interval allowes
                    sleepMillis = SPEC_INTERVAL - (nowMillis - lastUpdateMillis);
                    break;
            }
        }
        else
        {
            sleepMillis = BUFFERING_INTERVAL - (nowMillis - lastIdleMillis);
        }

        if(sleepMillis >= MIN_SLEEP) // sleep for sleepMillis milliseconds
        {
            millisToTimespec(nowMillis + sleepMillis, &timeout);
            printf("threadProc: sleeping for %d millis\n", (int)sleepMillis);
            ret = pthread_cond_timedwait(&updateCond, &updateMutex, &timeout);
        }
        else // send updates 
        {
            flushFlag = 0;
            sendUpdates();
            lastUpdateMillis = getMillis();
        }
    }
    unlock();
    printf("threadProc: update thread shut down.\n");
}

void *UpdateManager::staticThreadProc(void *arg)
{
    UpdateManager *inst = (UpdateManager *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}


