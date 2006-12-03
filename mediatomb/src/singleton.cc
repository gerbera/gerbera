/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    singleton.cc - this file is part of MediaTomb.
    
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

/// \file singleton.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "singleton.h"

using namespace zmm;

Ref<SingletonManager> SingletonManager::instance = nil;
Ref<Mutex> SingletonManager::mutex = Ref<Mutex>(new Mutex());

Ref<SingletonManager> SingletonManager::getInstance()
{
    if (instance == nil)
    {
        AUTOLOCK(mutex);
        if (instance == nil) // check again, because there is a very small chance
                             // that 2 threads tried to lock() concurrently
        {
            instance = zmm::Ref<SingletonManager>(new SingletonManager());
        }
    }
    return instance;
}

SingletonManager::~SingletonManager()
{
    if (mutex != nil)
        mutex = nil;
}

SingletonManager::SingletonManager() : Object()
{
    singletonStack = Ref<ObjectStack<Singleton<Object> > >(new ObjectStack<Singleton<Object> >(SINGLETON_CUR_MAX));
}

void SingletonManager::registerSingleton(Ref<Singleton<Object> > object)
{
    AUTOLOCK(mutex);
#ifdef LOG_TOMBDEBUG
    if (singletonStack->size() >= SINGLETON_CUR_MAX)
    {
        printf("%d singletons are active (SINGLETON_CUR_MAX=%d) and tried to add another singleton - check this!\n", singletonStack->size(), SINGLETON_CUR_MAX);
        print_backtrace();
        abort();
    }
#endif
    log_debug("registering new singleton... - %d -> %d\n", singletonStack->size(), singletonStack->size() + 1);
    singletonStack->push(object);
}

void SingletonManager::shutdown(bool complete)
{
    log_debug("start (%d objects)\n", singletonStack->size());
    AUTOLOCK(mutex);
    Ref<Singleton<Object> > object;
    while((object = singletonStack->pop()) != nil)
    {
        //log_debug("destoying... \n");
        //_print_backtrace(stdout);
        object->inactivateSingleton();
        object->shutdown();
        //object->destroyMutex();
    }
    if (complete && instance != nil)
        instance = nil;
    log_debug("end\n");
}

