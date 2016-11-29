/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sync.cc - this file is part of MediaTomb.
    
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

/// \file sync.cc

#include "sync.h"

using namespace zmm;

/* Mutex */

Mutex::Mutex(bool recursive) : Object()
{
#ifdef TOMBDEBUG
    this->recursive = recursive;
    lock_level = 0;
    
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, (recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_ERRORCHECK));
    pthread_mutex_init(&mutex_struct, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
#else
    if (recursive)
    {
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex_struct, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
    }
    else
        pthread_mutex_init(&mutex_struct, NULL);
#endif
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex_struct);
}

#ifdef TOMBDEBUG
void Mutex::lock()
{
    pthread_mutex_lock(&mutex_struct);
    pthread_t this_thread = pthread_self();
    if (lock_level && ! recursive && this_thread == locking_thread)
        errorExit(_("same thread tried to lock non-recursive mutex twice"));
    doLock(false);
    autolock = false;
}

void Mutex::unlock(bool autolock)
{
    if (! recursive && autolock != this->autolock)
    {
        if (autolock)
            errorExit(_("unlock() called by autolock, but not locked by getAutolock()?? - seems to be an error in sync.cc..."));
        else
            errorExit(_("unlock() called, but locked by an getAutolock()!"));
    }
    pthread_t this_thread = pthread_self();
    if (lock_level <= 0)
     errorExit(_("tried to unlock not locked mutex"));
    if (this_thread != locking_thread)
     errorExit(_("a different thread tried to unlock the locked mutex"));
    doUnlock(false);
    pthread_mutex_unlock(&mutex_struct);
}


void Mutex::errorExit(String error)
{
    printf("%s\n", error.c_str());
    print_backtrace();
    abort();
}

void Mutex::unlockAutolock()
{
    unlock(true);
}

void Mutex::doLock(bool cond)
{
    lock_level++;
    if (! recursive && lock_level > 1)
        errorExit(_("managed to lock a non-recursive thread twice? cond: ") + cond);
    locking_thread = pthread_self();
}

void Mutex::doUnlock(bool cond)
{
    if (cond && recursive && lock_level != 1)
        errorExit(_("tried to unlock a recursive mutex via a cond->wait() or cond->timedwait() call, which is locked more than once - is this correct?"));
    if (--lock_level < 0)
        errorExit(_("unlocked a thread somehow, but it wasn't locked. cond: ") + cond);
}

#endif // TOMBDEBUG

/* Cond */

Cond::Cond(Ref<Mutex> mutex) : Object()
{
    this->mutex = mutex;
    pthread_cond_init(&cond_struct, NULL);
}

Cond::~Cond()
{
    pthread_cond_destroy(&cond_struct);
}

#ifdef TOMBDEBUG
void Cond::wait()
{
    bool autolock_save = mutex->autolock;
    mutex->doUnlock(true);
    pthread_cond_wait(&cond_struct, mutex->getMutex());
    mutex->doLock(true);
    mutex->autolock = autolock_save;
}

int Cond::timedwait(struct timespec *timeout)
{
    mutex->doUnlock(true);
    int ret = pthread_cond_timedwait(&cond_struct, mutex->getMutex(), timeout);
    mutex->doLock(true);
    return ret;
}

/*
void Cond::checkwait()
{
    if (! mutex->isLocked())
        mutex->errorExit(_("tried to do a cond_wait with an unlocked mutex"));
    if (mutex->lock_level > 1)
        mutex->errorExit(_("pthread_cond(timed)wait called for a (recursive) mutex, that is locked more than once!"));
    pthread_t this_thread = pthread_self();
    pthread_t mutex_thread = mutex->getLockingThread();
    if (this_thread != mutex_thread)
        mutex->errorExit(_("tried to do a cond_wait with a mutex locked by another thread"));
}
*/
#endif // TOMBDEBUG


MutexAutolock::MutexAutolock(Ref<Mutex> mutex, bool unlocked)
{
    this->mutex = mutex;
#ifdef TOMBDEBUG
    if (! unlocked)
        mutex->lockAutolock();
#else
    pmutex = mutex->getMutex();
    if (! unlocked)
        pthread_mutex_lock(pmutex);
#endif
    locked = ! unlocked;
}

#ifdef TOMBDEBUG
MutexAutolock::~MutexAutolock()
{
    if (locked)
        mutex->unlockAutolock();
}

void MutexAutolock::unlock()
{
    if (locked)
        mutex->unlockAutolock();
    else
        mutex->errorExit(_("tried to unlock a not-locked autolock"));
    locked = false;
}

void MutexAutolock::relock()
{
    if (! locked)
        mutex->lockAutolock();
    else
        mutex->errorExit(_("tried to relock a locked autolock"));
    locked = true;
}
#endif
