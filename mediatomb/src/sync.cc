/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    sync.cc - this file is part of MediaTomb.
    
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

/// \file sync.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "sync.h"

using namespace zmm;

/* Mutex */

Mutex::Mutex(bool recursive) : Object()
{
#ifdef LOG_TOMBDEBUG
    this->recursive = recursive;
    lock_level = 0;
#endif
    int res;
    if (recursive)
    {
        pthread_mutexattr_t mutex_attr;
        res = pthread_mutexattr_init(&mutex_attr);
        res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex_struct, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
    }
    else
        pthread_mutex_init(&mutex_struct, NULL);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex_struct);
}

#ifdef LOG_TOMBDEBUG
void Mutex::lock()
{
    pthread_t this_thread = pthread_self();
    if (lock_level && ! recursive && this_thread == locking_thread)
        errorExit(_("same thread tried to lock non-recursive mutex twice"));
    pthread_mutex_lock(&mutex_struct);
    doLock();
    autolock = false;
}

void Mutex::unlock(bool autolock)
{
    if (! recursive && autolock != this->autolock)
    {
        if (autolock)
            errorExit(_("unlock() called by autolock, but not locked by an getAutolock()?? - seems to be an error in sync.cc..."));
        else
            errorExit(_("unlock() called, but locked by an getAutolock()!"));
    }
    pthread_t this_thread = pthread_self();
    if (lock_level <= 0)
     errorExit(_("tried to unlock not locked mutex"));
    if (this_thread != locking_thread)
     errorExit(_("a different thread tried to unlock the locked mutex"));
    doUnlock();
    pthread_mutex_unlock(&mutex_struct);
}


void Mutex::errorExit(String error)
{
    //log_error(error.c_str());
    Exception e(error);
    e.printStackTrace();
    exit(1);
}

void Mutex::unlockAutolock()
{
    unlock(true);
}

#endif // LOG_TOMBDEBUG

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

#ifdef LOG_TOMBDEBUG
void Cond::wait()
{
    mutex->doUnlock();
    pthread_cond_wait(&cond_struct, mutex->getMutex());
    mutex->doLock();
}

int Cond::timedwait(struct timespec *timeout)
{
    mutex->doUnlock();
    int ret = pthread_cond_timedwait(&cond_struct, mutex->getMutex(), timeout);
    mutex->doLock();
    return ret;
}

void Cond::checkwait()
{
    if (! mutex->isLocked())
        mutex->errorExit(_("tried to do a cond_wait with an unlocked mutex"));
    pthread_t this_thread = pthread_self();
    pthread_t mutex_thread = mutex->getLockingThread();
    if (this_thread != mutex_thread)
        mutex->errorExit(_("tried to do a cond_wait with a mutex locked by another thread"));
}
#endif // LOG_TOMBDEBUG


MutexAutolock::MutexAutolock(Ref<Mutex> mutex)
{
    this->mutex = mutex;
#ifdef LOG_TOMBDEBUG
    mutex->lockAutolock();
#else
    mutex->lock();
#endif
}

MutexAutolock::~MutexAutolock()
{
#ifdef LOG_TOMBDEBUG
    mutex->unlockAutolock();
#else
    mutex->unlock();
#endif
}

