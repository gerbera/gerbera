/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    singleton.h - this file is part of MediaTomb.
    
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

/// \file singleton.h

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include "sync.h"
#include "zmmf/zmmf.h"

#define SINGLETON_CUR_MAX 15

template <class T> class Singleton;

class SingletonManager : public zmm::Object
{
public:
    static zmm::Ref<SingletonManager> getInstance();
    SingletonManager();
    
    void registerSingleton(zmm::Ref<Singleton<zmm::Object> > object);
    virtual void shutdown(bool complete = false);
    
protected:
    static zmm::Ref<SingletonManager> instance;
    static zmm::Ref<Mutex> mutex;
    
    zmm::Ref<zmm::ObjectStack<Singleton<zmm::Object> > > singletonStack;
};

template <class T>
class Singleton : public zmm::Object
{
public:
    static zmm::Ref<T> getInstance()
    {
        if (! singletonActive)
            throw _ServerShutdownException(_("singleton is currently inactive!"));
        if (instance == nil)
        {
            AUTOLOCK(mutex);
            if (! singletonActive)
                throw _ServerShutdownException(_("singleton is currently inactive!"));
            if (instance == nil) // check again, because there is a very small chance
                                 // that 2 threads tried to lock() concurrently
            {
                zmm::Ref<T> tmpInstance = zmm::Ref<T>(new T());
                tmpInstance->registerSingleton();
                tmpInstance->init();
                instance = tmpInstance;
            }
        }
        return instance;
    }
    
protected:
    
    virtual ~Singleton() { }
    
    virtual void init() { }
    virtual void shutdown() { }
    
    static zmm::Ref<Mutex> mutex;
    static zmm::Ref<T> instance;
    static bool singletonActive;
    
    virtual void registerSingleton()
    {
        SingletonManager::getInstance()->registerSingleton(zmm::Ref<Singleton<Object> >((Singleton<Object> *)this));
    }
    
private:
    
    virtual void inactivateSingleton()
    {
        //log_debug("%d %d\n", singletonActive, instance.getPtr());
        singletonActive = false;
        instance = nil;
    }
    virtual void reactivateSingleton() { singletonActive = true; }
    
    friend class SingletonManager;
};

#define SINGLETON_MUTEX(klass, recursive) template <> zmm::Ref<Mutex> Singleton<klass>::mutex = zmm::Ref<Mutex>(new Mutex(recursive))
//template <class T> zmm::Ref<Mutex> Singleton<T>::mutex = zmm::Ref<Mutex>(new Mutex());
template <class T> zmm::Ref<T> Singleton<T>::instance = nil;
template <class T> bool Singleton<T>::singletonActive = true;

#endif // __SINGLETON_H__
