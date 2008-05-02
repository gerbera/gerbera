/*MT*/

/// \file storage_cache.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

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
#ifdef LOG_TOMBDEBUG
    assert(mutex->isLocked());
#endif
    return idHash->get(id);
}

Ref<CacheObject> StorageCache::getObjectDefinitely(int id)
{
#ifdef LOG_TOMBDEBUG
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
#ifdef LOG_TOMBDEBUG
    assert(mutex->isLocked());
#endif
    return locationHash->get(location);
}

void StorageCache::checkLocation(Ref<CacheObject> obj)
{
#ifdef LOG_TOMBDEBUG
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
#ifdef LOG_TOMBDEBUG
    assert(mutex->isLocked());
#endif
    // TODO - much better...
    // for now just clear cache if it gets too full
    if (idHash->size() >= maxfill || locationHash->size() >= maxfill)
    {
        idHash->clear();
        locationHash->clear();
    }
}


