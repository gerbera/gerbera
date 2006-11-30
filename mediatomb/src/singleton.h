/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    singleton.h - this file is part of MediaTomb.
    
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

/// \file singleton.h

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include "sync.h"
#include "zmmf/zmmf.h"

template <class T> class Singleton;

class SingletonManager : public zmm::Object
{
public:
    static zmm::Ref<SingletonManager> getInstance();
    SingletonManager();
    
    void registerSingleton(zmm::Ref<Singleton<zmm::Object> > object);
    void shutdown();
    
protected:
    static zmm::Ref<SingletonManager> instance;
    static zmm::Ref<Mutex> mutex;
    
    zmm::Ref<zmm::ObjectStack<Singleton<zmm::Object> > > singletonStack;
};

template <class T>
class Singleton : public zmm::Object
{
public:
    
    Singleton()
    {
        SingletonManager::getInstance()->registerSingleton(zmm::Ref<Singleton<Object> >((Singleton<Object> *)this));
    }
    
    virtual ~Singleton() { };
    
    static zmm::Ref<T> getInstance()
    {
        if (instance == nil)
        {
            AUTOLOCK(mutex);
            if (instance == nil) // check again, because there is a very small chance
                                 // that 2 threads tried to lock() concurrently
            {
                instance = zmm::Ref<T>(new T());
                instance->init();
            }
        }
        return instance;
    }
    
protected:
    virtual void init() { };
    virtual void shutdown() { };
    static zmm::Ref<T> instance;
    static zmm::Ref<Mutex> mutex;
    
    friend class SingletonManager;
};

#define SINGLETON_MUTEX(klass, recursive) template <> zmm::Ref<Mutex> Singleton<klass>::mutex = zmm::Ref<Mutex>(new Mutex(recursive))
//template <class T> zmm::Ref<Mutex> Singleton<T>::mutex = zmm::Ref<Mutex>(new Mutex());
template <class T> zmm::Ref<T> Singleton<T>::instance = nil;

#endif // __SINGLETON_H__

