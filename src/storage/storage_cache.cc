/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    storage_cache.cc - this file is part of MediaTomb.
    
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

/// \file storage_cache.cc

#include "storage_cache.h"

using namespace zmm;

StorageCache::StorageCache()
{
    capacity = STORAGE_CACHE_CAPACITY;
    maxfill = STORAGE_CACHE_MAXFILL;
    idHash = Ref<DBOHash<int,CacheObject> >(new DBOHash<int,CacheObject>(capacity, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    locationHash = Ref<DSOHash<Array<CacheObject> > >(new DSOHash<Array<CacheObject> >(capacity));
    hasBeenFlushed = false;
    mutex = Ref<Mutex> (new Mutex());
}

void StorageCache::clear()
{
    AUTOLOCK(mutex);
    idHash->clear();
    locationHash->clear();
}

Ref<CacheObject> StorageCache::getObject(int id)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    return idHash->get(id);
}

Ref<CacheObject> StorageCache::getObjectDefinitely(int id)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    Ref<CacheObject> obj = idHash->get(id);
    if (obj == nil)
    {
        ensureFillLevelOk();
        obj = Ref<CacheObject>(new CacheObject());
        idHash->put(id, obj);
    }
    return obj;
}

void StorageCache::addChild(int id)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    Ref<CacheObject> obj = idHash->get(id);
    if (obj != nil && obj->knowsNumChildren())
    {
        obj->setNumChildren(obj->getNumChildren() + 1);
    }
}

bool StorageCache::removeObject(int id)
{
    AUTOLOCK(mutex);
    Ref<CacheObject> obj = getObject(id);
    if (obj == nil)
        return false;
    if (obj->knowsLocation())
        locationHash->remove(obj->getLocation());
    return idHash->remove(id);
}

Ref<Array<CacheObject> > StorageCache::getObjects(String location)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    return locationHash->get(location);
}

void StorageCache::checkLocation(Ref<CacheObject> obj)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    if (! obj->knowsLocation())
        return;
    String location = obj->getLocation();
    Ref<Array<CacheObject> > objects = locationHash->get(location);
    if (objects == nil)
    {
        objects = Ref<Array<CacheObject> >(new Array<CacheObject>());
        locationHash->put(location, objects);
    }
    else
    {
        for (int i=0;i<objects->size();i++)
        {
            if (objects->get(i) == obj)
                return;
        }
    }
    objects->append(obj);
}

bool StorageCache::flushed()
{
    if (! hasBeenFlushed)
        return false;
    hasBeenFlushed = false;
    return true;
}

/* private */

void StorageCache::ensureFillLevelOk()
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    // TODO - much better...
    // for now just clear cache if it gets too full
    if (idHash->size() >= maxfill || locationHash->size() >= maxfill)
    {
        hasBeenFlushed = true;
        idHash->clear();
        locationHash->clear();
    }
}
