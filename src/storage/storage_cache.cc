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

#include <cassert>

#include "storage_cache.h"

using namespace zmm;
using namespace std;

StorageCache::StorageCache()
{
    capacity = STORAGE_CACHE_CAPACITY;
    maxfill = STORAGE_CACHE_MAXFILL;
    idHash = make_shared<unordered_map<int,Ref<CacheObject> > >();
    locationHash = make_shared<unordered_map<zmm::String, Ref<Array<CacheObject> > > >();
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
    try {
        return idHash->at(id);
    } catch(out_of_range& nope) {
        return nil;
    }
}

Ref<CacheObject> StorageCache::getObjectDefinitely(int id)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    Ref<CacheObject> obj = nil;
    try {
        obj = idHash->at(id);
    } catch (const out_of_range& ex) {
        ensureFillLevelOk();
        obj = Ref<CacheObject>(new CacheObject());
        idHash->emplace(id, obj);
    }
    return obj;
}

void StorageCache::addChild(int id)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    Ref<CacheObject> obj = nil;
    try {
        obj = idHash->at(id);
        if (obj->knowsNumChildren())
            obj->setNumChildren(obj->getNumChildren() + 1);
    } catch(const out_of_range& ex) {} // id not found
}

bool StorageCache::removeObject(int id)
{
    AUTOLOCK(mutex);
    Ref<CacheObject> obj = getObject(id);
    if (obj == nil)
        return false;
    if (obj->knowsLocation())
        locationHash->erase(obj->getLocation());
    return idHash->erase(id);
}

Ref<Array<CacheObject> > StorageCache::getObjects(String location)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    try {
        return locationHash->at(location);
    } catch (const out_of_range& ex) {
        return nil;
    }
}

void StorageCache::checkLocation(Ref<CacheObject> obj)
{
#ifdef TOMBDEBUG
    assert(mutex->isLocked());
#endif
    if (! obj->knowsLocation())
        return;
    String location = obj->getLocation();
    Ref<Array<CacheObject> > objects = nil;
    try {
        objects = locationHash->at(location);
        for (int i=0;i<objects->size();i++)
        {
            if (objects->get(i) == obj)
                return;
        }
    } catch (const out_of_range& ex) {
        objects = Ref<Array<CacheObject> >(new Array<CacheObject>());
        locationHash->emplace(location, objects);
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
