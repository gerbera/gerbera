/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    singleton.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file singleton.h

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include "sync.h"
#include "zmmf/zmmf.h"

#define SINGLETON_CUR_MAX 10

template <class T> class Singleton;

class SingletonManager : public zmm::Object
{
public:
    static zmm::Ref<SingletonManager> getInstance();
    SingletonManager();
    virtual ~SingletonManager();
    
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
        if (! instance->singletonActive)
            throw _Exception(_("singleton is currently inactive!"));
        if (instance == nil)
        {
            AUTOLOCK(mutex);
            if (! instance->singletonActive)
                throw _Exception(_("singleton is currently inactive!"));
            if (instance == nil) // check again, because there is a very small chance
                                 // that 2 threads tried to lock() concurrently
            {
                zmm::Ref<T> tmpInstance = zmm::Ref<T>(new T());
                tmpInstance->init();
                tmpInstance->registerSingleton();
                instance = tmpInstance;
            }
        }
        return instance;
    }
    
protected:
    
    virtual ~Singleton() { mutex = nil; }
    
    virtual void init() { }
    virtual void shutdown() { }
    
    static zmm::Ref<Mutex> mutex;
    static zmm::Ref<T> instance;
    
    virtual void registerSingleton()
    {
        SingletonManager::getInstance()->registerSingleton(zmm::Ref<Singleton<Object> >((Singleton<Object> *)this));
    }
    
    static bool singletonActive;
private:
    
    virtual void inactivateSingleton()
    {
        //log_debug("%d %d\n", singletonActive, instance.getPtr());
        singletonActive = false;
        instance = nil;
    }
    //void activateSingleton() { singletonActive = true; }
    
    /*
    virtual void destroyMutex()
    {
        printf("--- try to destoy mutex...\n");
        mutex = nil;
        _print_backtrace(stdout);
        printf("=== did it work?\n");
    }
    */
    
    friend class SingletonManager;
};

#define SINGLETON_MUTEX(klass, recursive) template <> zmm::Ref<Mutex> Singleton<klass>::mutex = zmm::Ref<Mutex>(new Mutex(recursive))
//template <class T> zmm::Ref<Mutex> Singleton<T>::mutex = zmm::Ref<Mutex>(new Mutex());
template <class T> zmm::Ref<T> Singleton<T>::instance = nil;
template <class T> bool Singleton<T>::singletonActive = true;

#endif // __SINGLETON_H__

