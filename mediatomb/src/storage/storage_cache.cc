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
    hash = Ref<DBOHash<int,CacheObject> >(new DBOHash<int,CacheObject>(capacity, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    mutex = Ref<Mutex> (new Mutex());
}

void StorageCache::clear()
{
    AUTOLOCK(mutex);
    hash->clear();
}

Ref<CacheObject> StorageCache::getObject(int id)
{
    return hash->get(id);
}

Ref<CacheObject> StorageCache::getObjectDefinitly(int id)
{
    Ref<CacheObject> obj = hash->get(id);
    if (obj == nil)
    {
        ensureFillLevelOk();
        obj = Ref<CacheObject>(new CacheObject());
        hash->put(id, obj);
    }
    return obj;
}

bool StorageCache::removeObject(int id)
{
    AUTOLOCK(mutex);
    return hash->remove(id);
}


/* privates */

void StorageCache::ensureFillLevelOk()
{
    // TODO - much better...
    // for now just clear cache if it get too full
    if (hash->size() + 1 > maxfill)
        hash->clear();
}


