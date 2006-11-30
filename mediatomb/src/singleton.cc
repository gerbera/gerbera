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
    
    $Id: object.h 1123 2006-11-03 01:03:30Z leo $
*/

/// \file singleton.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "singleton.h"

using namespace zmm;

#define SINGLETON_MANAGEMENT_INITIAL_CAPACITY 30

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

SingletonManager::SingletonManager() : Object()
{
    singletonStack = Ref<ObjectStack<Singleton<Object> > >(new ObjectStack<Singleton<Object> >(SINGLETON_MANAGEMENT_INITIAL_CAPACITY));
}

void SingletonManager::registerSingleton(Ref<Singleton<Object> > object)
{
    log_debug("registering new singleton... - %d -> %d\n", singletonStack->size(), singletonStack->size() + 1);
    singletonStack->push(object);
}

void SingletonManager::shutdown()
{
    log_debug("start (%d objects)\n", singletonStack->size());
    AUTOLOCK(mutex);
    Ref<Singleton<Object> > object;
    while((object = singletonStack->pop()) != nil)
    {
        object->shutdown();
    }
    log_debug("end\n");
}

