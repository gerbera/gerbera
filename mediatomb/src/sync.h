/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    sync.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file sync.h

#ifndef __SYNC_H__
#define __SYNC_H__

#include "common.h"
#include <pthread.h>

class Mutex;

class MutexAutolock : public zmm::Object
{
public:
    ~MutexAutolock();
protected:
    MutexAutolock(zmm::Ref<Mutex> mutex);
    zmm::Ref<Mutex> mutex;
    friend class Mutex;
};

class Mutex : public zmm::Object
{
public:
    Mutex(bool recursive = false);
    virtual ~Mutex();
#ifndef LOG_TOMBDEBUG
    inline void lock() { pthread_mutex_lock(&mutex_struct); }
    inline void unlock() { pthread_mutex_unlock(&mutex_struct); }
#else
    void lock();
    void unlock(bool autolock = false);
#endif
    inline zmm::Ref<MutexAutolock> getAutolock() { return zmm::Ref<MutexAutolock>(new MutexAutolock(zmm::Ref<Mutex>(this))); }
protected:
    pthread_mutex_t mutex_struct;
    inline pthread_mutex_t* getMutex() { return &mutex_struct; }
    
#ifdef LOG_TOMBDEBUG
    void errorExit(zmm::String error);
    inline pthread_t getLockingThread() { return locking_thread; }
    inline void doLock() { lock_level++; locking_thread = pthread_self(); autolock = false; }
    inline void doUnlock() { lock_level--; };
    inline bool isLocked() { return lock_level > 0; }
    inline void lockAutolock() { lock(); autolock = true; }
    void unlockAutolock();
    int lock_level;
    bool recursive;
    bool autolock;
    pthread_t locking_thread;
    
    friend class MutexAutolock;
#endif
    
    friend class Cond;
};

class Cond : public zmm::Object
{
public:
    Cond(zmm::Ref<Mutex> mutex);
    virtual ~Cond();
    inline void signal() { pthread_cond_signal(&cond_struct); }
    inline void broadcast() { pthread_cond_broadcast(&cond_struct); }
#ifndef LOG_TOMBDEBUG
    inline void wait() { pthread_cond_wait(&cond_struct, mutex->getMutex()); }
    inline int timedwait(struct timespec *timeout) { return pthread_cond_timedwait(&cond_struct, mutex->getMutex(), timeout); }
#else
    void wait();
    int timedwait(struct timespec *timeout);
    void checkwait();
#endif
protected:
    pthread_cond_t cond_struct;
    zmm::Ref<Mutex> mutex;
    
#ifdef LOG_TOMBDEBUG
    
#endif
};

#endif // __SYNC_H__

