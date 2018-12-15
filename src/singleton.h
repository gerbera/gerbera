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

#include "exceptions.h"
#include "zmm/zmmf.h"
#include <mutex>

#define SINGLETON_CUR_MAX 15

template <class T, class MutexT = std::mutex>
class Singleton;

class SingletonManager : public zmm::Object {
public:
    static zmm::Ref<SingletonManager> getInstance();
    SingletonManager();

    void registerSingleton(zmm::Ref<Singleton<zmm::Object>> object);
    virtual void shutdown(bool complete = false);

protected:
    static zmm::Ref<SingletonManager> instance;
    static std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;

    zmm::Ref<zmm::ObjectStack<Singleton<zmm::Object>>> singletonStack;
};

template <class T, class MutexT>
class Singleton : public zmm::Object {
public:
    typedef MutexT mutex_type;

    static zmm::Ref<T> getInstance()
    {
        if (!singletonActive)
            throw _ServerShutdownException(_("singleton is currently inactive!"));
        if (instance == nullptr) {
            AutoLock lock(mutex);
            if (!singletonActive)
                throw _ServerShutdownException(_("singleton is currently inactive!"));
            if (instance == nullptr) // check again, because there is a very small chance
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
    virtual zmm::String getName() = 0;

protected:
    virtual ~Singleton() {}

    virtual void init() {}
    virtual void shutdown() {}

    static MutexT mutex;
    using AutoLock = std::lock_guard<MutexT>;

    static zmm::Ref<T> instance;
    static bool singletonActive;

    virtual void registerSingleton()
    {
        SingletonManager::getInstance()->registerSingleton(zmm::Ref<Singleton<Object>>((Singleton<Object>*)this));
    }

private:
    virtual void inactivateSingleton()
    {
        //log_debug("%d %d\n", singletonActive, instance.getPtr());
        singletonActive = false;
        instance = nullptr;
    }
    virtual void reactivateSingleton() { singletonActive = true; }

    friend class SingletonManager;
};

template <class T, class MutexT>
zmm::Ref<T> Singleton<T, MutexT>::instance = nullptr;
template <class T, class MutexT>
bool Singleton<T, MutexT>::singletonActive = true;
// Without the {} it's a declaration, not a definition.
template <class T, class MutexT>
MutexT Singleton<T, MutexT>::mutex {};

#endif // __SINGLETON_H__
