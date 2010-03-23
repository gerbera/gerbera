/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sync.h - this file is part of MediaTomb.
    
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

/// \file sync.h

#ifndef __SYNC_H__
#define __SYNC_H__

#include "common.h"
#include <pthread.h>

#define AUTOLOCK_DEFINE_ONLY() zmm::Ref<MutexAutolock> mutex_autolock;
#define AUTOLOCK_NOLOCK(mutex) zmm::Ref<MutexAutolock> mutex_autolock = mutex->getAutolock(true);
#define AUTOLOCK(mutex) zmm::Ref<MutexAutolock> mutex_autolock = mutex->getAutolock();
#define AUTOLOCK_NO_DEFINE(mutex) mutex_autolock = mutex->getAutolock();
#define AUTOUNLOCK() mutex_autolock->unlock();
#define AUTORELOCK() mutex_autolock->relock();
#define LOCK(mutex) mutex->lock();
#define UNLOCK(mutex) mutex->unlock();

class Mutex;

class MutexAutolock : public zmm::Object
{
public:
    
#ifndef TOMBDEBUG
    inline ~MutexAutolock() { if (locked) pthread_mutex_unlock(pmutex); }
    inline void unlock() { pthread_mutex_unlock(pmutex); locked = false; }
    inline void relock() { pthread_mutex_lock(pmutex); locked = true; }
#else
    ~MutexAutolock();
    void unlock();
    void relock();
#endif
protected:
    MutexAutolock(zmm::Ref<Mutex> mutex, bool unlocked = false);
    zmm::Ref<Mutex> mutex;
    bool locked;
#ifndef TOMBDEBUG
    pthread_mutex_t *pmutex;
#endif
    
    friend class Mutex;
};

class Mutex : public zmm::Object
{
public:
    Mutex(bool recursive = false);
    virtual ~Mutex();
#ifndef TOMBDEBUG
    inline void lock() { pthread_mutex_lock(&mutex_struct); }
    inline void unlock() { pthread_mutex_unlock(&mutex_struct); }
#else
    void lock();
    void unlock(bool autolock = false);
#endif
    inline zmm::Ref<MutexAutolock> getAutolock(bool unlocked = false) { return zmm::Ref<MutexAutolock>(new MutexAutolock(zmm::Ref<Mutex>(this), unlocked)); }
protected:
    pthread_mutex_t mutex_struct;
    inline pthread_mutex_t* getMutex() { return &mutex_struct; }
    
#ifdef TOMBDEBUG
    void errorExit(zmm::String error);
    inline pthread_t getLockingThread() { return locking_thread; }
    void doLock(bool cond);
    void doUnlock(bool cond);
public:
    inline bool isLocked() { return lock_level > 0; }
protected:
    inline void lockAutolock() { lock(); autolock = true; }
    void unlockAutolock();
    int lock_level;
    bool recursive;
    bool autolock;
    pthread_t locking_thread;
#endif
    
    friend class MutexAutolock;
    friend class Cond;
};

class Cond : public zmm::Object
{
public:
    Cond(zmm::Ref<Mutex> mutex);
    virtual ~Cond();
    inline void signal() { pthread_cond_signal(&cond_struct); }
    inline void broadcast() { pthread_cond_broadcast(&cond_struct); }
#ifndef TOMBDEBUG
    inline void wait() { pthread_cond_wait(&cond_struct, mutex->getMutex()); }
    inline int timedwait(struct timespec *timeout) { return pthread_cond_timedwait(&cond_struct, mutex->getMutex(), timeout); }
#else
    void wait();
    int timedwait(struct timespec *timeout);
    //void checkwait();
#endif
protected:
    pthread_cond_t cond_struct;
    zmm::Ref<Mutex> mutex;
};

#endif // __SYNC_H__
